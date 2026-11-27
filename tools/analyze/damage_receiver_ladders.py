"""Recover the distinct receiving HP and team-force multiplier ladders."""
import argparse
import hashlib
import json
import math
import struct
from pathlib import Path

from tools.analyze.damage_content_row import _number
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

CELLS = 32
SELECTORS = 8
ROLES = {"hp": (0x80433608, 157, "8003D1EC"),
         "force": (0x80433610, 158, "8003D260")}


def read_team_rows(image, root):
    sides = struct.unpack(">2I", image.read(root, 8, "data"))
    if sides[1] - sides[0] != SELECTORS * 4:
        raise ValueError("damage ladder side-table extent changed")
    grids = []
    for team, side in enumerate(sides):
        pointers = struct.unpack(">8I", image.read(side, SELECTORS * 4, "data"))
        if any(right - left != CELLS * 4 for left, right in zip(pointers, pointers[1:])):
            raise ValueError("damage ladder row stride changed")
        rows = []
        for address in pointers:
            values = struct.unpack(">32f", image.read(address, CELLS * 4, "data"))
            if any(not math.isfinite(x) or not .01 <= x <= 4 for x in values):
                raise ValueError("damage ladder value outside native domain")
            rows.append({"address": f"{address:08X}", "cells": [round(x, 6) for x in values]})
        grids.append({"team": team, "rows": rows})
    return grids


def emit_team_rows(key, role, selected):
    if not 0 <= selected < SELECTORS:
        raise ValueError("damage ladder selector outside verified table")
    return [f"{key}={side['team']}|" + "|".join(_number(x) for x in side["rows"][selected]["cells"])
            for side in role["sides"]]


def recover(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    if hashlib.sha256(image.read(0x8003D12C, 312, "text")).hexdigest() != (
            "8edd9d8bfee85eaab539b37d88d90ad636e6b861a34a50636122afbbde874ea5"):
        raise ValueError("receiver ladder indices changed")
    roles = {}
    for name, (root, selector, multiply) in ROLES.items():
        roles[name] = {"root": f"{root:08X}", "actor_selector_offset": selector,
                       "multiply": multiply, "sides": read_team_rows(image, root)}
    return {"kind": "gf-damage-receiver-ladders", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
            "cells_per_row": CELLS, "selectors_per_role": SELECTORS, "roles": roles,
            "hp_index": "probe=max(0, mirrored HP - raw record+0 base); skip above 200; "
                        "min(31, 32 - floor(32*probe/min(max HP,200)))",
            "force_index": "min(31, 32 - floor(32*mirrored receiving slot force/force cap)); not HP",
            "native_scope": "opt-in direct hits with split roles; native HP and pre-hit receiving team ledger"}


def profile_rows(report, selector):
    if selector is None:
        return []  # No active-component proof, so do not infer either selector.
    lines = []
    for name, offset in (("hp", 1), ("force", 2)):
        lines.extend(emit_team_rows(f"damage-receive-{name}", report["roles"][name], selector[offset]))
    return lines


def export(dol, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("receiver ladder output already exists")
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
