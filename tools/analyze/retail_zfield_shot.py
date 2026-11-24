"""Authored following-field B for the four Zeta models.

Their shared neutral B-shot row (state 0, substate 0, selector 44, callback
801487C4) ends in a child effect (800B2190, phase table 802FC7AC) rather
than an origin factory. The native field API already expresses a
following area with core damage and a paid placement, so this module binds
the exact row, channel-2 debit, body material pair and locomotion roles,
and authors the field geometry, lifetime and damage explicitly.
"""
from tools.analyze.retail_basic_shot_test import whole_body_pose

MODELS = {0x0705, 0x070B, 0x070C, 0x070D}
CALLBACK = 0x801487C4
DEBIT = 0x8006DBE0
SOURCE_CHANNEL = 2
FIELD_RADIUS = 90
FIELD_FORWARD = 80
FIELD_HEIGHT = 40
FIELD_TICKS = 30
CORE_DAMAGE = 10
CORE_INTERVAL = 6


def battle_test(model, joints, role_clips, pace, level, actions):
    if model not in MODELS or level[0] < 1 or level[0] > 10000:
        raise ValueError("Zeta field pilot needs its exact model and health")
    walk = pace["native_units_per_tick_x100"]
    if not 100 <= walk <= 1042:
        raise ValueError("Zeta field pilot needs its source-relative pace")
    if any(actions.get(k) for k in ("refused_bindings", "open_path_boundaries", "unresolved_stores")):
        raise ValueError("Zeta field pilot needs its exact installer scope")
    neutral = []
    for profile in actions.get("profiles", []):
        for group in profile.get("groups", []):
            if group.get("group") != "B-shot":
                continue
            for row in group.get("rows", []):
                if (0, 0) in map(tuple, row.get("contexts", [])):
                    neutral.append(row)
    if (not neutral or any(
            (row.get("state"), row.get("substate"), row.get("selector"), row.get("handler_pair"),
             row.get("class_callback_candidate")) != (0, 0, 44, [61, 0], f"{CALLBACK:08X}")
            for row in neutral)):
        raise ValueError("Zeta field pilot needs its exact neutral B-shot row")
    roles = {name: (g, c) for name, g, c in role_clips}
    required = ("ground-stop", "ground-travel", "air-continuation", "ground-dash-forward")
    if any(name not in roles for name in required) or len(joints) < 2:
        raise ValueError("Zeta field pilot needs its exact locomotion clips")
    lines = [
        "GFBORG-GAMEPLAY 1",
        "# Authored following-field B; exact source row/debit, field geometry and damage authored.",
        "variant=normal", f"health={level[0]}",
        f"movement=grounded-v1|{walk//100}.{walk%100:02d}|13|15",
        "rig-prefix=visual/body.prefix", "pose-profile=visual/body.pose", "body-record=0",
        "body-material=diffuse-overwrite-pair", "idle-loop=authored-bit", "walk-loop=authored-bit",
        f"idle={roles['ground-stop'][0]}:{roles['ground-stop'][1]}",
        f"walk={roles['ground-travel'][0]}:{roles['ground-travel'][1]}",
        f"air={roles['air-continuation'][0]}:{roles['air-continuation'][1]}",
        f"dash={roles['ground-dash-forward'][0]}:{roles['ground-dash-forward'][1]}",
        "stock=z-stock|1|180",
        f"field=z-hit|3|sphere|{FIELD_RADIUS}|{FIELD_RADIUS}|{FIELD_RADIUS}|{FIELD_TICKS}|"
        f"{FIELD_FORWARD}|{FIELD_HEIGHT}|190|90|220|120",
        "field-follow=z-hit|source",
        f"field-core-damage=z-hit|enemies|{FIELD_RADIUS}|{CORE_DAMAGE}|{CORE_INTERVAL}",
        "field-debit=z-hit|z-stock|1",
        "move=z-hit|b|10|grounded|28|none",
        "entry-stock=z-hit|z-stock|1",
    ]
    return ("\n".join(lines) + "\n").encode("ascii"), whole_body_pose(joints), {
        "kind": "authored following-field B; exact row/debit/body pair, field geometry and damage authored",
        "retail_behavior_verified": False, "preview_clip": "none",
        "body_binding": "strict record zero with validated diffuse-overwrite pair",
        "source_profile": {"callback": f"{CALLBACK:08X}", "selector": 44, "state": 0, "substate": 0,
                           "handler_pair": [61, 0],
                           "child": {"factory": "800B2190", "phase_table": "802FC7AC",
                                     "callbacks": ["800B26CC", "800B2CE4"]}},
        "source_debit": {"site": f"{DEBIT:08X}", "channel": SOURCE_CHANNEL, "cost": 1, "commit": True},
        "authored": {"field_radius": FIELD_RADIUS, "forward": FIELD_FORWARD, "height": FIELD_HEIGHT,
                     "ticks": FIELD_TICKS, "damage": CORE_DAMAGE, "interval": CORE_INTERVAL,
                     "stock": "one paid unit, refill 180", "note": "child is a following effect; the "
                     "source damage/area receiver remains unclaimed"},
    }
