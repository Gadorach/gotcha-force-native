"""Read-only export of qualified G-Red motion operands and the MVP stage's grids."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from tools.analyze.gred_movement import CONFIG, qualify_setup
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.native_battle_mvp import blocks, read
from tools.extract.source_image import DolImage, ReadOnlyFile


STAGE_SHA256 = "bb9699d30c62dc46674f15043911ccae70b5c4ecf38adcfde723c858b4db2e9c"


def encode(image, setup):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified G-Red motion source")
    data, config = qualify_setup(image, setup)
    if config[1] or int.from_bytes(config[8:12], "big") != 12:
        raise ValueError("unreviewed normal movement class")
    raw = bytearray(372)
    struct.pack_into(">IHHIII", raw, 0, 0x47464D4D, 8, 0x615, 0xE2B4439A, len(raw), 12)
    raw[20] = config[1]
    raw[32:56] = data[0x18:0x30]
    raw[56:66] = image.read(CONFIG + 0xC0, 10)
    raw[66:74] = image.read(0x80433838, 8)
    raw[74:82] = image.read(0x80433858, 8)
    raw[82:90] = image.read(0x80433860, 8)
    raw[92:104] = setup[672 + 16 * 12:672 + 17 * 12]  #qualified constructor rank16
    raw[104:124] = config[0x44:0x58]
    for index, address in enumerate((0x804375D0, 0x804375EC, 0x804375F0, 0x80437078)):
        raw[124 + index * 4:128 + index * 4] = image.read(address, 4)
    raw[140:152] = data[0x30:0x3C]
    raw[152:200] = data[0x44:0x74]
    raw[200:212] = data[0x74:0x80]
    raw[212:260] = data[0xF0:0x120]
    raw[260:280] = data[0x120:0x134]
    raw[280:296] = image.read(0x802D38C4, 16)
    for index, address in enumerate((0x804375B8, 0x804375BC, 0x804375C0)):
        raw[296 + index * 4:300 + index * 4] = image.read(address, 4)
    for index, address in enumerate((0x804374F0, 0x80437508, 0x8043750C)):
        raw[308 + index * 4:312 + index * 4] = image.read(address, 4)
    for index, address in enumerate((0x80437568, 0x8043756C, 0x80437524, 0x80437528)):
        raw[320 + index * 4:324 + index * 4] = image.read(address, 4)
    raw[336:338] = data[0x42:0x44]
    raw[338] = data[0x3E]
    raw[340:344] = image.read(0x80437534, 4)
    raw[344:348] = image.read(0x80437558, 4)
    raw[348:352] = config[0xAC:0xB0]
    raw[352:356] = image.read(0x804375F8, 4)
    raw[356:360] = image.read(0x804375D4, 4)
    raw[360:364] = image.read(0x8043751C, 4)
    raw[364:368] = image.read(0x8043762C, 4)
    raw[368:372] = image.read(0x80437648, 4)
    return bytes(raw)


def export_profile(dol, setup, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("motion profile already exists")
    with ReadOnlyFile(dol) as source, ReadOnlyFile(setup) as profile:
        if not 0 < source.size <= 8 << 20:
            raise ValueError("source DOL extent")
        if profile.size != 1064:
            raise ValueError("setup extent")
        raw = encode(DolImage(source.read(0, source.size), SOURCE_SHA256),
                     profile.read(0, profile.size))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return dict(bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest())


def export_grids(bundle, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("stage grid output already exists")
    raw = read(bundle, 32 << 20)
    if hashlib.sha256(raw).hexdigest() != STAGE_SHA256:
        raise ValueError("unqualified MVP stage473 bundle")
    parts = blocks(raw)
    if len(parts) != 4:
        raise ValueError("stage slot count")
    files = {}
    for name, data in zip(("floor.stih", "secondary.stih", "tertiary.stih"), parts[1:]):
        if (not 44 <= len(data) <= 16 << 20 or data[:4] != b"STIH"
                or struct.unpack_from(">I", data, 4)[0] != len(data)):
            raise ValueError("source stage grid extent")
        files[name] = data
    output.mkdir(parents=True, exist_ok=False)
    manifest = {"kind": "three original static stage query grids, not contact/actor readiness",
                "source_sha256": STAGE_SHA256, "files": {}}
    for name, data in files.items():
        with (output / name).open("xb") as stream:
            stream.write(data)
        manifest["files"][name] = dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest())
    with (output / "manifest.json").open("x") as stream:
        json.dump(manifest, stream, indent=2)
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    profile = commands.add_parser("profile")
    profile.add_argument("dol", type=Path)
    profile.add_argument("setup", type=Path)
    profile.add_argument("output", type=Path)
    grids = commands.add_parser("stage-grids")
    grids.add_argument("bundle", type=Path)
    grids.add_argument("output", type=Path)
    args = parser.parse_args()
    result = (export_profile(args.dol, args.setup, args.output) if args.command == "profile"
              else export_grids(args.bundle, args.output))
    print(json.dumps(result))


if __name__ == "__main__":
    main()
