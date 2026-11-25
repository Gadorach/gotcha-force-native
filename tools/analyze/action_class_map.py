"""Map the original two-level action-class dispatch.

The action entry (`8006FACC`) selects a family table with `actor+0x580`; each
family handler selects a sub-table with `actor+0x581`. This tool reads those
tables from the DOL. With a private decomp database it also records each
handler's match status; without it, only the addresses are emitted.

Metadata only: no matching source is copied.
"""
import argparse
import bisect
import hashlib
import json
import sqlite3
import struct
from pathlib import Path

from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

FAMILY_TABLE = 0x802D3B68
FAMILY_COUNT = 32
SUBTABLES = {
    0x802D3B7C: ("b-shot", 16),
    0x802D3BD4: ("b-close-default", 16),
    0x802D3BE8: ("b-close-model-000a", 8),
    0x802D3C48: ("x", 16),
    0x802D3C5C: ("class-3", 16),
    0x802D3C7C: ("class-4", 16),
}
TEXT_LOW = 0x80003100
TEXT_HIGH = 0x802B0160


def read_table(image, address, count):
    entries = []
    for index in range(count):
        try:
            value = struct.unpack(">I", image.read(address + 4 * index, 4, "data"))[0]
        except Exception:
            break
        if not TEXT_LOW <= value < TEXT_HIGH:
            break
        entries.append(value)
    return entries


def _functions(db):
    connection = sqlite3.connect(f"file:{db}?mode=ro", uri=True)
    rows = []
    try:
        for name, size, status, score in connection.execute(
                "select name,size,status,best_score from functions"):
            try:
                address = int(name.split("_")[1], 16)
            except (IndexError, ValueError):
                continue
            rows.append((address, size, name, status, score))
    finally:
        connection.close()
    rows.sort()
    return rows


def _status(rows, starts, address):
    if not rows:
        return None
    index = bisect.bisect_right(starts, address) - 1
    for candidate in range(max(0, index - 2), min(len(rows), index + 3)):
        low, size, name, status, score = rows[candidate]
        if low <= address < low + size:
            return {"name": name, "status": status, "score": score, "size": size}
    return {"name": "?", "status": "?", "score": None, "size": 0}


def recover(image, db=None):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    rows = _functions(Path(db)) if db else []
    starts = [row[0] for row in rows]
    families = []
    for index, handler in enumerate(read_table(image, FAMILY_TABLE, FAMILY_COUNT)):
        entry = {"class": index, "handler": f"{handler:08X}"}
        found = _status(rows, starts, handler)
        if found:
            entry.update(found)
        families.append(entry)
    subtables = []
    for address, (name, count) in SUBTABLES.items():
        entries = []
        for index, handler in enumerate(read_table(image, address, count)):
            entry = {"index": index, "handler": f"{handler:08X}"}
            found = _status(rows, starts, handler)
            if found:
                entry.update(found)
            entries.append(entry)
        subtables.append({"name": name, "address": f"{address:08X}", "entries": entries})
    return {
        "kind": "gf-retail-action-class-map",
        "revision": 1,
        "source_dol_sha256": SOURCE_SHA256,
        "family_entry": "8006FACC",
        "family_index": "actor+0x580",
        "sub_index": "actor+0x581",
        "family_table": {"address": f"{FAMILY_TABLE:08X}", "entries": families},
        "subtables": subtables,
        "notes": [
            "Two-level dispatch: family byte then sub-class byte select shared handlers.",
            "Handler status comes from the optional private decomp database; no source is copied.",
        ],
    }


def export(dol, output, db=None):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("action class map output already exists")
    with ReadOnlyFile(dol) as source:
        if not 0 < source.size <= 8 << 20:
            raise ValueError("source DOL extent")
        payload = recover(DolImage(source.read(0, source.size), SOURCE_SHA256), db)
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode()
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    matched = sum(1 for entry in payload["family_table"]["entries"]
                  if entry.get("status") == "matched")
    return {"bytes": len(raw), "sha256": hashlib.sha256(raw).hexdigest(),
            "families": len(payload["family_table"]["entries"]),
            "matched_families": matched}


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("dol", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--db", type=Path, default=None,
                        help="optional private decomp database for match status")
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.output, args.db)))


if __name__ == "__main__":
    main()
