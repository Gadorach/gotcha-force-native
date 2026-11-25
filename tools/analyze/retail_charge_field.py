"""Authored charge + following field for the shared X-charge models.

The source X callback 80156428 dispatches on +0x581 through 0x804346D8;
mode 0 runs 80156460 -> 0x8034267C -> 801564B0/801565CC/801566F0, which
drive program 0xC1 through 8006D144/8006E1AC and the 800B22F4 velocity
helper. There is no origin factory: it is a charged movement program. The
native controller's existing dash is the matching motion, and a following
field carries the authored contact area.
"""
from tools.analyze.retail_basic_shot_test import whole_body_pose

MODELS = {0x0806, 0x0809, 0x080F}
CALLBACK = 0x80156428
FIELD_RADIUS = 240
FIELD_HEIGHT = 40
FIELD_TICKS = 30
CHARGE_TICKS = 20
CORE_DAMAGE = 10
CORE_INTERVAL = 6


def battle_test(model, joints, role_clips, pace, level, actions):
    if model not in MODELS or level[0] < 1 or level[0] > 10000:
        raise ValueError("charge pilot needs its exact model and health")
    walk = pace["native_units_per_tick_x100"]
    if not 100 <= walk <= 1042:
        raise ValueError("charge pilot needs its source-relative pace")
    if any(actions.get(k) for k in ("refused_bindings", "open_path_boundaries", "unresolved_stores")):
        raise ValueError("charge pilot needs its exact installer scope")
    neutral = []
    for profile in actions.get("profiles", []):
        for group in profile.get("groups", []):
            if group.get("group") != "X-charge":
                continue
            for row in group.get("rows", []):
                if (0, 0) in map(tuple, row.get("contexts", [])):
                    neutral.append(row)
    if (not neutral or any(
            (row.get("state"), row.get("selector"), row.get("handler_pair"),
             row.get("class_callback_candidate")) != (2, 44, [61, 0], f"{CALLBACK:08X}")
            for row in neutral)):
        raise ValueError("charge pilot needs its exact neutral X-charge row")
    roles = {name: (g, c) for name, g, c in role_clips}
    required = ("ground-stop", "ground-travel", "air-continuation", "ground-dash-forward")
    if any(name not in roles for name in required) or len(joints) < 2:
        raise ValueError("charge pilot needs its exact locomotion clips")
    lines = [
        "GFBORG-GAMEPLAY 1",
        "# Authored charge with following field; exact source program, geometry and damage authored.",
        "variant=normal", f"health={level[0]}",
        f"movement=grounded-v1|{walk//100}.{walk%100:02d}|13|15",
        "rig-prefix=visual/body.prefix", "pose-profile=visual/body.pose", "body-record=0",
        f"idle={roles['ground-stop'][0]}:{roles['ground-stop'][1]}",
        f"walk={roles['ground-travel'][0]}:{roles['ground-travel'][1]}",
        f"air={roles['air-continuation'][0]}:{roles['air-continuation'][1]}",
        f"dash={roles['ground-dash-forward'][0]}:{roles['ground-dash-forward'][1]}",
        f"field=charge-hit|3|sphere|{FIELD_RADIUS}|{FIELD_RADIUS}|{FIELD_RADIUS}|{FIELD_TICKS}|0|"
        f"{FIELD_HEIGHT}|190|90|220|120",
        "field-follow=charge-hit|source",
        f"field-core-damage=charge-hit|enemies|{FIELD_RADIUS}|{CORE_DAMAGE}|{CORE_INTERVAL}",
        "move=charge-hit|x|10|grounded|28|none",
        f"move-charge=charge-hit|{CHARGE_TICKS}",
    ]
    return ("\n".join(lines) + "\n").encode("ascii"), whole_body_pose(joints), {
        "kind": "authored charge with following field; exact X program, geometry and damage authored",
        "retail_behavior_verified": False, "preview_clip": "none",
        "source_profile": {"callback": f"{CALLBACK:08X}", "state": 2, "selector": 44,
                           "handler_pair": [61, 0], "sda_base": "8043B5A0",
                           "dispatch_table": "804346D8", "state_table": "8034267C",
                           "program": "0xC1", "velocity_helper": "800B22F4",
                           "source": "charged movement program, no origin factory"},
        "authored": {"charge_ticks": CHARGE_TICKS, "field_radius": FIELD_RADIUS,
                     "height": FIELD_HEIGHT, "ticks": FIELD_TICKS, "damage": CORE_DAMAGE,
                     "interval": CORE_INTERVAL,
                     "note": "the source contact shape/damage is unclaimed; the native dash "
                     "reuses the profile dash speed and the field is authored"},
    }
