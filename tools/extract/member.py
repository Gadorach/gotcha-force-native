"""Extract exactly one AFS member by verified filename, refusing overwrites.

The stored member name is never used as an output path. All extraction results
must remain in ignored original/ or local/ storage.
"""
import argparse
import mmap
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/"analyze"))
from formats import afs_entries, region


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("name")
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    output = args.output.resolve()
    if not any(output.is_relative_to(root/name) for name in ("original", "local")):
        raise ValueError("output must stay in local/ or original/")
    with args.archive.open("rb") as f, mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ) as data:
        matches = [e for e in afs_entries(data) if e["name"] == args.name]
        if len(matches) != 1:
            raise ValueError(f"expected one member, found {len(matches)}")
        entry = matches[0]
        output.parent.mkdir(parents=True, exist_ok=True)
        with output.open("xb") as out:
            out.write(region(data, entry["offset"], entry["size"]))
    print(f"extracted index={entry['index']} bytes={entry['size']} name={entry['name']}")


if __name__ == "__main__":
    main()
