"""Emit a metadata-only API map for a retail Borg roster export.

The map records what the engine was given for each Borg: identity, movement
operands and their labelled uses, locomotion roles, the bound action row, the
capability that binds it, audio indices and the preview kind. It never copies
model, texture or audio bytes, so the output is safe to publish alongside the
portable API documentation.

Read-only on the roster; refuses to overwrite an existing output.
"""
import argparse
import json
from pathlib import Path

REVISION = 1
CAPABILITY_KEYS = (
    ("charge_field_source", "charge-field"),
    ("zeta_field_source", "zeta-field"),
    ("roach_bomb_source", "roach-bomb"),
    ("icbm_source", "icbm"),
    ("spawner_shot_source", "spawner-shot"),
    ("spawner_static_source", "spawner-static"),
    ("flying_field_source", "flying-field"),
    ("nu_field_source", "nu-field"),
    ("staged_field_source", "staged-field"),
)


def _text(path):
    return path.read_text(encoding="utf-8", errors="replace")


def _manifest(path):
    fields = {}
    for line in _text(path).splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            fields[key.strip()] = value.strip()
    return fields


def _catalog(path):
    fields = {}
    for line in _text(path).splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            fields.setdefault(key.strip(), value.strip())
    return fields


def _operands(summary):
    operands = summary.get("movement_operands") or {}
    binding = operands.get("class_installer_binding") or {}
    selected = binding.get("selected_record")
    records = operands.get("records") or []
    if not isinstance(selected, int) or not 0 <= selected < len(records):
        return None
    record = records[selected]
    return {
        "record": selected,
        "record_sha256": record.get("record_sha256"),
        "values": {key: (value or {}).get("value")
                   for key, value in sorted((record.get("operands") or {}).items())},
    }


def _actions(summary):
    out = []
    candidates = summary.get("action_candidates") or {}
    for profile in candidates.get("profiles") or []:
        for group in profile.get("groups") or []:
            rows = []
            for row in group.get("rows") or []:
                contexts = [list(map(int, context)) for context in row.get("contexts") or []]
                if [0, 0] not in contexts:
                    continue
                rows.append({"state": row.get("state"), "substate": row.get("substate"),
                             "selector": row.get("selector"),
                             "handler_pair": row.get("handler_pair"),
                             "callback": row.get("class_callback_candidate"),
                             "context_count": len(contexts)})
            if rows:
                out.append({"group": group.get("group"),
                            "config": profile.get("config"),
                            "context_table": profile.get("context_table"),
                            "main_handler": profile.get("main_handler"),
                            "neutral_rows": rows})
    return out


def _capability(summary):
    for key, name in CAPABILITY_KEYS:
        if summary.get(key) is not None:
            return name
    receipt = summary.get("basic_shot_battle_test")
    if receipt is not None:
        return receipt.get("kind") or "basic-shot"
    if summary.get("basic_melee_battle_test") is not None:
        return "basic-melee"
    if summary.get("held_fire_battle_test") is not None:
        return "held-fire"
    return None


def _audio(summary, folder):
    bindings = summary.get("sfx_bindings") or {}
    landing = bindings.get("landing_candidate") or {}
    pcm = landing.get("lab_pcm")
    return {
        "landing": {"bank": landing.get("bank"), "tone": landing.get("tone"),
                    "trigger_qualified": landing.get("trigger_qualified"),
                    "lab_pcm": pcm,
                    "pcm_present": bool(pcm and (folder / pcm).exists())},
        "other_sfx": bindings.get("other"),
        "launch_sound_authored": bool(
            (summary.get("basic_shot_battle_test") or {}).get("authored_launch_sounds")),
    }


def borg_entry(folder):
    summary = json.loads((folder / "source-index.json").read_bytes())
    manifest = _manifest(folder / "borg.manifest")
    catalog = _catalog(folder / "catalog.txt")
    pace = summary.get("lab_ground_pace") or {}
    movement = summary.get("movement_candidates") or {}
    return {
        "id": summary.get("id"),
        "name": manifest.get("name"),
        "collection_number": catalog.get("collection-number"),
        "tribe": catalog.get("tribe"),
        "hp": (summary.get("hp_by_level") or [None])[0],
        "ratings_def_sht_atk_spd": summary.get("ratings_def_sht_atk_spd"),
        "logical_joints": len(summary.get("logical_joints") or []),
        "wrapper_slots": len(summary.get("model_wrapper_slots") or []),
        "movement": {
            "candidate_types": movement.get("candidate_types"),
            "candidates": [{"config": candidate.get("config"),
                            "type": candidate.get("source_type"),
                            "flags": candidate.get("flags")} for candidate in
                           (movement.get("candidates") or [])],
            "pace": {"operand": pace.get("operand"),
                     "source_value": pace.get("source_value"),
                     "native_units_per_tick_x100": pace.get("native_units_per_tick_x100")}
                    if pace else None,
            "operands": _operands(summary),
            "jump_display": summary.get("raw_jump_display"),
            "move_display_slots": summary.get("raw_move_display_slots"),
        },
        "roles": [{"name": name, "group": group, "choice": choice}
                  for name, group, choice in (summary.get("role_preview_clips") or [])],
        "actions": _actions(summary),
        "capability": _capability(summary),
        "audio": _audio(summary, folder),
        "preview_declaration": bool(summary.get("preview_declaration")),
    }


def build(roster):
    roster = Path(roster)
    folders = sorted(path for path in roster.iterdir()
                     if path.is_dir() and (path / "source-index.json").exists())
    if not folders:
        raise ValueError("API map needs a roster export with source-index.json files")
    extracted = roster / "roster-extraction.json"
    source = json.loads(extracted.read_bytes()) if extracted.exists() else {}
    return {
        "kind": "generated Borg API map; metadata only, no model, texture or audio bytes",
        "revision": REVISION,
        "roster_revision": source.get("revision"),
        "borg_count": len(folders),
        "borgs": [borg_entry(folder) for folder in folders],
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("roster", type=Path, help="exported roster directory")
    parser.add_argument("output", type=Path, help="API map JSON to create")
    args = parser.parse_args(argv)
    if args.output.exists():
        raise SystemExit("refusing to overwrite the API map")
    data = build(args.roster)
    args.output.write_text(json.dumps(data, indent=1, sort_keys=True) + "\n", encoding="utf-8")
    print(f"{data['borg_count']} Borgs -> {args.output}")


if __name__ == "__main__":
    main()
