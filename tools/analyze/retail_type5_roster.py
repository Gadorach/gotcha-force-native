"""Census the shared type-5 area rows, key selectors, and scoped ancestors."""
import argparse
from collections import Counter
from dataclasses import dataclass
from hashlib import sha256
from io import BytesIO
import json
import math
from pathlib import Path
import struct

from tools.analyze import retail_dragon_area as area, retail_dragon_carrier as dragon, retail_type89 as type89
from tools.analyze.reaction_projectile_map import word
from tools.analyze.retail_action_links import ActionLinker, direct_branch, immediate_argument, literal_flow
from tools.analyze.retail_code_atlas import address
from tools.analyze.retail_projectile_census import (
    AFS_SHA256, ATLAS_SHA256, INSTALLERS_SHA256, SOURCE_SHA256, callers, export_census, read_metadata,
)
from tools.extract.retail_borg_roster import MEMBER_LIMIT, PzzArchive, RangeFile


FACTORIES = {0x80085E00: ("credited-bucket-relative", 0, 0x80088AA0),
             0x80085F28: ("fixed-bucket", 1, 0x8008893C)}
KEY_COUNT = 27
COMMON_MEMBER = 696
REACTIONS = 0x802C4760
PARENT_CENSUS = "gg4e08-small-projectile-roster.json"
PARENT_CENSUS_SHA256 = "6165047678ba28677e36710661cbdae7f53fc1dfeae89e345cb33118c16d0e4f"
TEXT = (
    *area.TEXT,
    (0x80085F28, 288, "3b9881adfd9d42d60065444456ae65ee0e0db044d21b816facf9b0ec53db79d2"),
    (0x8007E200, 1084, "47042a30e6220d5de32bfaeb52c01c55f36b3c5c005057018ca513ae08494d86"),
    (0x800E0A30, 844, "8103e7ed14df49e0061911901e2b20264cea236f557e9b85918a6a3c62a226d5"),
    (0x800FEAE8, 448, "fd0829145dcd4b9c0b18b63a9169fca6ddb58ea2ed2427736c30f0fac62aad47"),
    (0x801292C8, 516, "b744c1858d8d9e008a75f35e602393b2c85810cdf5fc0948908910b0cabb3675"),
    (0x801E7F28, 60, "8805dca0c59a482c23a6106dd6d126fe80341f03293ba6e90014b40640d164e9"),
    (0x801E7F74, 108, "a11932aded525593c34e0112ede0ed815fde0ac14f9fc7f08eeafbe47cc50885"),
    (0x801FAD10, 276, "aeaacbdb24ef68f20daca96a08dedc1025b7497201f1504d48f6c5e24e110505"),
    (0x801FAE24, 80, "85ef06556c20e2e7049bbb00eeb496cbaf92b9d30faa1fb9c700cd57f052286a"),
    (0x801FB060, 612, "0c428e8bfda14e0f93a7e97b66d47418eca1a01349440794a4fb9a524b764dd3"),
    (0x801304B8, 704, "75d567d05a4ea6ef6537d8d513e3f1f1822187128f2586e81b2cdf4cca442406"),
    (0x800E058C, 312, "d1760c991f463afadd23c634578ce59b8a4a3404894b6fc37e83ca143d2cd320"),
    (0x800FE668, 280, "6a67ec1905ac26db2127e96d5719893728ea4984941548190ae1028f1b5d6f32"),
)


@dataclass(frozen=True)
class Selector:
    handler: int
    base: int
    stride: int
    count: int
    offset: int
    fmt: str
    digest: str
    sites: tuple[int, ...]


SELECTORS = {
    "type-15": Selector(0x8007E200, 0x802D6D68, 56, 10, 45, "b",
                        "cd0f0b8bda5592281c37b7b65d45d379694eec46529b8f7752be68fd7ad131c9",
                        (0x8007E284, 0x8007E5FC)),
    "type-47": Selector(0x800BEE1C, dragon.ROW_TABLE, 48, 6, 46, "b",
                        dragon.ROW_TABLE_SHA, (0x800BF00C, 0x800BF0D8)),
    "type-41": Selector(0x800E0A30, 0x80317064, 20, 4, 16, "i",
                        "b2c99a97c574bae01afbb6a4b251c1b63403220db6ffaf8f4eef689fd5c5f09b",
                        (0x800E0AAC, 0x800E0AD4, 0x800E0B18, 0x800E0B4C, 0x800E0D5C)),
    "type-33": Selector(0x800FEAE8, 0x8031EF80, 32, 3, 1, "b",
                        "4f70c99937992a0fa2e4bc83cc6e24f8eee2d0ec722d6b1981b38e781a7ad649",
                        (0x800FEB34, 0x800FEC8C)),
    "parent-801292C8": Selector(0x801292C8, 0x80330B08, 36, 3, 30, "h",
                                "68751d3d884c5220478a91548f50bd2f4dd91c9d3282a25c7183311b2c14af8f",
                                (0x80129354, 0x801294B0)),
}
def link(image, site, target):
    value = word(image, site)
    if value & 0xFC000003 != 0x48000001 or direct_branch(site, value) != target:
        raise ValueError(f"area call link changed at {address(site)}")


def common_bank(archive):
    if len(archive.names) <= COMMON_MEMBER or archive.names[COMMON_MEMBER] != b"cmn_data.pzz" or archive.names.count(b"cmn_data.pzz") != 1:
        raise ValueError("type-5 common collision member identity")
    payload = archive.payload(COMMON_MEMBER, MEMBER_LIMIT)
    bank, _ = PzzArchive(RangeFile(BytesIO(payload), 0, len(payload))).read(0, area.COMMON_COLLISION_SIZE)
    if len(bank) != area.COMMON_COLLISION_SIZE or sha256(bank).hexdigest() != area.COMMON_COLLISION_SHA:
        raise ValueError("type-5 common collision bank identity")
    return bank


def area_rows(image, bank):
    table = image.read(area.TABLE, KEY_COUNT*36, "data")
    if sha256(table).hexdigest() != area.TABLE_SHA:
        raise ValueError("type-5 area row table identity")
    rows, groups = [], {}
    for key, (visual, group, hold, material, initial, cap, growth, motion, motion_step, other, cue) in enumerate(
        struct.iter_unpack(">4h6fi", table)
    ):
        if ((visual, cue) != (22, 12) or not 0 <= group < 256 or not 0 < initial < cap or growth <= 0
                or hold < 1 or material not in (0, 1)
                or not all(map(math.isfinite, (initial, cap, growth, motion, motion_step, other)))):
            raise ValueError("unexpected type-5 area operand")
        if group not in groups:
            queue = bank[4*group:4*group+4]
            step = queue[0]
            at = 0x400+80*step
            shape = bank[at:at+80]
            if queue[1:] != b"\xff"*3 or len(shape) != 80:
                raise ValueError(f"type-5 expected singleton common descriptor for group {group}")
            kind, matrix, flags, index, first, last, tail = struct.unpack_from(">BBHHhhH", shape)
            if (kind, matrix, flags, first, last, tail) != (0, 0, 1, 0, -1, 0) or not 1 <= index <= 27:
                raise ValueError("type-5 common descriptor header")
            center_radius = struct.unpack_from(">4f", shape, 12)
            reaction = REACTIONS+24*index
            raw_power, = struct.unpack(">H", image.read(reaction, 2, "data"))
            if center_radius != (0., 0., 0., 310.) or raw_power not in (20, 30, 40, 100):
                raise ValueError("type-5 common descriptor or power disagreement")
            groups[group] = {"group": group, "descriptor": step, "shape": "sphere", "local_center": [0, 0, 0],
                             "raw_radius": 310, "reaction_index": index, "reaction_record": address(reaction),
                             "raw_power": raw_power}
        rows.append({"key": key, "row": address(area.TABLE+36*key), "common_group": group, "visual_record": visual,
                     "area_audio_id": cue, "material_selector": material, "hold_operand": hold, "initial_scale": initial,
                     "scale_cap": cap, "growth_operand": growth, "other_float_operands": [motion, motion_step, other]})
    return rows, [groups[group] for group in sorted(groups)]


def selector_tables(image):
    tables, by_site = {}, {}
    for family, spec in SELECTORS.items():
        blob = image.read(spec.base, spec.stride*spec.count, "data")
        dispatch = spec.base+len(blob)
        if sha256(blob).hexdigest() != spec.digest or word(image, dispatch+4, "data") != spec.handler:
            raise ValueError(f"type-5 parent table or dispatch mismatch: {family}")
        keys = [struct.unpack_from(">"+spec.fmt, blob, n*spec.stride+spec.offset)[0] for n in range(spec.count)]
        if any(not -1 <= key < KEY_COUNT for key in keys) or (-1 in keys and family != "type-15"):
            raise ValueError(f"type-5 parent key bounds: {family}")
        tables[family] = keys
        for pc in spec.sites:
            eligible = [3] if pc == 0x800E0AAC else [i for i, key in enumerate(keys) if key >= 0]
            by_site[pc] = {
                "kind": "indexed-parent-row", "parent_domain": family, "parent_handler": address(spec.handler),
                "parent_table": address(spec.base), "parent_stride": spec.stride, "operand_offset": spec.offset,
                "operand_encoding": spec.fmt, "area_key_by_parent_key": [key if key >= 0 else None for key in keys],
                "eligible_parent_keys": eligible, "possible_area_keys": sorted({keys[i] for i in eligible}),
            }
            if pc == 0x800E0AAC:
                by_site[pc]["site_guard"] = "type-41 row 3 and nonzero owner command latch"
    return tables, by_site


def static_call_sites(image, atlas, selectors, type89_rows):
    linker = ActionLinker(image, {}, ())
    sites = []
    for factory in FACTORIES:
        for pc, record in sorted(callers(image, atlas, factory, KEY_COUNT).items()):
            function = int(record["function"], 16)
            words, incoming, successors, direct, frontier, exhausted = linker.graph(
                function, atlas["functions"][record["function"]]["size"], 480)
            if exhausted or frontier or direct.get(pc) != factory:
                raise ValueError(f"type-5 no bounded argument graph at {address(pc)}")
            local = immediate_argument(words, incoming, pc, 5)
            flow = literal_flow(function, words, successors, linker.data).get(pc, {}).get(5)
            if pc in selectors:
                if local is not None or flow is not None or selectors[pc]["parent_handler"] != address(function):
                    raise ValueError(f"type-5 indexed selector changed at {address(pc)}")
                source = selectors[pc]
            elif pc in type89.AREA_SITES:
                at = type89.AREA_SITES[pc]
                if word(image, at) != 0x88BF017D or local is not None or flow is not None or function != type89.EXIT:
                    raise ValueError("type-5 cached type-89 byte changed")
                source = {"kind": "cached-parent-byte", "parent_domain": "type-89", "read_site": address(at),
                          "write_site": "8016CE48", "object_offset": "0x17D",
                          "parent_table": address(type89.TABLE), "parent_stride": type89.STRIDE,
                          "parent_operand_offset": 3, "bounded_parent_rows": len(type89_rows),
                          "possible_area_keys": sorted({row["area_key"] for row in type89_rows})}
            else:
                if local is None or local != flow or not 0 <= local < KEY_COUNT:
                    raise ValueError(f"type-5 fixed key proof mismatch at {address(pc)}")
                source = {"kind": "literal-r5", "key": local}
            sites.append({"site": address(pc), "function": address(function), "factory": address(factory), "key_source": source})
    return sites


def normal_predecessors(image, atlas, selections, parent_census, tables):
    models = {r["id"]: r for r in selections["borgs"]}
    if len(models) != 206 or set(models) != {r["id"] for r in atlas["borgs"]}:
        raise ValueError("type-5 selected normal profile coverage")
    linked = []
    for candidate in parent_census["candidates"]:
        family = candidate["family"]
        if family not in ("type-15", "type-47"):
            continue
        model, parent_key = candidate["borg"], candidate["key"]
        if models[model]["profile"] != candidate["profile"] or models[model]["spec_record"] != candidate["spec_record"]:
            raise ValueError("type-5 parent profile differs from the standalone selection")
        key = tables[family][parent_key]
        if key < 0:
            continue  # The negative-selector branch requests a different child, not type 5.
        if models[model]["spec_record"] != 0:
            raise ValueError("type-5 parent source is not normal")
        linked.append({
            "borg": model, "profile": candidate["profile"], "parent": {"family": family, "key": parent_key,
                                                                        "site": candidate["site"]},
            "descendant": {"key": key, "sites": [address(pc) for pc in SELECTORS[family].sites],
                           "scope": "terminal carrier branches only"},
            "ancestor_scope": ("conditional actor byte, not qualified receipt of an extra bomb"
                               if candidate.get("additional_guard") else "static release candidate"),
            **({"additional_guard": candidate["additional_guard"]} if candidate.get("additional_guard") else {}),
        })

    roach = models["gg4e08:0f05"]
    profile = atlas["profiles"][roach["profile"]]
    shots = [r for g in profile["groups"] if g["group"] == "B-shot" for r in g["rows"]]
    if (roach["profile"] != "p168" or roach["spec_record"] != 0 or profile["config"] != "803929C8"
            or len(shots) != 1 or (shots[0]["state"], shots[0]["substate"], shots[0]["selector"]) != (0, 0, 45)
            or ["801E7F74", "phase-1-entry-increment-candidate"] not in shots[0]["code_path"]["phase_candidates"]
            or word(image, 0x803929AC, "data") != 0x801E7F74 or word(image, 0x803A2AE0, "data") != 0x801FB060
            or word(image, 0x801FAD6C) != 0x90A3000C or word(image, 0x801E7FB8) != 0x38800000):
        raise ValueError("Roach placed-carrier route changed")
    link(image, 0x801E7FA8, 0x8006DBE0)
    link(image, 0x801E7FBC, 0x801FAD10)
    linked.append({
        "borg": roach["id"], "profile": roach["profile"], "ancestor_scope": "ordinary B phase-one paid arm",
        "parent": {"constructor": "801FAD10", "key": 0, "site": "801E7FBC", "debit": "801E7FA8"},
        "descendant": {"key": 26, "sites": ["801FB294"], "scope": "inner-phase area request; not the separate proximity exit"},
    })
    for model, selected, site, debit, key, secondary in (
        ("gg4e08:0106", "p018", 0x80130580, 0x8013055C, 2, [10]),
        ("gg4e08:0107", "p019", 0x801305E8, 0x801305AC, 3, [11, 12]),
    ):
        if (models[model]["profile"] != selected or models[model]["spec_record"] != 0
                or models[model]["post_callback"] != "801304B8"):
            raise ValueError("placed type-41 source profile changed")
        link(image, site, 0x800E058C)
        link(image, debit, 0x8006DBE0)
        linked.append({
            "borg": model, "profile": selected, "ancestor_scope": "model-guarded paid B placement in owner post-hook",
            "parent": {"family": "type-41", "key": key, "site": address(site), "debit": address(debit),
                       "constructor_secondary_operands": secondary},
            "descendant": {"key": tables["type-41"][key], "sites": [address(pc) for pc in SELECTORS["type-41"].sites
                                                                if pc != 0x800E0AAC or key == 3],
                           "scope": "later type-41 branches; placement, command, and area keys are separate"},
        })
    linked += type89.ancestors(image, atlas, selections)
    return sorted(linked, key=lambda r: (r["borg"], r["parent"]["site"]))


def recover(image, archive, atlas, selections):
    if any(r["source_dol_sha256"] != SOURCE_SHA256 for r in (atlas, selections)) or image.sha256 != SOURCE_SHA256:
        raise ValueError("type-5 source identity")
    for at, size, digest in TEXT:
        if sha256(image.read(at, size, "text")).hexdigest() != digest:
            raise ValueError(f"type-5 text signature mismatch: {address(at)}")
    if struct.unpack(">3I", image.read(area.PHASE_TABLE, 12, "data")) != area.PHASES:
        raise ValueError("type-5 area phase dispatch")
    dragon.carrier_index(image)  # Pin its nonadjacent row table and reviewed carrier tail.
    bank = common_bank(archive)
    rows, groups = area_rows(image, bank)
    tables, selectors = selector_tables(image)
    sites = static_call_sites(image, atlas, selectors, type89.rows(image))
    parent = read_metadata(Path(__file__).resolve().parents[2] / "docs/api-map" / PARENT_CENSUS, PARENT_CENSUS_SHA256)
    predecessors = normal_predecessors(image, atlas, selections, parent, tables)
    resolved = {key for s in sites for key in s["key_source"].get("possible_area_keys", (
        [s["key_source"]["key"]] if "key" in s["key_source"] else []))}
    counts = Counter(s["key_source"]["kind"] for s in sites)
    return {
        "kind": "gf-retail-type5-roster", "revision": 2, "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "code_atlas_sha256": ATLAS_SHA256,
        "standalone_installers_sha256": INSTALLERS_SHA256, "parent_census_sha256": PARENT_CENSUS_SHA256,
        "common_bank": {"member": COMMON_MEMBER, "pzz_slot": 0, "sha256": area.COMMON_COLLISION_SHA,
                        "reaction_table": address(REACTIONS), "groups": groups},
        "counts": {"factory_rows": len(rows), "common_groups": len(groups), "direct_call_sites": len(sites),
                   "literal_site_keys": counts["literal-r5"], "indexed_parent_sites": counts["indexed-parent-row"],
                   "cached_parent_sites": counts["cached-parent-byte"],
                   "resolved_area_keys": len(resolved), "candidate_rows": len(predecessors),
                   "known_ancestor_models": len({r["borg"] for r in predecessors})},
        "factory_rows": rows, "call_sites": sites, "candidates": predecessors,
        "area_keys_not_resolved_to_any_site": sorted(set(range(KEY_COUNT))-resolved),
        "allocators": [{"entry": address(entry), "allocation": path, "submode": mode, "allocator": address(allocator),
                        "class": 3, "requested_bucket": 8, "descriptor_cells": 2,
                        "retains_parent_and_credit": True} for entry, (path, mode, allocator) in FACTORIES.items()],
        "notes": [
            "The 45 atlas edges are direct calls into area factories, not necessarily direct actor actions.",
            "Literal keys do not identify an owner. Indexed and cached keys describe bounded parent rows and not every local branch guard.",
            "Only the scoped ancestor links are associated with a Borg. No area is attributed to a button by analogy.",
            "Common raw power and radius are descriptor operands, not HP removed or a live radius. No current gameplay is emitted.",
        ],
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "atlas", "selections", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export_census(args.dol, args.afs, args.atlas, args.selections, args.output, recover), sort_keys=True))
