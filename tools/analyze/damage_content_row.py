"""Build a native `damage-vs` row from the recovered amount tables.

Given the two models and the battle-state values, read the four original rows
and emit the content factors the native engine consumes:

    credit = credit_difficulty(side, settings) x attacker_rank(difficulty)
    class  = defender_class_row(side, model selector, settings)
    rank   = receiver_rank(side, difficulty)

The four engine factors are `credit`, `class`, `rank` and `relative`; their
product is the original pre-tier product. A battle can request several defender
models and receives a ready-to-append `damage-vs` block, capped at the engine's
16 rows. The known G-Red -> Sasuke hit reproduces
`damage-vs=10|0.867|1.13|1.15|1.0` and amount 22.

Read-only: the DOL and the public selector JSON are inputs.
"""
import argparse
import json
import struct
from pathlib import Path

from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

CREDIT_ROOT = 0x804335E0
ATTACKER_RANK_ROOT = 0x804335F8
CLASS_ROOT = 0x80433600
RECEIVER_RANK_ROOT = 0x80433618


def _u32(image, at):
    return struct.unpack(">I", image.read(at, 4, "data"))[0]


def _f32(image, at):
    return struct.unpack(">f", image.read(at, 4, "data"))[0]


def _row(image, root, side):
    return _u32(image, root + 4 * side)


def _selector(selectors, model):
    key = f"{model:04x}"
    if key in selectors["models"]:
        return list(selectors["models"][key]), None
    if key in selectors["combined_models"]:
        return list(selectors["combined_models"][key][0]), 0
    raise ValueError(f"no damage selectors for model {key}")


def _number(value):
    text = f"{value:.6f}".rstrip("0").rstrip(".")
    if not text:
        return "0"
    return text if "." in text else text + ".0"


def build(image, selectors, attacker, defender, base, relative):
    selector, component = _selector(selectors, defender["model"])
    credit_row = _row(image, CREDIT_ROOT, attacker["side"])
    credit = _f32(image, credit_row + 4 * attacker["settings"])
    attacker_rank = _f32(image,
                         _row(image, ATTACKER_RANK_ROOT, attacker["side"])
                         + 4 * attacker["rank"])
    class_row = _u32(image, _row(image, CLASS_ROOT, defender["side"])
                     + 4 * selector[0])
    class_value = _f32(image, class_row + 4 * defender["settings"])
    rank_value = _f32(image,
                      _row(image, RECEIVER_RANK_ROOT, defender["side"])
                      + 4 * defender["rank"])
    product = credit * attacker_rank * class_value * rank_value * relative
    factors = [credit * attacker_rank, class_value, rank_value, relative]
    amount = int(base * product)
    line = (f"damage-vs={defender['model']}|"
            + "|".join(_number(value) for value in factors))
    return {
        "kind": "gf-damage-content-row",
        "revision": 1,
        "source_dol_sha256": SOURCE_SHA256,
        "selectors": {"triple": selector, "combined_component": component},
        "attacker": dict(attacker),
        "defender": dict(defender),
        "stages": {
            "credit_difficulty": round(credit, 6),
            "attacker_rank": round(attacker_rank, 6),
            "defender_class": round(class_value, 6),
            "receiver_rank": round(rank_value, 6),
            "relative": round(relative, 6),
        },
        "base": base,
        "product": round(product, 6),
        "amount": amount,
        "content": {"line": line,
                    "factors": [round(value, 6) for value in factors]},
        "notes": [
            "credit combines the original credit-difficulty and attacker-rank "
            "stages; class and rank are the defender stages.",
            "HP/force/matchup/tier stages are neutral here and stay content "
            "or battle state.",
            "Combined forms use the first component until the active component "
            "is verified.",
        ],
    }


def build_rows(image, selectors, attacker, defenders, base, relative):
    if not 1 <= len(defenders) <= 16:
        raise ValueError("a battle needs between 1 and 16 defender rows")
    models = [defender["model"] for defender in defenders]
    if len(set(models)) != len(models):
        raise ValueError("duplicate defender model")
    rows = [build(image, selectors, attacker, defender, base, relative)
            for defender in defenders]
    return {
        "kind": "gf-damage-content-block",
        "revision": 1,
        "source_dol_sha256": SOURCE_SHA256,
        "attacker": dict(attacker),
        "base": base,
        "rows": rows,
        "block": "\n".join(row["content"]["line"] for row in rows),
        "notes": [
            "One row per defender model, capped at the engine's 16-row limit.",
            "`block` is the ready-to-append damage-vs block for a battle.",
        ],
    }


def load(dol, selectors_path, attacker, defender, base, relative):
    with ReadOnlyFile(dol) as source:
        if not 0 < source.size <= 8 << 20:
            raise ValueError("source DOL extent")
        image = DolImage(source.read(0, source.size), SOURCE_SHA256)
        selectors = json.loads(Path(selectors_path).read_bytes())
        if selectors.get("kind") != "gf-borg-damage-selectors":
            raise ValueError("wrong selector artifact")
        return build(image, selectors, attacker, defender, base, relative)


def load_rows(dol, selectors_path, attacker, defenders, base, relative):
    with ReadOnlyFile(dol) as source:
        if not 0 < source.size <= 8 << 20:
            raise ValueError("source DOL extent")
        image = DolImage(source.read(0, source.size), SOURCE_SHA256)
        selectors = json.loads(Path(selectors_path).read_bytes())
        if selectors.get("kind") != "gf-borg-damage-selectors":
            raise ValueError("wrong selector artifact")
        return build_rows(image, selectors, attacker, defenders, base, relative)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("dol", type=Path)
    parser.add_argument("selectors", type=Path)
    parser.add_argument("--attacker-model", type=lambda text: int(text, 16),
                        required=True)
    parser.add_argument("--defender-model", action="append", required=True,
                        type=lambda text: int(text, 16))
    parser.add_argument("--attacker-side", type=int, choices=(0, 1), default=0)
    parser.add_argument("--defender-side", type=int, choices=(0, 1), default=1)
    parser.add_argument("--attacker-settings", type=int, choices=range(32),
                        default=0)
    parser.add_argument("--defender-settings", type=int, choices=range(32),
                        default=0)
    parser.add_argument("--attacker-rank", type=int, choices=range(32),
                        default=0)
    parser.add_argument("--defender-rank", type=int, choices=range(32),
                        default=0)
    parser.add_argument("--base", type=int, default=20)
    parser.add_argument("--relative", type=float, default=1.0)
    args = parser.parse_args()
    attacker = {"model": args.attacker_model, "side": args.attacker_side,
                "settings": args.attacker_settings, "rank": args.attacker_rank}
    defenders = [{"model": model, "side": args.defender_side,
                  "settings": args.defender_settings, "rank": args.defender_rank}
                 for model in args.defender_model]
    print(json.dumps(load_rows(args.dol, args.selectors, attacker, defenders,
                               args.base, args.relative), sort_keys=True))


if __name__ == "__main__":
    main()
