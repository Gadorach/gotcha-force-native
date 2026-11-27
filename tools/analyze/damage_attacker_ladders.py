"""Recover outgoing HP/force curves and bind only proved direct move selectors."""
import argparse
import hashlib
import json
from pathlib import Path

from tools.analyze import damage_contact_power, damage_receiver_ladders as ladders
from tools.analyze.reaction_attack_table import SIZE
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

ROLES = {"hp": (0x804335E8, 6, "8003CF48"),
         "force": (0x804335F0, 7, "8003CFC0")}


def recover(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    if hashlib.sha256(image.read(0x8003CEC0, 268, "text")).hexdigest() != (
            "6055733be5fabec03225c14eccb813bffc492dcf794aba44049c263516c625e4"):
        raise ValueError("attacker ladder index paths changed")
    roles = {key: {"root": f"{root:08X}", "attack_selector_offset": offset,
                   "multiply": multiply, "sides": ladders.read_team_rows(image, root)}
             for key, (root, offset, multiply) in ROLES.items()}
    return {"kind": "gf-damage-attacker-ladders", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
            "cells_per_row": ladders.CELLS, "roles": roles,
            "hp_index": "pre-hit mirrored attacker HP; skip above 200; "
                        "min(31, 32 - floor(32*HP/min(max HP,200))); no subtraction",
            "force_index": "min(31, 32 - floor(32*mirrored attacking slot force/force cap))",
            "native_scope": "living attackers on opted-in direct moves with both split roles; no untracked force lookup"}


def move_selectors(image, binding):
    power = damage_contact_power.move_power(image, binding)
    table = int(power["table"], 16)
    rows = []
    for entry in power["entries"]:
        index = entry["index"]
        pair = list(image.read(table + index * SIZE + 6, 2, "data"))
        rows.append({"index": index, "base": entry["hp_base"], "selectors": pair})
    choices = {tuple(row["selectors"]) for row in rows}
    if any(not 1 <= row["base"] <= 10000 for row in rows):
        pair, status = None, "zero or out-of-range HP base in source route"
    elif len(choices) != 1:
        pair, status = None, "source selectors disagree"
    else:
        value, = choices
        if any(not 0 <= selector < ladders.SELECTORS for selector in value):
            pair, status = None, "source selector outside verified rows"
        else:
            pair, status = list(value), "qualified selector pair"
    return {"table": power["table"], "entries": rows, "selectors": pair, "status": status}


def profile_rows(report, image, bindings):
    moves = {move: move_selectors(image, binding) for move, binding in sorted(bindings.items())}
    choices = {tuple(move["selectors"]) for move in moves.values() if move["selectors"] is not None}
    lines = []
    shared = None
    if len(choices) == 1:
        shared, = choices
        lines += ladders.emit_team_rows("damage-credit-hp", report["roles"]["hp"], shared[0])
        lines += ladders.emit_team_rows("damage-credit-force", report["roles"]["force"], shared[1])
        lines += [f"damage-credit-ladders={name}" for name, move in moves.items() if move["selectors"] is not None]
    return {"selectors": list(shared) if shared is not None else None, "moves": moves, "lines": lines,
            "status": "shared pair for verified moves" if lines else
                      "different source pairs in one package" if choices else "no qualified direct moves"}


def export(dol, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("attacker ladder output already exists")
    with ReadOnlyFile(dol) as source:
        if not 0 < source.size <= 8 << 20:
            raise ValueError("source DOL extent")
        result = recover(DolImage(source.read(0, source.size), SOURCE_SHA256))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("x", encoding="ascii") as stream:
        stream.write(json.dumps(result, indent=2, sort_keys=True) + "\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dol", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    export(args.dol, args.output)
