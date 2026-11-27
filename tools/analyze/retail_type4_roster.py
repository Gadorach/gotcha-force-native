"""Census the shared type-4 key-0 factory and replay its presentation lifetime."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import retail_type32_roster as beam, retail_type5_roster as area
from tools.analyze import retail_type4_ancestors as ancestors
from tools.analyze import retail_type9_lifetime as lifetime
from tools.analyze.retail_action_links import ActionLinker, immediate_argument, literal_flow, written_register
from tools.analyze.retail_projectile_census import AFS_SHA256, ATLAS_SHA256, SOURCE_SHA256, address, callers, export_census, read_metadata
from tools.analyze.retail_type76_paid import ROSTER_SHA256 as TYPE76_SHA256


FACTORY, UPDATE, PHASES, KEY0 = 0x8001BDA8, 0x8001BEC8, 0x802CA140, 0x80433648
MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"
CASES = (("clock-one", 1.0, 1, True), ("clock-half", 0.5, 1, True),
         ("empty-shape-pool", 1.0, 0, True), ("inactive-parent-storage-retained", 1.0, 1, False))
ROW = re.compile(r"type4-life: [0-3](?: [0-9]{1,2}){4}(?: [0-9]\.[0-9]{5}){3}")
TEXT = (
    (FACTORY, 288, "01586cb4fa31c8c0f19871852fa315ee27958073bd45cff37c3f73355b129b00"),
    (UPDATE, 60, "a39ed2484ae4244becd090789e6348f1ba55d7ac8e5bb3979e21d7442749899b"),
    (0x8001BF04, 292, "bed0e5343c27b6ef244cfc6fe2a0f8b19efacf0f5c4c9530fecd0f80f3f460b8"),
    (0x8001C028, 368, "6f7b9392cacf2da6a3bd5a102f265160af5913f4e87450773f242da2ab2c4e3b"),
    (0x8001C198, 20, "9212e9caa3a25e54a968e0a58a25f7946023df4677e5701fd0ee94e4396d8819"),
    (0x8001C1AC, 72, "277a03907004decafdc9a358fbf0be39daafad0145bdd8c8ea22b19fd6f4215b"),
    (0x8001C294, 52, "0fe61179e99eb9afa6a59d0cb75d8fc18c760377927e1617a93135898c1f9b10"),
    (0x8007E200, 1084, "47042a30e6220d5de32bfaeb52c01c55f36b3c5c005057018ca513ae08494d86"),
    (0x800FD09C, 492, "702496f89e2d6bf514625520ce1eacad7fb87c658bf5fc00abccf06730830703"),
    *tuple(entry for entry in lifetime.TEXT if entry[0] in
           (0x80089A94, 0x8008997C, 0x80089898, 0x80088E50, 0x80089554, 0x8008A28C, 0x80089AEC)),
)


def pinned_data(image, at, size, digest):
    data = image.read(at, size, "data")
    if sha256(data).hexdigest() != digest:
        raise ValueError(f"type-4 source data mismatch at {address(at)}")
    return data


def key0_row(image):
    stage_count, stages = struct.unpack(">2I", pinned_data(
        image, KEY0, 8, "21579bdc7874484534aa11a9ba488a55e3bc8ef90019b7254e404bc97d385f1a"))
    if (stage_count, stages) != (2, 0x802CA128):
        raise ValueError("type-4 key-0 sequence changed")
    rows = pinned_data(image, stages, stage_count*12, "f298b46607d9abe1134d4c9a3194d294dd2e3573b0b9bf2d04512eefc8cec53f")
    pinned_data(image, 0x802CA0B0, 120, "920e38d59837a3dabd3fb7a9e1a1db785971cedf79afe1a4b7f691c3761faf3c")
    return {"key": 0, "table_row": address(KEY0), "stages": [
        {"visual_record": visual, "active_visits": duration, "scale_curve": address(scale),
         "secondary_curve": address(secondary) if secondary else None}
        for visual, duration, scale, secondary in struct.iter_unpack(">2h2I", rows)
    ]}


def position_register(words, incoming, call):
    pc = call
    for _ in range(8):
        prior = pc-4
        if incoming.get(pc) != {prior} or prior not in words:
            break
        word = words[prior]
        op = word >> 26
        if op in (16, 18, 19):
            break
        try:
            writer = written_register(word)
        except ValueError:
            if op not in (38, 44, 48, 50, 52, 54, 59, 63):
                break
            writer = None
        if writer == 4:
            base = word >> 16 & 31
            if op == 14 and base in (30, 31) and word & 0xFFFF == 0x20:
                return base
            break
        pc = prior
    raise ValueError(f"type-4 position argument has no local +0x20 proof at {address(call)}")


def call_index(image, atlas):
    index = callers(image, atlas, FACTORY, 1)
    if len(index) != 33 or len({r["function"] for r in index.values()}) != 20:
        raise ValueError("type-4 reviewed atlas edge coverage changed")
    linker = ActionLinker(image, {}, ())
    result = {}
    for pc, row in sorted(index.items()):
        at = int(row["function"], 16)
        words, incoming, successors, direct, frontier, exhausted = linker.graph(at, atlas["functions"][row["function"]]["size"], 480)
        if (frontier or exhausted or direct.get(pc) != FACTORY or immediate_argument(words, incoming, pc, 5) != 0
                or literal_flow(at, words, successors, linker.data).get(pc, {}).get(5) != 0):
            raise ValueError(f"type-4 bounded direct call or literal key changed at {address(pc)}")
        result[pc] = {"site": address(pc), "function": row["function"], "key": 0, "position_offset": "0x20",
                      "position_base_register": position_register(words, incoming, pc)}
    return result


def candidate_index(image, atlas, selections, sites):
    picked = {b["id"]: b for b in selections["borgs"]}
    if len(selections["borgs"]) != 206 or len(picked) != 206:
        raise ValueError("type-4 standalone selection scope changed")
    for at, caller in ((0x8007E298, "8007E200"), (0x8007E610, "8007E200"),
                       (0x800FD144, "800FD09C"), (0x800FD16C, "800FD09C")):
        if sites[at]["function"] != caller:
            raise ValueError("type-4 reviewed predecessor function changed")
    candidates = []
    bank, resolved = area.selector_tables(image)
    if set(resolved[0x8007E284]["eligible_parent_keys"]) != set(range(6)) or bank["type-15"][6:] != [-1]*4:
        raise ValueError("type-4 type-15 alternate selector domain changed")
    small = read_metadata(MAPS / area.PARENT_CENSUS, area.PARENT_CENSUS_SHA256)
    for row in small["candidates"]:
        if row["family"] != "type-15" or bank["type-15"][row["key"]] >= 0:
            continue
        model = row["borg"]
        if (row["spec_record"] != 0 or picked[model]["profile"] != row["profile"] or picked[model]["spec_record"] != 0
                or {r["group"] for r in row["action_candidates"]} != {"X-charge"}):
            raise ValueError("type-4 type-15 scope changed")
        beam.link(image, int(row["site"], 16), 0x8007DB5C)
        candidates.append({"borg": model, "profile": row["profile"], "action": "X-charge",
                           "parent": {"type": 15, "key": row["key"], "site": row["site"]},
                           "descendant_sites": ["8007E298", "8007E610"],
                           "scope": "negative area selector on the contact-latch or stage-probe arm"})
    normal76 = read_metadata(MAPS / "gg4e08-type76-roster.json", TYPE76_SHA256)
    for row in normal76["candidates"]:
        model = row["borg"]
        if picked[model]["profile"] != row["profile"] or picked[model]["spec_record"] != 0 or row["spec_record"] != 0:
            raise ValueError("type-4 type-76 normal scope changed")
        beam.link(image, int(row["site"], 16), 0x800FCD38)
        candidates.append({"borg": model, "profile": row["profile"], "action": row["action"],
                           "parent": {"type": 76, "key": row["parent_key"], "site": row["site"]},
                           "descendant_sites": ["800FD144", "800FD16C"],
                           "scope": "zero-HP or expiry arm, not the outgoing contact arm"})
    if len(candidates) != 7 or {r["borg"] for r in candidates} != {"gg4e08:0101", "gg4e08:0d01", "gg4e08:0d04"}:
        raise ValueError("type-4 scope join coverage changed")
    extended, parent_links = ancestors.recover(image, atlas, picked, sites)
    return candidates+extended, parent_links


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 1024 or len(lines) != len(CASES):
        raise ValueError("type-4 lifetime report extent")
    result = []
    for i, ((label, clock, shapes, parent_active), line) in enumerate(zip(CASES, lines, strict=True)):
        values = line.split()[1:]
        if (ROW.fullmatch(line) is None or tuple(map(int, values[:5])) != (i, 9, 39, 40, 41)
                or tuple(map(float, values[5:])) != (2.0, 2.5, 3.125)):
            raise ValueError(f"type-4 stage or resource lifetime changed: {label}")
        result.append({"case": label, "owner_clock_step": clock, "offered_shape_cells": shapes,
                       "parent_active_at_init": parent_active, "visual_switch_visit": 9, "phase_two_visit": 39,
                       "phase_three_visit": 40, "class2_and_descriptor_return_visit": 41,
                       "stage_scales": [2.0, 2.5, 3.125], "collision_allocation_calls": 0, "attack_registry_entries": 0})
    return result


def export(dol, afs, atlas, selections, fixture, module, output):
    def recover(image, _archive, code_atlas, installed):
        beam.verify_text(image, TEXT)
        for site, target in ((0x8001BDE0, 0x80088AA0), (0x8001C1DC, 0x80088E50)):
            beam.link(image, site, target)
        row = key0_row(image)
        phases = struct.unpack(">4I", pinned_data(
            image, PHASES, 16, "03791f68e4fceb7aecc3cc8b0a03969b32220515d46c8815fed63655eea28667"))
        sites = call_index(image, code_atlas)
        candidates, parent_links = candidate_index(image, code_atlas, installed, sites)
        args = [str(Path(fixture).resolve(strict=True)), str(Path(dol).resolve(strict=True)),
                str(Path(module).resolve(strict=True))]
        proc = run(args, capture_output=True, timeout=45)
        if proc.returncode or proc.stderr:
            raise ValueError(f"type-4 fixture failed: {proc.stderr.decode('utf-8', 'replace').strip()}")
        cases = decode_report(proc.stdout.decode("ascii"))
        joined = {site for r in candidates for site in r["descendant_sites"]}
        return {
            "kind": "gf-retail-type4-roster", "revision": 2, "source_dol_sha256": SOURCE_SHA256,
            "source_afs_sha256": AFS_SHA256, "code_atlas_sha256": ATLAS_SHA256,
            "small_parent_census_sha256": area.PARENT_CENSUS_SHA256, "type76_census_sha256": TYPE76_SHA256,
            "factory": address(FACTORY), "update": address(UPDATE), "class": 2, "bucket_request": 24,
            "dispatch": {"table": address(PHASES), "phases": [address(v) for v in phases]},
            "factory_rows": [row], "call_sites": list(sites.values()), "candidates": candidates, "lifetime_cases": cases,
            "parent_dispatch_links": parent_links,
            "counts": {"direct_sites": len(sites), "functions": len({r["function"] for r in sites.values()}),
                       "scoped_sites": len(joined), "candidate_rows": len(candidates),
                       "models": len({r["borg"] for r in candidates}), "lifetime_cases": len(cases), "visits": 41*len(cases)},
            "notes": [
                "Coverage is direct atlas edges into this factory, not every object tagged type 4. Unjoined source edges assign no Borg or damage.",
                "Extended ancestry is static: an installed phase, paid leaf, exact constructor, and downstream exit branch are separate proofs.",
                "Actor-contact area requests are not the zero-HP or expiry effects. The type-52 socket ordinal and forms remain unbound.",
                "The row and lifecycle apply to key 0 only. The fixture uses the original actor-relative bucket and empty render records.",
                "The parent scale at creation is 0.75; initialization reads the new value 2 from retained parent storage. Later setting it to 7 does not drive the child's scale.",
                "The inactive-parent case retains the same storage; it does not simulate parent cell reuse or arbitrary lifetime.",
                "No shape is allocated or published in the tested effect lifetime. Parent/other child damage, live presentation and native gameplay remain separate.",
            ],
        }

    return export_census(dol, afs, atlas, selections, output, recover)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "atlas", "selections", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.atlas, args.selections, args.fixture, args.module, args.output), sort_keys=True))
