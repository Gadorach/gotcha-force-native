"""Export qualified G-Red movement-input tables and airborne-resource operands.

GFMI is an input/numeric profile, not an actor, animation map or physics policy.
Inputs are read-only; output must be new and belongs outside Git.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from tools.analyze.gred_movement import COMMANDS, qualify_setup
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile


def encode(image, setup):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified G-Red movement-input source")
    data, config = qualify_setup(image, setup)
    if config[1] != 0 or not config[0x11]:
        raise ValueError("normal G-Red movement/resource policy")
    immediates = []
    for address, instruction in (
        (0x800689AC, 0x38A00014), (0x80068A14, 0x38A00034),
        (0x800689FC, 0x38000002), (0x800689F0, 0x38000003),
        (0x80068A2C, 0x38000014), (0x80068ABC, 0x38000010),
    ):
        if image.read(address, 4, "text") != struct.pack(">I", instruction):
            raise ValueError("source direction-buffer instruction mismatch")
        immediates.append(instruction & 0xFFFF)
    if image.read(0x80003334, 8, "text") != bytes.fromhex("3da0804361adb5a0"):
        raise ValueError("source small-data base")
    if image.read(0x8008B788, 4, "text") != bytes.fromhex("386d8388"):
        raise ValueError("source octant-table addressing")
    raw = bytearray(192)
    struct.pack_into(">IHHII", raw, 0, 0x47464D49, 1, 0x615, 0xE2B4439A, len(raw))
    raw[16:20] = bytes([config[1], 1, config[0x11], 0])
    raw[20:28] = bytes([*immediates, len(COMMANDS), 0])
    struct.pack_into(">I", raw, 28, 0x00FF1F7F)  #8008BBC0's declared source-input mask
    raw[32:40] = image.read(0x80433928, 8)
    raw[40:56] = image.read(0x802DA770, 16)
    for index, (address, _, _) in enumerate(COMMANDS):
        row = image.read(address, 12)
        if struct.unpack_from(">H", row)[0] != 1:
            raise ValueError("unreviewed multi-step input command")
        raw[56 + index * 12:68 + index * 12] = row
    raw[176:190] = data[4:18]
    return bytes(raw)


def export(dol, setup, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("movement-input output already exists")
    with ReadOnlyFile(dol) as source, ReadOnlyFile(setup) as profile:
        if profile.size != 1064:
            raise ValueError("setup extent")
        raw = encode(DolImage(source.read(0, source.size), SOURCE_SHA256),
                     profile.read(0, profile.size))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return dict(bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest(),
                model=0x615, source_sha256=SOURCE_SHA256)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dol", type=Path)
    parser.add_argument("setup", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.setup, args.output)))


if __name__ == "__main__":
    main()
