"""Authored following field for 0205's staged child chain.

0205's action dispatch is three levels deep. `80156FFC` selects
`803429C4`/`803429D4` on byte `+0x582`, the entry dispatches again on
`+0x581` through `803429E4`/`803429F8`, and the leaves dispatch on
`+0x540`. The extractor finds no single class callback for the model, and
the neutral `(0,0)` entry only clears the carried velocity and halfwords,
runs the `8020BA78`/`8020BA9C`/`8020BA54` vector calls and emits.

The payload arrives in later substates: `800B2190` child allocations at
`801578F8`, `80158300`, `80158518` and `801588A4`, plus one `80088AA0`
allocation at `80159198`; the `800B22F4` velocity helper appears with
them. There is no origin row and no channel debit anywhere in the chain,
so the native expression is a free owner-following field on a grounded
move, matching the free-child treatment of the flying bosses.

The staged structure is recorded but not run: only the neutral input is
bound, and the field radius, damage, interval and timing are authored.
"""
from tools.analyze.retail_basic_shot_test import whole_body_pose

MODEL = 0x0205
CONFIG = "80342AD8"
CONTEXT_TABLE = "80342D28"
MAIN_HANDLER = 0x80156FFC
STAGE_TABLES = {"state": "803429C4/803429D4", "substate": "803429E4/803429F8",
                "leaf": "+0x540"}
CHILD_SITES = ["801578F8", "80158300", "80158518", "801588A4", "80159198"]
STATE, SELECTOR = 1, 44

AUTHORED_RADIUS = 120
AUTHORED_HEIGHT_BAND = 40
AUTHORED_TICKS = 30
AUTHORED_DAMAGE = 10
AUTHORED_INTERVAL = 6


def battle_test(model, joints, role_clips, pace, level, actions):
    if model != MODEL or level[0] < 1 or level[0] > 10000 or len(joints) < 2:
        raise ValueError("staged field needs its exact model, health and body")
    if any(actions.get(k) for k in ("refused_bindings", "open_path_boundaries", "unresolved_stores")):
        raise ValueError("staged field needs its exact installer scope")
    walk = pace["native_units_per_tick_x100"]
    if not 100 <= walk <= 1042:
        raise ValueError("staged field needs its source-relative pace")
    profiles = actions.get("profiles") or []
    if (len(profiles) != 1 or profiles[0].get("config") != CONFIG or
            profiles[0].get("context_table") != CONTEXT_TABLE or
            profiles[0].get("main_handler") != f"{MAIN_HANDLER:08X}"):
        raise ValueError("staged field needs its exact installer profile")
    neutral = []
    for group in profiles[0].get("groups", []):
        if group.get("group") != "B-shot":
            continue
        for row in group.get("rows", []):
            if (0, 0) in map(tuple, row.get("contexts", [])):
                neutral.append(row)
    if (not neutral or any(
            (row.get("state"), row.get("selector"), row.get("handler_pair"),
             row.get("class_callback_candidate")) != (STATE, SELECTOR, [61, 0], None)
            for row in neutral)):
        raise ValueError("staged field needs its exact unresolved neutral row")
    roles = {name: (g, c) for name, g, c in role_clips}
    required = ("ground-stop", "ground-travel", "air-continuation", "ground-dash-forward")
    if any(name not in roles for name in required):
        raise ValueError("staged field needs its exact locomotion clips")
    lines = [
        "GFBORG-GAMEPLAY 1",
        "# Authored following field for a staged child chain; only the neutral input is bound.",
        "variant=normal", f"health={level[0]}",
        f"movement=grounded-v1|{walk//100}.{walk%100:02d}|13|15",
        "rig-prefix=visual/body.prefix", "pose-profile=visual/body.pose", "body-record=0",
        f"idle={roles['ground-stop'][0]}:{roles['ground-stop'][1]}",
        f"walk={roles['ground-travel'][0]}:{roles['ground-travel'][1]}",
        f"air={roles['air-continuation'][0]}:{roles['air-continuation'][1]}",
        f"dash={roles['ground-dash-forward'][0]}:{roles['ground-dash-forward'][1]}",
        f"field=stage-hit|3|sphere|{AUTHORED_RADIUS}|{AUTHORED_RADIUS}|{AUTHORED_RADIUS}|"
        f"{AUTHORED_TICKS}|0|{AUTHORED_HEIGHT_BAND}|190|90|220|120",
        "field-follow=stage-hit|source",
        f"field-core-damage=stage-hit|enemies|{AUTHORED_RADIUS}|{AUTHORED_DAMAGE}|{AUTHORED_INTERVAL}",
        "move=stage-hit|b|10|grounded|28|none",
    ]
    return ("\n".join(lines) + "\n").encode("ascii"), whole_body_pose(joints), {
        "kind": "authored following field for a staged child chain; "
                "only the neutral input is bound",
        "retail_behavior_verified": False, "preview_clip": "none",
        "source_profile": {"config": CONFIG, "context_table": CONTEXT_TABLE,
                           "main_handler": f"{MAIN_HANDLER:08X}",
                           "state": STATE, "selector": SELECTOR, "button": "b",
                           "handler_pair": [61, 0],
                           "class_callback_candidate": None,
                           "stage_tables": STAGE_TABLES, "child_sites": CHILD_SITES,
                           "debit": "none in the scoped chain",
                           "unbound": "the three-level staged chain; only the neutral "
                                      "entry (velocity reset, vector calls, emit) is bound"},
        "authored": {"field_radius": AUTHORED_RADIUS, "height_band": AUTHORED_HEIGHT_BAND,
                     "ticks": AUTHORED_TICKS, "damage": AUTHORED_DAMAGE,
                     "interval": AUTHORED_INTERVAL,
                     "note": "the neutral entry carries no payload; the child allocations "
                             "live in later substates and the staged chain is not run, so "
                             "the field geometry, damage and timing are authored"},
    }
