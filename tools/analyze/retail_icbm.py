"""Authored ICBM X for the Death ICBM pair (0d06/0d07).

The private deep-0d06 receipt records the no-stock X request, the exact
terminal eligibility/life/hold phases, the supplied floor/terrain impact
to a type-73 area request, and the empty terminal geometry/damage service
as unresolved. The bodies have no motion bank. The native free static
projectile is the matching shape: a no-stock launch, a floor landing and
an authored expiry area.
"""
from tools.analyze.retail_basic_shot_test import whole_body_pose

MODELS = {0x0D06, 0x0D07}
SOURCE_RECEIPT = "deep-0d06-death-icbm.json"
PROJECTILE_SPEED = 10
PROJECTILE_RADIUS = 40
PROJECTILE_LIFETIME = 30
ORIGIN_HEIGHT = 60
BLAST_RADIUS = 300
BLAST_DAMAGE = 20
AUTHORED_PACE = 600


def battle_test(model, joints, level, actions):
    if model not in MODELS or level[0] < 1 or level[0] > 10000:
        raise ValueError("ICBM pilot needs its exact model and health")
    if any(actions.get(k) for k in ("refused_bindings", "open_path_boundaries", "unresolved_stores")):
        raise ValueError("ICBM pilot needs its exact installer scope")
    neutral = []
    for profile in actions.get("profiles", []):
        for group in profile.get("groups", []):
            if group.get("group") != "X":
                continue
            for row in group.get("rows", []):
                if (0, 0) in map(tuple, row.get("contexts", [])):
                    neutral.append(row)
    if (not neutral or any(
            (row.get("state"), row.get("selector"), row.get("handler_pair"),
             row.get("class_callback_candidate")) != (0, 45, [61, 0], None)
            for row in neutral)):
        raise ValueError("ICBM pilot needs its exact neutral X row")
    if len(joints) < 2:
        raise ValueError("ICBM pilot needs its record-zero body")
    lines = [
        "GFBORG-GAMEPLAY 1",
        "# Authored ICBM; exact no-stock X row, geometry and damage authored.",
        "variant=normal", f"health={level[0]}",
        f"movement=grounded-v1|{AUTHORED_PACE//100}.{AUTHORED_PACE%100:02d}|13|15",
        "rig-prefix=visual/body.prefix", "pose-profile=visual/body.pose", "body-record=0",
        "body-motion=static-grounded",
        f"projectile=icbm|bullet|1|{PROJECTILE_SPEED}|0|1|0|{PROJECTILE_RADIUS}|{PROJECTILE_LIFETIME}|0|land-on-floor",
        "actor-contact=icbm|ignore",
        f"blast=icbm|expiry|{BLAST_RADIUS}|{BLAST_DAMAGE}|opponent",
        "look=icbm|40|255|120|80",
        f"origin=icbm|0|0|0|{ORIGIN_HEIGHT}|fixed",
        "move=icbm|x|10|grounded|40|none",
        "free-release=icbm",
        "release=icbm|12|none|0|icbm",
    ]
    return ("\n".join(lines) + "\n").encode("ascii"), whole_body_pose(joints), {
        "kind": "authored ICBM; exact no-stock X row, geometry and damage authored",
        "retail_behavior_verified": False, "preview_clip": "none",
        "source_profile": {"row": {"state": 0, "substate": 0, "selector": 45,
                                   "handler_pair": [61, 0], "callback": None},
                           "receipt": SOURCE_RECEIPT,
                           "source": "no-stock X terminal, type-73 area request, floor/terrain impact"},
        "authored": {"speed": PROJECTILE_SPEED, "radius": PROJECTILE_RADIUS,
                     "lifetime": PROJECTILE_LIFETIME, "origin_height": ORIGIN_HEIGHT,
                     "blast_radius": BLAST_RADIUS, "damage": BLAST_DAMAGE, "pace": AUTHORED_PACE,
                     "note": "the source terminal geometry, type-73 service, damage and real "
                     "lifetime are unresolved; this is an authored bomb proxy"},
    }
