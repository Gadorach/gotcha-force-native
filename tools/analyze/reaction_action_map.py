"""Map raw retail collision queues to the per-model reaction attack tables.

Each character's `pl####hit.bin` block has 4-byte queues at +0xA20 and
0x50-byte steps at +0xAA0. This inventory does not bind controller input
contexts to the queues; a move needs an independently proven selector.
"""
import argparse
import hashlib
import json
from pathlib import Path

from tools.analyze import reaction_emit, reaction_installer_tables
from tools.analyze.reaction_attack_table import COUNT, entry
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage, ReadOnlyFile

MODELS = ("0615", "000a", "061e")
def _model(image, payload, model, base, installer):
    groups = []
    used = {}
    for group in range(reaction_emit.GROUPS):
        rows = reaction_emit.read_group(payload, group)
        groups.append({"group": group, "steps": [step for step, _ in rows]})
        used.update(rows)
    steps = {str(step): {"step": step, "table": index,
                         **(entry(image, base, index) if index < COUNT else {})}
             for step, index in sorted(used.items())}
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
        if not 0 < source.size <= 8 << 20:
            raise ValueError("source DOL extent")
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
        "revision": 2,
        "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": afs_sha.hexdigest(),
        "models": models,
        "notes": [
            "Each group contains up to four step bytes at +0xA20; any signed negative byte terminates.",
            "The low byte of the step halfword at +4 indexes the model's 24-byte table.",
            "Queues are not controller input contexts; see reaction-projectile-map.json for verified release selectors.",
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
