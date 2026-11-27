"""Join five proved parent dispatch chains to the type-4 call census."""
from dataclasses import dataclass

from tools.analyze import reaction_installer_tables, retail_type32_roster as beam
from tools.analyze.retail_action_links import ActionLinker, literal_flow
from tools.analyze.retail_code_atlas import address


@dataclass(frozen=True)
class Parent:
    kind: int
    constructor: int
    tag_store: int
    update_store: int
    update: int
    phase_load: int
    phase_table: int
    phase: int
    branch: int
    sites: tuple[int, ...]


PARENTS = {
    p.kind: p for p in (
        Parent(33, 0x800FE668, 0x800FE6D0, 0x800FE6D4, 0x800FE780, 0x800FE7F8, 0x8031EFE0, 1, 0x800FEAE8,
               (0x800FEB78, 0x800FEBB8)),
        Parent(52, 0x80128DF8, 0x80128E9C, 0x80128EA0, 0x80128F54, 0x80128FCC, 0x80330B74, 1, 0x801292C8,
               (0x80129398, 0x801293D8)),
        Parent(74, 0x800F9400, 0x800F9464, 0x800F9468, 0x800F9518, 0x800F9590, 0x8031D37C, 1, 0x800F97AC,
               (0x800F9838, 0x800F9878)),
        Parent(77, 0x800FB220, 0x800FB284, 0x800FB288, 0x800FB330, 0x800FB3A8, 0x8031E930, 1, 0x800FB56C,
               (0x800FB620, 0x800FB648)),
        Parent(98, 0x801F49C8, 0x801F4A28, 0x801F4A2C, 0x801F4AC8, 0x801F4B24, 0x803A25E0, 2, 0x801F59FC,
               (0x801F5A3C,)),
    )
}
TEXT = (
    (0x800FE668, 280, "6a67ec1905ac26db2127e96d5719893728ea4984941548190ae1028f1b5d6f32"),
    (0x801313BC, 476, "9b2253c8bcad096107f9e9b1c0e07266793c5407993f6b1e1de964e4b8bc6f04"),
    (0x800FEAE8, 448, "fd0829145dcd4b9c0b18b63a9169fca6ddb58ea2ed2427736c30f0fac62aad47"),
    (0x800CF6FC, 392, "429e7aa2e7e289ac3a0c056ba20eddd76d6b71675c0c82e36ce599816f484a2d"),
    (0x80128DF8, 348, "f992dbf07b446de0cacf0100233b26fcf9a54be6f5c77eb4b4e1a47e810a4641"),
    (0x801292C8, 516, "b744c1858d8d9e008a75f35e602393b2c85810cdf5fc0948908910b0cabb3675"),
    (0x800F7740, 136, "c6baf7020776e8053568f492801610d7229732724088761c93768d7ccaab3fd1"),
    (0x800F77C8, 192, "bd3afef0658084ca3262375187f7188fed3c5982a74f8c7a4f737bbf8b87ea1f"),
    (0x800F7BF4, 196, "1b2ca949d570d58f6d6c901447664ad0fa1707f7de78ab2aa909255a351fb5ee"),
    (0x800F7CD0, 168, "f610e06ec65b495d7203f4209d40670cb5aef2d914d8156f179116df43c5947a"),
    (0x800F9400, 280, "65401bc92f585d8b8fa3ef920c79e50d69872177ba5c0e394f9a3185e0c0d1ef"),
    (0x800F97AC, 440, "e7891be09163c2422493d085911311febdf215f60bf0a45da78c04704dc253c7"),
    (0x80188DF8, 132, "a77ebb5a57e0c4c8978ead493d388ce3df752caa5ab5c4cedb408fb361eab150"),
    (0x800FB220, 272, "39a72d59ef73db15a39f7c25323eb582e64ca2e66556c5f9662939dab8a2522e"),
    (0x800FB56C, 512, "9da83478fe307d1e8ef398228c7c70b29cc609908ca37627ccc98071a363baae"),
    (0x801D5C84, 104, "130d9741e81610d6223e33f44267accf6b44b711bfacb64e928a0e2058e62ce1"),
    (0x801D61A8, 124, "118b84bbf13ecb6aa1b3702e466946bee8ed625bd0884a5cdbcac10b3c24a5df"),
    (0x801F49C8, 256, "b783b57229a7ff24dd1a96b0020516657d161c10884e8b303d3681b85f225c4b"),
    (0x801F59FC, 104, "a485045e4f59ded74e8d0516cd501a3c993a95ff3bd5a290d447c98659bb9add"),
)
# Substates 1..3 share a slot. The atlas omits some later phases; read the slot.
RELAY33 = tuple(zip((0, 1, 2, 3, 4), (0x80335CDC, 0x80335CE8, 0x80335CE8, 0x80335CE8, 0x80335CF4),
                    (0x801309B8, 0x80130D34, 0x80130D34, 0x80130D34, 0x80131184),
                    (0x80130A6C, 0x80130E2C, 0x80130E2C, 0x80130E2C, 0x80131244), strict=True))
RELAY52 = tuple(zip((0, 1, 2, 3, 4), (0x8030CE5C, 0x8030CE68, 0x8030CE68, 0x8030CE68, 0x8030CE74),
                    (0x800CEC34, 0x800CEFFC, 0x800CEFFC, 0x800CEFFC, 0x800CF3F4),
                    (0x800CECB0, 0x800CF0EC, 0x800CF0EC, 0x800CF0EC, 0x800CF488), strict=True))
PHASE_74 = ((0, 0x8031D140, 0x800F7CD0, None),)
PHASE_77 = ((0, 0x80362674, 0x80188DF8, None),)
PHASE_98 = ((0, 0x8038DBF4, 0x801D61A8, None),)


def recover(image, atlas, picked, sites):
    beam.verify_text(image, TEXT)
    linker = ActionLinker(image, {}, ())
    static = {r["id"]: r for r in atlas["borgs"]}
    flow_cache = {}

    def dataflow(entry, pc, expected):
        if entry not in flow_cache:
            words, _, outgoing, _, _, exhausted = linker.graph(entry, atlas["functions"][address(entry)]["size"], 480)
            if exhausted:
                raise ValueError(f"type-4 predecessor graph budget at {address(entry)}")
            flow_cache[entry] = words, literal_flow(entry, words, outgoing, linker.data)
        words, flow = flow_cache[entry]
        actual = flow.get(pc, {})
        if pc not in words or any(actual.get(reg) != value for reg, value in expected.items()):
            raise ValueError(f"type-4 predecessor operand changed at {address(pc)}")

    for p in PARENTS.values():
        dataflow(p.constructor, p.tag_store, {6: p.kind})
        dataflow(p.constructor, p.update_store, {5: p.update})
        dataflow(p.update, p.phase_load, {4: p.phase_table})
        if (beam.word(image, p.tag_store) != 0x98C30010 or beam.word(image, p.update_store) != 0x90A3000C
                or beam.word(image, p.phase_load) != 0x7D84002E
                or beam.word(image, p.phase_table+4*p.phase, "data") != p.branch
                or any(sites[pc]["function"] != address(p.branch) for pc in p.sites)):
            raise ValueError(f"type-4 predecessor dispatch or type tag changed: {p.kind}")

    # Cursor reset and bounded updates for the two variable-selector aircraft.
    dataflow(0x800F7580, 0x800F75A0, {0: 0x800F7740})
    dataflow(0x800F7740, 0x800F7774, {0: 0})
    dataflow(0x800F77C8, 0x800F7804, {0: 0})
    dataflow(0x801D5BA4, 0x801D5BC4, {0: 0x801D5C84})
    dataflow(0x801D5C84, 0x801D5CBC, {0: 0})
    dataflow(0x800F7BF4, 0x800F7C34, {4: 0x8031D140})
    dataflow(0x800F9400, 0x800F9460, {7: 0})
    for pc, op in ((0x800F7774, 0x981F0148), (0x800F7804, 0x98030148),
                   (0x800F7D40, 0x889F0148), (0x800F7D44, 0x7C840774),
                   (0x800F7D5C, 0x540007BE), (0x800F9460, 0x98E30011),
                   (0x801D5CBC, 0x981F014E), (0x801D61EC, 0x889F014E), (0x801D61F0, 0x7C840774),
                   (0x801D61FC, 0x68000001), (0x801D6200, 0x981F014E)):
        if beam.word(image, pc) != op:
            raise ValueError(f"type-4 aircraft selector bound changed at {address(pc)}")

    candidates = []

    def join(model, profile, action, state, family, path, leaf, debit, channel, pc, kind, keys, operands, scope):
        ident = f"gg4e08:{model:04x}"
        install, table = reaction_installer_tables.model_table(image, model)
        selected = picked[ident]
        if (selected["spec_record"] != 0 or selected["profile"] != profile or selected["installer"] != address(install)
                or profile not in static[ident]["profiles"] or static[ident]["reaction_table"] != address(table)):
            raise ValueError(f"type-4 scoped normal model changed: {ident}")
        action_rows = [r for g in atlas["profiles"][profile]["groups"] if g["group"] == action for r in g["rows"]
                       if r["classification"] == "selector-candidate" and r["state"] == state]
        for sub, phase_table, phase_one, relay in path:
            matched = [r["code_path"] for r in action_rows if r["substate"] == sub
                       and r.get("code_path", {}).get("family") == address(family)]
            if not matched or any(
                beam.word(image, int(c["family_table"], 16)+4*state, "data") != family
                or ("phase_table" in c and c["phase_table"] != address(phase_table))
                for c in matched
            ) or beam.word(image, phase_table+4, "data") != phase_one:
                raise ValueError(f"type-4 installed phase slot changed: {ident}/{action}/{sub}")
            if relay is not None:
                beam.link(image, relay, leaf)
            elif phase_one != leaf:
                raise ValueError("type-4 action phase needs a relay")

        parent = PARENTS[kind]
        beam.link(image, pc, parent.constructor)
        beam.link(image, debit, beam.DEBIT)
        dataflow(leaf, pc, operands)
        dataflow(leaf, debit, {4: channel, 5: 1, 6: 1})
        candidates.append({
            "borg": ident, "profile": profile, "action": action, "state": state, "family": address(family),
            "action_phase": 1, "substates": [sub for sub, *_ in path],
            "phase_one_entries": sorted({address(phase) for _, _, phase, _ in path}),
            "debit": {"site": address(debit), "channel": channel, "cost": 1},
            "parent": {"type": kind, "keys": keys, "site": address(pc), "constructor": address(parent.constructor),
                       "selector_scope": scope},
            "descendant_sites": [address(v) for v in parent.sites],
            "scope": ("phase-two carrier exit with +0x15B clear" if kind == 98
                      else "zero-HP or expiry arm, not the separate actor-contact area arm"),
        })

    for action in ("B-shot", "X"):
        join(0x0106, "p018", action, 0, 0x801307C4, RELAY33, 0x801313BC, 0x80131548, 2,
             0x8013156C, 33, [2], {4: 2, 5: 2}, "exact model, positive +0x6EF; literal row and ordinal 2")
    for model, pc, debit, key, operand in ((0x0601, 0x800CF760, 0x800CF740, 0, "+0x6E8"),
                                            (0x0617, 0x800CF858, 0x800CF834, 1, "+0x6E8 plus 2")):
        join(model, "p077", "B-shot", 0, 0x800CEA98, RELAY52, 0x800CF6FC, debit, 0, pc, 52, [key], {5: key, 6: 0xFFFFFFFF},
             f"exact model and literal row; socket ordinal from {operand} not bound")
    join(0x0D00, "p152", "X", 1, 0x800F7BF4, PHASE_74, 0x800F7CD0, 0x800F7D1C, 2, 0x800F7D48, 74,
         [0], {5: 1}, "factory writes row 0; parent ordinal 0..3 from the reviewed reset and two-bit update")
    for model, profile, pc, key in ((0x0D01, "p153", 0x80188E54, 0), (0x0D05, "p157", 0x80188E64, 1)):
        join(model, profile, "X", 1, 0x80188DA4, PHASE_77, 0x80188DF8, 0x80188E2C, 2, pc, 77, [key], {4: key},
             "normal model branch and literal row")
    join(0x0E03, "p163", "X", 2, 0x801D615C, PHASE_98, 0x801D61A8, 0x801D61D4, 2, 0x801D61F4, 98,
         [0, 1], {}, "row cursor +0x14E: reviewed reset and XOR 1 update")

    if len(candidates) != 8:
        raise ValueError("type-4 extended ancestor coverage changed")
    return candidates, [{
        "type": p.kind, "constructor": address(p.constructor), "update_store": address(p.update_store),
        "update": address(p.update), "phase_table": address(p.phase_table), "phase": p.phase,
        "callback": address(p.branch), "type4_sites": [address(pc) for pc in p.sites],
    } for p in PARENTS.values()]
