"""Index type-32 carrier domains, call operands, and scoped action ancestry."""
import argparse
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze import reaction_emit, reaction_installer_tables
from tools.analyze.retail_action_links import ActionLinker, direct_branch, immediate_argument, literal_flow
from tools.analyze.retail_projectile_census import (
    AFS_SHA256, ATLAS_SHA256, INSTALLERS_SHA256, SOURCE_SHA256, address, callers, export_census, queue_metadata,
)


SHARED, SCRATCH, PRESENTATION = 0x800E19A8, 0x801DEB68, 0x80165184
SEGMENT, SEGMENT_BIND, DEBIT = 0x80139A6C, 0x80139BBC, 0x8006DBE0
DOMAINS = {
    SHARED: ("model-relative-endpoints", 0x800E1AB4, 0x80317248, 32, 20,
             "608c000299b9b52e1a1f30adfe5f49760b01af99e4bc6976d225057833a30ef1",
             0x80317698, (0x800E1B64, 0x800E1CD8, 0x800E211C, 0x800E21AC), (3, 8, 6)),
    SCRATCH: ("borrowed-endpoints", 0x801DEC90, 0x803914B8, 40, 10,
              "4b9781de3a7439a20b2086d1b994c9c7eaf3994ce5f5fe0fe8fe42aa685d918c",
              0x80391648, (0x801DECE0, 0x801DEF80, 0x801DF404, 0x801DF448), (3, 8, 4)),
    PRESENTATION: ("three-part-update", 0x8016528C, 0x8034E550, 24, 1,
                   "85a7061612f032c969710dc1b1f0245511492412941997c2e89a33d99ce220ee",
                   0x8034E568, (0x80165318, 0x801653DC, 0x8016544C, 0x80165460), (2, 24, 3)),
}
TEXT = (
    (SHARED, 268, "53c6d03a633a0c505235558e91959a51ab52fda5bd19a1f2e3f04e3b12e5c914"),
    (0x800E1AB4, 176, "23a5783019a4163ce559da1022a5325f9c47ce2b63703e24171c813537f1116b"),
    (0x800E1B64, 372, "3ccbf025d3f7db0a0bb5e1412676461f647e1be018fbd91e7675e2913ee09af3"),
    (0x800E1CD8, 1092, "c8508585f55e48a24f738da164f5e21946f92026674e03ee273ad9bfa5fc800d"),
    (SCRATCH, 296, "bde9d039e79832648d1e4cb8eb05d728a13548e376c9d36fb5031d0c4a438011"),
    (0x801DEC90, 80, "21c64f43df92fa97f981e9debcb1517a22e5ea87a5fe3182c4c82134682684c2"),
    (0x801DECE0, 672, "e89cd8835198df615e639284407766cf65be9de302d2fd3dcada70278a78715a"),
    (0x801DEF80, 1156, "97e5de60c309f204b00e12b5e98ac191d75b723adcb2e19094d40dc741b38f90"),
    (PRESENTATION, 264, "e59863db32e4bd085576309a6a67acf8f45bdbfb3b505608fd8859ef372bece0"),
    (0x8016528C, 140, "033d6f1b747f4432e81760e8701e81276c4d20ac4e1f54022c4c37f02a97b4c3"),
    (0x80165318, 196, "6d807e32948f876ae3c35b24d35dbe8ee341d211ded2a676a0572589207701c0"),
    (0x801653DC, 112, "a81d7de23e6ee22112688680a1993a9133fac5a61e93a5795e5f78297c2508d9"),
    (0x80165608, 288, "6b9ad631d9ac80cfe13ff73fb0c5e220cc0579a1b295a70e004a63839cb13510"),
    (0x80165728, 200, "be5123d01ef0b219a100bacac4a9e455b67140a90145721f2dd4e4b0c93dfdb0"),
    (SEGMENT, 276, "db22643dc6df145cd28ece8589fd60a63a2e439ebc8fdad8563976d2902a6097"),
    (SEGMENT_BIND, 148, "274a37cf5c8f02f523e13f360af2b35540b8e1e5145cc800a8f90fc52eb1ae66"),
)
ACTION_TEXT = (
    (0x800B9378, 244, "1502fc68af3d2e7a465c44ee7c3ec7de5016fd50c563cf6c55e95388f872039b"),
    (0x80109710, 156, "3b5905307ec089ae67db664916c6f629387009974d09aef4e81e2c240c1b0757"),
    (0x8010369C, 60, "5ea23e623bf059c40c2a0911a886e4b1bba505c8c952a10ae8ee021cf4a6bb8e"),
    (0x801037F8, 404, "18b01056d448fa744d2647892dbca9ad263a6aad0f6b5b1bcbe016eb0ff00d78"),
    (0x800CE138, 76, "d634c092c714bb9ffef7bf22231b58e4eeeb7f26dbaf7cb0f0a23fae43d6b1f9"),
    (0x800CE184, 80, "167b8adbcd51239970210b6fcfe4a681e6e43c5bc5420feb639b42eee7ffcf92"),
    (0x800CE298, 164, "468a8adcebc826fca8621845d083b2280e7b7d16f653d0682053ef7a69c2f3a2"),
    (0x80179814, 60, "acdd5d8ce760e011640f091d3de0fbe3c78207d6dc659332a992bf04dcb613f2"),
    (0x801799BC, 204, "ab1723d516a7de0ee746fe163111bbf8f223f122fb61507b709ae7982dcc8035"),
    (0x8018ED4C, 52, "c14182d0a842bc4bea25e06ea22adaebe10135949ae2752896a60469c7247451"),
    (0x8018ED80, 40, "3bdb43c6297b721bec878bcc84f11632f2634d373b61db6ae50dc48a3fdd2897"),
    (0x8018EDC4, 68, "6a9665b92d07c1ca1d09383a5f3c1aa7b40a74c86b2d70483829881a92d70688"),
    (0x8018EE84, 32, "33c48e6334685f7ba0e2dfe4cf80c7628d377b09ae09891a80b3330afb709894"),
    (0x80189508, 60, "9c5da627d1790204275628834f394dcb733d14626df6e17c11074a966120cdde"),
    (0x80189544, 164, "9a17a6a0458a67744bd404d6c17f3abaf194dd2debe53543a5a382740553c443"),
    (0x801D75A0, 60, "66dc0780ff7fabfdc7cb1492d46b3129de210414b8e50e205c790ed137985c0e"),
    (0x801D75DC, 352, "6de737135b0abf39bf1944a5f3ed08d2eaec34268a8a909753b8d008d54e1740"),
    (0x801E7BE0, 408, "0495488540f86d55146937f37e04b6384515d835306b34cc033330b0f67249f9"),
)
LITERALS = {
    SHARED: {0x800BD32C: 0, 0x800CE308: 4, 0x800FAFC8: 10, 0x80103900: 1, 0x80109FB8: 6,
             0x80109FD4: 18, 0x8012A768: 12, 0x8012A778: 13, 0x8013F048: 2, 0x8018EDE4: 17,
             0x8018EDF4: 15, 0x801C0438: 11, 0x801D9EEC: 14, 0x801E7C90: 16, 0x801E7CAC: 19},
    SCRATCH: {0x80132BC4: 1, 0x80132BDC: 4, 0x801895B8: 8, 0x801D67B8: 2, 0x801D7714: 9, 0x801FBA5C: 3},
    PRESENTATION: {0x80160E20: 0},
}
# Default/override in already-pinned functions. These guards resolve r4, not an installed action.
MODEL_SELECTORS = {0x800B9450: (0x050F, 0, 7), 0x80109798: (0x0510, 5, 6)}


def word(image, at, kind="text"):
    return struct.unpack(">I", image.read(at, 4, kind))[0]


def link(image, site, target):
    insn = word(image, site)
    if insn & 0xFC000003 != 0x48000001 or direct_branch(site, insn) != target:
        raise ValueError(f"type-32 call link mismatch at {address(site)}")


def verify_text(image, spans):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("type-32 source identity")
    for start, size, digest in spans:
        if sha256(image.read(start, size, "text")).hexdigest() != digest:
            raise ValueError(f"type-32 source text mismatch at {address(start)}")


def row_index(image):
    verify_text(image, TEXT)
    for site, target in ((0x800E20C0, SEGMENT), (0x801DF2B0, SEGMENT), (0x801DED2C, 0x8008AC80),
                         (0x80139BE0, 0x8008AC80)):
        link(image, site, target)
    domains, rows = [], {}
    for factory, (kind, update, base, stride, count, digest, phase_table, phases, allocation) in DOMAINS.items():
        raw = image.read(base, stride*count, "data")
        if (sha256(raw).hexdigest() != digest
                or struct.unpack(">4I", image.read(phase_table, 16, "data")) != phases):
            raise ValueError("type-32 table, extent or dispatch changed")
        domains.append({"factory": address(factory), "update": address(update), "domain": kind,
                        "table": address(base), "stride": stride, "rows": count, "phase_table": address(phase_table),
                        "phase_entries": [address(v) for v in phases],
                        "class": allocation[0], "bucket_request": allocation[1], "descriptor_request": allocation[2]})
        for key in range(count):
            at = key*stride
            row = {"factory": address(factory), "key": key, "row": address(base+at)}
            if factory == SHARED:
                selector, socket = struct.unpack_from(">hb", raw, at)
                row.update(source_socket_selector=socket, source_socket_offset=struct.unpack_from(">3f", raw, at+12),
                           collision_path={"kind": "type-11-child-request", "site": "800E20C0", "selector": selector})
            elif factory == SCRATCH:
                # The initializer binds on byte +36 == 0. The periodic
                # child call uses the opposite (positive-byte) arm.
                selector, flag = (struct.unpack_from(">b", raw, at+offset)[0] for offset in (14, 36))
                if flag not in (0, 1):
                    raise ValueError("type-32 scratch collision branch domain changed")
                row.update(collision_mode_byte=flag, collision_path={
                    "kind": "type-11-child-request" if flag else "carrier-opening-bind",
                    "site": "801DF2B0" if flag else "801DED2C", "selector": selector})
            else:
                row.update(presentation_records=list(raw[at:at+3]),
                           collision_path={"kind": "unqualified", "reason": "distinct-class-2-update"})
            rows[factory, key] = row
    return domains, rows


def factory_sites(image, atlas):
    linker = ActionLinker(image, {}, ())
    all_sites = {}
    for factory, definition in DOMAINS.items():
        count = definition[4]
        found = callers(image, atlas, factory, count)
        expected = set(LITERALS[factory]) | (set(MODEL_SELECTORS) if factory == SCRATCH else set())
        if set(found) != expected:
            raise ValueError("type-32 atlas factory coverage changed")
        for pc, row in found.items():
            link(image, pc, factory)
            at = int(row["function"], 16)
            words, incoming, succ, *_ = linker.graph(at, atlas["functions"][row["function"]]["size"], 1800)
            local = immediate_argument(words, incoming, pc, 4)
            flow = literal_flow(at, words, succ, linker.data).get(pc, {}).get(4)
            if pc in MODEL_SELECTORS:
                model, default, override = MODEL_SELECTORS[pc]
                if local is not None or flow is not None or not (0 <= default < count and 0 <= override < count):
                    raise ValueError("type-32 model selector domain changed")
                source = {"kind": "model-guard", "model": f"{model:04X}", "equal_key": override, "otherwise_key": default}
            else:
                key = LITERALS[factory][pc]
                if local != key or flow != key or not 0 <= key < count:
                    raise ValueError(f"type-32 r4 literal flow mismatch at {address(pc)}")
                source = {"kind": "literal-r4", "key": key}
            all_sites[pc] = {"factory": address(factory), "function": row["function"], "site": row["site"],
                             "key_source": source}
    return all_sites


def recover(image, archive, atlas, selections):
    verify_text(image, ACTION_TEXT)
    installed = {int(r["id"][-4:], 16): r for r in selections["borgs"]}
    static = {int(r["id"][-4:], 16): r for r in atlas["borgs"]}
    if (len(installed) != 206 or set(installed) != set(static)
            or any(d["source_dol_sha256"] != SOURCE_SHA256 or len(d["borgs"]) != 206 for d in (atlas, selections))):
        raise ValueError("type-32 source identity or selected profile coverage")
    domains, rows = row_index(image)
    sites = factory_sites(image, atlas)
    entries = {0x80321684: 0x801037F8, 0x80392558: 0x801E7BE0, 0x8030CDC0: 0x800CE298,
               0x80352B58: 0x801799BC, 0x80365868: 0x8018EDC4, 0x803629AC: 0x80189544, 0x8038E004: 0x801D75DC}
    entries.update({0x8030CDA8+4*i: 0x800CE184 for i in range(5)})
    if any(word(image, va, "data") != entry for va, entry in entries.items()):
        raise ValueError("type-32 actor dispatch entry changed")
    for pc, target in ((0x8018ED64, 0x8018ED80), (0x8018EE90, 0x8018ED80),
                       (0x8018ED94, 0x80179814), (0x801038B4, DEBIT)):
        link(image, pc, target)
    candidates = []

    def join(model, profile, action, state, family, pc, condition, debit=None, phase_table=None):
        picked = installed[model]
        source = static[model]
        if picked["profile"] != profile or picked["spec_record"] != 0 or profile not in source["profiles"]:
            raise ValueError("type-32 selected normal source changed")
        installer, reaction_table = reaction_installer_tables.model_table(image, model)
        if address(installer) != picked["installer"] or address(reaction_table) != source["reaction_table"]:
            raise ValueError("type-32 character ownership changed")
        matching = [r for g in atlas["profiles"][profile]["groups"] if g["group"] == action for r in g["rows"]
                    if r["classification"] == "selector-candidate" and r["state"] == state
                    and r.get("code_path", {}).get("family") == address(family)
                    and (phase_table is None or r["code_path"].get("phase_table") == address(phase_table))]
        if not matching:
            raise ValueError("type-32 installed action candidate changed")
        site = sites[pc]
        factory, key = int(site["factory"], 16), site["key_source"]["key"]
        collision = rows[factory, key]["collision_path"]
        if collision["kind"] == "unqualified":
            raise ValueError("type-32 unqualified action ownership")
        block = reaction_emit.qualified_hit_block(archive, model)
        if debit:
            link(image, debit[0], DEBIT)
        candidates.append({
            "borg": picked["id"], "profile": profile, "action": action, "state": state, "family": address(family),
            "factory": site["factory"], "key": key, "site": site["site"], "condition": condition,
            "collision_path": collision, "normal_group_queue": queue_metadata(image, block, reaction_table, collision["selector"]),
            **({"debit": {"site": address(debit[0]), "channel": debit[1], "cost": 1}} if debit else {}),
        })

    join(0x0104, "p021", "X", 2, 0x8010369C, 0x80103900, "phase one, +0x1CEE nonzero and paid callback",
         (0x801038E8, 2), 0x80321680)
    for model, profile, site in ((0x0505, "p071", 0x801E7C90), (0x0511, "p072", 0x801E7CAC)):
        join(model, profile, "X", 2, 0x801E7BE0, site, "phase zero; threshold exceeded; model-guarded paid branch", (0x801E7C6C, 2))
    for model in (0x0602, 0x060A, 0x060C, 0x060E):
        join(model, "p079", "X-charge", 3, 0x800CE138, 0x800CE308, "phase one; +0x1CEE nonzero", phase_table=0x8030CDBC)
    for model in (0x0615, 0x0629):
        join(model, "p088", "B-charge", 3, 0x8018ED4C, 0x8018EDF4, "phase one; +0x1CEF positive; not G-Black")
    join(0x062A, "p089", "X-charge", 4, 0x8018EE84, 0x8018EDE4, "phase one; +0x1CEF positive; exact G-Black guard")
    join(0x0D03, "p155", "X", 2, 0x80189508, 0x801895B8, "phase zero; paid borrowed-endpoint factory",
         (0x80189598, 2), 0x803629AC)
    for action in ("B-shot", "B-close"):
        join(0x0E02, "p162", action, 0, 0x801D75A0, 0x801D7714, "same phase-zero paid branch",
             (0x801D7614, 0), 0x8038E004)

    joined = {r["site"] for r in candidates}
    resolved = set()
    for site in sites.values():
        key = site["key_source"]
        values = (key["key"],) if key["kind"] == "literal-r4" else (key["otherwise_key"], key["equal_key"])
        resolved.update((int(site["factory"], 16), v) for v in values)
    return {
        "kind": "gf-retail-type32-roster", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "code_atlas_sha256": ATLAS_SHA256, "standalone_installers_sha256": INSTALLERS_SHA256,
        "domains": domains, "factory_rows": list(rows.values()), "call_sites": [
            {**sites[pc], "installed_action_join": sites[pc]["site"] in joined} for pc in sorted(sites)],
        "rows_without_resolved_call_sources": [{"factory": address(factory), "key": key}
                                               for factory, key in sorted(set(rows)-resolved)],
        "type11_segment": {"factory": address(SEGMENT), "opening_init": address(SEGMENT_BIND),
                           "inlet": "borrowed endpoint addresses, separate parent and credited owner",
                           "collision_selector": "signed child key; negative bypasses bind"},
        "candidates": candidates,
        "counts": {"factory_domains": len(domains), "factory_rows": len(rows), "direct_call_sites": len(sites),
                   "resolved_key_sites": len(sites), "installer_profiles_examined": len(installed),
                   "candidate_rows": len(candidates), "candidate_models": len({r["borg"] for r in candidates}),
                   "literal_key_sites": sum(len(v) for v in LITERALS.values()), "model_guarded_key_sites": len(MODEL_SELECTORS),
                   "joined_sites": len(joined), "unjoined_sites": len(sites)-len(joined),
                   "queue_statuses": dict(Counter(r["normal_group_queue"]["status"] for r in candidates))},
        "notes": [
            "Type 32 does not identify a factory, selector table, or collision path by itself.",
            "A static collision selector and normal hit queue do not prove shape publication, overlap, or damage.",
            "The borrowed-endpoint family uses the same selector byte for a carrier bind or a type-11 child, under disjoint mode gates.",
            "The class-2 route is inventoried separately and has no admitted collision or installed action join.",
            "No combined-form or alternate-spec ownership, live timing, or gameplay is emitted.",
        ],
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "atlas", "selections", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export_census(args.dol, args.afs, args.atlas, args.selections, args.output, recover), sort_keys=True))
