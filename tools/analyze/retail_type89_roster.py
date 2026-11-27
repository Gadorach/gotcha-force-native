"""Census of bounded type-89 selectors and installed action ancestors."""
import argparse
from collections import Counter
from hashlib import sha256
from pathlib import Path
import struct

from tools.analyze import reaction_emit, reaction_installer_tables, retail_type89 as type89
from tools.analyze.retail_projectile_census import (
    AFS_SHA256, ATLAS_SHA256, INSTALLERS_SHA256, SOURCE_SHA256, address, callers, export_census, queue_metadata,
)


TEXT = (
    (0x800B8AD0, 32, "48ea1a85f4775729a8e61ce2bd664be238baaea3338d99ccbd1941193e5a5e2e"),
    (0x800B8AF0, 96, "fbf23e6904daa31be929d75f61c5c99e5dfd5f861b50a0dc7deffd8e64b9388c"),
    (0x800B8C44, 296, "4952d2478f2fef674e235e584a773771d450382ca605795f2240208dd786a9b6"),
    (0x800B8D6C, 420, "9dec13f8966a55d98b84abc59881c816fbe45fa325d4c0078c2dec4d4791ba11"),
    (0x800CB9D4, 60, "8702cd5ec74a15257fdabf71d58dd92e3534038cdeadeabc46238a7ab6018e71"),
    (0x800CBA38, 200, "7202b92d369f9fe2621e40a9e7320771043fcd6400345f5327e6b3a130dfe253"),
    (0x800CE5DC, 144, "01b1de75a1838afd823d830baa2a6be2c062c12889916042b9c04ad3acd3edb2"),
    (0x8010821C, 52, "4237bcfbfa0e40724c9c7a5b19e7d222bbb40002bb0e8b7c716585bc07036852"),
    (0x80108508, 40, "22aa759d7a26389359a68e7e4c904b9be07884b7bd03fb42a8e249378c78d857"),
    (0x80108530, 92, "a1c0a07ff52d3cd104018d9c4579e5fb807f15ce1cc6befb029c29a3fb4059ca"),
    (0x8010C9D8, 56, "f18367d1f42b13be133c71ae2d231e036766e0bf15856a5dd96d3d4eb2bcaa9f"),
    (0x8010CB48, 676, "df54a20bb04403ee867e2b7923e53a37cb2bf842fd270353d57f2055142bd23d"),
    (0x801346B8, 52, "b251c166af45d0ba44e341d53c7ce46fe3e88b48f210bf4f8066f6eadbe226a9"),
    (0x801346EC, 40, "b3350913192628159d4c7260d56ad61c26683ba0541091a7884a3d92d994e077"),
    (0x80134714, 92, "19513f48fc731b33dad967ed21701a14a7d15abf00a5cef71f6a158ddd9e94cd"),
)
LITERALS = {
    0x800CBA78: 0, 0x800CBA90: 12, 0x800CBAA8: 13, 0x800CBAC0: 14, 0x800CE624: 2,
    0x80108564: 15, 0x80134748: 1, 0x8018EE54: 16, 0x801B395C: 11,
}
INDEXED = (0x800B8E84, 0x800B8E9C)
PAIR_TABLE, SWITCH_TABLE = 0x802FF31C, 0x802FF33C
TIMER_ROWS, TIMER_POINTERS = 0x80350598, 0x803505E0
DEBIT = 0x8006DBE0
UNQUALIFIED = {0x800CBA78: 0x0600, 0x800CBA90: 0x0608, 0x800CBAA8: 0x0616, 0x800CBAC0: 0x061C}


def pinned_data(image, at, size, digest):
    raw = image.read(at, size, "data")
    if sha256(raw).hexdigest() != digest:
        raise ValueError(f"type-89 data signature mismatch at {address(at)}")
    return raw


def indexed_selectors(image):
    raw = pinned_data(image, PAIR_TABLE, 32, "34debdb93eeb6244414a68b2d5e6d69bb24bce323f5f9c6a64dc4441f53356b5")
    switch = pinned_data(image, SWITCH_TABLE, 68, "85974805cb698900972dda53e2ad93a16db1763edbc82a1c48c5eb272f59a256")
    # The default is bank 0; only two of the 17 guarded model arms use bank 1.
    bank_one = [0x0503+i for i, (arm,) in enumerate(struct.iter_unpack(">I", switch)) if arm == 0x800B8E3C]
    if bank_one != [0x0506, 0x0512]:
        raise ValueError("type-89 indexed model domain changed")
    banks = []
    for bank in range(2):
        pairs = list(struct.iter_unpack(">bb", raw[16*bank:16*(bank+1)]))
        stop = next(i for i, (first, _) in enumerate(pairs) if first < 0)
        if (pairs[stop:] != [(-1, -1)]*(8-stop)
                or any(not 0 <= first < type89.COUNT or not -1 <= second < type89.COUNT
                       for first, second in pairs[:stop])):
            raise ValueError("type-89 cursor or pair extent changed")
        banks.append({"bank": bank, "model_guard": "otherwise" if bank == 0 else [f"{m:04X}" for m in bank_one],
                      "cursor_start": 0, "first_negative_cursor": stop,
                      "ordered_keys_by_cursor": [[first] if second == -1 else [first, second]
                                                 for first, second in pairs[:stop]]})
    return {"table": address(PAIR_TABLE), "model_dispatch": address(SWITCH_TABLE),
            "actor_cursor_offset": "0x6EB", "actor_timer_offset": "0x558", "cursor_reset_site": "800B8D44",
            "cursor_stride": 2, "bank_stride": 16, "banks": banks}


def factory_rows(image):
    rows = type89.rows(image)
    raw = pinned_data(image, TIMER_ROWS, 72, "7bcc8fd75ee88b6923ddb578aaa99c9498a1cbcc6f62c21cb6febcd3f20d4689")
    pointers = pinned_data(image, TIMER_POINTERS, 68, "3f80ec8b3e5794e12e4b295bcbc9b2088f361cfb57181efad7f1c908b9a216fc")
    for row, (pointer,) in zip(rows, struct.iter_unpack(">I", pointers), strict=True):
        at = pointer-TIMER_ROWS
        if at not in (0, 24, 48):
            raise ValueError("type-89 timing pointer outside reviewed tables")
        operand = image.read(int(row["row"], 16), type89.STRIDE, "data")
        joint, = struct.unpack_from(">h", operand)
        row.update(socket_selector=joint, socket_local_point=struct.unpack_from(">3f", operand, 12),
                   timer_program=address(pointer),
                   phase_duration_units=[duration for _, duration, _ in struct.iter_unpack(">hhI", raw[at:at+24])])
    return rows


def recover(image, archive, atlas, selections):
    type89.verify_text(image, TEXT)
    models = {int(r["id"][-4:], 16): r for r in selections["borgs"]}
    static = {int(r["id"][-4:], 16): r for r in atlas["borgs"]}
    if (set(models) != set(static) or len(models) != 206
            or any(d["source_dol_sha256"] != SOURCE_SHA256 or len(d["borgs"]) != 206 for d in (atlas, selections))):
        raise ValueError("type-89 selected profile coverage or source identity")
    scoped = {int(r["borg"][-4:], 16): r for r in type89.ancestors(image, atlas, selections)}
    rows = factory_rows(image)
    indexed = indexed_selectors(image)
    sites = callers(image, atlas, type89.FACTORY, len(rows))
    if set(sites) != set(LITERALS) | set(INDEXED):
        raise ValueError("type-89 direct call coverage changed")
    for pc, site in sites.items():
        if site["adjacent_li_r4_key"] != LITERALS.get(pc):
            raise ValueError("type-89 literal key mismatch")
        type89.link(image, pc, type89.FACTORY)
        site["key_source"] = ({"kind": "literal-r4", "key": LITERALS[pc]} if pc in LITERALS else
                              {"kind": "indexed-pair", "table": address(PAIR_TABLE), "pair_byte": INDEXED.index(pc)})

    entries = {0x802FF314: 0x800B8C44, 0x802FF318: 0x800B8D6C, 0x8030A560: 0x800CB9D4,
               0x8030A674: 0x800CBA38, 0x8030CD20: 0x800CE5DC,
               0x8043450C: 0x8010CB48, 0x80324128: 0x80108530, 0x80336DA0: 0x80134714}
    if any(type89.word(image, at, "data") != target for at, target in entries.items()):
        raise ValueError("type-89 action dispatch entry changed")
    for at, target in ((0x800B8ADC, 0x800B8AF0), (0x80108234, 0x80108508),
                       (0x8010851C, 0x8010C9D8), (0x801346D8, 0x801346EC), (0x80134700, 0x8010C9D8)):
        type89.link(image, at, target)
    result = []

    def join(model, profile, action, state, family, leaf, condition, casts, debit=None):
        observed = models[model]
        if observed["profile"] != profile or observed["spec_record"] != 0 or profile not in static[model]["profiles"]:
            raise ValueError("type-89 normal installer selection changed")
        installer, table = reaction_installer_tables.model_table(image, model)
        if address(installer) != observed["installer"] or address(table) != static[model]["reaction_table"]:
            raise ValueError("type-89 actor source changed")
        contexts = [r for g in atlas["profiles"][profile]["groups"] if g["group"] == action for r in g["rows"]
                    if r["classification"] == "selector-candidate" and r["state"] == state
                    and r.get("code_path", {}).get("family") == address(family)]
        if not contexts:
            raise ValueError("type-89 action not found in selected profile")
        block = reaction_emit.qualified_hit_block(archive, model)
        if debit:
            type89.link(image, debit[0], DEBIT)
        for pc, key, cursor in casts:
            if sites[pc]["function"] != address(leaf) or (pc in LITERALS and LITERALS[pc] != key):
                raise ValueError("type-89 selected call leaf changed")
            result.append({
                "borg": observed["id"], "profile": profile, "spec_record": observed["spec_record"],
                "action": action, "action_state": state, "action_family": address(family),
                "site": address(pc), "key": key, "condition": condition,
                "opening_queue": queue_metadata(image, block, table, rows[key]["group"]),
                **({"cursor": cursor, "order_at_cursor": INDEXED.index(pc)+1} if cursor is not None else {}),
                **({"same_leaf_debit": {"site": address(debit[0]), "channel": debit[1], "cost": 1}} if debit else {}),
            })

    shared_cue = "reviewed model arm; actor phase one; +0x1B03 and +0x542 both zero; paid callback"
    join(0x0306, "p041", "X", 2, 0x8010821C, 0x80108530, shared_cue,
         [(0x80108564, 15, None)], (0x80108550, 2))
    join(0x0304, "p043", "X", 2, 0x801346B8, 0x80134714, shared_cue,
         [(0x80134748, 1, None)], (0x80134734, 2))
    for model, profile, action, bank in ((0x0503, "p068", "B-charge", 0), (0x0506, "p073", "B-shot", 1),
                                         (0x050F, "p074", "B-charge", 0), (0x0512, "p075", "B-shot", 1)):
        series = indexed["banks"][bank]["ordered_keys_by_cursor"]
        join(model, profile, action, 3, 0x800B8AD0, 0x800B8D6C, "actor phase two; timer not positive; nonnegative pair byte",
             [(INDEXED[order], key, cursor) for cursor, keys in enumerate(series) for order, key in enumerate(keys)])
    # The X dispatcher indexes +0x581. Every installed state-two X row
    # points at the inert sibling, not the model-guarded paid leaf at index 7.
    p076 = atlas["profiles"]["p076"]
    configured = [(r["state"], r["substate"], r["code_path"]["substate_entry"]) for g in p076["groups"]
                  if g["group"] == "X" for r in g["rows"]]
    if configured != [(2, i, "800CBA10") for i in range(5)]:
        raise ValueError("type-89 index-seven refused action scope changed")
    type89.link(image, 0x800CBA58, DEBIT)
    for pc, model in UNQUALIFIED.items():
        if models[model]["profile"] != "p076":
            raise ValueError("type-89 index-seven model selection changed")
        sites[pc]["unjoined"] = {"required_model": f"{model:04X}", "dispatch_index": 7,
                                 "installed_X_substates": [0, 1, 2, 3, 4],
                                 "reason": "no-qualified-index-seven-inlet"}
    for model in (0x0602, 0x060A, 0x060C, 0x060E):
        join(model, "p079", "X", 2, 0x800CE5DC, 0x800CE5DC, "actor phase zero; paid callback",
             [(0x800CE624, 2, None)], (0x800CE610, 2))
    for model, action, state, family, debit in ((0x062A, "B-charge", 3, 0x8018ED4C, None),
                                               (0x0F03, "B-shot", 0, 0x801B372C, (0x801B392C, 0))):
        ancestor = scoped[model]
        pc = int(ancestor["parent"]["site"], 16)
        join(model, ancestor["profile"], action, state, family, int(sites[pc]["function"], 16), ancestor["ancestor_scope"],
             [(pc, ancestor["parent"]["key"], None)], debit)

    joined = {int(r["site"], 16) for r in result}
    if joined != set(sites)-set(UNQUALIFIED):
        raise ValueError("type-89 reviewed joins do not cover the selector domain")
    return {
        "kind": "gf-retail-type89-roster", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "code_atlas_sha256": ATLAS_SHA256, "standalone_installers_sha256": INSTALLERS_SHA256,
        "factory": address(type89.FACTORY), "factory_rows": rows, "call_sites": [sites[pc] for pc in sorted(sites)],
        "pair_selector": indexed, "candidates": result,
        "counts": {"roster_profiles_examined": len(models), "candidate_models": len({r["borg"] for r in result}),
                   "direct_call_sites": len(sites), "literal_sites": len(LITERALS), "indexed_sites": len(INDEXED),
                   "candidate_rows": len(result), "factory_rows": len(rows), "unjoined_sites": len(UNQUALIFIED),
                   "queue_statuses": dict(Counter(r["opening_queue"]["status"] for r in result))},
        "notes": [
            "Installed action rows and guarded factory requests are static joins, not playback or actor birth.",
            "The indexed pair counts requests in order, not allocations. Each cursor is one timed step; a negative first byte stops the sequence.",
            "The same_leaf_debit field is only for a debit in the factory-calling leaf; its absence does not mean a move is free.",
            "Opening queues are from the paired normal actor hit block. Raw operands are not damage, and the type-5 exit is separate from the carrier.",
            "No live scheduling, damage attribution, alternate-spec or combined-form ownership, or native behavior is emitted.",
        ],
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "atlas", "selections", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(export_census(args.dol, args.afs, args.atlas, args.selections, args.output, recover))
