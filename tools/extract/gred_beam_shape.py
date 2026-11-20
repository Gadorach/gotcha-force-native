"""Export the exact normal G-Red slot-2 bank and beam hit row for local tests."""
import argparse
import hashlib
import json
from pathlib import Path

from tools.extract.gred_locomotion import SOURCE_SHA256
from tools.extract.pzz import PzzArchive
from tools.extract.source_image import DolImage, ReadOnlyFile


def export(dol, member, destination):
    out = Path(destination)
    if out.exists() or out.is_symlink():
        raise FileExistsError("beam-shape output already exists")
    with ReadOnlyFile(member) as source:
        if source.size != 847872 or hashlib.sha256(source.read(0, source.size)).hexdigest() != (
                "4486023852bd01a0e7ced06983064c2c8f8492e6dff9994b4a4f011a508e226f"):
            raise ValueError("expected the original pl0615.pzz AFS member")
        archive = PzzArchive(source)
        if len(archive.entries) != 14:
            raise ValueError("unexpected G-Red PZZ directory")
        bank, _ = archive.read(2, 7840)
    with ReadOnlyFile(dol) as source:
        image = DolImage(source.read(0, source.size), SOURCE_SHA256)
        hit = image.read(0x80365EC0, 24, "data")
    raw = bank + hit
    digest = hashlib.sha256(raw).hexdigest()
    if len(bank) != 7840 or digest != "7d82e74ab38253bd3822c165f05287b30dfd8e222fd309adea04dd5b4ead9359":
        raise ValueError("unexpected credited collision bank or hit row")
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("xb") as stream:
        stream.write(raw)
    return {"bytes": len(raw), "sha256": digest, "member": 587, "block": 2, "hit_row": 0}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for arg in ("dol", "member", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.member, args.output), sort_keys=True))


if __name__ == "__main__":
    main()
