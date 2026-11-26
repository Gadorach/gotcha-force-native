"""Map the direct calls of the reaction scaffold `8002E2A8`.

The function is 5,672 bytes and 1,418 instructions. Its gameplay-critical
middle is small: the amount call at `8002F108` and the HP write at `8002F1D8`.
The surrounding calls are flag, reaction and presentation helpers. This tool
decodes the direct `bl` sites from the qualified DOL and publishes their order,
targets and labelled phases, so the reaction phase can be implemented against a
stable map instead of a call-graph edge list.

Addresses, order and labels only: no code or asset bytes.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

FUNCTION = 0x8002E2A8
SIZE = 5672
END = FUNCTION + SIZE
AMOUNT = 0x8002F108
HP_WRITE = 0x8002F1D8

ROLE_NOTES = {
    0x8002F8DC: "subtracts the amount from the team force record, adds the "
                "per-actor ledgers and a six-slot hit-history ring",
    0x8002FCBC: "guards object+0x2AE and calls impact effect 800F04B4 with a "
                "volume-derived selector",
    0x8002FC18: "impact effect variant 800F088C with a threshold-derived "
                "selector",
    0x8003D344: "writes the amount to actor+0x1C6 HP and actor+0x41C ledger",
    0x8003CD5C: "the 19-stage amount routine",
    0x800300BC: "takes the mode from s8(attacker move-table entry +0x0E); "
                "writes receiver yaw +0x284 and pitch +0x282; native "
                "implements modes 1 and 3",
}

# What each target does, from the matched source or the inspected call shape.
# `confidence` distinguishes matched helpers from inferred presentation roles.
ROLES = {
    0x8002F8D0: ("set-flag", "matched"),
    0x800061A8: ("queue-reaction", "matched"),
    0x800300BC: ("reaction-direction", "open"),
    0x8003CD5C: ("amount", "recovered"),
    0x8003D344: ("hp-write", "matched"),
    0x8002F8DC: ("force-ledger-update", "open"),
    0x8018214C: ("reaction-helper", "open"),
    0x80006C24: ("slot-update", "matched"),
    0x800DB250: ("effect-values", "open"),
    0x8002FCBC: ("impact-effect", "matched"),
    0x8010E158: ("effect-spawn", "open"),
    0x8013F300: ("object-spawn", "matched"),
    0x80176EC0: ("object-spawn", "matched"),
    0x801EC1BC: ("reaction-effect", "matched"),
    0x801DFF24: ("reaction-effect", "matched"),
    0x80019550: ("color", "matched"),
    0x8002FC18: ("impact-effect", "open"),
}


def decode_calls(data, base):
    """Return direct `bl` sites in a little-endian-by-address code range."""
    if len(data) % 4:
        raise ValueError("code extent alignment")
    calls = []
    for offset in range(0, len(data), 4):
        word = struct.unpack_from(">I", data, offset)[0]
        if word >> 26 != 18 or not (word & 1):
            continue
        displacement = word & 0x03FFFFFC
        if displacement & 0x02000000:
            displacement -= 0x04000000
        if word & 2:
            target = displacement & 0xFFFFFFFF
        else:
            target = (base + offset + displacement) & 0xFFFFFFFF
        calls.append({"site": f"{base + offset:08X}",
                      "target": f"{target:08X}"})
    return calls


def _phase(site):
    if site < AMOUNT:
        return "pre-amount"
    if site <= HP_WRITE:
        return "amount"
    return "reaction"


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
        note = ROLE_NOTES.get(int(call["target"], 16))
        if note is not None:
            call["note"] = note
    if len(calls) != 47:
        raise ValueError(f"call count changed: {len(calls)}")
    sites = {call["site"]: call for call in calls}
    if sites.get(f"{AMOUNT:08X}", {}).get("role") != "amount":
        raise ValueError("amount call moved")
    if sites.get(f"{HP_WRITE:08X}", {}).get("role") != "hp-write":
        raise ValueError("HP write moved")
    counts = {}
    for call in calls:
        counts[call["phase"]] = counts.get(call["phase"], 0) + 1
    return {
        "kind": "gf-reaction-scaffold-map",
        "revision": 1,
        "source_dol_sha256": SOURCE_SHA256,
        "function": {
            "address": f"{FUNCTION:08X}",
            "size": SIZE,
            "instructions": 1418,
            "arguments": {
                "r3": "attacker side object",
                "r4": "receiver side object",
            },
        },
        "boundary": {
            "amount_call": f"{AMOUNT:08X}",
            "hp_write": f"{HP_WRITE:08X}",
            "amount_data": "r3 = credit row, r4 = attacker actor, "
                           "r5 = receiver actor",
        },
        "phases": counts,
        "calls": calls,
        "notes": [
            "Direct bl sites only; internal branches and tail calls are not "
            "listed.",
            "The amount and HP write are the gameplay boundary; the reaction "
            "band is mostly presentation and reaction helpers.",
            "Roles marked 'matched' come from matched helpers; 'open' and "
            "'unknown' roles are inferred from the call shape.",
        ],
    }


def export(dol, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("reaction scaffold output already exists")
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
