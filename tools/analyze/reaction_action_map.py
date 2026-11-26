"""Map retail action groups to the shared reaction attack table.

Each character's `pl####hit.bin` block has a 4-byte queue at +0xA20 (first
byte = step index, 0xFF terminates) and 0x50-byte steps at +0xAA0; step
+4 is the 24-byte attack-table index. The artifact records, for the pilot
Borgs, each group's step list and each step's reaction fields.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from tools.analyze import reaction_installer_tables
from tools.analyze.reaction_attack_table import COUNT, entry
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage, ReadOnlyFile

MODELS = ("0615", "000a", "061e")
QUEUE = 0xA20
STEPS = 0xAA0
STEP_SIZE = 0x50
GROUPS = 32
MAX_STEP = 63


def _model(image, payload, model, base, installer):
    groups = []
    used = []
    for group in range(GROUPS):
        raw = payload[QUEUE + group * 4:QUEUE + group * 4 + 4]
        steps = []
        for byte in raw:
            if byte == 0xFF:
                break
            if byte not in steps:
                steps.append(byte)
            if byte not in used:
                used.append(byte)
        groups.append({"group": group, "steps": steps})
    steps = {}
    for step in sorted(used):
        if step > MAX_STEP:
            raise ValueError(f"step index outside budget: {step}")
        row = payload[STEPS + step * STEP_SIZE:STEPS + (step + 1) * STEP_SIZE]
        index = struct.unpack_from(">H", row, 4)[0]
        steps[str(step)] = ({"step": step, "table": index,
                              **entry(image, base, index)}
                            if index < COUNT else {"step": step, "table": index})
    return {
        "model": model,
        "installer": f"{installer:08X}",
        "table": f"{base:08X}",
        "hit_block_sha256": hashlib.sha256(payload).hexdigest(),
        "groups": groups,
        "steps": steps,
    }


def recover(dol_path, afs_path):
    with ReadOnlyFile(dol_path) as source:
        image = DolImage(source.read(0, source.size), SOURCE_SHA256)
    with Path(afs_path).open("rb") as stream:
        archive = RetailArchive(stream)
        models = []
        for model in MODELS:
            name = f"pl{model}hit.bin".encode("ascii")
            if name not in archive.names:
                raise ValueError(f"missing hit block: {model}")
            index = archive.names.index(name)
            payload = archive.payload(index, 1 << 20)
            if len(payload) != 7840:
                raise ValueError(f"unexpected hit block extent: {model}")
            installer, base = reaction_installer_tables.model_table(
                image, int(model, 16))
            models.append(_model(image, payload, model, base, installer))
        afs_sha = hashlib.sha256()
        # Record identity without copying the archive.
        with Path(afs_path).open("rb") as stream:
            for chunk in iter(lambda: stream.read(1 << 20), b""):
                afs_sha.update(chunk)
    return {
        "kind": "gf-reaction-action-map",
        "revision": 1,
        "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": afs_sha.hexdigest(),
        "models": models,
        "notes": [
            "Group bytes are the 4-byte queue at +0xA20; 0xFF terminates.",
            "Step +4 indexes the shared 24-byte table at 80354160.",
            "The queue group is selected by the source action byte; mapping "
            "that byte to a packaged move is still open.",
        ],
    }


def export(dol_path, afs_path, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("reaction action map output already exists")
    payload = recover(dol_path, afs_path)
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode()
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"bytes": len(raw), "sha256": hashlib.sha256(raw).hexdigest(),
            "models": len(payload["models"])}


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("dol", type=Path)
    parser.add_argument("afs", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.output), sort_keys=True))


if __name__ == "__main__":
    main()
