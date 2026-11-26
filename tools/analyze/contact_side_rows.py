"""Export the side-paired contact rows from the original tables.

The credit difficulty, credit rank and receiver rank roots each hold two row
pointers (team 0 and team 1). This tool reads the first scalar of each row and
emits the paired values used by `damage-side` content. Numeric API data only.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

ROOTS = {
    "credit_difficulty": (0x804335E0, (1.02, 0.92)),
    "credit_rank": (0x804335F8, (0.85, 0.85)),
    "receiver_rank": (0x80433618, (1.15, 1.15)),
}


def recover(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    rows = {}
    addresses = {}
    for name, (root, expected) in ROOTS.items():
        values = []
        pointers = []
        for variant in range(2):
            pointer = struct.unpack(">I", image.read(root + 4 * variant, 4, "data"))[0]
            value = struct.unpack(">f", image.read(pointer, 4, "data"))[0]
            if abs(value - expected[variant]) > 1e-6:
                raise ValueError(f"{name} variant {variant} changed: {value}")
            pointers.append(f"{pointer:08X}")
            values.append(round(value, 6))
        rows[name] = values
        addresses[name] = {"root": f"{root:08X}", "rows": pointers}
    return {
        "kind": "gf-contact-side-rows",
        "revision": 1,
        "source_dol_sha256": SOURCE_SHA256,
        "variants": [
            {"team": 0,
             "credit_difficulty": rows["credit_difficulty"][0],
             "credit_rank": rows["credit_rank"][0],
             "receiver_rank": rows["receiver_rank"][0]},
            {"team": 1,
             "credit_difficulty": rows["credit_difficulty"][1],
             "credit_rank": rows["credit_rank"][1],
             "receiver_rank": rows["receiver_rank"][1]},
        ],
        "addresses": addresses,
        "notes": [
            "These are the paired rows used by the credit side; the battle scalar and receiver health/force rows still come from their own tables.",
            "Emit one damage-side row per variant instead of binding a single side.",
        ],
    }


def export(dol, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("contact side rows output already exists")
    with ReadOnlyFile(dol) as source:
        if not 0 < source.size <= 8 << 20:
            raise ValueError("source DOL extent")
        payload = recover(DolImage(source.read(0, source.size), SOURCE_SHA256))
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode()
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"bytes": len(raw), "sha256": hashlib.sha256(raw).hexdigest(),
            "variants": len(payload["variants"])}


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("dol", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.output)))


if __name__ == "__main__":
    main()
