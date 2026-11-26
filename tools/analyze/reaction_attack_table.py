"""Publish the shared 24-byte action table used by the reaction stage.

The battle pair record points at `actor+0x27C`; `8006F5EC` sets that to
`80354160`. The pair code indexes the table with the action-step byte and
reads the reaction mode (+0x0E), flags (+0x10), yaw/pitch offsets
(+0x14/+0x15) and repeat byte (+0x16). The first 38 entries use the
reaction modes the source switch handles; later entries use the default
mode and are outside the validated prefix.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

TABLE = 0x80354160
SIZE = 24
COUNT = 64
VALIDATED = 38


def recover(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    entries = []
    for index in range(COUNT):
        row = image.read(TABLE + index * SIZE, SIZE, "data")
        entries.append({
            "index": index,
            "mode": struct.unpack_from(">b", row, 0x0E)[0],
            "flags": struct.unpack_from(">h", row, 0x10)[0],
            "yaw": struct.unpack_from(">b", row, 0x14)[0],
            "pitch": struct.unpack_from(">b", row, 0x15)[0],
            "repeat": row[0x16],
            "validated": index < VALIDATED,
        })
    return {
        "kind": "gf-reaction-attack-table",
        "revision": 1,
        "source_dol_sha256": SOURCE_SHA256,
        "table": f"{TABLE:08X}",
        "entry_size": SIZE,
        "count": COUNT,
        "validated_prefix": VALIDATED,
        "entries": entries,
        "pointer_chain": {
            "actor_field": "+0x27C",
            "pair_record": {"actor": "+0x20", "other": "+0x24",
                            "step": "+0x28", "table": "+0x2C",
                            "move": "+0x11"},
            "setters": {"8006F5EC": f"{TABLE:08X}",
                        "8008AE60": "80344760"},
            "pair_allocator": "8008AC80",
            "pair_update": "8008A65C",
        },
        "notes": [
            "Indices are the action-step byte (record+4) copied to pair+0x11.",
            "The source switch handles modes 0..5; other values take the "
            "default actor+0x11C vector.",
            "The validated prefix is 38 entries; later entries use the "
            "default mode and need the action-step mapping before use.",
        ],
    }


def export(dol, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("reaction attack table output already exists")
    with ReadOnlyFile(dol) as source:
        if not 0 < source.size <= 8 << 20:
            raise ValueError("source DOL extent")
        payload = recover(DolImage(source.read(0, source.size), SOURCE_SHA256))
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode()
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"bytes": len(raw), "sha256": hashlib.sha256(raw).hexdigest(),
            "entries": len(payload["entries"])}


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("dol", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.output), sort_keys=True))


if __name__ == "__main__":
    main()
