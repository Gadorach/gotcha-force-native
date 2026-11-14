"""Extract all declared PZZ blocks into fixed numeric files in a NEW directory."""
import argparse
import hashlib
import json
from pathlib import Path
from tools.extract.pzz import PzzArchive
from tools.extract.source_image import ReadOnlyFile


def extract(archive, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("output already exists")
    with ReadOnlyFile(archive) as source:
        bank = PzzArchive(source)
        output.mkdir(mode=0o700)
        report = {"format": 1, "blocks": []}
        try:
            for index, entry in enumerate(bank.entries):
                data, consumed = bank.read(index)
                name = f"{index:03d}.bin"
                with (output / name).open("xb") as stream:
                    stream.write(data)
                report["blocks"].append({**entry, "path": name, "decoded_bytes": len(data),
                                         "consumed_bytes": consumed,
                                         "sha256": hashlib.sha256(data).hexdigest()})
            with (output / "pzz.json").open("x") as stream:
                json.dump(report, stream, indent=2)
            return report
        except Exception as error:
            with (output / "pzz-failed.json").open("x") as stream:
                json.dump({"type": type(error).__name__, "complete": False}, stream)
            raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    report = extract(args.archive, args.output)
    print(f"Extracted {len(report['blocks'])} blocks")


if __name__ == "__main__":
    main()
