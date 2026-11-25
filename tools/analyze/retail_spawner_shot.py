"""Authored two-shot B for 010a from the shared 80-byte child.

The source release pays one channel-2 unit and the state machine
80199490 -> 8036FB48 spawns rows 11 and 12 through 800853EC. Each row is
a moving hitbox: joint 25 (a child-rig index), speed 25.0 (+0x2C), timer
60.0 (+0x10). The child asset is in the shared effect archive and its
placement is animation-sampled, so the native projectile uses an authored
chest origin, look and damage while keeping the exact speed, lifetime,
order and debit.
"""
from tools.analyze.retail_basic_shot_test import whole_body_pose

MODEL = 0x010A
CALLBACK = 0x80199490
CONFIG = "8036FD68"
CONTEXT_TABLE = "8036FFB8"
MAIN_HANDLER = "8019889C"
ROWS = ((11, 25.0, 60), (12, 25.0, 60))
STOCK_CAP = 1
AUTHORED_DAMAGE = 8
AUTHORED_RADIUS = 16
ORIGIN_JOINT = 12
ORIGIN_HEIGHT = 60


def battle_test(model, joints, role_clips, pace, level, actions):
    if model != MODEL or level[0] < 1 or level[0] > 10000:
        raise ValueError("spawner shot pilot needs its exact model and health")
    walk = pace["native_units_per_tick_x100"]
    if not 100 <= walk <= 1042:
        raise ValueError("spawner shot pilot needs its source-relative pace")
    if any(actions.get(k) for k in ("refused_bindings", "open_path_boundaries", "unresolved_stores")):
        raise ValueError("spawner shot pilot needs its exact installer scope")
    profile = next((p for p in actions.get("profiles", []) if p.get("config") == CONFIG), None)
    if (profile is None or profile.get("context_table") != CONTEXT_TABLE or
            profile.get("main_handler") != MAIN_HANDLER):
        raise ValueError("spawner shot pilot needs its exact installer profile")
    neutral = []
    for group in profile.get("groups", []):
        if group.get("group") != "B-shot":
            continue
        for row in group.get("rows", []):
            if (0, 0) in map(tuple, row.get("contexts", [])):
                neutral.append(row)
    if (not neutral or any(
            (row.get("state"), row.get("selector"), row.get("handler_pair"),
             row.get("class_callback_candidate")) != (2, 44, [61, 0], f"{CALLBACK:08X}")
            for row in neutral)):
        raise ValueError("spawner shot pilot needs its exact neutral B-shot row")
    roles = {name: (g, c) for name, g, c in role_clips}
    required = ("ground-stop", "ground-travel", "air-continuation", "ground-dash-forward")
    if any(name not in roles for name in required) or len(joints) <= ORIGIN_JOINT:
        raise ValueError("spawner shot pilot needs its exact locomotion clips and chest joint")
    lines = [
        "GFBORG-GAMEPLAY 1",
        "# Authored two-shot moving hitbox; exact rows/debit, origin/look/damage authored.",
        "variant=normal", f"health={level[0]}",
        f"movement=grounded-v1|{walk//100}.{walk%100:02d}|13|15",
        "rig-prefix=visual/body.prefix", "pose-profile=visual/body.pose", "body-record=0",
        f"idle={roles['ground-stop'][0]}:{roles['ground-stop'][1]}",
        f"walk={roles['ground-travel'][0]}:{roles['ground-travel'][1]}",
        f"air={roles['air-continuation'][0]}:{roles['air-continuation'][1]}",
        f"dash={roles['ground-dash-forward'][0]}:{roles['ground-dash-forward'][1]}",
        f"stock=spawn-stock|{STOCK_CAP}|180",
    ]
    for index, (key, speed, lifetime) in enumerate(ROWS):
        name = "spawn-a" if index == 0 else f"spawn-{chr(ord('a') + index)}"
        lines.extend((
            f"projectile={name}|bullet|1|{speed:g}|0|0|0|{AUTHORED_RADIUS}|{lifetime}|{AUTHORED_DAMAGE}|ignore-floor",
            f"origin={name}|{ORIGIN_JOINT}|0|0|{ORIGIN_HEIGHT}|target-height",
            f"look={name}|28|255|170|90",
        ))
    lines.extend((
        "move=spawn-shot|b|10|grounded|28|none",
        "entry-stock=spawn-shot|spawn-stock|1",
        "release=spawn-shot|12|spawn-stock|1|spawn-a",
        "release-followup=spawn-shot|12|spawn-b",
    ))
    return ("\n".join(lines) + "\n").encode("ascii"), whole_body_pose(joints), {
        "kind": "authored two-shot moving hitbox; exact rows/debit, origin/look/damage authored",
        "retail_behavior_verified": False, "preview_clip": "none",
        "source_profile": {"config": CONFIG, "context_table": CONTEXT_TABLE,
                           "main_handler": MAIN_HANDLER, "callback": f"{CALLBACK:08X}",
                           "state": 2, "selector": 44, "handler_pair": [61, 0],
                           "rows": [{"key": key, "speed": speed, "lifetime": lifetime}
                                    for key, speed, lifetime in ROWS],
                           "child": {"factory": "80084600", "state_table": "802DA018"},
                           "origin": "animation-sampled child transform"},
        "source_debit": {"site": "8006DBE0", "channel": 2, "cost": 1, "commit": True},
        "authored": {"damage": AUTHORED_DAMAGE, "radius": AUTHORED_RADIUS,
                     "origin_joint": ORIGIN_JOINT, "origin_height": ORIGIN_HEIGHT,
                     "stock": "one paid pair, refill 180",
                     "note": "the child asset is in the shared effect archive and the "
                     "source placement is animation-sampled; the chest origin, look and "
                     "damage are authored"},
    }
