"""Authored paid following field for 0f03 Nu III.

The whole 0f01-0f03 family shares the state-2 handler `801B3824`. It pays
`8006DBE0(actor, 0, 1, 1)` and then branches on the model id at `+0x3E8`:
`0F01` spawns table child 74 through `80082824`, `0F02` calls
`800C3BE0(actor, 61)`, and every other model, `0F03` included, calls
`8016CC24(actor, 11)`.

`8016CC24` allocates through `80088AA0`, stores type 89 and key `11`,
and installs `8016CD1C` at child `+0x0C`. The initializer samples the
owner's pose, binds its opening shape, and caches area key 16. The
authored owner-following field remains a proxy, not this carrier's
shape, motion, or area lifecycle.

The exact debit and the exact child call are bound; the radius, damage,
interval, height band and stock capacity are authored.
"""
from tools.analyze.retail_basic_shot_test import whole_body_pose

MODEL = 0x0F03
CALLBACK = 0x801B372C
MAIN_HANDLER = 0x801B36F4
STATE_TABLE = "80380EE8"
SHARED_STATE2 = "801B3824"
DEBIT_SITE = "801B392C"
CHILD_CALL = "8016CC24"
CHILD_VARIANT = 11
CHILD_UPDATE = "8016CD1C"
FAMILY_PACE = 333  # the documented 0F01/0F02 family pace

AUTHORED_RADIUS = 120
AUTHORED_HEIGHT_BAND = 40
AUTHORED_TICKS = 30
AUTHORED_DAMAGE = 8
AUTHORED_INTERVAL = 6
STOCK_CAP = 1


def battle_test(model, joints, level, actions):
    if model != MODEL or level[0] < 1 or level[0] > 10000 or len(joints) != 3:
        raise ValueError("Nu III field needs its exact model, health and three-joint body")
    if any(actions.get(k) for k in ("refused_bindings", "open_path_boundaries", "unresolved_stores")):
        raise ValueError("Nu III field needs its exact installer scope")
    neutral = []
    for profile in actions.get("profiles", []):
        for group in profile.get("groups", []):
            if group.get("group") != "B-shot":
                continue
            for row in group.get("rows", []):
                if (0, 0) in map(tuple, row.get("contexts", [])):
                    neutral.append(row)
    if (not neutral or any(
            (row.get("state"), row.get("selector"), row.get("handler_pair"),
             row.get("class_callback_candidate")) != (0, 45, [61, 0], f"{CALLBACK:08X}")
            for row in neutral)):
        raise ValueError("Nu III field needs its exact neutral B row")
    lines = [
        "GFBORG-GAMEPLAY 1",
        "# Authored paid following field; exact debit and child call, authored geometry and damage.",
        "variant=normal", f"health={level[0]}",
        f"movement=grounded-v1|{FAMILY_PACE//100}.{FAMILY_PACE%100:02d}|13|15",
        "rig-prefix=visual/body.prefix", "pose-profile=visual/body.pose", "body-record=0",
        "body-motion=static-grounded",
        f"stock=f03-stock|{STOCK_CAP}|180",
        f"field=f03-hit|3|sphere|{AUTHORED_RADIUS}|{AUTHORED_RADIUS}|{AUTHORED_RADIUS}|"
        f"{AUTHORED_TICKS}|0|{AUTHORED_HEIGHT_BAND}|190|90|220|120",
        "field-follow=f03-hit|source",
        f"field-core-damage=f03-hit|enemies|{AUTHORED_RADIUS}|{AUTHORED_DAMAGE}|{AUTHORED_INTERVAL}",
        "field-debit=f03-hit|f03-stock|1",
        "move=f03-hit|b|10|grounded|28|none",
    ]
    return ("\n".join(lines) + "\n").encode("ascii"), whole_body_pose(joints), {
        "kind": "authored paid following field; exact debit and child call, "
                "geometry and damage authored",
        "retail_behavior_verified": False, "preview_clip": "none",
        "source_profile": {"callback": f"{CALLBACK:08X}", "state": 0, "selector": 45,
                           "button": "b", "handler_pair": [61, 0],
                           "main_handler": f"{MAIN_HANDLER:08X}", "state_table": STATE_TABLE,
                           "shared_state2": SHARED_STATE2, "debit_site": DEBIT_SITE,
                           "debit": "8006DBE0(actor,0,1,1) channel 0 cost 1",
                           "child_call": CHILD_CALL, "child_variant": CHILD_VARIANT,
                           "child_type": 89, "child_update": CHILD_UPDATE,
                           "child_motion": "initializer samples owner pose; active motion not bound",
                           "exit_area_key": 16,
                           "sibling_routes": {"0F01": "80082824 table child 74",
                                              "0F02": "800C3BE0(actor,61)"}},
        "authored": {"field_radius": AUTHORED_RADIUS, "height_band": AUTHORED_HEIGHT_BAND,
                     "ticks": AUTHORED_TICKS, "damage": AUTHORED_DAMAGE,
                     "interval": AUTHORED_INTERVAL, "stock_cap": STOCK_CAP,
                     "pace": FAMILY_PACE,
                     "note": "the paid following field is a proxy; type-89 shape, motion, "
                             "exit area, damage and timing are not bound"},
    }
