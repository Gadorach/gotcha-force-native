"""Map type-76 parent keys to installed routes without assigning child damage."""
import argparse
import json
from pathlib import Path
import struct

from tools.analyze import reaction_emit, reaction_installer_tables, retail_type32_roster as beam
from tools.analyze import retail_type9_roster as descendants
from tools.analyze.retail_action_links import ActionLinker, immediate_argument, literal_flow
from tools.analyze.retail_projectile_census import (
    AFS_SHA256, ATLAS_SHA256, INSTALLERS_SHA256, SOURCE_SHA256, address, callers, export_census, queue_metadata,
)


FACTORY, UPDATE, INIT, ACTIVE, DISPATCH = 0x800FCD38, 0x800FCE50, 0x800FCEE8, 0x800FD09C, 0x8031EED8
ROW_TABLE, SOCKET_TABLE = descendants.TYPE76_TABLE, 0x8031EE00
ROW_COUNT = 6
# Model guard, primary row, ordinal source (actor byte, or literal zero).
SITES = {
    0x800F7F20: (0x0D04, 2, 0x151),
    0x8015C754: (0x0612, 1, None),
    0x8015C770: (0x0620, 3, None),
    0x8015C78C: (0x0624, 4, None),
    0x8015C7A8: (0x0626, 5, None),
    0x80188F7C: (0x0D01, 0, 0x148),
}
NORMAL = (
    # Model, profile, installed family and state, paid leaf, reset, debit, request.
    (0x0D01, "p153", 0x80188EC4, 2, 0x80362680, 0x80188F18, 0x80188A64, 0x80188A98, 0x80188F4C, 0, 0x80188F7C,
     ("B-shot", "B-close")),
    (0x0D04, "p156", 0x800F7DC0, 2, 0x8031D14C, 0x800F7EA4, 0x800F77C8, 0x800F7828, 0x800F7EF0, 2, 0x800F7F20,
     ("X",)),
)
TEXT = (
    *tuple(s for s in descendants.TEXT if s[0] in (FACTORY, ACTIVE, 0x800FD2D0)),
    (UPDATE, 152, "1c70c5d911fb03f9a14f57810a3b8751b8f6e5fbc0ec92ea104fbf7b20fda33f"),
    (INIT, 436, "b685ef3e111db1353fc2af23346e70a874d27e5eb3871176fd24f1de669fea81"),
    (0x800FD288, 72, "11914f00c8dad9a10c4fe430fb64b42ade290b09ae630d16d929608980a6fec7"),
    (0x800F7EA4, 172, "5025becad5e8e3a34e7eee2003a982af04bbbf349a214067250ffc7c371eca7c"),
    (0x800F7DC0, 204, "045619889603df6fad7f79c21b88c2dd00c78c20d0d0d4dde16f8ef95005be6c"),
    (0x800F77C8, 192, "bd3afef0658084ca3262375187f7188fed3c5982a74f8c7a4f737bbf8b87ea1f"),
    (0x8015C588, 32, "48ea1a85f4775729a8e61ce2bd664be238baaea3338d99ccbd1941193e5a5e2e"),
    (0x8015C5A8, 60, "5212587a35c95aecbe6205ae07794e0dd98cf9fb41a04edc31e3a2a5dd4d14d1"),
    (0x8015C6E8, 268, "3f46cf0e75ff3e5125fc67e5897b39d5839c462b853754c53b6d9887b1470f55"),
    (0x80188EC4, 60, "4694bb358df7103a149c4138bc541b9bc734be48ae2eb81f334d052d031a233c"),
    (0x80188F18, 148, "c1a0a0df4a87cb746a4db305a11155f0943e99bcdbf033b27f9f4e9e444fb195"),
    (0x80188A64, 172, "ff87c84d13695a9f7bf5c42ac2597c368a6f1402e24af383aaf3444bbfe01abb"),
)
GUARDS = {0x800F7F20: 0x800F7EBC, 0x8015C754: 0x8015C740, 0x8015C770: 0x8015C75C,
          0x8015C78C: 0x8015C778, 0x8015C7A8: 0x8015C794}


def row_index(image):
    beam.verify_text(image, TEXT)
    read = descendants.pinned_data
    raw = read(image, ROW_TABLE, ROW_COUNT*32, "8b067fe9d3015062afdc967b4dd5b2c1bacc120b04b8e63b3ca7243672ced13d")
    sockets = struct.unpack(">6I", read(image, SOCKET_TABLE, 24, "7a90fc32339cc6a230e0eb66c6858f921c8a8690ab465ff3474ce2a3632e0619"))
    phases = struct.unpack(">4I", read(image, DISPATCH, 16, "f420637dd4e27c20b677ecfb62176341d40f8b5ec88d986672d6bc622be59e5c"))
    block_at = 0x8031ED40
    block = read(image, block_at, SOCKET_TABLE-block_at, "4568a95642a7564d479679bf59b06b0d3b01400f4bb6b7191ffb91bd4273f755")
    rows = []
    for key, at in enumerate(sockets):
        group, *_, lane, child = struct.unpack_from(">4h5f2h", raw, 32*key)
        # Calls resolve ordinals 0/1 for rows 0 and 2; the other rows are literal 0.
        variants = []
        for ordinal in range(2 if key in (0, 2) else 1):
            source = at+24*ordinal
            if not block_at <= source <= SOCKET_TABLE-24:
                raise ValueError("type-76 socket variant outside reviewed table")
            visual, socket, x, y, z = struct.unpack_from(">2h3f", block, source-block_at)
            variants.append({"ordinal": ordinal, "row": address(source), "visual_record": visual,
                             "socket": socket, "socket_offset": [x, y, z]})
        if (lane, child) not in ((0, 0), (1, 1)) or not 0 <= group < 256:
            raise ValueError("type-76 descendant selector outside reviewed domain")
        rows.append({"key": key, "row": address(ROW_TABLE+32*key), "carrier_group": group, "socket_variants": variants,
                     "exit_descendant": {"type": 8+lane, "key": child, "site": "800FD314" if lane == 0 else "800FD324"}})
    for pc, callee in ((0x800FCD6C, 0x80088AA0), (0x800FD03C, 0x8008AC80), (0x800FD100, 0x800FD2D0),
                       (0x800FD254, 0x80084170), (0x800FD26C, 0x800FD2D0), (0x800FD2AC, 0x8008AFF0),
                       (0x800FD314, descendants.PARENT), (0x800FD324, descendants.EFFECT),
                       (0x800FD144, 0x8001BDA8), (0x800FD16C, 0x8001BDA8)):
        beam.link(image, pc, callee)
    return rows, phases


def call_index(image, atlas):
    found = callers(image, atlas, FACTORY, ROW_COUNT)
    if found.keys() != SITES.keys():
        raise ValueError("type-76 factory site coverage changed")
    linker = ActionLinker(image, {}, ())
    sites = {}
    for pc, (model, key, cursor) in sorted(SITES.items()):
        entry = found[pc]["function"]
        function = int(entry, 16)
        words, incoming, successors, direct, frontier, exhausted = linker.graph(function, atlas["functions"][entry]["size"], 320)
        flow = literal_flow(function, words, successors, linker.data).get(pc, {})
        if (frontier or exhausted or direct.get(pc) != FACTORY or flow.get(4) != key
                or (pc in GUARDS and beam.word(image, GUARDS[pc]) != (0x2C000000 | model))):
            raise ValueError(f"type-76 model or parent row selector changed at {address(pc)}")
        if cursor is None:
            if immediate_argument(words, incoming, pc, 4) != key or flow.get(5) != 0:
                raise ValueError("type-76 literal row or ordinal changed")
            sub = {"kind": "literal-r5", "values": [0]}
        else:
            if (flow.get(5) is not None
                    or (beam.word(image, pc-8), beam.word(image, pc-4)) != (0x88BF0000 | cursor, 0x7CA50774)):
                raise ValueError("type-76 ordinal read changed")
            # After the original constructor returns, increment then mask the cursor to one bit.
            tail = tuple(beam.word(image, pc+i) for i in (4, 8, 12, 16, 20, 24))
            expected = (0x887F0000 | cursor, 0x38030001, 0x981F0000 | cursor,
                        0x881F0000 | cursor, 0x540007FE, 0x981F0000 | cursor)
            if tail != expected:
                raise ValueError("type-76 alternating cursor update changed")
            sub = {"kind": "increment-and-mask-cursor", "actor_offset": f"0x{cursor:X}", "values": [0, 1]}
        sites[pc] = {"site": address(pc), "function": entry, "parent_key": key, "ordinal_source": sub,
                     "model_selector": f"{model:04X}" if pc in GUARDS else None}
    return sites


def recover(image, archive, atlas, selections):
    rows, phases = row_index(image)
    sites = call_index(image, atlas)
    picked = {int(b["id"][-4:], 16): b for b in selections["borgs"]}
    static = {int(b["id"][-4:], 16): b for b in atlas["borgs"]}
    if (len(picked) != 206 or set(picked) != set(static)
            or any(d["source_dol_sha256"] != SOURCE_SHA256 or len(d["borgs"]) != 206 for d in (atlas, selections))):
        raise ValueError("type-76 installer census scope changed")

    def matches(profile, action, state, family):
        rows = [r for g in atlas["profiles"][profile]["groups"] if g["group"] == action for r in g["rows"]
                if r["classification"] == "selector-candidate" and r["state"] == state
                and r.get("code_path", {}).get("family") == address(family)]
        if any(beam.word(image, int(r["code_path"]["family_table"], 16)+4*state, "data") != family for r in rows):
            raise ValueError("type-76 installed family slot changed")
        return rows

    candidates = []
    for model, profile, family, state, table_slot, leaf, pre, reset, debit, channel, request, actions in NORMAL:
        installer, reaction = reaction_installer_tables.model_table(image, model)
        selected, actor = picked[model], static[model]
        row = rows[sites[request]["parent_key"]]
        if (selected["profile"] != profile or selected["spec_record"] != 0 or profile not in actor["profiles"]
                or selected["installer"] != address(installer) or actor["reaction_table"] != address(reaction)
                or selected["pre_callback"] != address(pre) or beam.word(image, table_slot+4, "data") != leaf
                or beam.word(image, reset) != (0x98030000 | SITES[request][2])):
            raise ValueError("type-76 scoped actor, phase entry or cursor reset changed")
        beam.link(image, debit, beam.DEBIT)
        block = reaction_emit.qualified_hit_block(archive, model)
        queue = queue_metadata(image, block, reaction, row["carrier_group"])
        if queue["status"] != "mixed-power":
            raise ValueError("type-76 normal scope no longer has a mixed queue")
        for action in actions:
            matched = matches(profile, action, state, family)
            if not matched or {r["substate"] for r in matched} != {0}:
                raise ValueError("type-76 scoped installed action changed")
            candidates.append({
                "borg": selected["id"], "profile": profile, "spec_record": 0, "action": action, "state": state,
                "family": address(family), "phase_one_leaf": address(leaf), "site": address(request), "parent_key": row["key"],
                "debit": {"site": address(debit), "channel": channel, "cost": 1},
                "conditional_ordinal_reset_site": address(reset), "source_ordinals": [0, 1],
                "carrier_group": row["carrier_group"], "normal_carrier_queue": queue, "exit_descendant": row["exit_descendant"],
            })
    if (picked[0x0620]["spec_record"] != 2 or picked[0x0620]["profile"] != "p098"
            or not matches("p098", "X", 3, 0x8015C588)
            or beam.word(image, 0x8034628C, "data") != 0x8015C6E8
            or any(model in picked for model in (0x0612, 0x0624, 0x0626))):
        raise ValueError("type-76 unresolved form branch scope changed")
    beam.link(image, 0x8015C594, 0x8015C5A8)
    for pc in sites:
        sites[pc]["ownership_status"] = ("scoped-normal-action" if pc in (0x800F7F20, 0x80188F7C) else
                                        "alternate-spec-ownership-unqualified" if pc == 0x8015C770 else
                                        "no-standalone-selection-for-guarded-model")
    return {
        "kind": "gf-retail-type76-roster", "revision": 1, "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "code_atlas_sha256": ATLAS_SHA256, "standalone_installers_sha256": INSTALLERS_SHA256,
        "factory": address(FACTORY), "dispatch": {"entry": address(UPDATE), "table": address(DISPATCH),
                                                "phases": [address(p) for p in phases]},
        "factory_rows": rows, "call_sites": list(sites.values()), "candidates": candidates,
        "unqualified_installed_route": {"borg": "gg4e08:0620", "profile": "p098", "spec_record": 2,
                                       "action": "X", "state": 3, "phase_two_leaf": "8015C6E8", "site": "8015C770"},
        "active_descendant_select": {"entry": "800FD2D0", "after_stop_calls": [
            {"site": "800FD100", "inlet": "nonzero-contact-latch"},
            {"site": "800FD26C", "inlet": "stage-probe-returned-nonzero"},
        ], "different_helper_for_zero_hp_or_expiry": "8001BDA8"},
        "counts": {"rows": len(rows), "direct_sites": len(sites), "normal_sites": 2, "unqualified_sites": 4,
                   "normal_action_routes": len(candidates), "normal_models": len({r["borg"] for r in candidates})},
        "notes": [
            "Rows and socket ordinals are distinct inputs; the ordinal does not change the carrier group or exit key.",
            "The credited actor's normal carrier queue is static. Both normal routes contain mixed power; no step has been assigned a hit or HP loss.",
            "The exit selector follows the type-76 stop request. It is not the parent shape bind, and not every way to stop goes through this selector.",
            "The 0620 phase leaf tests several model constants. Only the chosen alternate spec has a scoped static installed predecessor here; no form owns a queue in this receipt.",
            "No original action admission, shape publication, contact, live timing or native gameplay is qualified by this census.",
        ],
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "atlas", "selections", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export_census(args.dol, args.afs, args.atlas, args.selections, args.output, recover), sort_keys=True))
