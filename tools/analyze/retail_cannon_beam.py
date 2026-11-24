"""Authored charge beam for Ultimate Cannon's X-charge action.

The user-observed retail shape is a beam larger than the caster, fired
horizontally on the ground from a charged X. Its X-charge leaf requests two
programs and reaches no origin factory or paid release, so the beam length,
cadence, damage and stock cost here are authored.
"""
from hashlib import sha256

from tools.analyze.retail_projectile_origins import origin_tables  # noqa: F401  (documented scope)


MODEL = 0x0C04
CALLBACK = 0x800B0F18
SPAN = 0x300
POSE = (4, 0)
PROGRAM_SITES = (0x800B1024, 0x800B10B8)


def verify_source(dol, actions):
    if sha256(dol.read(CALLBACK, SPAN, "text")).hexdigest() != _digest(dol):
        raise ValueError("cannon beam callback signature")
    rows = []
    for profile in actions["profiles"]:
        rows.extend(r for g in profile["groups"] if g["group"] == "X-charge" for r in g["rows"]
                    if (0, 0) in map(tuple, r["contexts"]))
    if (len(rows) != 1 or (rows[0]["state"], rows[0]["substate"], rows[0]["selector"],
                           rows[0].get("class_callback_candidate")) != (3, 0, 44, f"{CALLBACK:08X}") or
            tuple(rows[0].get("entry_clip_candidate", {}).get("clip", ())) != POSE or
            [int(q["site"], 16) for q in rows[0].get("request_candidates", ()) if q["kind"] == "program"] !=
            list(PROGRAM_SITES)):
        raise ValueError("cannon beam needs the exact charged X context and its two program requests")


def _digest(dol):
    return sha256(dol.read(CALLBACK, SPAN, "text")).hexdigest()


def source_beam(dol, actions):
    verify_source(dol, actions)
    return {"model": f"{MODEL:04X}", "action_group": "X-charge", "raw_context": [0, 0],
            "callback": f"{CALLBACK:08X}", "pose": list(POSE),
            "program_requests": [f"{site:08X}" for site in PROGRAM_SITES],
            "origin_evidence": "none; the leaf reaches no origin factory or paid release",
            "scope": "authored charged horizontal beam; stock cost, length and damage are native tuning",
            "live_equivalence_verified": False}


def lines(level):
    """Extra gameplay lines appended to Ultimate Cannon's existing profile."""
    _, initial, limit, mode, _ = level[:5]
    if not 1 <= initial == limit <= 255 or mode != 0:
        raise ValueError("cannon beam stock policy")
    return [
        "projectile=cannon-beam|beam|1|1|0|0|0|60|40|10|ignore-floor",
        "sustained-beam=cannon-beam|1800|6|0",
        "look=cannon-beam|180|180|220|255",
        "origin=cannon-beam|0|0|45|70|target-height",
        "move=cannon-beam|x|12|grounded|60|4:0",
        "activation=cannon-beam|release-window|20|90",
        "release=cannon-beam|20|shot-stock|2|cannon-beam",
    ]


def receipt(source):
    return {"source_callback": source["callback"], "source_pose": source["pose"],
            "source_program_requests": source["program_requests"],
            "source_origin_evidence": source["origin_evidence"],
            "native_beam": {"length": 1800, "interval": 6, "turn": 0, "radius": 60, "lifetime": 40,
                            "damage": 10, "floor": "ignore-floor", "stock_cost": 2,
                            "look": {"size": 180, "rgb": [180, 220, 255]},
                            "charge": {"activation": "release-window", "hold": [20, 90]},
                            "form": "horizontal ground beam, no committed pitch"},
            "authored": source["scope"]}
