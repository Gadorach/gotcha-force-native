"""Export the ordinary G-Red beam's local HSD bank from a qualified efct.pzz."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from tools.extract.pzz import PzzArchive
from tools.extract.source_image import ReadOnlyFile


MEMBER_SIZE = 585728
MEMBER_SHA256 = "89a0de2ccf3652b5c1b5c12f3bd9a4c923908904c6da01a22e82eef7d1b565bf"
ORDINARY_BANK_SIZE = 417955
ORDINARY_BANK_SHA256 = "a376090145b09de577454723c20058abc216279d738b83770b355206dfb9f0b5"


def export(member, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("beam-art output already exists")
    with ReadOnlyFile(member) as source:
        if source.size != MEMBER_SIZE or hashlib.sha256(source.read(0, source.size)).hexdigest() != MEMBER_SHA256:
            raise ValueError("expected the qualified original efct.pzz member")
        archive = PzzArchive(source)
        if len(archive.entries) != 3:
            raise ValueError("unexpected common effect block count")
        # 42878's ordinary mode29=0 selects block 1, not the other
        # model bank or the separate shared texture resource.
        decoded, _ = archive.read(1, ORDINARY_BANK_SIZE + 1)
        if len(decoded) != ORDINARY_BANK_SIZE + 1 or struct.unpack_from(">I", decoded)[0] != ORDINARY_BANK_SIZE:
            raise ValueError("unexpected ordinary effect bank extent")
        bank = decoded[:ORDINARY_BANK_SIZE]
        if hashlib.sha256(bank).hexdigest() != ORDINARY_BANK_SHA256:
            raise ValueError("unexpected ordinary effect bank digest")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(bank)
    return {"bytes": len(bank), "sha256": ORDINARY_BANK_SHA256, "member": 697, "block": 1, "record": 0}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("member", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.member, args.output), sort_keys=True))


if __name__ == "__main__":
    main()
