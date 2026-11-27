"""Inventory type-8/type-9 relay keys and replay type-9 common shape admission."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import retail_type32_roster as beam
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_action_links import ActionLinker, immediate_argument, literal_flow
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import (
    AFS_SHA256, ATLAS_SHA256, SOURCE_SHA256, address, callers, encode, read_metadata,
)
from tools.analyze.retail_type5_roster import common_bank
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage


PARENT, EFFECT = 0x800FC208, 0x800FC6A0
TABLE, STRIDE, ROW_COUNT = 0x8031EB60, 48, 7
TYPE76_TABLE, SCRATCH_WORKERS = 0x8031EE18, 0x80391658
INIT, BIND, AMOUNT_TABLE = 0x800FC810, 0x8008AE3C, 0x802C4760
GROUP, DESCRIPTOR = 84, 122
TYPE32_DIGEST = "947c3048c451682f2e2e7b694a4e42d5ec98dd5b3a5690b00ebfefc8b1cf3806"
LITERALS = {0x8018D234: (PARENT, 5), 0x801C0398: (PARENT, 6),
            0x801DF69C: (EFFECT, 3), 0x801DF8CC: (EFFECT, 4)}
MIRRORS = {0x800FC428: "type-8-initializer", 0x800FC678: "type-8-active"}
INDEXED = {0x800FD314: (PARENT, 0), 0x800FD324: (EFFECT, 1)}
ROW = re.compile(r"type9-bind: [0-6] [01] [0-9]{1,3} [01] [0-9] [0-9]{1,3} [0-9]")
TEXT = (
    (PARENT, 292, "a76123aad2604c1b0f4f430bf9195ec90e0e564a7ef4516d2898e9f484ffc5fb"),
    (0x800FC32C, 80, "e3459ea3be7ff9f79741a95acd545f94b5f913167b15aed9e914e658e8336242"),
    (0x800FC37C, 196, "b36d91a77aa642c6082254e0b93e29f1b85f518e559cd6f8a1612f1a2b07a14f"),
    (0x800FC440, 608, "86161fa3075faab2da93b27a78c95b4bf204523fdfad2ed652225205da0cd448"),
    (EFFECT, 288, "4050c866c59419d4115c98a2d59cfb666b9d57191878a4e2a496f47f20eaff87"),
    (0x800FC7C0, 80, "7dd085af31393d70efb8ae3692e9843be03a60cc0839d1fa038fc7d210cb091d"),
    (INIT, 420, "bdf61d3d9a73878b720924ec8a637e7bbf487f8a8cba932248d39645cc10da2b"),
    (0x800FD2D0, 104, "40db03855975bc14a76fb5601752008c6b3b590b09b56b6c57cacfb9cca862c6"),
    (0x800FCD38, 280, "10393b32823c52bbef533872a6e45d28387754710e28afc3c12095246fa8a1d1"),
    (0x800FD09C, 492, "702496f89e2d6bf514625520ce1eacad7fb87c658bf5fc00abccf06730830703"),
    (0x801DF648, 144, "055ff13a8da10e8ebcdfa6212d8108566a64aa9d30d94d434c2bea0da02fc30d"),
    (0x801DF878, 144, "6a5d8c892832fceb37045762b4c695e98c60713cb5ee20e092d663b4a71ade27"),
    (0x8018D1D8, 116, "7c0ce9599c6b013c0672bbc40b388504334467d0b9f27baabe53562645989d9c"),
    (0x801C033C, 116, "e17a32f68851f3e7c38f41963065b4794a26dbcb37726fd2e6879721e0763edf"),
    (0x801DEF80, 1156, "97e5de60c309f204b00e12b5e98ac191d75b723adcb2e19094d40dc741b38f90"),
    (BIND, 36, "4a5238df2bbfcfc97ddc6e932511a41407c18a16b307bcfa3eb7827b46ecd02f"),
    (0x8008AE60, 400, "ed773c3f3f4973120e9f352561f67629417f7a374b39e2215c65901ffba46c62"),
)


def pinned_data(image, at, size, digest):
    data = image.read(at, size, "data")
    if sha256(data).hexdigest() != digest:
        raise ValueError(f"type-9 source data mismatch at {address(at)}")
    return data


def source_index(image, bank):
    beam.verify_text(image, TEXT)
    for pc, callee in ((0x800FD100, 0x800FD2D0), (0x800FD26C, 0x800FD2D0),
                       (0x800FC870, BIND), (0x8008AE4C, 0x8008AE60)):
        beam.link(image, pc, callee)
    table = pinned_data(image, TABLE, ROW_COUNT*STRIDE,
                        "737febc58d2728b8431a4e35dcdb55485013ddc6c9d24bca0295097f2db5e51e")
    parent_table = pinned_data(image, TYPE76_TABLE, 6*32,
                               "8b067fe9d3015062afdc967b4dd5b2c1bacc120b04b8e63b3ca7243672ced13d")
    effect_data = pinned_data(image, 0x8031ED10, 12,
                              "ff845adb7f21956b6d6fbe02016a1512685b5d571bc0c28b34dba65e2b48a244")
    phases = pinned_data(image, 0x8031ED1C, 32,
                         "dc811ad581020e1133297d5111b7c755acaddb3163ad09c2f07c652895db00e7")
    workers = struct.unpack(">10I", pinned_data(image, SCRATCH_WORKERS, 40,
                                                "9ecbd8765799eda29cce231f0e3622339015ff2a8cfab6fc1cbf5ab9b0bc7d97"))
    effect_rows = []
    for key in range(ROW_COUNT):
        off = key*STRIDE
        group, parent_operand = struct.unpack_from(">2h", table, off)
        y, xz = struct.unpack_from(">2f", table, off+36)
        if group != GROUP:
            raise ValueError("type-9 row no longer requests its common collision group")
        effect_rows.append({"key": key, "row": address(TABLE+off), "common_group": group,
                            "type8_short_operand": parent_operand, "xz_scale_operand": xz, "y_scale_operand": y})
    parent_rows = []
    for i in range(6):
        lane, key = struct.unpack_from(">2h", parent_table, i*32+28)
        if lane not in (0, 1) or not 0 <= key < ROW_COUNT:
            raise ValueError("type-76 selector outside reviewed domains")
        parent_rows.append({"row_index": i, "row": address(TYPE76_TABLE+32*i), "lane": lane,
                            "descendant_type": 8+lane, "descendant_key": key})
    raw_record = pinned_data(image, AMOUNT_TABLE, 24, "52253b38116d55f3b6d8d597f5ffbedf4d62c138753a6b4ed05a4429299d582c")
    off = 0x400+DESCRIPTOR*80
    kind, matrix, flags, scale_mode, index, last, tail, *sphere = struct.unpack_from(">BBHHhhH4f", bank, off)
    raw_power, = struct.unpack_from(">H", raw_record)
    if (bank[GROUP*4:GROUP*4+4] != bytes((DESCRIPTOR, 255, 255, 255))
            or (kind, matrix, flags, scale_mode, index, last, tail) != (0, 0, 1, 9, 0, -1, 0)
            or sphere != [0., 0., 0., 150.] or raw_power != 0):
        raise ValueError("type-9 common shape or amount record changed")
    initializer, active, phase2, phase3 = struct.unpack_from(">4I", phases, 16)
    return effect_rows, parent_rows, workers, {
        "group": GROUP, "descriptor": DESCRIPTOR, "shape": "sphere", "raw_radius": sphere[3],
        "local_center": sphere[:3], "amount_index": index, "amount_record": address(AMOUNT_TABLE), "raw_power": raw_power,
    }, {
        "dispatch": "8031ED2C", "phase_entries": [address(v) for v in (initializer, active, phase2, phase3)],
        "configured_active_limit": struct.unpack_from(">h", effect_data, 2)[0], "post_bind_cut": "800055FC",
    }


def factory_sites(image, atlas, parent_rows, workers):
    linker = ActionLinker(image, {}, ())
    sites = {}
    for factory in (PARENT, EFFECT):
        found = callers(image, atlas, factory, ROW_COUNT)
        expected = {pc for pc, (target, _) in (LITERALS | INDEXED).items() if target == factory}
        if factory == EFFECT:
            expected.update(MIRRORS)
        if found.keys() != expected:
            raise ValueError("type-8/type-9 factory call coverage changed")
        for pc, record in found.items():
            beam.link(image, pc, factory)
            at = int(record["function"], 16)
            words, incoming, successors, direct, frontier, exhausted = linker.graph(
                at, atlas["functions"][record["function"]]["size"], 480)
            local = immediate_argument(words, incoming, pc, 5)
            flow = literal_flow(at, words, successors, linker.data).get(pc, {}).get(5)
            if frontier or exhausted or direct.get(pc) != factory:
                raise ValueError(f"type-9 bounded selector graph changed at {address(pc)}")
            if pc in LITERALS:
                key = LITERALS[pc][1]
                if local != key or flow != key or not 0 <= key < ROW_COUNT:
                    raise ValueError(f"type-9 literal selector changed at {address(pc)}")
                source = {"kind": "literal-r5", "possible_keys": [key]}
                if at in workers:
                    source["type32_borrowed_rows"] = [i for i, worker in enumerate(workers) if worker == at]
            elif pc in MIRRORS:
                if local is not None or flow is not None or [beam.word(image, pc-d) for d in (8, 4)] != [0x88BF0011, 0x7CA50774]:
                    raise ValueError(f"type-8 source-key copy changed at {address(pc)}")
                source = {"kind": "signed-type8-object-key", "object_offset": "0x11", "phase": MIRRORS[pc]}
            else:
                if local is not None or flow is not None or beam.word(image, pc-8) != 0xA8A4001E:
                    raise ValueError(f"type-76 descendant key load changed at {address(pc)}")
                lane = INDEXED[pc][1]
                eligible = [r for r in parent_rows if r["lane"] == lane]
                source = {"kind": "type76-row-signed-key", "lane": lane, "eligible_rows": [r["row_index"] for r in eligible],
                          "possible_keys": sorted({r["descendant_key"] for r in eligible})}
            sites[pc] = {"factory": address(factory), "function": record["function"], "site": address(pc), "key_source": source}
    type8_keys = sorted({key for site in sites.values() if site["factory"] == address(PARENT)
                         for key in site["key_source"]["possible_keys"]})
    for pc in MIRRORS:
        sites[pc]["key_source"]["possible_keys_through_reviewed_type8_calls"] = type8_keys
    return sites


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 1024 or len(lines) != 2*ROW_COUNT:
        raise ValueError("type-9 bind report extent")
    cases = []
    for i, line in enumerate(lines):
        key, spare = divmod(i, 2)
        phase = 1 if spare else 2
        if (ROW.fullmatch(line) is None
                or tuple(map(int, line.split()[1:])) != (key, spare, GROUP, spare, phase, DESCRIPTOR if spare else 0, 0)):
            raise ValueError(f"type-9 group, shape or frontier mismatch: {key}/{spare}")
        cases.append({"key": key, "spare_shape_cells": spare, "allocated": True, "requested_group": GROUP,
                      "bound_descriptor": DESCRIPTOR if spare else None, "phase_at_cut": phase,
                      "reached_post_bind_frontier": bool(spare), "attack_registry_count_at_cut": 0})
    return cases


def export(dol, afs, atlas, type32, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type-9 output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    atlas = read_metadata(atlas, ATLAS_SHA256)
    type32 = read_metadata(type32, TYPE32_DIGEST)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        bank = common_bank(RetailArchive(stream))
        if stamp != archive_stamp(stream):
            raise ValueError("type-9 AFS changed during extraction")
    rows, parent_rows, workers, collision, phase_data = source_index(image, bank)
    sites = factory_sites(image, atlas, parent_rows, workers)
    candidates = []
    for row in type32["candidates"]:
        if row["factory"] != address(beam.SCRATCH):
            continue
        for pc, site in sites.items():
            source = site["key_source"]
            if row["key"] in source.get("type32_borrowed_rows", ()):
                candidates.append({k: row[k] for k in ("borg", "profile", "action", "state", "condition")})
                candidates[-1].update(parent_call=row["site"], parent_key=row["key"], descendant_site=address(pc),
                                      descendant_key=source["possible_keys"][0], descendant_scope="later terrain crossing")
    if [(r["borg"], r["parent_key"], r["descendant_key"]) for r in candidates] != [("gg4e08:0d03", 8, 4)]:
        raise ValueError("type-9 scoped parent action join changed")
    args = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True))]
    result = run(args, input=bank, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"type-9 fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode_report(result.stdout.decode("ascii"))
    effect_sites = [r for r in sites.values() if r["factory"] == address(EFFECT)]
    effect_keys = {key for row in effect_sites for key in
                   row["key_source"].get("possible_keys", row["key_source"].get("possible_keys_through_reviewed_type8_calls", ()))}
    payload = {
        "kind": "gf-retail-type9-roster", "revision": 1, "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "code_atlas_sha256": ATLAS_SHA256, "type32_census_sha256": TYPE32_DIGEST,
        "factories": {"type8": address(PARENT), "type9": address(EFFECT), "type76_predecessor": "800FCD38"},
        "common_collision": collision, "type9_dispatch": phase_data, "factory_rows": rows, "type76_rows": parent_rows,
        "call_sites": [sites[pc] for pc in sorted(sites)], "candidates": candidates, "cases": cases,
        "rows_without_resolved_type9_call_keys": sorted(set(range(ROW_COUNT))-effect_keys),
        "counts": {"type8_call_sites": len(sites)-len(effect_sites), "type9_call_sites": len(effect_sites),
                   "rows": len(rows), "candidate_rows": len(candidates),
                   "bind_cases": len(cases)},
        "notes": [
            "Factory call coverage is direct atlas edges; a copied type-8 key preserves the supplied selector.",
            "Only the reviewed Saucer worker has a scoped installed action ancestor here. The other edges are not action assignments.",
            "For each declared key, one carrier and descriptor allocate with distinct parent and credited actor. Shape slots are supplied separately.",
            "The replay enters the original opening leaf and stops after bind, before presentation initialization; it does not run an attack registry pass.",
            "Zero raw power is a source operand, not a measured overlap, reaction, damage, visual, or lifetime result. No native gameplay is emitted.",
        ],
    }
    raw = encode(payload, expanded=("factory_rows", "call_sites", "candidates", "type76_rows", "cases"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), **payload["counts"]}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "atlas", "type32", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.atlas, args.type32, args.fixture, args.module, args.output), sort_keys=True))
