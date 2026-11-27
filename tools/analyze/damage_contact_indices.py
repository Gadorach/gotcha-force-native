"""Export the indexed credit/receiver curves without guessing a mode scheduler."""
import argparse
from hashlib import sha256
import json
import math
from pathlib import Path
import struct

from tools.analyze import damage_content_row as rows
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

SETTINGS, RANKS = 32, 8
VERIFIED = (
    (0x80006ABC, 184, "9e588ad6c753b97f29c29a48491f2ec614c07e2bad627e223c1e67c412dd3ba1"),
    (0x800561E4, 24, "73a1dd1c30e5ca0d69e5674f216b85c2c47fa6a691dea1bf9db728f266c62313"),
    (0x80173680, 156, "b505feb0e9fc6440aa5c72eba7295f1ee93a36b14bd129cbb5d357f4df7ac452"),
)


def scene_setting_writes(state, bias=0):
    if state == 1:
        return [bias & 31, (31 - bias) & 31, None, None]
    if state == 2:
        return [7, 7, 7, 7]
    return [None] * 4  # No writes, not a neutral or zero table.


def recover(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    if any(sha256(image.read(at, size, "text")).hexdigest() != digest for at, size, digest in VERIFIED):
        raise ValueError("damage selector initialization proof changed")
    roots = (rows.CREDIT_ROOT, rows.ATTACKER_RANK_ROOT, rows.CLASS_ROOT, rows.RECEIVER_RANK_ROOT)
    pairs = [[rows._row(image, root, team) for team in (0, 1)] for root in roots]
    if [right - left for left, right in pairs] != [SETTINGS * 4, RANKS * 4, rows.CLASS_COUNT * 4, RANKS * 4]:
        raise ValueError("indexed damage side extent changed")

    def factors(at, count):
        values = [value for value, in struct.iter_unpack(">f", image.read(at, 4 * count, "data"))]
        if any(not math.isfinite(value) or not .01 <= value <= 4 for value in values):
            raise ValueError("indexed damage factor outside native domain")
        return {"address": f"{at:08X}", "cells": [round(value, 6) for value in values]}

    sides = []
    for team in (0, 1):
        credit, offense, class_anchor, defense = (table[team] for table in pairs)
        classes = [rows._u32(image, class_anchor + i * 4) for i in range(rows.CLASS_COUNT)]
        if classes != list(range(classes[0], classes[0] + SETTINGS * 4 * rows.CLASS_COUNT, SETTINGS * 4)):
            raise ValueError("indexed receiver class row extent changed")
        sides.append({"team": team, "credit_setting": factors(credit, SETTINGS),
                      "credit_rank": factors(offense, RANKS), "receive_rank": factors(defense, RANKS),
                      "receive_setting": [factors(at, SETTINGS) for at in classes]})
    return {"kind": "gf-damage-contact-indices", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
            "setting_count": SETTINGS, "rank_count": RANKS, "sides": sides,
            "source": {
                "table_roots": [f"{root:08X}" for root in roots],
                "side_selector": "actor+998 != 0; distinct from the settings and rank indices",
                "setting_index_slot_offset": 136,
                "setting_array": "803C4E60", "setting_writer": "80006ABC", "writer_state_offset": 50,
                "state_1_bias_scene_offset": 70, "state_1": ["bias & 31", "(31 - bias) & 31", None, None],
                "state_2": scene_setting_writes(2), "other_states": scene_setting_writes(0),
                "rank_initializer": "800561E4..800561F8", "rank_actor_offset": 1082,
                "rank_source": "Scene[62 + s8(actor+996)]",
                "rank3_active_slot_writer": "80173710", "rank3_writer_value": 3,
            },
            "scope": "optional caller-selected battle settings by native team and rank by actor slot; "
                     "no original mode/state, conditional amount or rank scheduler"}


def profile_lines(report, receiver_class):
    if receiver_class is not None and not 0 <= receiver_class < rows.CLASS_COUNT:
        raise ValueError("indexed receiver class outside verified table")

    def format_row(key, team, row):
        return f"damage-{key}={team}|" + "|".join(map(rows._number, row["cells"]))

    lines = []
    for side in report["sides"]:
        team = side["team"]
        lines.extend(format_row(f"credit-{key}", team, side[f"credit_{key}"]) for key in ("setting", "rank"))
        if receiver_class is not None:
            lines.append(format_row("receive-setting", team, side["receive_setting"][receiver_class]))
            lines.append(format_row("receive-rank", team, side["receive_rank"]))
    return lines


def export(dol, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("indexed damage output already exists")
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
