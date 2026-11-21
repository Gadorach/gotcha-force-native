"""Export Sasuke's exact normal collision bank and row 0 for the local beam trial."""
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
        raise FileExistsError("receiver output already exists")
    with ReadOnlyFile(member) as source:
        if source.size != 1165312 or hashlib.sha256(source.read(0, source.size)).hexdigest() != (
                "d58b8ca4bf80cb58543d32392af48d8f25fd797fa96065df0904ddbe5200d084"):
            raise ValueError("expected the original pl000a.pzz AFS member")
        archive = PzzArchive(source)
        if len(archive.entries) != 14:
            raise ValueError("unexpected Sasuke PZZ directory")
        bank, _ = archive.read(2, 7840)
    with ReadOnlyFile(dol) as source:
        image = DolImage(source.read(0, source.size), SOURCE_SHA256)
        hit = image.read(0x802D4160, 24, "data")
    raw = bank + hit
    digest = hashlib.sha256(raw).hexdigest()
    if len(bank) != 7840 or digest != "f503b42a950adb404f84a308d156ea7e89585d078ba9c48b6a0d50c5fca434a3":
        raise ValueError("unexpected Sasuke collision bank or hit row")
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("xb") as stream:
        stream.write(raw)
    return {"bytes": len(raw), "sha256": digest, "member": 495, "block": 2, "body": 9, "hit_row": 0}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for arg in ("dol", "member", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.member, args.output), sort_keys=True))


if __name__ == "__main__":
    main()
