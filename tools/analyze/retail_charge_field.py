"""Authored charge/stance + following field for the shared charge models.

The source X callback 80156428 dispatches on +0x581 through 0x804346D8;
mode 0 runs 80156460 -> 0x8034267C -> 801564B0/801565CC/801566F0, which
drive program 0xC1 through 8006D144/8006E1AC and the 800B22F4 velocity
helper. There is no origin factory: it is a charged movement program. The
native controller's existing dash is the matching motion, and a following
field carries the authored contact area.

0004 Acceleration Ninja is the steerable stance, not the 0xC1 program.
Its neutral X row dispatches 80162418 -> 8034C72C (byte +0x582) ->
80162780 -> 8034C75C (byte +0x581) -> 801627C8 -> 8034C770 (byte
+0x540). The entry leaf 80162804 sets the forward speed from the model
movement operand at config+0x2C and starts the steering clock; the
recovered controller audit records the two actor-bound contact shapes and
their animation-progress activation. The native dash plus a following
field is the same actor-bound shape, and its geometry/damage stay
authored because the shapes are model-local, not in the shared bank.
"""
from tools.analyze.retail_basic_shot_test import whole_body_pose

# model -> route. group/state/selector/callback are the exact neutral row;
# every other key is receipt metadata.
ROUTES = {
    0x0806: {"group": "X-charge", "state": 2, "selector": 44, "button": "x",
             "callback": 0x80156428, "kind": "shared X charge program",
             "profile": {"sda_base": "8043B5A0", "dispatch_table": "804346D8",
                         "state_table": "8034267C", "program": "0xC1"}},
    0x0809: {"group": "X-charge", "state": 2, "selector": 44, "button": "x",
             "callback": 0x80156428, "kind": "shared X charge program",
             "profile": {"sda_base": "8043B5A0", "dispatch_table": "804346D8",
                         "state_table": "8034267C", "program": "0xC1"}},
    0x080F: {"group": "X-charge", "state": 2, "selector": 44, "button": "x",
             "callback": 0x80156428, "kind": "shared X charge program",
             "profile": {"sda_base": "8043B5A0", "dispatch_table": "804346D8",
                         "state_table": "8034267C", "program": "0xC1"}},
    # The deeper neutral-B state machines share the native charge+field
    # shape: their callbacks dispatch through nested state tables and end
    # in the same 800B22F4 velocity helper rather than an origin factory.
    0x080A: {"group": "B-shot", "state": 0, "selector": 44, "button": "b",
             "callback": 0x801C04E8, "kind": "shared B charge program",
             "profile": {"program": "0xC1"}},
    0x0308: {"group": "B-shot", "state": 0, "selector": 44, "button": "b",
             "callback": 0x8010A508, "kind": "shared B charge program",
             "profile": {"program": "0xC1"}},
    0x0004: {"group": "X", "state": 2, "selector": 44, "button": "x",
             "callback": 0x80162780, "kind": "steerable stance",
             "source": "steerable stance: the entry leaf sets forward speed from "
                       "the model operand and allocates its actor-bound child",
             "profile": {"state_table": "8034C72C", "substate_table": "8034C75C",
                         "leaf_table": "8034C770", "entry_leaf": "80162804",
                         "speed_source": "2 x the model movement operand at config+0x2C",
                         "contact": "two model-local actor-bound shapes, "
                                    "activated by animation progress"}},
}
MODELS = {m for m, r in ROUTES.items() if r["button"] == "x" and r["group"] == "X-charge"}
B_MODELS = {m: r["callback"] for m, r in ROUTES.items() if r["button"] == "b"}
ALL_MODELS = set(ROUTES)

FIELD_RADIUS = 240
FIELD_HEIGHT = 40
FIELD_TICKS = 30
CHARGE_TICKS = 20
CORE_DAMAGE = 10
CORE_INTERVAL = 6


def battle_test(model, joints, role_clips, pace, level, actions):
    if model not in ROUTES or level[0] < 1 or level[0] > 10000:
        raise ValueError("charge pilot needs its exact model and health")
    route = ROUTES[model]
    callback, state, selector = route["callback"], route["state"], route["selector"]
    group_name, button = route["group"], route["button"]
    walk = pace["native_units_per_tick_x100"]
    if not 100 <= walk <= 1042:
        raise ValueError("charge pilot needs its source-relative pace")
    if any(actions.get(k) for k in ("refused_bindings", "open_path_boundaries", "unresolved_stores")):
        raise ValueError("charge pilot needs its exact installer scope")
    neutral = []
    for profile in actions.get("profiles", []):
        for group in profile.get("groups", []):
            if group.get("group") != group_name:
                continue
            for row in group.get("rows", []):
                if (0, 0) in map(tuple, row.get("contexts", [])):
                    neutral.append(row)
    if (not neutral or any(
            (row.get("state"), row.get("selector"), row.get("handler_pair"),
             row.get("class_callback_candidate")) != (state, selector, [61, 0], f"{callback:08X}")
            for row in neutral)):
        raise ValueError("charge pilot needs its exact neutral charge row")
    roles = {name: (g, c) for name, g, c in role_clips}
    required = ("ground-stop", "ground-travel", "air-continuation", "ground-dash-forward")
    if any(name not in roles for name in required) or len(joints) < 2:
        raise ValueError("charge pilot needs its exact locomotion clips")
    lines = [
        "GFBORG-GAMEPLAY 1",
        f"# Authored {route['kind']} with following field; exact source program, "
        "geometry and damage authored.",
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
        f"move=charge-hit|{button}|10|grounded|28|none",
        f"move-charge=charge-hit|{CHARGE_TICKS}",
    ]
    note = ("the source contact shape/damage is unclaimed; the native dash "
            "reuses the profile dash speed and the field is authored")
    if route["kind"] == "steerable stance":
        note = ("the source sets forward speed from its own movement operand and "
                "activates model-local contact shapes by animation progress; the "
                "native dash and following field are authored")
    return ("\n".join(lines) + "\n").encode("ascii"), whole_body_pose(joints), {
        "kind": f"authored {route['kind']} with following field; exact "
                f"{button.upper()} program, geometry and damage authored",
        "retail_behavior_verified": False, "preview_clip": "none",
        "source_profile": {"callback": f"{callback:08X}", "state": state, "selector": selector,
                           "button": button, "handler_pair": [61, 0],
                           "velocity_helper": "800B22F4",
                           "source": route.get(
                               "source", "charged movement/state program, no origin factory"),
                           **route["profile"]},
        "authored": {"charge_ticks": CHARGE_TICKS, "field_radius": FIELD_RADIUS,
                     "height": FIELD_HEIGHT, "ticks": FIELD_TICKS, "damage": CORE_DAMAGE,
                     "interval": CORE_INTERVAL, "note": note},
    }
