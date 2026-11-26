"""Join the source action contexts to the per-Borg reaction groups.

The source action candidate row's first context byte selects the
`pl####hit.bin` queue group; that group's step list indexes the per-Borg
24-byte reaction table. This tool reads the pilot roster's
`source-index.json` action candidates and the reaction action map, then
publishes the complete context -> group -> step -> reaction chain.
"""
import argparse
import hashlib
import json
from pathlib import Path

from tools.analyze import reaction_action_map

MODELS = ("0615", "000a", "061e")
# The installer's active action config for each pilot (the alternate profile
# is selected for Sasuke and Proto King by the model conditional).
ACTIVE_CONFIG = {"0615": "80365AC8", "000a": "802D3EE0",
                 "061e": "80344A38"}


def _actions(index, name, reaction, config):
    data = json.loads(index.read_text())
    profiles = [profile for profile in
                data.get("action_candidates", {}).get("profiles", [])
                if str(profile.get("config", "")).lower() == config.lower()]
    if len(profiles) != 1:
        raise ValueError(f"active action profile not found: {name}")
    reactions = {entry["group"]: entry for entry in reaction["groups"]}
    steps = reaction["steps"]
    actions = []
    for group in profiles[0].get("groups", []):
        contexts = []
        seen = set()
        for row in group.get("rows", []):
            for context in row.get("contexts", []):
                context = [int(value) for value in context]
                key = tuple(context)
                if key in seen:
                    continue
                seen.add(key)
                queue_group = context[0]
                if queue_group not in reactions:
                    continue
                steps_out = [steps.get(str(step)) for step in reactions[queue_group]["steps"]]
                contexts.append({"context": context, "group": queue_group,
                                 "steps": [step for step in steps_out if step]})
        actions.append({"group": group.get("group"), "mode": group.get("mode"),
                        "contexts": contexts})
    return actions


def recover(dol, afs, root):
    action_map = reaction_action_map.recover(dol, afs)
    models = {entry["model"].lower(): entry for entry in action_map["models"]}
    output = {}
    for model in MODELS:
        directory = Path(root) / "borgs" / f"gg4e08-{model}"
        index = directory / "source-index.json"
        if not index.is_file():
            raise FileNotFoundError(index)
        output[model] = {"table": models[model]["table"],
                         "actions": _actions(index, model, models[model],
                                             ACTIVE_CONFIG[model])}
    return {
        "kind": "gf-reaction-context-map",
        "revision": 1,
        "source_action_map": "reaction-action-map.json",
        "models": output,
        "notes": [
            "Each action row's first context byte is the queue group index.",
            "The group's step list is resolved through the per-Borg table.",
            "A packager can emit reaction-mode/reaction-offset from the "
            "selected context; unsupported source modes need a labelled "
            "native fallback.",
        ],
    }


def export(dol, afs, root, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("reaction context map output already exists")
    payload = recover(dol, afs, root)
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
    parser.add_argument("root", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.root, args.output),
                     sort_keys=True))


if __name__ == "__main__":
    main()
