"""Export the measured G-Red -> Sasuke boost/contact damage chain.

This is a recovery artifact, not a gameplay rule. It records what the original
was observed to do for one pair and one contact path, with the DOL addresses
that supply each scalar, so the arithmetic can be re-checked without the game.

The live observation (kakeru_battle, boost+forward) was:

    credit record 80365EC0, base halfword 20
    receiver 805DDBC0, model 000A, HP 180
    20 x 0.867 x 1.13 x 1.15 = 22.533329 -> 22
    apply caller 8002F1DC, writer 8003D344, ledger actor+0x41C

The 0.867 matchup scalar is not a stored float (the search over the data
sections finds no 0.867); it is the live accumulator between the amount call
and D024, so it is recorded as an observation input. The other scalars are read
from the DOL at the addresses below and must match exactly.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

BASE = 0x80365EC0
SCALAR_ROW = 0x802C6820
SCALAR_INDEX = 16
RANK_ROW = 0x802C7C80
TIER_ROW = 0x802C7CA0
MATCHUP_FACTOR = 0.867
CREDIT_MODEL = 0x0615
RECEIVER_MODEL = 0x000A


def combine(base, matchup, scalar, rank, tier, relative=1.0):
    value = float(base)
    for factor in (matchup, scalar, rank, tier, relative):
        value *= factor
    return int(value), value


def _data(image, at, size):
    return image.read(at, size, "data")


def recover(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")

    base = struct.unpack(">H", _data(image, BASE, 2))[0]
    scalar = struct.unpack(">f", _data(image, SCALAR_ROW + 4 * SCALAR_INDEX, 4))[0]
    rank = struct.unpack(">f", _data(image, RANK_ROW, 4))[0]
    tier = struct.unpack(">f", _data(image, TIER_ROW, 4))[0]
    if base != 20:
        raise ValueError(f"base halfword changed: {base}")
    if abs(scalar - 1.13) > 1e-6:
        raise ValueError(f"scalar row changed: {scalar}")
    if abs(rank - 1.15) > 1e-6:
        raise ValueError(f"rank row changed: {rank}")
    if abs(tier - 1.0) > 1e-6:
        raise ValueError(f"tier row changed: {tier}")

    amount, product = combine(base, MATCHUP_FACTOR, scalar, rank, tier)
    if amount != 22:
        raise ValueError(f"amount changed: {product} -> {amount}")

    return {
        "kind": "gf-borg-contact-damage",
        "revision": 1,
        "credit": f"gg4e08:{CREDIT_MODEL:04x}/0",
        "receiver": f"gg4e08:{RECEIVER_MODEL:04x}/0",
        "path": "boost-contact",
        "source_dol_sha256": SOURCE_SHA256,
        "base": {"value": base, "address": f"{BASE:08X}"},
        "factors": {
            "matchup": {
                "value": MATCHUP_FACTOR,
                "source": "live accumulator between the amount call and 8003D024",
                "stored_as_float": False,
            },
            "scalar": {
                "value": round(scalar, 6),
                "address": f"{SCALAR_ROW:08X}",
                "index": SCALAR_INDEX,
                "row": "0.97..1.16 linear ladder",
            },
            "receiver_rank": {
                "value": round(rank, 6),
                "address": f"{RANK_ROW:08X}",
                "index": 0,
            },
            "tier": {
                "value": round(tier, 6),
                "address": f"{TIER_ROW:08X}",
                "index": 0,
                "actor_byte": "actor+0x6CA = 0",
            },
            "relative": {
                "value": 1.0,
                "source": "receiver+180 = 1.0, adjusted neutrally",
            },
        },
        "product": round(product, 6),
        "amount": amount,
        "observation": {
            "actor": "805DDBC0",
            "model": RECEIVER_MODEL,
            "hp_before": 180,
            "hp_after": 158,
            "ledger": 22,
            "caller": "8002F1DC",
            "writer": "8003D344",
        },
        "notes": [
            "The amount is a live boost/contact observation, not a beam-table result.",
            "The raw GFDA beam replay gives 19/20 for the same pair and caps because it applies the GFDA rows, including receiver difficulty 0.9.",
            "generalises_to_other_pairs is false until the matchup and scalar selections are recovered per pair.",
        ],
        "retail_behaviour_verified": True,
        "generalises_to_other_pairs": False,
    }


def export(dol, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("contact damage output already exists")
    with ReadOnlyFile(dol) as source:
        if not 0 < source.size <= 8 << 20:
            raise ValueError("source DOL extent")
        image = DolImage(source.read(0, source.size), SOURCE_SHA256)
        payload = recover(image)
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode()
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"bytes": len(raw), "sha256": hashlib.sha256(raw).hexdigest(),
            "amount": payload["amount"]}


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("dol", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.output)))


if __name__ == "__main__":
    main()
