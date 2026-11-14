"""Read a user-supplied raw GameCube image into a NEW offline output directory."""
import argparse
import hashlib
import json
from pathlib import Path
from tools.extract.source_image import GcmImage, ReadOnlyFile


def safe_name(name):
    parts = name.split("/")
    reserved = {"con", "prn", "aux", "nul", ".git",
                *(f"com{i}" for i in range(1, 10)),
                *(f"lpt{i}" for i in range(1, 10))}
    if not parts or any(not p or p in (".", "..") or p.endswith((" ", "."))
                        or any(c in p for c in "\\:\0")
                        or any(ord(c) < 32 for c in p)
                        or p.split(".")[0].casefold() in reserved
                        or p.casefold() == ".git" for p in parts):
        raise ValueError("unsafe output file name")
    return parts


def extract(image, output, names=None, include_dol=True):
    image, output = Path(image), Path(output)
    with ReadOnlyFile(image) as source:
        disc = GcmImage(source)
        selected = sorted(disc.files if names is None else names)
        if len(set(selected)) != len(selected):
            raise ValueError("duplicate selected source path")
        # Check all source names and case-folded file/directory collisions before
        # creating output. A malformed late member must not overwrite an earlier one.
        files, directories = set(), set()
        for name in selected:
            parts = safe_name(name)
            disc.file(name)
            key = "/".join(parts).casefold()
            parents = {"/".join(parts[:i]).casefold() for i in range(1, len(parts))}
            if key in files or key in directories or parents & files:
                raise ValueError("output path collision")
            files.add(key)
            directories.update(parents)
        dol = disc.main_dol().data if include_dol else None
        if output.exists() or output.is_symlink():
            raise FileExistsError("output directory already exists")
        # Resolve only an existing parent; never traverse a symlink created by
        # an extracted FST member (the extractor creates regular files only).
        output.parent.mkdir(parents=True, exist_ok=True)
        parent = output.parent.resolve(strict=True)
        destination = parent / output.name
        destination.mkdir(mode=0o700)
        report = {"format": 1, "disc_id_hex": disc.disc_id, "files": []}
        try:
            for name in selected:
                member = disc.file(name)
                path = destination / "files" / name
                path.parent.mkdir(parents=True, exist_ok=True)
                digest = hashlib.sha256()
                with path.open("xb") as stream:
                    for offset in range(0, member.size, 4 << 20):
                        block = member.read(offset, min(4 << 20, member.size - offset))
                        stream.write(block)
                        digest.update(block)
                report["files"].append({"path": "files/" + name, "bytes": member.size,
                                        "sha256": digest.hexdigest()})
            if dol is not None:
                (destination / "sys").mkdir()
                with (destination / "sys/main.dol").open("xb") as stream:
                    stream.write(dol)
                report["files"].append({"path": "sys/main.dol", "bytes": len(dol),
                                        "sha256": hashlib.sha256(dol).hexdigest()})
            with (destination / "extraction.json").open("x") as stream:
                json.dump(report, stream, indent=2)
            return report
        except Exception as error:
            with (destination / "extraction-failed.json").open("x") as stream:
                json.dump({"type": type(error).__name__, "complete": False}, stream)
            raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", type=Path)
    parser.add_argument("output", type=Path, nargs="?")
    parser.add_argument("--list", action="store_true")
    parser.add_argument("--file", action="append", dest="names")
    parser.add_argument("--no-dol", action="store_true")
    args = parser.parse_args()
    if args.list:
        with ReadOnlyFile(args.image) as source:
            disc = GcmImage(source)
            for name, (_, size) in sorted(disc.files.items()):
                print(f"{size}\t{name}")
        return
    if args.output is None:
        parser.error("OUTPUT is required unless --list is used")
    report = extract(args.image, args.output, args.names, not args.no_dol)
    print(f"Extracted {len(report['files'])} files into {args.output}")


if __name__ == "__main__":
    main()
