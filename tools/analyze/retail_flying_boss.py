"""Authored flying-boss field for the four slot-activating models.

`0e01`-`0e04` share the cruise handler `800633C8` (selector 27) and the
boost handler `80063644` (selector 28). The movement-operand schema labels
every read: meter cap `+0x004`, refill `+0x008`, refill delay `+0x006`,
cruise planar target `+0x02C`, cruise pitch term `+0x044`, boost planar
`+0x058`, planar change `+0x05C`, boost clock `+0x064` and vertical bases
`+0x068`/`+0x070`.

Their neutral B row scans four bits and activates type-36 carried children
through `8013212C`. The child update `801322DC` hands its shape to
`8008AFF0`, which is the same collision handoff the Roach and ICBM pilots
use, so the contact exists; only the damage amount is unrecovered because
the HP writer `8003D358` is not statically reachable.

The native `cruise-boost` pilot carries the exact meter and cruise
operands with `body-motion=static-cruise`, and an owner-following field
carries the recovered corner contact. Radius, damage, interval, height and
boost cost per tick are authored; the corner layout (±132/±232) and the
meter operands are recorded in the receipt.
"""
from tools.analyze.retail_basic_shot_test import whole_body_pose

# 0e04 shares every operand and row shape but its posed body exceeds the
# native combat camera domain, so it stays indexed until that view limit is
# settled.
MODELS = {0x0E01, 0x0E02, 0x0E03}
CALLBACKS = {0x0E01: 0x801D5964, 0x0E02: 0x801D75A0,
             0x0E03: 0x801D5ECC, 0x0E04: 0x801D66CC}
# Recovered operands shared by all four; 0x02C differs per model.
EXPECTED = {"004": 10000, "006": 0, "008": 2000, "044": 2.0, "058": 40.0,
            "05C": 0.0, "064": 30.0, "068": 0.0, "070": 0.0}
CRUISE = {0x0E01: 10.0, 0x0E02: 10.0, 0x0E03: 0.0, 0x0E04: 10.0}
CORNER_LAYOUT = [(0, 132, 232), (0, -132, -232), (0, 132, -232), (0, -132, 232)]

AUTHORED_HEIGHT = 50
AUTHORED_COST = 1
AUTHORED_RADIUS = 240
AUTHORED_HEIGHT_BAND = 40
AUTHORED_TICKS = 30
AUTHORED_DAMAGE = 10
AUTHORED_INTERVAL = 6
MIN_WALK = 40  # native grounded-v1 floor, used when the recovered target is 0


def walk_for(model):
    """Native target from the shared pace normalisation, floored at 0.40."""
    return max(MIN_WALK, int(CRUISE[model] * 500 / 12 + 0.5))


def battle_test(model, joints, role_clips, operands, level, actions):
    if model not in MODELS or level[0] < 1 or level[0] > 10000 or len(joints) < 2:
        raise ValueError("flying boss field needs its exact model, health and body")
    if any(actions.get(k) for k in ("refused_bindings", "open_path_boundaries", "unresolved_stores")):
        raise ValueError("flying boss field needs its exact installer scope")
    if operands is None:
        raise ValueError("flying boss field needs the movement operands")
    binding = operands.get("class_installer_binding") or {}
    selected = binding.get("selected_record")
    if binding.get("status") != "one-static-slot-0-record" or type(selected) is not int \
            or not 0 <= selected < len(operands.get("records") or []):
        raise ValueError("flying boss field needs the bound slot-0 record")
    values = {key: (record.get("value") if record else None)
              for key, record in operands["records"][selected]["operands"].items()}
    if any(values.get(key) != want for key, want in EXPECTED.items()):
        raise ValueError("flying boss field needs its exact cruise and meter operands")
    if values.get("02C") != CRUISE[model]:
        raise ValueError("flying boss field needs its exact cruise planar target")
    neutral = []
    for profile in actions.get("profiles", []):
        for group in profile.get("groups", []):
            if group.get("group") != "B-shot":
                continue
            for row in group.get("rows", []):
                if (0, 0) in map(tuple, row.get("contexts", [])):
                    neutral.append(row)
    callback = CALLBACKS[model]
    if (not neutral or any(
            (row.get("state"), row.get("selector"), row.get("handler_pair"),
             row.get("class_callback_candidate")) != (0, 44, [61, 0], f"{callback:08X}")
            for row in neutral)):
        raise ValueError("flying boss field needs its exact neutral B row")
    walk = walk_for(model)
    lines = [
        "GFBORG-GAMEPLAY 1",
        "# Authored flying-boss field; exact cruise/meter operands, authored geometry and damage.",
        "variant=normal", f"health={level[0]}",
        f"movement=grounded-v1|{walk//100}.{walk%100:02d}|13|15",
        "rig-prefix=visual/body.prefix", "pose-profile=visual/body.pose", "body-record=0",
        "body-motion=static-cruise",
        f"cruise-boost={AUTHORED_HEIGHT}|{EXPECTED['004']}|{AUTHORED_COST}|"
        f"{EXPECTED['008']}|{EXPECTED['006']}|{int(EXPECTED['064'])}",
        f"field=wing-hit|3|sphere|{AUTHORED_RADIUS}|{AUTHORED_RADIUS}|{AUTHORED_RADIUS}|"
        f"{AUTHORED_TICKS}|0|{AUTHORED_HEIGHT_BAND}|190|90|220|120",
        "field-follow=wing-hit|source",
        f"field-core-damage=wing-hit|enemies|{AUTHORED_RADIUS}|{AUTHORED_DAMAGE}|{AUTHORED_INTERVAL}",
        "move=wing-hit|b|10|airborne|28|none",
    ]
    return ("\n".join(lines) + "\n").encode("ascii"), whole_body_pose(joints), {
        "kind": "authored flying-boss field; exact cruise/meter operands, "
                "geometry and damage authored",
        "retail_behavior_verified": False, "preview_clip": "none",
        "source_profile": {"callback": f"{callback:08X}", "state": 0, "selector": 44,
                           "button": "b", "handler_pair": [61, 0],
                           "cruise_handler": "800633C8", "cruise_selector": 27,
                           "boost_handler": "80063644", "boost_selector": 28,
                           "child_allocator": "8013212C", "child_update": "801322DC",
                           "shape_handoff": "8008AFF0", "debit": "none in the scoped state",
                           "operands": {key: EXPECTED[key] for key in sorted(EXPECTED)}
                           | {"02C": CRUISE[model]},
                           "corner_layout": [list(p) for p in CORNER_LAYOUT],
                           "walk": walk,
                           **({"walk_authored": True, "recovered_cruise": CRUISE[model]}
                              if CRUISE[model] == 0.0 else {})},
        "authored": {"field_radius": AUTHORED_RADIUS, "height_band": AUTHORED_HEIGHT_BAND,
                     "ticks": AUTHORED_TICKS, "damage": AUTHORED_DAMAGE,
                     "interval": AUTHORED_INTERVAL, "cruise_height": AUTHORED_HEIGHT,
                     "boost_cost_per_tick": AUTHORED_COST,
                     "note": "the type-36 child contact exists (shape handoff 8008AFF0) "
                             "but its damage amount is unrecovered; radius, damage, "
                             "interval, height and boost cost are authored"},
    }
