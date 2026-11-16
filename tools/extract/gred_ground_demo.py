"""Prepare a new, local source-ground lab pack; no inputs are modified."""
import argparse
import hashlib
import json
from pathlib import Path

from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.gred_locomotion import recover as programs, recover_height
from tools.extract.gred_motion import encode as motion, STAGE_SHA256
from tools.extract.gred_movement_input import encode as inputs
from tools.extract.native_battle_mvp import blocks, read
from tools.extract.source_image import DolImage, ReadOnlyFile


def prepare(dol, setup, stage, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("source-ground demo output already exists")
    with ReadOnlyFile(dol) as source, ReadOnlyFile(setup) as profile:
        if profile.size != 1064:
            raise ValueError("setup extent")
        image = DolImage(source.read(0, source.size), SOURCE_SHA256)
        setup_bytes = profile.read(0, profile.size)
    raw = read(stage, 32 << 20)
    if hashlib.sha256(raw).hexdigest() != STAGE_SHA256:
        raise ValueError("qualified stage473 bundle required")
    parts = blocks(raw)
    if len(parts) != 4 or any(p[:4] != b"STIH" for p in parts[1:]):
        raise ValueError("three original stage query grids required")
    files = {"locomotion.gfmp": programs(image), "pose-height.gfhc": recover_height(image),
             "movement.gfmi": inputs(image, setup_bytes), "motion.gfmm": motion(image, setup_bytes),
             "floor.stih": parts[1], "secondary.stih": parts[2], "tertiary.stih": parts[3]}
    output.mkdir(parents=True, exist_ok=False)
    manifest = {"kind": "source ground lab operands; not a full original controller",
                "source_dol_sha256": SOURCE_SHA256, "stage_bundle_sha256": STAGE_SHA256,
                "files": {}}
    for name, data in files.items():
        with (output/name).open("xb") as stream:
            stream.write(data)
        manifest["files"][name] = {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}
    with (output/"manifest.json").open("x") as stream:
        json.dump(manifest, stream, indent=2)
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dol", type=Path)
    parser.add_argument("setup", type=Path)
    parser.add_argument("stage", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(json.dumps(prepare(args.dol, args.setup, args.stage, args.output)))


if __name__ == "__main__":
    main()
