"""Authored charge beam for Ultimate Cannon's X-charge action.

The reviewed charged-X paths request type-71 key 0, whose active callback
requests type-11 group 2. The source ray, cadence, damage and debit schedule
are not qualified by that join. The native beam stays authored.
"""
from hashlib import sha256
import struct

from tools.analyze.retail_action_links import direct_branch


MODEL = 0x0C04
CALLBACK = 0x800B0F18
POSE = (4, 0)
PROGRAM_SITES = (0x800B1024, 0x800B10B8)
CARRIER = 0x800EC1F4
CARRIER_SITES = (0x800B0FF4, 0x800B13EC)
TEXT = (
    (CALLBACK, 76, "ecde564dd4e86801edd9ad1e606cf556fc134b34001830a7073a9e20de323932"),
    (0x800B0FA0, 168, "ccc93bf0b2e6b535d9754cccf0072594efd3568fd79a580893a867a788ced9e6"),
    (0x800B136C, 192, "454f194877c9bdb46cfe87ce212d55045a36d49ef1701aff7a2bdaab58efcda8"),
    (CARRIER, 248, "79260bf3f3889d80a002f4e280eddf48da8fac6cf79969cfdf84f71df437577b"),
    (0x800EC60C, 368, "95f2f17d8c61d58098d251bc9b9fc6139d480133df634b4b8414b659f1f94b2e"),
)


def verify_source(dol, actions):
    if any(sha256(dol.read(pc, size, "text")).hexdigest() != digest for pc, size, digest in TEXT):
        raise ValueError("cannon beam callback signature")
    for site in CARRIER_SITES:
        value, = struct.unpack(">I", dol.read(site, 4, "text"))
        if value & 0xFC000003 != 0x48000001 or direct_branch(site, value) != CARRIER:
            raise ValueError("cannon beam type-71 carrier call changed")
    if dol.read(0x8031B1C0, 2, "data") != b"\x00\x02":
        raise ValueError("cannon beam type-11 selector changed")
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


def source_beam(dol, actions):
    verify_source(dol, actions)
    return {"model": f"{MODEL:04X}", "action_group": "X-charge", "raw_context": [0, 0],
            "callback": f"{CALLBACK:08X}", "pose": list(POSE),
            "program_requests": [f"{site:08X}" for site in PROGRAM_SITES],
            "origin_evidence": "reviewed type-71 carrier request; exact ray origin and timing are not bound",
            "source_parent_route": {"factory": f"{CARRIER:08X}", "type": 71, "key": 0,
                                    "sites": [f"{pc:08X}" for pc in CARRIER_SITES],
                                    "segment_site": "800EC75C", "segment_key": 2},
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
             **({"source_parent_route": source["source_parent_route"]} if "source_parent_route" in source else {}),
            "native_beam": {"length": 1800, "interval": 6, "turn": 0, "radius": 60, "lifetime": 40,
                            "damage": 10, "floor": "ignore-floor", "stock_cost": 2,
                            "look": {"size": 180, "rgb": [180, 220, 255]},
                            "charge": {"activation": "release-window", "hold": [20, 90]},
                            "form": "horizontal ground beam, no committed pitch"},
            "authored": source["scope"]}
