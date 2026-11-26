"""Export the two measured G-Red move bases against Sasuke.

The move base is the first halfword of the credit record the attack selects.
Both G-Red records live in the actor configuration block at `0x80365AC8`:

    80365EC0  B-close / boost contact  base 20  amount 22
    80366058  X                        base 30  amount 33

The factors are the measured ones for this pair and side: credit difficulty
1.02, attacker rank 0.85, no conditional half, defender class row 1.13, receiver rank 1.15,
tier 1.0, relative 1.0. This is numeric API data, not model/texture/audio
bytes.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

CONFIG = 0x80365AC8
MOVES = (
    {"group": "B-close/boost", "record": 0x80365EC0, "base": 20, "amount": 22,
     "verified": True},
    {"group": "X", "record": 0x80366058, "base": 30, "amount": 33,
     "verified": True},
    {"group": "B-shot", "record": 0x803661F0, "base": 30, "amount": 33,
     "verified": False},
)
FACTORS = {
    "credit_difficulty": 1.02,
    "credit_rank": 0.85,
    "conditional_half": False,
    "scalar": 1.13,
    "receiver_rank": 1.15,
    "tier": 1.0,
    "relative": 1.0,
}


def amount_for(base):
    value = float(base)
    for key in ("credit_difficulty", "credit_rank", "scalar",
                "receiver_rank", "tier", "relative"):
        value *= FACTORS[key]
    return int(value), value


def recover(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    rows = []
    for move in MOVES:
        base = struct.unpack(">H", image.read(move["record"], 2, "data"))[0]
        if base != move["base"]:
            raise ValueError(f"base changed for {move['group']}: {base}")
        amount, product = amount_for(base)
        if amount != move["amount"]:
            raise ValueError(f"amount changed for {move['group']}: {product}")
        rows.append({
            "group": move["group"],
            "record": f"{move['record']:08X}",
            "config_offset": f"{move['record'] - CONFIG:03X}",
            "base": base,
            "product": round(product, 6),
            "amount": amount,
            "verified": move["verified"],
        })
    return {
        "kind": "gf-borg-move-bases",
        "revision": 1,
        "credit": "gg4e08:0615/0",
        "receiver": "gg4e08:000a/0",
        "source_dol_sha256": SOURCE_SHA256,
        "config_block": f"{CONFIG:08X}",
        "factors": FACTORS,
        "moves": rows,
        "notes": [
            "Both records are per-Borg config rows; the shared pool at 802D4160 (base 5) is the other side's record in the Mana battle.",
            "The X and B-close amounts are captured from live original hits; the shared pool range per action is still being mapped.",
            "The B-shot entry is the third record in the 0x198-stride table and is inferred, not captured; it is marked verified=false.",
            "The published amounts are first-hit values (tier 0). The X action's 33, 27 and 16 are the ReceiverTier row [1.0, 0.8, 0.5, 0.25] indexed by actor+0x6CA incrementing 0 -> 1 -> 2.",
            "This is one pair and one side; do not treat it as a roster-wide damage rule.",
        ],
        "retail_behaviour_verified": True,
        "generalises_to_other_pairs": False,
    }


def export(dol, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("move-base output already exists")
    with ReadOnlyFile(dol) as source:
        if not 0 < source.size <= 8 << 20:
            raise ValueError("source DOL extent")
        payload = recover(DolImage(source.read(0, source.size), SOURCE_SHA256))
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode()
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"bytes": len(raw), "sha256": hashlib.sha256(raw).hexdigest(),
            "amounts": [row["amount"] for row in payload["moves"]]}


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("dol", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.output)))


if __name__ == "__main__":
    main()
