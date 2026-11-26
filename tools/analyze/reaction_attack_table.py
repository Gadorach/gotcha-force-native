"""Publish the per-Borg 24-byte action tables used by the reaction stage.

Each retail installer writes `actor+0x27C` to a model-specific table. The
pair record copies that pointer to `+0x2C`; the pair code indexes the table
with the action-step byte and reads the reaction mode (+0x0E), flags
(+0x10), yaw/pitch offsets (+0x14/+0x15) and repeat byte (+0x16).

`80354160` is the generic default written by `8006F5EC`; the retail Borg
installers override it (G-Red `80365EC0`, Sasuke `802D4160`, Proto King
`80344C88`). The artifact records both the default and the pilot tables.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

DEFAULT_TABLE = 0x80354160
INSTALLER_TABLE = 0x802D3224
MODELS = (0x0615, 0x000A, 0x061E)
# The installer writes a literal table address; these are the three pilot
# installers' `stw ..., 0x27C` constants.
MODEL_TABLES = {0x0615: 0x80365EC0, 0x000A: 0x802D4160, 0x061E: 0x80344C88}
SIZE = 24
COUNT = 64


def _source_table(image, table, model, stride=4):
    base, = struct.unpack(">I", image.read(table + 4 * (model >> 8), 4, "data"))
    return base + stride * (model & 255)


def installer_address(image, model):
    address = _source_table(image, INSTALLER_TABLE, model)
    return struct.unpack(">I", image.read(address, 4, "data"))[0]


def entry(image, table, index):
    row = image.read(table + index * SIZE, SIZE, "data")
    return {
        "index": index,
        "mode": struct.unpack_from(">b", row, 0x0E)[0],
        "flags": struct.unpack_from(">h", row, 0x10)[0],
        "yaw": struct.unpack_from(">b", row, 0x14)[0],
        "pitch": struct.unpack_from(">b", row, 0x15)[0],
        "repeat": row[0x16],
    }


def _entries(image, table):
    return [entry(image, table, index) for index in range(COUNT)]


def recover(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    models = []
    for model in MODELS:
        table = MODEL_TABLES[model]
        models.append({"model": f"{model:04X}", "table": f"{table:08X}",
                       "installer": f"{installer_address(image, model):08X}",
                       "entries": _entries(image, table)})
    return {
        "kind": "gf-reaction-attack-table",
        "revision": 2,
        "source_dol_sha256": SOURCE_SHA256,
        "entry_size": SIZE,
        "count": COUNT,
        "default_table": {"table": f"{DEFAULT_TABLE:08X}",
                          "entries": _entries(image, DEFAULT_TABLE)},
        "models": models,
        "pointer_chain": {
            "actor_field": "+0x27C",
            "pair_record": {"actor": "+0x20", "other": "+0x24",
                            "step": "+0x28", "table": "+0x2C",
                            "move": "+0x11"},
            "installer_table": f"{INSTALLER_TABLE:08X}",
            "default_setter": "8006F5EC",
            "pair_allocator": "8008AC80",
            "pair_update": "8008A65C",
        },
        "notes": [
            "The installer table at 802D3224 maps a model to its installer; "
            "the three pilot tables are the installer store constants.",
            "The pair record copies actor+0x27C to +0x2C.",
            "Indices are the action-step byte (record+4) copied to pair+0x11.",
            "The source switch handles modes 0..5; other values take the "
            "default actor+0x11C vector.",
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
            "models": len(payload["models"])}


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("dol", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.output), sort_keys=True))


if __name__ == "__main__":
    main()
