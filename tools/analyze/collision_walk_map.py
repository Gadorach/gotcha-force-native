"""Map the direct calls of the collision walk `8002BB14`.

The function is 3,712 bytes and 928 instructions. It is the walk above the pair
loop: actor-list iteration, contact accumulator resets, terrain/height queries,
transform helpers, then the pair loop at `8002C4CC` which reaches the reaction
scaffold `8002E2A8`.

This tool decodes the direct `bl` sites from the qualified DOL and publishes
their order, phase and labelled role. Addresses, order and labels only: no code
or asset bytes.
"""
import argparse
import hashlib
import json
from pathlib import Path

from tools.analyze.reaction_scaffold_map import decode_calls
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

FUNCTION = 0x8002BB14
SIZE = 3712
INSTRUCTIONS = 928
PAIR_LOOP_CALL = 0x8002C4CC
PAIR_LOOP_TARGET = 0x8002DB58
REACTION = 0x8002E2A8

# Roles come from the target's own source shape (matched or best source).
ROLES = {
    0x8002CC78: ("actor-list-walk", "matched"),
    0x8002CDA4: ("clear-contact-accumulators", "open"),
    0x8003F320: ("terrain-height-query", "matched"),
    0x8002CAA8: ("apply-effect-scale", "matched"),
    0x8002DB58: ("pair-loop", "open"),
    0x8003D444: ("floor-correction-7d6", "open"),
    0x8003D3E8: ("floor-correction-6c4", "open"),
    0x8005C290: ("state-dispatch", "open"),
    0x800681E4: ("actor-update", "matched"),
    0x8006ACE8: ("set-state", "open"),
    0x800680D4: ("actor-compare", "matched"),
    0x8006AB04: ("set-flag-data", "open"),
    0x8005BBC0: ("actor-helper", "open"),
    0x80159168: ("node-allocate", "matched"),
    0x8006ABD4: ("pair-collision-check", "open"),
    0x8005A580: ("actor-helper", "open"),
    0x800466DC: ("effect-cleanup", "matched"),
    0x8020B7A8: ("matrix-vector", "open"),
    0x8020BAB8: ("vector-normalize", "open"),
    0x8020BAFC: ("length-squared", "open"),
    0x8002E1C8: ("global-state-init", "matched"),
    0x8002CE44: ("register-effect-ids", "matched"),
    0x8002C994: ("effect-process", "open"),
    0x8002D7C4: ("walk-helper", "open"),
    0x8002DF08: ("walk-helper", "open"),
    0x8002CF80: ("walk-helper", "open"),
    0x8002D35C: ("walk-helper", "open"),
    0x8002D58C: ("walk-helper", "open"),
    0x8002D198: ("walk-helper", "open"),
    0x8002CB20: ("list-scan", "open"),
    0x8002F8DC: ("reaction-state", "open"),
}


def _phase(site):
    if site < 0x8002BE00:
        return "broad-phase"
    if site < 0x8002C000:
        return "geometry"
    if site <= PAIR_LOOP_CALL:
        return "apply"
    return "post"


def recover(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    code = image.read(FUNCTION, SIZE, "text")
    calls = decode_calls(code, FUNCTION)
    for call in calls:
        role, confidence = ROLES.get(int(call["target"], 16),
                                     ("unclassified", "unknown"))
        call["role"] = role
        call["confidence"] = confidence
        call["phase"] = _phase(int(call["site"], 16))
    if len(calls) != 68:
        raise ValueError(f"call count changed: {len(calls)}")
    sites = {call["site"]: call for call in calls}
    pair = sites.get(f"{PAIR_LOOP_CALL:08X}")
    if pair is None or pair["target"] != f"{PAIR_LOOP_TARGET:08X}":
        raise ValueError("pair-loop call moved")
    counts = {}
    for call in calls:
        counts[call["phase"]] = counts.get(call["phase"], 0) + 1
    return {
        "kind": "gf-collision-walk-map",
        "revision": 1,
        "source_dol_sha256": SOURCE_SHA256,
        "function": {
            "address": f"{FUNCTION:08X}",
            "size": SIZE,
            "instructions": INSTRUCTIONS,
        },
        "pair_loop": {
            "call": f"{PAIR_LOOP_CALL:08X}",
            "target": f"{PAIR_LOOP_TARGET:08X}",
            "reaction": f"{REACTION:08X}",
            "outer": {"array": "803C477C", "count": "r13-21328"},
            "inner": {"array": "803C417C", "count": "r13-21330"},
            "mode": {
                "direct": "[actor+88] == 2 or record+16 bit 0 set",
                "nearest": "otherwise one nearest valid target; initial "
                           "distance 400000000.0",
            },
            "filters": [
                {"kind": "carrier-flags",
                 "rule": "carrier node+2 bits 0x0800/0x1000/0x2000 and "
                         "record+18 bits 0x8000/0x4000"},
                {"kind": "receiver",
                 "rule": "receiver+474 bit 0x02 clear"},
                {"kind": "validity", "call": "8002FD7C", "argument": 2,
                 "rule": "different actor; owner-invulnerability; same-team "
                         "flag 0x80; attack-class flags 0x3C0/0xC00; six "
                         "eight-entry immunity keys at +0x20C; target mask "
                         "at +0x272"},
                {"kind": "volume", "call": "80039F6C",
                 "rule": "both radii > 0.1 and centre distance <= radius sum"},
                {"kind": "handler", "call": "80030490",
                 "rule": "dispatch (outer category << 3) | inner category; "
                         "0xFF rejects"},
            ],
            "setup": {"call": "80030348",
                      "rule": "run the two +88 category setup handlers"},
            "scratch": ["803B04D0", "803B0368", "803B0720"],
            "nearest_distance": "8020BC88",
        },
        "phases": counts,
        "calls": calls,
        "notes": [
            "Direct bl sites only; internal branches and tail calls are not "
            "listed.",
            "The apply band ends at the pair-loop call, which is the entry to "
            "the reaction scaffold.",
            "The pair loop selects direct hits or the nearest valid target; "
            "the target arrays and counts are runtime BSS.",
            "Roles come from the target's own source shape; 'open' and "
            "'unknown' labels are inferred and can move with better evidence.",
        ],
    }


def export(dol, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("collision walk output already exists")
    with ReadOnlyFile(dol) as source:
        if not 0 < source.size <= 8 << 20:
            raise ValueError("source DOL extent")
        payload = recover(DolImage(source.read(0, source.size), SOURCE_SHA256))
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode()
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"bytes": len(raw), "sha256": hashlib.sha256(raw).hexdigest(),
            "calls": len(payload["calls"]), "phases": payload["phases"]}


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("dol", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.output), sort_keys=True))


if __name__ == "__main__":
    main()
