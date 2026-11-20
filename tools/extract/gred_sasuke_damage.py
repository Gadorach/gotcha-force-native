"""Export the exact ordinary normal G-Red/Sasuke damage coefficient slices.

The result is local numeric data, not an executable rule, live actor,
or a dump of the whole source damage matrix.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct

from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile


GRED, SASUKE = 0x0615, 0x000A


def recover(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified G-Red/Sasuke source revision")

    def read(at, size):
        return image.read(at, size, "data")

    def ptr(at):
        return struct.unpack(">I", read(at, 4))[0]

    def category(model):
        root = ptr(0x802F2E28 + 4 * (model >> 8))
        return struct.unpack(">h", read(root + 2 * (model & 255), 2))[0]

    if (category(GRED), category(SASUKE)) != (4, 0):
        raise ValueError("unqualified source matchup categories")
    if read(0x802C5D60 + 4 * 4, 4) != struct.pack(">f", 1.0):
        raise ValueError("unqualified source matchup factor")
    if read(0x802E895C, 2) != struct.pack(">h", 200) or read(0x802DF884, 2) != struct.pack(">h", 180):
        raise ValueError("unqualified source level-zero HP caps")
    if read(0x80365EC0, 2) != struct.pack(">H", 20):
        raise ValueError("unqualified source beam row amount")

    def row(root, count, group=None):
        first = ptr(root)
        return read(first if group is None else ptr(first + 4 * group), 4 * count)

    # Exact normal 000A data-slot-0 bytes +9C..+9E are 1/1/0.
    # Both ownership selectors are zero here. Do not include other
    # modes, actors, difficulties beyond index 2, or ranks/tiers past 3.
    payload = b"".join((
        row(0x804335E0, 3), row(0x804335E8, 32, 0), row(0x804335F0, 32, 0), row(0x804335F8, 4),
        row(0x80433600, 3, 1), row(0x80433608, 32, 1), row(0x80433610, 32, 0), row(0x80433618, 4),
        read(0x802C7CA0, 16),
    ))
    if len(payload) != 584 or any(not math.isfinite(f) or not 0 <= f <= 16 for f, in struct.iter_unpack(">f", payload)):
        raise ValueError("coefficient extent/domain")
    return struct.pack(">4sHHHHI", b"GFDA", 1, GRED, SASUKE, 0, 600) + payload


def export(dol, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("G-Red/Sasuke damage output already exists")
    with ReadOnlyFile(dol) as source:
        if not 0 < source.size <= 8 << 20:
            raise ValueError("source DOL extent")
        raw = recover(DolImage(source.read(0, source.size), SOURCE_SHA256))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return dict(bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest(), source_sha256=SOURCE_SHA256)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dol", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.output)))


if __name__ == "__main__":
    main()
