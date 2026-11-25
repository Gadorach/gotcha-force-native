"""Summarise the private matching decompilation for the battle-engine ranges.

The decomp database is a local, private artifact. This tool records only
metadata - function addresses, sizes, match status and best score - grouped by
the engine ranges the attack/movement recovery already identified. It does not
copy matching source into the output.

Use it to see which shared battle functions are already byte-matched and which
are still open before reimplementing a subsystem.
"""
import argparse
import hashlib
import json
import sqlite3
from pathlib import Path

DEFAULT_DB = Path("/home/gadorach/Projects/GotchaForce/local/decomp-wtf/.decomp/decomp.db")
RANGES = (
    ("scene-actor", 0x80027000, 0x8002B000),
    ("collision-pair-reaction", 0x8002B000, 0x80031000),
    ("damage", 0x8003C000, 0x8003E000),
    ("action", 0x8006E000, 0x80073000),
)


def function_address(name):
    try:
        return int(name.split("_")[1], 16)
    except (IndexError, ValueError):
        return None


def group(rows):
    result = []
    for label, low, high in RANGES:
        selected = [row for row in rows if low <= row["address"] < high]
        matched = [row for row in selected if row["status"] == "matched"]
        open_rows = [row for row in selected if row["status"] != "matched"]
        result.append({
            "range": label,
            "low": f"{low:08X}",
            "high": f"{high:08X}",
            "functions": len(selected),
            "matched": len(matched),
            "open": len(open_rows),
            "largest_matched": sorted(matched, key=lambda row: -row["size"])[:10],
            "largest_open": sorted(open_rows, key=lambda row: -row["size"])[:10],
        })
    return result


def inventory(db):
    db = Path(db)
    if not db.is_file():
        raise ValueError(f"decomp database not found: {db}")
    connection = sqlite3.connect(f"file:{db}?mode=ro", uri=True)
    rows = []
    try:
        for name, size, status, score in connection.execute(
                "select name,size,status,best_score from functions"):
            address = function_address(name)
            if address is None:
                continue
            rows.append({"address": address, "name": name, "size": int(size),
                         "status": status, "score": score})
    finally:
        connection.close()
    rows.sort(key=lambda row: row["address"])
    return {
        "kind": "gf-decomp-battle-inventory",
        "revision": 1,
        "database": db.name,
        "source": "private matching decompilation",
        "total_functions": len(rows),
        "total_matched": sum(1 for row in rows if row["status"] == "matched"),
        "ranges": group(rows),
        "notes": [
            "Metadata only: no matching source is copied into this artifact.",
            "Use largest_open to choose the next subsystem to recover or reimplement.",
        ],
    }


def export(db, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("decomp inventory output already exists")
    payload = inventory(db)
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode()
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"bytes": len(raw), "sha256": hashlib.sha256(raw).hexdigest(),
            "functions": payload["total_functions"],
            "matched": payload["total_matched"]}


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--db", type=Path, default=DEFAULT_DB)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.db, args.output)))


if __name__ == "__main__":
    main()
