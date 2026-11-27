"""Index the wall helper, scoped ancestors, and the selector-0 effect lifetime."""
import argparse
from collections import Counter
from dataclasses import dataclass
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import retail_type32_roster as beam
from tools.analyze import retail_type4_ancestors as siblings, retail_type5_roster as area
from tools.analyze import retail_type5_sibling_exit as exits, retail_type5_sibling_stage as stage
from tools.analyze.retail_action_links import ActionLinker, immediate_argument, literal_flow
from tools.analyze.retail_code_atlas import address
from tools.analyze.retail_projectile_census import (
    AFS_SHA256, ATLAS_SHA256, INSTALLERS_SHA256, SOURCE_SHA256, callers, export_census, read_metadata,
)
from tools.analyze.retail_type76_paid import ROSTER_SHA256 as TYPE76_SHA256


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"
STAGE, EFFECT, FACTORY = 0x80084170, 0x80019550, 0x8001959C
TYPE89_SHA256 = "1474719e96bfc7b391c610e8535ee3d0a105290ebc33ecc612232f1e21fcfbb6"
CASES = ("clock-one", "clock-half", "no-shapes", "parent-inactive", "no-class2", "no-descriptor")
ROW = re.compile(r"wall-effect: [0-5] [01](?: [0-9]{1,2}){3}(?: [0-9]\.[0-9]{5}){3}")
TEXT = (
    *tuple(s for s in stage.TEXT if s[0] in (STAGE, EFFECT, FACTORY)),
    (0x800196D0, 60, "7eaa87daf2c89e1c50f7c07f0f861c7a9e45ff260be9cc74dbc08032b01bde7c"),
    (0x8001970C, 68, "aa92febd2600614fe7cbb4e4d32cbec6863f2393535c6cc218d915c365a541fe"),
    (0x80019750, 60, "37a597d38b783346c22697fa6de37a00a5903957eb11d1c9d6df3daaf2e1da0e"),
    (0x8001978C, 20, "9212e9caa3a25e54a968e0a58a25f7946023df4677e5701fd0ee94e4396d8819"),
    (0x800197A0, 72, "feb8feea9e3106ffc4d5b05e23e345d5da533a720770cd1065c644217ee80078"),
    (0x80019860, 240, "1836a52b4720d810cedfcfe3f3c0e9720b110251cf6f10ed40eb959b1449533e"),
    (0x80019950, 132, "772e20902700701da19b8e30231afd7b7419fcd2efc5a16641ff2685bfdca385"),
    (0x8008893C, 356, "ca5d566a4630c0e0846ee4c3fed9ab52590fc334db7c2d539a4eb2d5a473f276"),
)


@dataclass(frozen=True)
class Parent:
    constructor: int
    update_store: int
    # Updater, indexed load site, table, slot; type 89 has an inner phase table.
    phases: tuple[tuple[int, int, int, int], ...]
    callback: int
    site: int


PARENTS = {
    14: Parent(0x8006EE14, 0x8006EE9C, ((0x8006F0CC, 0x8006F0FC, 0x802D3AC4, 1),), 0x8006F268, 0x8006F3C8),
    15: Parent(0x8007DB5C, 0x8007DBEC, ((0x8007DCEC, 0x8007DD64, 0x802D6F98, 1),), 0x8007E200, 0x8007E5D8),
    41: Parent(0x800E058C, 0x800E05F8, ((0x800E06C4, 0x800E073C, 0x803170B4, 1),), 0x800E0A30, 0x800E0D3C),
    47: Parent(0x800BE948, 0x800BE9F4, ((0x800BEC20, 0x800BEC98, 0x80301C08, 1),), 0x800BEE1C, 0x800BF0B4),
    76: Parent(0x800FCD38, 0x800FCDA4, ((0x800FCE50, 0x800FCEC8, 0x8031EED8, 1),), 0x800FD09C, 0x800FD254),
    89: Parent(0x8016CC24, 0x8016CC80, ((0x8016CD1C, 0x8016CDB0, 0x80350624, 1),
                                       (0x8016CFE8, 0x8016D024, 0x80350634, 0)), 0x8016D060, 0x8016D2AC),
    **{kind: Parent(p.constructor, p.update_store, ((p.update, p.phase_load, p.phase_table, p.phase),),
                    p.branch, stage.STAGE_CALLS[kind])
       for kind, p in siblings.PARENTS.items() if kind in stage.STAGE_CALLS},
}


def calls(image, atlas):
    site_groups = {callee: callers(image, atlas, callee, 256) for callee in (STAGE, EFFECT, FACTORY)}
    if tuple(map(len, site_groups.values())) != (55, 11, 1) or set(site_groups[FACTORY]) != {0x80019588}:
        raise ValueError("wall helper direct-call coverage changed")
    linker = ActionLinker(image, {}, ())
    rows, histogram = {}, Counter()
    for callee in (STAGE, EFFECT):
        for pc, row in sorted(site_groups[callee].items()):
            entry = int(row["function"], 16)
            if pc == 0x8002F87C:
                if callee != EFFECT or beam.word(image, pc-4) != 0x88BD0009:
                    raise ValueError("wall helper dynamic contact byte changed")
                evidence = {"kind": "dynamic-contact-descriptor-byte", "load_site": "8002F878",
                            "record_offset": 9, "selector_values": None}
            else:
                words, incoming, outgoing, direct, frontier, exhausted = linker.graph(
                    entry, atlas["functions"][row["function"]]["size"], 1000)
                reg = 4 if callee == STAGE else 5
                local = immediate_argument(words, incoming, pc, reg)
                value = literal_flow(entry, words, outgoing, linker.data).get(pc, {}).get(reg)
                if (exhausted or frontier or direct.get(pc) != callee
                        or (local != value and (pc, local, value) != (0x801B9F78, None, 0))):
                    raise ValueError(f"wall helper call argument changed at {address(pc)}")
                if callee == STAGE:
                    histogram[value] += 1
                    evidence = {"kind": "unused-wrapper-r4", "value": value}
                elif value == 0:
                    evidence = {"kind": "literal-effect-selector", "value": 0, "flow": "local" if local is not None else "merged"}
                else:
                    raise ValueError("wall helper selector no longer matches key 0")
            rows[pc] = {"site": address(pc), "function": row["function"], "target": address(callee), "operand": evidence}
    if histogram != {0: 1, 1: 26, 8: 3, 255: 25} or beam.word(image, 0x80084190) != 0x389D0020:
        raise ValueError("wall wrapper's unused argument or source-position replacement changed")
    return rows, histogram


def parent_index(image, atlas, sites):
    linker = ActionLinker(image, {}, ())

    def literal(entry, pc, r):
        words, _, outgoing, _, _, exhausted = linker.graph(entry, atlas["functions"][address(entry)]["size"], 1000)
        if exhausted:
            raise ValueError("wall helper parent graph budget")
        return literal_flow(entry, words, outgoing, linker.data).get(pc, {}).get(r)

    result = []
    for kind, parent in sorted(PARENTS.items()):
        updater = parent.phases[0][0]
        op = beam.word(image, parent.update_store)
        if op >> 26 != 36 or op & 0xFFFF != 12 or literal(parent.constructor, parent.update_store, (op >> 21) & 31) != updater:
            raise ValueError(f"wall helper parent update store changed: {kind}")
        for i, (entry, load, table, slot) in enumerate(parent.phases):
            target = parent.phases[i+1][0] if i+1 < len(parent.phases) else parent.callback
            if (beam.word(image, load) != 0x7D84002E or literal(entry, load, 4) != table
                    or beam.word(image, table+4*slot, "data") != target):
                raise ValueError(f"wall helper parent phase chain changed: {kind}")
        expected = EFFECT if kind == 14 else STAGE
        if sites[parent.site]["function"] != address(parent.callback) or sites[parent.site]["target"] != address(expected):
            raise ValueError(f"wall helper parent callback no longer owns its call: {kind}")
        result.append({"parent_type": kind, "constructor": address(parent.constructor), "update": address(updater),
                       "callback": address(parent.callback), "site": address(parent.site),
                       "dispatch_slots": [address(table+4*slot) for _, _, table, slot in parent.phases]})
    return result


def candidate_index(image, selections, area_receipt, small, type76, type89):
    picked = {r["id"]: r for r in selections["borgs"]}
    if len(picked) != 206:
        raise ValueError("wall helper standalone selection coverage")
    result = []

    def join(source, parent, kind, scope, actions=None):
        model = picked[source["borg"]]
        if model["spec_record"] != 0 or model["profile"] != source["profile"] or source.get("spec_record", 0) != 0:
            raise ValueError("wall helper candidate not in reviewed normal selection")
        p = PARENTS[kind]
        beam.link(image, int(parent["site"], 16), p.constructor)
        result.append({"borg": source["borg"], "profile": source["profile"], "parent": {**parent, "type": kind},
                       "actions": actions if actions else [source["action"]] if "action" in source else [],
                       "effect_selector": 0, "callback_site": address(p.site), "ancestor_scope": scope})

    for r in small["candidates"]:
        kind = int(r["family"].split("-")[1])
        if kind in (14, 15, 47):
            parent = {"site": r["site"], "key": r["key"]}
            for field in ("additional_guard", "release_condition", "required_model_at_call"):
                if r.get(field) is not None:
                    parent[field] = r[field]
            actions = sorted({a["group"] for a in r.get("action_candidates", ())})
            join(r, parent, kind, "static small-carrier ancestor; only its later terrain branch", actions)
    for r in type76["candidates"]:
        join(r, {"site": r["site"], "key": r["parent_key"]}, 76, "static normal parent; not the outgoing hit or zero-HP exit")
    for r in type89["candidates"]:
        join(r, {"site": r["site"], "key": r["key"], "condition": r["condition"]}, 89,
             "static type-89 candidate; only the terrain-gated subphase")
    for r in area_receipt["candidates"]:
        p = r["parent"]
        if p.get("family") == "type-41":
            join(r, {"site": p["site"], "key": p["key"], "debit": p["debit"]}, 41,
                 "static normal paid placement; the type-41 stage branch")
        elif "branches" in r["descendant"]:
            join(r, {"site": p["site"], "keys": p["keys"], "selector_scope": p["selector_scope"]}, p["type"],
                 "static normal predecessor; distinct from the actor-contact area request")
    return sorted(result, key=lambda r: (r["borg"], r["parent"]["site"], r["actions"]))


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 1024 or len(lines) != len(CASES):
        raise ValueError("wall effect report extent")
    rows = []
    for i, (label, line) in enumerate(zip(CASES, lines, strict=True)):
        made = i < 4
        times, scales = ((21, 22, 23), (1., 2., 3.)) if made else ((0, 0, 0), (0., 0., 0.))
        values = line.split()[1:]
        if (ROW.fullmatch(line) is None or tuple(map(int, values[:5])) != (i, made, *times)
                or tuple(map(float, values[5:])) != scales):
            raise ValueError(f"wall effect lifetime, curve or allocation changed: {label}")
        rows.append({"case": label, "class2_supplied": i != 4, "descriptor_supplied": i != 5, "shape_slot_supplied": i != 2,
                     "owner_clock_step": 0.5 if i == 1 else 1, "parent_active_at_init": i != 3,
                     "allocated": made, "scale_samples_at_visits_1_5_21": list(scales) if made else None,
                     "phase_two_visit": times[0] or None, "phase_three_visit": times[1] or None,
                     "return_visit": times[2] or None, "attack_registry_entries": 0})
    return rows


def export(dol, afs, atlas, selections, fixture, module, output):
    def recover(image, _archive, code_atlas, installed):
        beam.verify_text(image, TEXT)
        sites, histogram = calls(image, code_atlas)
        chain = parent_index(image, code_atlas, sites)
        for pc, target in ((0x8008420C, EFFECT), (0x80019588, FACTORY), (0x800195D4, 0x8008893C), (0x800197D0, 0x80088E50)):
            beam.link(image, pc, target)
        if (image.read(0x802C7ED0, 4, "data") != b"\0\0\0\1"
                or struct.unpack(">4I", image.read(0x802C8164, 16, "data")) != (0x8001970C, 0x80019750, 0x8001978C, 0x800197A0)
                or beam.word(image, 0x802C8174, "data") != 0x80019860 or beam.word(image, 0x802C8184, "data") != 0x80019950
                or beam.word(image, 0x802C8154, "data") != 0x802C7F54
                or struct.unpack(">2h2I", image.read(0x802C7F54, 12, "data")) != (21, 20, 0x802C7EF4, 0x802C7F34)):
            raise ValueError("wall effect selector-0 presentation row changed")
        receipts = tuple(read_metadata(MAPS / name, digest) for name, digest in (
            ("gg4e08-type5-roster.json", exits.CENSUS_SHA256), (area.PARENT_CENSUS, area.PARENT_CENSUS_SHA256),
            ("gg4e08-type76-roster.json", TYPE76_SHA256), ("gg4e08-type89-roster.json", TYPE89_SHA256)))
        candidates = candidate_index(image, installed, *receipts)
        args = [str(Path(fixture).resolve(strict=True)), str(Path(dol).resolve(strict=True)),
                str(Path(module).resolve(strict=True))]
        proc = run(args, capture_output=True, timeout=45)
        if proc.returncode or proc.stderr:
            raise ValueError(f"wall effect fixture failed: {proc.stderr.decode('utf-8', 'replace').strip()}")
        cases = decode_report(proc.stdout.decode("ascii"))
        return {
            "kind": "gf-retail-wall-effect-census", "revision": 1, "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
            "code_atlas_sha256": ATLAS_SHA256, "standalone_installers_sha256": INSTALLERS_SHA256,
            "type5_census_sha256": exits.CENSUS_SHA256, "small_parent_census_sha256": area.PARENT_CENSUS_SHA256,
            "type76_census_sha256": TYPE76_SHA256, "type89_census_sha256": TYPE89_SHA256,
            "wrapper_argument_histogram": dict(histogram), "call_sites": list(sites.values()), "parent_dispatch_links": chain,
            "candidates": candidates, "cases": cases,
            "selector_zero": {"factory": address(FACTORY), "record": "802C7F54", "visual_record": 21,
                              "active_updates": 20, "object_tag": 0, "class": 2, "bucket": 31, "descriptor_cells": 1},
            "counts": {"stage_sites": 55, "effect_helper_sites": 11, "known_selector_zero_sites": 10,
                       "unresolved_dynamic_selectors": 1, "candidate_rows": len(candidates),
                       "candidate_models": len({r["borg"] for r in candidates}), "lifetime_cases": len(cases)},
            "notes": [
                "The stage wrapper overwrites r4 before geometry. Its observed call operands are not effect selectors.",
                "The direct contact path reads a descriptor byte; its selector values are unresolved here.",
                "Parent slots are static ancestry, not live terrain admission. Type 14 uses a different terrain kernel and calls the effect helper directly.",
                "The selector-zero fixture uses a direct request, empty render records, and the original fixed bucket; it publishes no collider.",
                "No damage, provenance for other effect modes, live scheduling, or native gameplay is claimed.",
            ],
        }

    return export_census(dol, afs, atlas, selections, output, recover)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "atlas", "selections", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.atlas, args.selections, args.fixture, args.module, args.output), sort_keys=True))
