"""Authored single-shot B for 0f04 from the shared 80-byte child.

The B state machine (callback 801F0EC4, state table 803A15B8) spawns row
23 through 800853EC. Row 23 carries speed 25.0 (+0x2C) and timer 60.0
(+0x10), joint 25 in the child rig, and no visible channel debit. The
body has no motion bank. The native free static projectile keeps the exact
speed/lifetime and authors the origin, look and damage.
"""
from tools.analyze.retail_basic_shot_test import whole_body_pose

MODEL = 0x0F04
CALLBACK = 0x801F0EC4
CONFIG = "803A15D8"
CONTEXT_TABLE = "803A17B0"
MAIN_HANDLER = "801F0E8C"
ROW_KEY = 23
ROW_SPEED = 25.0
ROW_LIFETIME = 60
AUTHORED_DAMAGE = 8
AUTHORED_RADIUS = 16
ORIGIN_JOINT = 2
ORIGIN_HEIGHT = 60
AUTHORED_PACE = 600


def battle_test(model, joints, level, actions):
    if model != MODEL or level[0] < 1 or level[0] > 10000:
        raise ValueError("static spawner shot needs its exact model and health")
    if any(actions.get(k) for k in ("refused_bindings", "open_path_boundaries", "unresolved_stores")):
        raise ValueError("static spawner shot needs its exact installer scope")
    profile = next((p for p in actions.get("profiles", []) if p.get("config") == CONFIG), None)
    if (profile is None or profile.get("context_table") != CONTEXT_TABLE or
            profile.get("main_handler") != MAIN_HANDLER):
        raise ValueError("static spawner shot needs its exact installer profile")
    neutral = []
    for group in profile.get("groups", []):
        if group.get("group") != "B-shot":
            continue
        for row in group.get("rows", []):
            if (0, 0) in map(tuple, row.get("contexts", [])):
                neutral.append(row)
    if (not neutral or any(
            (row.get("state"), row.get("selector"), row.get("handler_pair"),
             row.get("class_callback_candidate")) != (0, 44, [61, 0], f"{CALLBACK:08X}")
            for row in neutral)):
        raise ValueError("static spawner shot needs its exact neutral B-shot row")
    if len(joints) <= ORIGIN_JOINT:
        raise ValueError("static spawner shot needs its record-zero body")
    lines = [
        "GFBORG-GAMEPLAY 1",
        "# Authored 80-byte spawner hitbox; exact key/speed, origin/look/damage authored.",
        "variant=normal", f"health={level[0]}",
        f"movement=grounded-v1|{AUTHORED_PACE//100}.{AUTHORED_PACE%100:02d}|13|15",
        "rig-prefix=visual/body.prefix", "pose-profile=visual/body.pose", "body-record=0",
        "body-motion=static-grounded",
        f"projectile=spawn|bullet|1|{ROW_SPEED:g}|0|0|0|{AUTHORED_RADIUS}|{ROW_LIFETIME}|"
        f"{AUTHORED_DAMAGE}|ignore-floor",
        f"origin=spawn|{ORIGIN_JOINT}|0|0|{ORIGIN_HEIGHT}|target-height",
        "look=spawn|28|255|170|90",
        "move=spawn-shot|b|10|grounded|28|none",
        "free-release=spawn-shot",
        "release=spawn-shot|12|none|0|spawn",
    ]
    return ("\n".join(lines) + "\n").encode("ascii"), whole_body_pose(joints), {
        "kind": "authored 80-byte spawner hitbox; exact key/speed, origin/look/damage authored",
        "retail_behavior_verified": False, "preview_clip": "none",
        "source_profile": {"config": CONFIG, "context_table": CONTEXT_TABLE,
                           "main_handler": MAIN_HANDLER, "callback": f"{CALLBACK:08X}",
                           "state": 0, "selector": 44, "handler_pair": [61, 0],
                           "row": {"key": ROW_KEY, "speed": ROW_SPEED, "lifetime": ROW_LIFETIME},
                           "child": {"factory": "800853EC", "state_table": "803A15B8"},
                           "origin": "animation-sampled child transform",
                           "debit": "no visible channel debit in the scoped state"},
        "authored": {"damage": AUTHORED_DAMAGE, "radius": AUTHORED_RADIUS,
                     "origin_joint": ORIGIN_JOINT, "origin_height": ORIGIN_HEIGHT,
                     "note": "the child asset is in the shared effect archive and the "
                     "source placement is animation-sampled; the chest origin, look and "
                     "damage are authored"},
    }
