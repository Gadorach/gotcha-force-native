"""Publish the per-Borg 24-byte action tables used by the reaction stage.

Each retail installer writes `actor+0x27C` to a model-specific table. The
pair record copies that pointer to `+0x2C`; the pair code indexes the table
with the action-step byte and reads the reaction mode (+0x0E), flags
(+0x10), yaw/pitch offsets (+0x14/+0x15) and repeat byte (+0x16).

The installer's literal table address is decoded from the DOL, so the
artifact covers the authored basic-shot cohort and deduplicates the
tables shared by several models.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from tools.analyze import reaction_installer_tables
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

SIZE = 24
COUNT = 64  # Review budget, not a proven physical extent of every table.


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
    installer = reaction_installer_tables.recover(image)
    tables = {}
    models = []
    for row in installer["models"]:
        table = row["table"]
        models.append({"model": row["model"], "installer": row["installer"],
                       "table": table})
        if table and table not in tables:
            tables[table] = _entries(image, int(table, 16))
    return {
        "kind": "gf-reaction-attack-table",
        "revision": 4,
        "source_dol_sha256": SOURCE_SHA256,
        "entry_size": SIZE,
        "count": COUNT,
        "tables": tables,
        "models": models,
        "pointer_chain": {
            "actor_field": "+0x27C",
            "pair_record": {"actor": "+0x20", "other": "+0x24",
                            "step": "+0x28", "table": "+0x2C",
                            "move": "+0x11"},
            "installer_table": "802D3224",
            "example_store": {"site": "8006F5EC", "table": "802D4160"},
            "pair_allocator": "8008AC80",
            "pair_update": "8008A65C",
        },
        "notes": [
            "The model table is the literal returned by the bounded installer paths for actor+0x27C.",
            "Models are the authored basic-shot cohort; tables are "
            "deduplicated by address.",
            "The pair record copies actor+0x27C to +0x2C.",
            "Indices are the low byte of the step halfword at +4, copied to pair+0x11.",
            "Count is a fixed inspection window, not the verified extent of each table. Use an independently recovered queue to establish a live index.",
            "800300BC initializes a direction for modes 0..4. Other values skip vector initialization in this function.",
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
            "models": len(payload["models"]), "tables": len(payload["tables"])}


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("dol", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.output), sort_keys=True))


if __name__ == "__main__":
    main()
