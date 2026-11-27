"""Resolve type-11 segment selectors from their six distinct parent callbacks."""
import argparse
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze import reaction_emit, reaction_installer_tables, retail_type32_roster as beam
from tools.analyze.retail_projectile_census import (
    AFS_SHA256, ATLAS_SHA256, INSTALLERS_SHA256, SOURCE_SHA256, address, callers, export_census, queue_metadata,
    read_metadata,
)


TYPE32_SHA256 = "947c3048c451682f2e2e7b694a4e42d5ec98dd5b3a5690b00ebfefc8b1cf3806"
TYPE32_PATH = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type32-roster.json"
TEXT = (
    (0x800EC1F4, 248, "79260bf3f3889d80a002f4e280eddf48da8fac6cf79969cfdf84f71df437577b"),
    (0x800EC2EC, 168, "05aa3afe31395e87c2ea5e987a4b6eb465d6c913f71b35400a8465af36f83c33"),
    (0x800EC4A4, 104, "b9fb49217381cab7dfd0cb6ce84e5d41dca1a24bcb66e5401ec1d2cbf113766d"),
    (0x800EC60C, 368, "95f2f17d8c61d58098d251bc9b9fc6139d480133df634b4b8414b659f1f94b2e"),
    (0x8018EEA4, 256, "fb4d65e0f8cd0e6cc6f3d13933cb63dac2ae27e3068b8a7a8792b972dd157f94"),
    (0x8018EFA4, 80, "5d7ef010fc481901edc5d6ef91d5c24464fc8e23f3b314524613d56bb5bd6bcf"),
    (0x8018F150, 144, "90a3b7c9ba725a34404c4aa7690eefb9eac3c809c95b48733fe7c646cb6db0b1"),
    (0x8018F28C, 276, "8808ac00aa326f6840b3d47714140529d40bca019a756c3e7a0b248086e2641c"),
    (0x801F2BF8, 328, "5c87001da4fc9ff66a1fe1834ad08cd4c356255ffba63b49770e8a328e2c6cb9"),
    (0x801F2D40, 124, "c269c99bc492309bd75b2461d5c7a3077335d9934f6a722c4075190704a4b688"),
    (0x801F308C, 828, "ac8652ecb1a5c4cdaa54f27f8b8107cfa7587fdf456e6567b03e9483993c127f"),
    (0x801F3784, 300, "b1f0a031eafbfa7cbbd25d42404a37933f82ec64a2578b4d20a2abe64de36c9e"),
    (0x801F38B0, 124, "88d8abaea788ba3cc160ec3db1b70884b7a50a26ffb874cd7a5080d457015e34"),
    (0x801F3B68, 828, "c91f3f544720539e95839906350b078ee73b08179b80807ddcc92ca70ca102e5"),
)
ACTION_TEXT = (
    (0x800B0F18, 76, "ecde564dd4e86801edd9ad1e606cf556fc134b34001830a7073a9e20de323932"),
    (0x800B0F64, 60, "6bc5cc4d36e3e80af5ddd7db4dc7db5caab661ef731ab36a9705e2b205a4761c"),
    (0x800B0FA0, 168, "ccc93bf0b2e6b535d9754cccf0072594efd3568fd79a580893a867a788ced9e6"),
    (0x800B1330, 60, "b83e61c7bf73ef80b13abf87ffba9c740add208092f8aa643f30558d23600b47"),
    (0x800B136C, 192, "454f194877c9bdb46cfe87ce212d55045a36d49ef1701aff7a2bdaab58efcda8"),
    (0x80175E70, 60, "0a807a810aef37de1d3ddcdff67baa68db5aab2236a50af6bd03ca14ed0d0a64"),
    (0x80175EAC, 60, "b1018e1da417d32bf361a08768800e575a4d906a9a15054e67c85c265796ae0e"),
    (0x801760B4, 188, "47c350c273f250fd2422fe505ec2e841abe5a085edbc89fa1edccd4acc7838c4"),
    (0x80189FA8, 64, "9b5a5e463538667413578acf69ac2e3ab2cb9de4e800bc35760c312a9e4e088b"),
    (0x80189FE8, 484, "216e24cf5e98dd862fe450f0f1de38b5f4762d1ea3b9ec64bd68a26746aaa4a4"),
    (0x8018A23C, 116, "22dd84a5244fdc8fb222aea7c36ad94469eeef4cb9224b6c1f7721494b5d83be"),
    (0x8018A348, 64, "2277c99484cb1eb4dfe978a45641b37b35ec54875447afd428470c574cd4c67a"),
    (0x8018A388, 56, "48ac16dab25c45e4be1ac528a933a2d4cec6234be48cbc019fb1de023b23efda"),
    (0x8018A464, 572, "bbff34f715e282bf7bab13deb40395ec7b07976f7d7a3e8720d4aa54a40a14cb"),
    (0x801D5ECC, 60, "e5927b93d3b8f10a026eb72de70a73ee4fbea1dacd566ca3c6324c6e186c6641"),
    (0x801D5F08, 180, "4470e5158506aa2882a923fc6d8524cec05ac97cfcb82d669b59ae6abede703f"),
    (0x801D6034, 156, "173b9606957a7a09385836efc933839f83902c88ec062624675b812cd1b6fc3c"),
)
# Function, parent constructor, parent type (when written), table layout and operand type.
SITES = {
    0x800E20C0: (0x800E1CD8, 0x800E19A8, 32, 0x80317248, 32, 0, "h", 20, "0x11", ("0x38", "0x9C")),
    0x800EC75C: (0x800EC60C, 0x800EC1F4, 71, 0x8031B1C0, 16, 0, "h", 1, "0x11", ("0x38", "0x9C")),
    0x8018F384: (0x8018F28C, 0x8018EEA4, 55, 0x8036EC50, 20, 3, "b", 1, "0x11", ("0x38", "0x9C")),
    0x801DF2B0: (0x801DEF80, 0x801DEB68, 32, 0x803914B8, 40, 14, "b", 10, "0x11", ("0x174", "0x180")),
    0x801F33A4: (0x801F308C, 0x801F2BF8, None, 0x803A1DD8, 44, 36, "i", 8, "0x13", ("0x150", "0x15C")),
    0x801F3E80: (0x801F3B68, 0x801F3784, None, 0x803A2330, 44, 36, "i", 8, "0x13", ("0x150", "0x15C")),
}
EXTRA_TABLES = {
    0x8031B1C0: (16, "7b779cc91eef331e89ea2e203103b93fa24f4aecd682db33ed8abf8da9f5164a"),
    0x8036EC50: (20, "48b0dfbc9fd66b68239d6af2cea4d85f167a5ccf61a1698a49bfdf07a71e1caa"),
    0x803A1DD8: (44*30, "f05def1cdaa893b3a558bf9a572140763dd3cfa92f75a68e3e16be985325a3f0"),
    0x803A2330: (44*8, "6904d2062a395d058662be57f7264c7d3eb18aa40ca5442bfd81a1e3d99fcc56"),
}
PARENT_CALLS = {
    0x800EC1F4: (0x800B0FF4, 0x800B13EC),
    0x8018EEA4: (0x80176140,),
    0x801F2BF8: (0x8018A294, 0x8018A61C),
    0x801F3784: (0x801D6094,),
}


def site_index(image, atlas):
    beam.verify_text(image, TEXT)
    _, type32 = beam.row_index(image)
    direct = callers(image, atlas, beam.SEGMENT, 32)
    if set(direct) != set(SITES):
        raise ValueError("type-11 direct site extent")
    for at, (size, digest) in EXTRA_TABLES.items():
        if sha256(image.read(at, size, "data")).hexdigest() != digest:
            raise ValueError("type-11 parent row extent or identity changed")
    if any(image.read(0x803A1DD8+8*44, 22*44, "data")):
        raise ValueError("type-11 excluded zero rows changed")
    for at, target in (
        (0x8031B204, 0x800EC4A4), (0x8043440C, 0x800EC60C),
        (0x8036EC68, 0x8018F150), (0x804356B4, 0x8018F28C),
        (0x803A2324, 0x801F308C), (0x803A2494, 0x801F3B68),
    ):
        if beam.word(image, at, "data") != target:
            raise ValueError("type-11 parent active dispatch changed")
    result = {}
    for pc, (entry, parent, kind, table, stride, offset, encoding, count, field, ends) in SITES.items():
        if direct[pc]["function"] != address(entry):
            raise ValueError("type-11 parent callback extent changed")
        beam.link(image, pc, beam.SEGMENT)
        groups = []
        for key in range(count):
            selector, = struct.unpack(">"+encoding, image.read(table+stride*key+offset, struct.calcsize(encoding), "data"))
            if (parent, key) in type32:
                path = type32[parent, key]["collision_path"]
                if path["selector"] != selector:
                    raise ValueError("type-11 table disagrees with type-32 selector")
                if path["kind"] != "type-11-child-request":
                    groups.append(None)
                    continue
            if not 0 <= selector < 32:
                raise ValueError("type-11 selector outside reviewed actor group domain")
            groups.append(selector)
        result[pc] = {
            "site": address(pc), "function": address(entry), "parent_factory": address(parent), "parent_type": kind,
            "endpoint_object_offsets": ends, "selector_source": {
                "row_index_object_offset": field, "table": address(table), "stride": stride, "operand_offset": offset,
                "encoding": encoding, "group_by_index": groups,
                **({"cache_object_offset": "0x14C", "cache_writer": "800E1C04"} if parent == beam.SHARED else {}),
                **({"excluded_zero_rows": list(range(8, 30))} if parent == 0x801F2BF8 else {}),
            },
        }
    return result


def action_index(image, archive, atlas, selections, sites):
    beam.verify_text(image, ACTION_TEXT)
    picked = {int(r["id"][-4:], 16): r for r in selections["borgs"]}
    statics = {int(r["id"][-4:], 16): r for r in atlas["borgs"]}
    if (len(picked) != 206 or set(picked) != set(statics)
            or any(m["source_dol_sha256"] != SOURCE_SHA256 or len(m["borgs"]) != 206 for m in (atlas, selections))):
        raise ValueError("type-11 selected normal scope changed")
    for factory, calls in PARENT_CALLS.items():
        if set(callers(image, atlas, factory, 32)) != set(calls):
            raise ValueError("type-11 upstream factory call extent changed")
        for pc in calls:
            beam.link(image, pc, factory)
    for at, target in (
        (0x80433AB8, 0x800B0F64), (0x80433ABC, 0x800B1330),
        (0x802FC460, 0x800B0FA0), (0x802FC470, 0x800B136C),
        (0x80351958, 0x801760B4), (0x80362CD4, 0x80189FE8), (0x80362CDC, 0x8018A23C),
        (0x80362D04, 0x8018A388), (0x80362D0C, 0x8018A464),
        (0x8038DBD4, 0x801D5F08), (0x8038DBDC, 0x801D6034),
    ):
        if beam.word(image, at, "data") != target:
            raise ValueError(f"type-11 upstream action dispatch changed at {address(at)}")
    x_rows = list(image.read(0x80362CE4, 16, "data"))
    charged = [list(pair) for pair in struct.iter_unpack(">bb", image.read(0x80362D2C, 20, "data"))]
    b_rows = list(image.read(0x8038DBE4, 16, "data"))[::2]
    if (set(x_rows) != set(b_rows) or set(b_rows) != set(range(8))
            or charged != [[4, -1], [0, 1], [5, -1], [2, 3], [6, 7]]*2):
        raise ValueError("type-11 upstream row selector domain changed")
    candidates = []

    def join(model, profile, action, state, family, parent_call, site, domain, selector_proof, debit=None, substate=None):
        p = picked[model]
        s = statics[model]
        installer, table = reaction_installer_tables.model_table(image, model)
        if (p["profile"] != profile or p["spec_record"] != 0 or profile not in s["profiles"]
                or p["installer"] != address(installer) or s["reaction_table"] != address(table)):
            raise ValueError("type-11 scoped actor source changed")
        if not any(r["classification"] == "selector-candidate" and r["state"] == state
                   and r.get("code_path", {}).get("family") == address(family)
                   and (substate is None or r["substate"] == substate)
                   for g in atlas["profiles"][profile]["groups"] if g["group"] == action for r in g["rows"]):
            raise ValueError("type-11 scoped installed action changed")
        groups = sites[site]["selector_source"]["group_by_index"]
        selectors = {groups[i] for i in domain}
        if len(selectors) != 1 or None in selectors:
            raise ValueError("type-11 scoped group is not uniquely proven")
        group = selectors.pop()
        block = reaction_emit.qualified_hit_block(archive, model)
        if debit:
            beam.link(image, debit[0], beam.DEBIT)
        candidates.append({
            "borg": p["id"], "profile": profile, "action": action, "state": state, "substate": substate,
            "parent_call": address(parent_call), "parent_factory": sites[site]["parent_factory"],
            "parent_row_indices": sorted(domain), "parent_row_source": selector_proof,
            "segment_site": address(site), "segment_key": group, "normal_queue": queue_metadata(image, block, table, group),
            **({"paid_arm": {"debit_site": address(debit[0]), "channel": debit[1], "cost": 1}} if debit else {}),
        })

    for sub, pc in ((0, 0x800B0FF4), (1, 0x800B13EC)):
        join(0x0C04, "p150", "X-charge", 3, 0x800B0F18, pc, 0x800EC75C, [0],
             {"kind": "literal-r4", "scope": "phase-zero entry; no debit in either reviewed leaf"}, substate=sub)
    join(0x0704, "p110", "X-charge", 2, 0x80175E70, 0x80176140, 0x8018F384, [0],
         {"kind": "literal-r4", "scope": "phase two, nonzero +0x1CF0 and exact model gate"})
    for model in (0x0E00, 0x0E05):
        join(model, "p160", "X", 2, 0x80189FA8, 0x8018A294, 0x801F33A4, sorted(set(x_rows)),
             {"kind": "remapped-actor-byte", "object_offset": "0x145", "writer": "8018A1AC",
              "table": "80362CE4", "bounded_operands": x_rows, "scope": "phase-two after paid phase-zero preparation"},
             (0x8018A01C, 2))
        join(model, "p160", "X-charge", 3, 0x8018A348, 0x8018A61C, 0x801F33A4, list(range(8)),
             {"kind": "ordered-pair-cursor", "object_offset": "0x145", "writer": "8018A5E0",
              "table": "80362D2C", "signed_parent_row_pairs": charged, "scope": "this phase-two paid branch only"},
             (0x8018A600, 2))
    for action in ("B-shot", "B-close"):
        join(0x0E03, "p163", action, 0, 0x801D5ECC, 0x801D6094, 0x801F3E80, sorted(set(b_rows)),
             {"kind": "three-test-direction-row", "object_offset": "0x14F", "writer": "801D5F9C",
              "table": "8038DBE4", "row_indices": b_rows, "scope": "phase-zero selection, phase-two paid request"},
             (0x801D6060, 0))
    return candidates


def recover(image, archive, atlas, selections):
    sites = site_index(image, atlas)
    candidates = action_index(image, archive, atlas, selections, sites)
    type32 = read_metadata(TYPE32_PATH, TYPE32_SHA256)
    for row in type32["candidates"]:
        path = row["collision_path"]
        if path["kind"] != "type-11-child-request":
            continue
        selector = sites[int(path["site"], 16)]["selector_source"]["group_by_index"][row["key"]]
        if selector != path["selector"]:
            raise ValueError("type-11 upstream type-32 receipt diverges")
        candidates.append({
            **{k: row[k] for k in ("borg", "profile", "action", "state")},
            "parent_factory": row["factory"], "parent_call": row["site"], "parent_row_indices": [row["key"]],
            "segment_site": path["site"], "segment_key": selector, "normal_queue": row["normal_group_queue"],
            "parent_row_source": {"kind": "pinned-type-32-receipt", "scope": row["condition"]},
        })
    candidates.sort(key=lambda r: (r["borg"], r["action"], r["parent_call"]))
    return {
        "kind": "gf-retail-type11-roster", "revision": 1, "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "code_atlas_sha256": ATLAS_SHA256, "standalone_installers_sha256": INSTALLERS_SHA256, "type32_census_sha256": TYPE32_SHA256,
        "segment_factory": address(beam.SEGMENT), "call_sites": list(sites.values()), "candidates": candidates,
        "counts": {"direct_segment_sites": len(sites), "resolved_selector_sites": len(sites),
                   "candidate_rows": len(candidates), "candidate_models": len({r["borg"] for r in candidates}),
                   "queue_statuses": dict(Counter(r["normal_queue"]["status"] for r in candidates))},
        "notes": [
            "Parent type and row selectors are not the type-11 child's key. An absent parent type tag is not inferred.",
            "Type-32 direct carrier binds are excluded; type-32 modes select either carrier or child ownership.",
            "Some constructor guards admit numeric values whose table rows are zero. Only proved nonzero-row inlets are joined.",
            "A normal actor group queue is static, not a register, contact, HP or whole-move receipt. No gameplay is emitted.",
        ],
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "atlas", "selections", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export_census(args.dol, args.afs, args.atlas, args.selections, args.output, recover), sort_keys=True))
