"""Qualify GG4E08's shared action row lookup and schedule writes.

This models the lookup once a group and actor state have been chosen.
It does not infer which inputs or gameplay states produce those fields.
"""
import argparse
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze.retail_code_atlas import MAX_SYMBOLS_BYTES, address, boundaries, object_sizes, read_bounded, roster_paths
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.retail_borg_placeholders import CATALOG_SHA256, read_catalog
from tools.extract.source_image import DolImage

# Exact GG4E08 functions reviewed to establish the row calculation and
# schedule writes. The pre-selector request producer is not qualified here.
SPANS = {
    0x80055568: ("tick-dispatch", 0x124, "4ef4d2db70068dec76e280773b3d58c546452599966914bffd4f8cbaf56472c7"),
    0x80065DFC: ("first-slot-61-relay", 0x28, "c594bd68034bba3cc31848d5a87038e876cdfc890c341310c2294b17e0bf21a5"),
    0x80069DD0: ("normal-selector", 0xD4, "d47e5fab253284676c8062bab436402e6ba6e75cfcc3e3df2c8b86b1ead76830"),
    0x80069EA4: ("directional-lookup", 0x13C, "8b0efdb5a565ee7ef2266109d0ee51d37118bcebb03821f06b9418cadaeac9d7"),
    0x80069FE0: ("flat-lookup", 0x124, "f8d9d9dfd94953141d97db2e49ced5c0525a95ef5154c2f2d2fb3771d61e9b26"),
    0x8006A104: ("apply-row", 0x2CC, "476ca1cbad57dc6ad22dbe4c919646740f7d517f813bcc9f7da70ccb7474253f"),
    0x8006A3D0: ("explicit-lookup", 0xA4, "6e3e98f9b1da00d7d1f37b88f3e81aeede6f6a6c9304d9ad4fd00e302699cc16"),
    0x8006C674: ("sector-classifier", 0xB4, "4381614f1242f91931948f08fe8952e48ab1ac34b5e9561a2e20f4150fd08110"),
    0x8006A6FC: ("first-handler-slot", 0x54, "4efe53e520303e6ad0cf238f8a53d503a8f6ad394e161cc93649b7484b847b0c"),
    0x8006A750: ("second-handler-slot", 0x64, "0512ba29e89a10d5dc9786ce20496633ed39f7b752b7c0313fa5afa0673e7233"),
}
HANDLER_TABLES = {"first": 0x802D36A8, "second": 0x802D37A8}


def normal_choice(mode, flags, stance, lower_handler, guard_applies):
    """Return the row before sector selection; None means refused.

    `guard_applies` means +0x595 == 0 and finite +0x694 > 0,
    after any work in the normal selector's prelookup path.
    """
    special_lower = flags & 0x40 or lower_handler == 4
    if mode == 1:
        if guard_applies:
            return None
        if flags & 0x100:
            return 5
        if flags & 0x80:
            return 4
        if flags & 0x20:
            return 2 if stance else 3 if flags & 0x40 else 0
        return 3 if special_lower else 1 if flags & 0x10 else 0
    if mode == 0:
        if flags & 0x80:
            choice = 4
        elif flags & 0x20:
            choice = 1 if stance else 2 if flags & 0x40 else 0
        else:
            choice = 2 if special_lower else 3 if flags & 0x200 else 0
        return None if guard_applies and choice != 3 else choice
    return None


def sector_from_angle(angle, lo_inner, hi_inner, lo_outer, hi_outer):
    """Classify an already-computed signed s16 angle; None means no target."""
    if angle is None:
        return 0
    if angle > hi_outer:
        return 2
    if angle > hi_inner:
        return 1
    if angle >= lo_inner:
        return 0
    return 3 if angle >= lo_outer else 2


def possible_choices(mode):
    known = (normal_choice(mode, flags << 4, stance, lower, False)
             for flags in range(64) for stance in (0, 1) for lower in (0, 4))
    return sorted({i for i in known if i is not None})


def review_cells(borgs, profiles):
    """Count data cells by profile reference; retain the exceptional cells."""
    choices = {mode: possible_choices(mode) for mode in (-1, 0, 1)}
    counts, exceptions = Counter(), []
    for borg in borgs:
        for ident in borg["profiles"]:
            counts["profile_references"] += 1
            profile = profiles[ident]
            for group in profile["groups"]:
                mode = group["mode"]
                counts[f"groups_mode_{mode}"] += 1
                for row in group["rows"]:
                    for choice, sector in row["contexts"]:
                        counts["data_cells"] += 1
                        cls = row["classification"]
                        possible = choice in choices[mode]
                        counts[f"{cls}_cells"] += 1
                        if not possible:
                            counts["not_selected_by_normal_lookup"] += 1
                            counts[f"not_selected_{cls}"] += 1
                        if cls == "selector-candidate" and possible:
                            counts["candidate_cells_not_excluded_by_choice"] += 1
                        if cls != "selector-candidate" or not possible:
                            exceptions.append({
                                "id": borg["id"], "profile": ident, "group": group["group"],
                                "choice": choice, "sector": sector, "classification": cls,
                                "excluded_by_normal_choice": not possible,
                            })
    counts["borgs"] = len(borgs)
    return counts, exceptions


def handler_tables(image, sizes, function_spans):
    tables = {}
    for slot, start in HANDLER_TABLES.items():
        if sizes.get(start) != 64 * 4:
            raise ValueError(f"{slot} handler table extent")
        targets = struct.unpack(">64I", image.read(start, 64 * 4, "data"))
        if any(pc not in function_spans for pc in targets):
            raise ValueError(f"{slot} handler table target is not a function entry")
        tables[slot] = {"address": address(start), "targets": [address(pc) for pc in targets]}
    return tables


def recover(image, catalog, symbols):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    for start, (_, size, digest) in SPANS.items():
        if sha256(image.read(start, size, "text")).hexdigest() != digest:
            raise ValueError(f"selector signature mismatch at {address(start)}")
    if image.read(0x8043762C, 4, "data") != bytes(4):  # r2 - 29684
        raise ValueError("selector zero constant mismatch")
    sizes = object_sizes(symbols)
    tables = handler_tables(image, sizes, boundaries(image, symbols))
    borgs, profiles, _ = roster_paths(image, catalog, sizes)
    counts, exceptions = review_cells(borgs, profiles)
    return {
        "kind": "gf-retail-action-selector", "revision": 1,
        "source_dol_sha256": SOURCE_SHA256, "catalog_sha256": CATALOG_SHA256,
        "symbol_metadata_sha256": sha256(symbols).hexdigest(),
        "functions": {role: address(start) for start, (role, _, _) in SPANS.items()},
        "normal": {
            "mode": "signed config byte at actor+0x4B0 -> 0xB8 + group",
            "group": "actor+0x585",
            "context_table": "actor+0x4EC -> word pointer for group",
            "mode_0": {"choice": "actor+0x586", "cell_byte_offset": "4 * choice",
                       "possible_choices": possible_choices(0), "commit_call": "8006A0F0"},
            "mode_1": {"choice": "actor+0x586", "sector": "actor+0x587",
                       "cell_byte_offset": "16 * choice + 4 * sector",
                       "possible_choices": possible_choices(1), "commit_call": "80069FC8"},
            "guard": "when +0x595 == 0 and +0x694 > 0: mode 1 refuses; mode 0 allows only choice 3",
            "sector_thresholds_in_test_order": ["spec+0x15C", "spec+0x158", "spec+0x15A", "spec+0x15E"],
        },
        "explicit": {
            "inputs": ["group", "choice", "sector"],
            "commit_calls": ["8006A434", "8006A460"],
            "note": "Mode 1 uses sector; otherwise the entry uses a supplied flat choice, without the normal mode check.",
        },
        "commit": {
            "shape": ["signed selector", "lane bits", "state", "substate"],
            "explicit_refusal": "signed selector == -1",
            "state_bytes": ["actor+0x580", "actor+0x581"],
            "handler_pair": "two signed bytes at actor+0x4F0 -> 2 * selector",
            "first": "always attempted on committed rows, stored at actor+0x5DA if first handler != -1",
            "second": "only if lane byte is exactly 3; stored at actor+0x5DB if second handler != -1",
            "zero": "A zero-filled row is not an explicit refusal; downstream behavior is not qualified.",
        },
        "tick_dispatch": {
            "tables": tables,
            "snapshot_slots": {"first": "800555CC", "second": "800555C4"},
            "order": [
                {"call": "800555C0", "callback": "actor+0x4BC", "lane_marker": 2},
                {"call": "800555F0", "callback": "second slot snapshot", "lane_marker": 1},
                {"call": "80055618", "callback": "first slot snapshot", "lane_marker": 0},
                {"call": "80055630", "callback": "actor+0x4C0", "lane_marker": 2},
            ],
            "lane_marker_field": "actor+0x578",
            "first_slot_61": "80065DFC calls the installed main handler at actor+0x4B4",
            "zero_row_first_slot_1": "8005F23C conditionally requests or resets a common program; no hit is established",
        },
        "notes": [
            "Profile ids refer to the same symbol-metadata revision of the GG4E08 code atlas.",
            "Counts are raw data cells per Borg/profile reference; choices not excluded are not proven reachable.",
            "The normal selector's prior busy/flag checks and the live input producers are not modeled.",
            "The explicit entry is not permission to use a disabled or absent retail table.",
            "Sector comparison is bounded to signed angle/threshold inputs, not a native atan replacement.",
        ],
        "counts": counts, "exceptional_cells": exceptions,
    }


def export(dol, catalog, symbols, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("action selector map output already exists")
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    payload = recover(image, read_catalog(catalog), read_bounded(symbols, MAX_SYMBOLS_BYTES))
    # The exception list is one record per line for an inspectable diff.
    items = []
    for key, value in sorted(payload.items()):
        if key == "exceptional_cells":
            text = "[\n" + ",\n".join("    " + json.dumps(row, sort_keys=True) for row in value) + "\n  ]"
        else:
            text = json.dumps(value, sort_keys=True)
        items.append(f"  {json.dumps(key)}: {text}")
    raw = ("{\n" + ",\n".join(items) + "\n}\n").encode("ascii")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), **payload["counts"]}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "catalog", "symbols", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.catalog, args.symbols, args.output), sort_keys=True))
