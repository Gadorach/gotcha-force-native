"""Translate the original damage-amount function `8003CD5C`.

The function is 378 instructions. Its shape is a fixed multiply chain: an
integer base from `record+0`, a carrier-relative term, then a sequence of
table-selected factors. This tool records the chain in execution order, the
selection path of each table factor, and the two live observations that pin
the arithmetic.

The tables are read read-only from the qualified DOL. Runtime tables (per-team
settings, borg HP mirror, team force) live in BSS and are supplied by an
observation snapshot, never guessed.

Numeric API data only: addresses, roles, indices and the applied scalars.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

FUNCTION = 0x8003CD5C
INSTRUCTIONS = 378

# The model-to-matchup index mapper 80066298: a 256-entry pointer table, each
# row a signed halfword array. The caller passes the model id as a big-endian
# u16, so byte 0 is the family and byte 1 the variant.
MODEL_INDEX_TABLE = 0x802F2E28
MATCHUP_MATRIX = 0x802C5D60

# The eight 2-entry pointer tables. `root[side]` is the row set for that side.
# Side is `actor+998 != 0`; it is a flag, not a signed owner id.
ROOTS = {
    "credit_difficulty": (0x804335E0, "float row", "attacker+136 -> byte at 803C4E60"),
    "attacker_hp_row": (0x804335E8, "row pointers", "record+6, then attacker HP index"),
    "team_force_row": (0x804335F0, "row pointers", "record+7, then team force index"),
    "attacker_rank": (0x804335F8, "float row", "attacker+1082 (difficulty)"),
    "defender_class_row": (0x80433600, "row pointers",
                           "model+156 (plXXXXdata.bin), then byte at 803C4E60"),
    "defender_hp_row_157": (0x80433608, "row pointers",
                            "model+157 (plXXXXdata.bin), then post-hit HP index"),
    "defender_hp_row_158": (0x80433610, "row pointers",
                            "model+158 (plXXXXdata.bin), then post-hit HP index"),
    "receiver_rank": (0x80433618, "float row", "defender+1082 (difficulty)"),
}

# Multiply chain in original execution order. `site` is the multiply address.
STAGES = (
    {"site": "CE18", "name": "carrier_relative", "op": "mul",
     "selection": "1 + 0.5 * (carrier+196 - 1)"},
    {"site": "CE48", "name": "attacker_1788", "op": "mul",
     "value": 2.0, "selection": "attacker+1788 != 0"},
    {"site": "CE94", "name": "credit_difficulty", "op": "mul",
     "table": "credit_difficulty",
     "side": "attacker+998 != 0",
     "index": "settings[attacker+136]"},
    {"site": "CEB8", "name": "attacker_conditional_half", "op": "mul",
     "value": 0.5,
     "condition": "settings[attacker+136] == 0 && attacker+998 != 0 "
                  "&& attacker+136 == 0"},
    {"site": "CF48", "name": "attacker_hp_row", "op": "mul",
     "table": "attacker_hp_row",
     "side": "attacker+998 != 0",
     "row": "record+6",
     "index": "32 - borg_hp[attacker+996] * 32 / "
              "(attacker+452 < 200 ? attacker+452 : 25)"},
    {"site": "CFC0", "name": "team_force_row", "op": "mul",
     "table": "team_force_row",
     "side": "attacker+998 != 0",
     "row": "record+7",
     "index": "32 - team_force[attacker+136] * 32 / team_force_cap[attacker+136]"},
    {"site": "CFC8", "name": "attacker_rank", "op": "mul",
     "table": "attacker_rank",
     "side": "attacker+998 != 0",
     "index": "attacker+1082"},
    {"site": "D024", "name": "matchup", "op": "mul",
     "table": "802C5D60",
     "index": "model_index(defender+1000) * 80 "
              "+ model_index(attacker+1000) * 4"},
    {"site": "D0B0", "name": "defender_1788", "op": "mul",
     "value": 0.5, "condition": "defender+1788 != 0"},
    {"site": "D100", "name": "defender_class_row", "op": "mul",
     "table": "defender_class_row",
     "side": "defender+998 != 0",
     "row": "model+156 (plXXXXdata.bin)",
     "index": "settings[defender+136]"},
    {"site": "D128", "name": "defender_conditional_half", "op": "mul",
     "value": 0.5,
     "condition": "settings[defender+136] == 0 && defender+998 != 0 "
                  "&& defender+136 == 0"},
    {"site": "D16C", "name": "relative", "op": "mul",
     "selection": "1 / (1 + 0.5 * (defender+180 - 1))"},
    {"site": "D1EC", "name": "defender_hp_row_157", "op": "mul",
     "table": "defender_hp_row_157",
     "side": "defender+998 != 0",
     "row": "model+157 (plXXXXdata.bin)",
     "index": "post-hit defender HP index"},
    {"site": "D260", "name": "defender_hp_row_158", "op": "mul",
     "table": "defender_hp_row_158",
     "side": "defender+998 != 0",
     "row": "model+158 (plXXXXdata.bin)",
     "index": "post-hit defender HP index"},
    {"site": "D270", "name": "receiver_rank", "op": "mul",
     "table": "receiver_rank",
     "side": "defender+998 != 0",
     "index": "defender+1082"},
    {"site": "D290", "name": "tier", "op": "mul",
     "table": "802C7CA0", "index": "defender+1738",
     "condition": "record+16 & 0x4000"},
    {"site": "D2A4", "name": "defender_1504_half", "op": "mul",
     "value": 0.5, "condition": "defender+1504 & 0x04000000 == 0"},
    {"site": "D2D0", "name": "same_player_quarter", "op": "mul",
     "value": 0.25,
     "condition": "attacker+136 == defender+136 && record+16 & 0x1000 == 0"},
    {"site": "D2FC", "name": "scale_divisor_40", "op": "div",
     "value": 40.0,
     "condition": "attacker+131 == 0 && record+16 & 0x1000 "
                  "&& defender+1436 & 0x1000"},
)

# Post-hit HP index: clamp to 0, skip above 200, divide by max HP (or 25 at
# the 200 sentinel), then 32 - value and clamp to 31.
HP_INDEX = ("post = max(0, borg_hp[+996] - base); if post > 200: skip; "
            "divisor = (+452 < 200 ? +452 : 25); "
            "index = min(32 - post * 32 / divisor, 31)")

# Live observations. The stage values are the measured scalars; values not
# listed are neutral (1.0). `source` marks which values were directly probed.
OBSERVATIONS = (
    {
        "state": "kakeru_battle",
        "base": 20,
        "factors": {
            "credit_difficulty": 1.02,
            "attacker_rank": 0.85,
            "matchup": 1.0,
            "defender_class_row": 1.13,
            "receiver_rank": 1.15,
            "tier": 1.0,
            "relative": 1.0,
        },
        "probed": ["matchup", "defender_class_row", "receiver_rank", "tier",
                   "relative"],
        "derived": ["credit_difficulty", "attacker_rank"],
        "amount": 22,
        "note": "The D024 product 0.867 is credit difficulty 1.02 x attacker "
                "rank 0.85; the attacker HP and team-force rows are neutral.",
    },
    {
        "state": "kakeru_x_tier0",
        "base": 30,
        "factors": {
            "credit_difficulty": 1.02,
            "attacker_rank": 0.85,
            "defender_class_row": 1.13,
            "receiver_rank": 1.15,
            "tier": 1.0,
        },
        "probed": ["tier"],
        "derived": ["credit_difficulty", "attacker_rank",
                    "defender_class_row", "receiver_rank"],
        "amount": 33,
        "note": "Per-move X record base 30 with the same pair factors: "
                "33.8 -> 33.",
    },
    {
        "state": "kakeru_x_tier1",
        "base": 30,
        "factors": {
            "credit_difficulty": 1.02,
            "attacker_rank": 0.85,
            "defender_class_row": 1.13,
            "receiver_rank": 1.15,
            "tier": 0.8,
        },
        "probed": ["tier"],
        "derived": ["credit_difficulty", "attacker_rank",
                    "defender_class_row", "receiver_rank"],
        "amount": 27,
        "note": "Second successive hit, tier 0.8: 27.04 -> 27.",
    },
    {
        "state": "kakeru_x_tier2",
        "base": 30,
        "factors": {
            "credit_difficulty": 1.02,
            "attacker_rank": 0.85,
            "defender_class_row": 1.13,
            "receiver_rank": 1.15,
            "tier": 0.5,
        },
        "probed": ["tier"],
        "derived": ["credit_difficulty", "attacker_rank",
                    "defender_class_row", "receiver_rank"],
        "amount": 16,
        "note": "Third successive hit, tier 0.5: 16.9 -> 16.",
    },
    {
        "state": "mana_combat_hud",
        "base": 5,
        "factors": {
            "credit_difficulty": 0.92,
            "attacker_rank": 0.85,
            "attacker_conditional_half": 0.5,
            "defender_class_row": 1.4,
            "receiver_rank": 1.15,
        },
        "probed": [],
        "derived": ["credit_difficulty", "attacker_rank",
                    "attacker_conditional_half", "defender_class_row",
                    "receiver_rank"],
        "amount": 3,
        "note": "0.92 x 0.85 x 0.5 x 1.4 x 1.15 x 5 = 3.147 -> 3. The "
                "recorded amount needs both the class row and receiver rank.",
    },
)


def evaluate(base, factors):
    """Apply the recovered chain. Missing named factors are neutral."""
    value = float(base)
    for stage in STAGES:
        factor = factors.get(stage["name"])
        if factor is None:
            continue
        if stage["op"] == "div":
            value /= factor
        else:
            value *= factor
    if value < 0.0:
        value = 0.0
    return int(value), value


def _data(image, at, size):
    return image.read(at, size, "data")


def _u32(image, at):
    return struct.unpack(">I", _data(image, at, 4))[0]


def _f32(image, at):
    return struct.unpack(">f", _data(image, at, 4))[0]


def _check(value, expected, what):
    if abs(value - expected) > 1e-6:
        raise ValueError(f"{what} changed: {value}")


def model_index(image, model):
    """Translate 80066298: model id -> matchup matrix index."""
    family = (model >> 8) & 0xFF
    if family >= 128:
        family -= 256
    row = _u32(image, MODEL_INDEX_TABLE + 4 * family)
    return struct.unpack(">h", _data(image, row + 2 * (model & 0xFF), 2))[0]


def matchup_cell(image, attacker, defender):
    """D024: matrix[defender index][attacker index], 20 floats per row."""
    return _f32(image, MATCHUP_MATRIX
                + model_index(image, defender) * 80
                + model_index(image, attacker) * 4)


def _table_map(image):
    tables = {}
    for name, (root, kind, index) in ROOTS.items():
        sides = []
        for side in (0, 1):
            pointer = _u32(image, root + 4 * side)
            sides.append(f"{pointer:08X}")
        tables[name] = {
            "root": f"{root:08X}",
            "kind": kind,
            "index": index,
            "sides": sides,
        }
    return tables


def recover(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")

    tables = _table_map(image)
    # Pin the cells that the two observations use.
    _check(_f32(image, _u32(image, 0x804335E0)), 1.02, "credit difficulty side 0")
    _check(_f32(image, _u32(image, 0x804335E4)), 0.92, "credit difficulty side 1")
    _check(_f32(image, _u32(image, 0x804335F8)), 0.85, "attacker rank side 0")
    _check(_f32(image, _u32(image, 0x804335FC)), 0.85, "attacker rank side 1")
    _check(_f32(image, _u32(image, 0x80433618)), 1.15, "receiver rank side 0")
    _check(_f32(image, _u32(image, 0x8043361C)), 1.15, "receiver rank side 1")

    # 80066298 maps G-Red 0x0615 to index 4 and Sasuke 0x000A to index 0;
    # the live probe read matrix row 0 / column 4 as 1.0.
    if model_index(image, 0x0615) != 4 or model_index(image, 0x000A) != 0:
        raise ValueError("model index mapping changed")
    _check(matchup_cell(image, 0x0615, 0x000A), 1.0, "matchup cell")
    _check(matchup_cell(image, 0x0615, 0x0615), 1.0, "mirror matchup cell")

    # Defender class row 1 for side 1 is 802C6820: index 16 = 1.13, 30 = 1.4.
    class_rows = _u32(image, 0x80433604)
    row_one = _u32(image, class_rows + 4)
    if row_one != 0x802C6820:
        raise ValueError(f"defender class row changed: {row_one:08X}")
    _check(_f32(image, row_one + 4 * 16), 1.13, "defender class index 16")
    _check(_f32(image, row_one + 4 * 30), 1.4, "defender class index 30")

    observed = []
    for entry in OBSERVATIONS:
        amount, product = evaluate(entry["base"], entry["factors"])
        if amount != entry["amount"]:
            raise ValueError(f"{entry['state']} amount changed: "
                             f"{product} -> {amount}")
        observed.append({
            "state": entry["state"],
            "base": entry["base"],
            "factors": dict(sorted(entry["factors"].items())),
            "probed": entry["probed"],
            "derived": entry["derived"],
            "product": round(product, 6),
            "amount": amount,
            "note": entry["note"],
        })

    return {
        "kind": "gf-damage-amount-model",
        "revision": 1,
        "source_dol_sha256": SOURCE_SHA256,
        "function": {
            "address": f"{FUNCTION:08X}",
            "instructions": INSTRUCTIONS,
            "arguments": {
                "r3": "credit record (u16 base at +0, rows at +6/+7, flags +16)",
                "r4": "attacker carrier (+144 actor, +196 relative)",
                "r5": "defender carrier",
            },
        },
        "base": {"field": "record+0", "type": "u16"},
        "stages": list(STAGES),
        "tables": tables,
        "hp_index": HP_INDEX,
        "settings": {
            "address": "803C4E60",
            "index": "actor+136",
            "role": "per-team byte that indexes the credit-difficulty and "
                    "defender-class rows",
        },
        "matchup": {
            "address": f"{MATCHUP_MATRIX:08X}",
            "shape": "20 x 20 floats (row stride 80)",
            "index": "matrix[model_index(defender+1000)]"
                     "[model_index(attacker+1000)]",
            "mapper": {
                "address": f"{MODEL_INDEX_TABLE:08X}",
                "function": "80066298",
                "rule": "row = s8(high model byte); "
                        "index = s16(row[low model byte])",
                "observed": [
                    {"model": "0615", "index": 4},
                    {"model": "000a", "index": 0},
                ],
            },
            "observed_cell": {"attacker": "0615", "defender": "000a",
                              "row": 0, "column": 4, "value": 1.0},
            "note": "The mapper is the only function the amount routine "
                    "calls; it is now resolved. The observed Kakeru cell is "
                    "neutral at [defender 0][attacker 4].",
        },
        "observed": observed,
        "corrections": [
            "0.867 is not a single stored float: it is credit difficulty 1.02 "
            "(CE94) times attacker difficulty rank 0.85 (CFC8).",
            "The 1.13 is not a battle-wide settings scalar: it is the defender "
            "class row 802C6820[16] at D100, selected by model+156 and the "
            "per-team settings byte 803C4E60[defender+136].",
            "The receiver rank 1.15 is 802C7C60[defender+1082] at D270.",
            "The matchup mapper 80066298 is a 256-entry pointer table at "
            "802F2E28: G-Red 0615 -> 4, Sasuke 000a -> 0, cell [0][4] = 1.0.",
            "The defender selectors at model+156/157/158 are the AFS bytes "
            "plXXXXdata.bin+156..158; the roster map is "
            "api-map/borg-damage-selectors.json (Sasuke 1,1,0; G-Red 2,2,2).",
            "The 0.85 rank is at 804335F8[side], not inside the 804335E0 "
            "credit-difficulty pair.",
        ],
        "notes": [
            "Numeric API data only; no code or asset bytes are reproduced.",
            "The runtime tables (803C4E60 settings, 803B069C borg HP, "
            "803B068C team force) are BSS and are supplied by an observation.",
            "The chain is a reference translation; the native engine still "
            "consumes content scalars for these factors.",
        ],
    }


def export(dol, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("damage amount output already exists")
    with ReadOnlyFile(dol) as source:
        if not 0 < source.size <= 8 << 20:
            raise ValueError("source DOL extent")
        payload = recover(DolImage(source.read(0, source.size), SOURCE_SHA256))
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode()
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"bytes": len(raw), "sha256": hashlib.sha256(raw).hexdigest(),
            "stages": len(payload["stages"]),
            "amounts": [entry["amount"] for entry in payload["observed"]]}


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("dol", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.output)))


if __name__ == "__main__":
    main()
