"""Inventory the reviewed type-14, type-15, type-47, type-49 and type-94 releases."""
import argparse
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze import reaction_emit as emit, reaction_installer_tables, retail_dragon_shot as dragon, retail_worm_shot as worm
from tools.analyze.reaction_projectile_map import selector_code, word
from tools.analyze.retail_action_links import direct_branch
from tools.analyze.retail_projectile_census import (
    AFS_SHA256, ATLAS_SHA256, INSTALLERS_SHA256, SOURCE_SHA256, address, candidate_census, export_census, queue_metadata,
)
from tools.analyze.retail_projectile_origins import origin_tables

FACTORIES = {
    "type-14": 0x8006EE14, "type-15": 0x8007DB5C, "type-47": dragon.FACTORY,
    **{f"type-{kind}": row[4] for kind, row in worm.FAMILIES.items()},
}
EXTRA_TEXT = (
    (0x800715D4, 36, "3cdd2ad074ad5dfab5f9e5817c06a9188420d3d4a0b70c9ce3ce7a8ab0da67ee"),
    (0x800715F8, 224, "7735b73fbd6b3149290d56bad8eed6be6202d6afbcb315708646b0717bbda4a8"),
    (0x800FF2BC, 80, "6fd537f5d08e83e327f7cf326132bcc5a8e832b48badf8fd0959ceb48ccdea48"),
    (0x800FF418, 188, "4d7718098a024195217efd94548567af5e30aefaa970b02de985bc2566fd59e8"),
    (0x801A0414, 32, "48ea1a85f4775729a8e61ce2bd664be238baaea3338d99ccbd1941193e5a5e2e"),
    (0x801A0434, 36, "81a89286f94c81c0c11cd39e8f825526d363620335dff64925e6b8f8d5ab93d4"),
    (0x801A0458, 152, "2ee1e0ea72c2777df944ed787a8754bd49c0d6ce9ecb70c762baf21901c82a7c"),
    (0x80166E30, 32, "48ea1a85f4775729a8e61ce2bd664be238baaea3338d99ccbd1941193e5a5e2e"),
    (0x80166E50, 84, "de537f259b1ebe8c32b4bf3c3dc3472e8a499cf9548997f6f0041cb08839e2d6"),
    (0x80166FA8, 600, "30f6972ada4411da7d2c0bb74570a9f89eb2b0508ed2aa007f334d18f00ca6f1"),
    (0x80119298, 60, "5701b20e6e73e5f19d2f3f1b80026de429a7a10376190a60a2f1d995b352cc6c"),
    (0x801193EC, 128, "67c37ff2d1d9e60407ef406b8e721525381ca8cd5e93d3d88b1b0a1cfaf832a2"),
)
TABLE_ENTRIES = {
    0x8031F004: 0x800FF418, 0x8043386C: 0x800715F8, 0x80435754: 0x801A0458,
    0x8034E72C: 0x80166FA8, 0x8032B604: 0x801193EC,
}
SOURCE_CALLS = {
    0x800715E4: 0x800FF2BC, 0x801A0420: 0x801A0434, 0x801A0444: 0x800FF2BC,
    0x80166E3C: 0x80166E50, 0x8011942C: 0x80119848,
}
NINJA_BOMBS = {
    0x0000: ((0x80071678, 0),),
    0x0003: ((0x80071690, 1), (0x800716A8, 2)),
    0x000A: ((0x800716C0, 3),),
    0x000C: ((0x801A04D8, 5),),
}
CHARGED_BOMBS = ((0x80167190, 8), (0x8016719C, 9), (0x801671A8, 6), (0x801671B4, 7))
DEBIT = 0x8006DBE0


def require_link(image, site, target):
    insn = word(image, site)
    if insn & 0xFC000003 != 0x48000001 or direct_branch(site, insn) != target:
        raise ValueError(f"small-carrier call link disagrees at {address(site)}")


def row_metadata(image, origins, family, selector):
    base, stride, count = emit.FACTORY_ROWS[family]
    rows = []
    for key in range(count):
        at = base+key*stride
        raw = image.read(at, stride, "data")
        group = emit.signed_byte(raw[selector["selector_offset"]])
        if group >= emit.GROUPS:
            raise ValueError("small-carrier collision group extent")
        if family in origins:
            origin = origins[family][key]
        else:
            offset = worm.FAMILIES[int(family.removeprefix("type-"))][3]
            joint = emit.signed_byte(raw[offset])
            if not 0 <= joint < 32:
                raise ValueError("Worm joint selector outside the reviewed prefix")
            origin = {"key": key, "basis": "credited-actor-joint", "source_joint_selector": joint,
                      "local_point": struct.unpack_from(">3f", raw, 4)}
        row = {**origin, "row": address(at), "collision_group_selector": group}
        if family == "type-49":
            follow = emit.signed_byte(raw[2])
            if follow != 7:
                raise ValueError("type-49 follow-up group mismatch")
            row["followup_group_selector"] = follow
        rows.append(row)
    return rows


def dragon_selector(ident, release, evidence):
    model = int(ident.removeprefix("gg4e08:"), 16)
    key = dragon.DRAGON_KEYS.get(model)
    if (key is None or release["key"] != key or evidence != {"pinned-model-dispatch"}
            or release["site"] != address(dragon.RELEASE) or release["required_model_at_call"] != f"{model:04X}"):
        return None
    return {"key_selector": {"kind": "model-gated-table", "table": address(dragon.JUMP_TABLE),
                             "arm": address(dragon.CASE_TARGETS[key])}}


def supplement(image, archive, atlas, selections, family, rows, sites):
    by_model = {int(row["id"][-4:], 16): row for row in selections["borgs"]}
    static = {int(row["id"][-4:], 16): row for row in atlas["borgs"]}
    direct_sites = {row["site"]: row for row in sites}
    results = []

    def join(model, action, state, sender, entry, calls, scope, *, debit=None, condition=None, stationary=False):
        observed = by_model[model]
        profile = observed["profile"]
        if profile not in static[model]["profiles"] or observed["installer"] != static[model]["installer"]:
            raise ValueError("small-carrier release lacks a selected installer profile")
        source_profile = atlas["profiles"][profile]
        contexts = []
        for group in source_profile["groups"]:
            if group["group"] != action:
                continue
            for row in group["rows"]:
                route = row.get("code_path", {})
                if (row["classification"] != "selector-candidate" or row["state"] != state
                        or route.get("family") != address(sender)):
                    continue
                raw = row["contexts"]
                if stationary:
                    if [0, 0] not in raw or route.get("phase_table") != "8032B5FC":
                        continue
                    raw = [[0, 0]]
                contexts.append({"group": action, "state": state, "substate": row["substate"],
                                 "selector": row["selector"], "context_rows": raw, "scope": scope,
                                 "release_entry": address(entry)})
        if not contexts or (stationary and len(contexts) != 1):
            raise ValueError(f"no exact action carrier context for model {model:04X}")
        installer, table = reaction_installer_tables.model_table(image, model)
        if address(installer) != observed["installer"] or not table or address(table) != static[model]["reaction_table"]:
            raise ValueError("small-carrier release owner or reaction table disagrees")
        unresolved = observed["spec_record"] != 0
        block = None if unresolved else emit.qualified_hit_block(archive, model)
        if debit is not None:
            require_link(image, debit, DEBIT)
        for order, (pc, key) in enumerate(calls, 1):
            row = rows[key]
            if (address(pc) not in direct_sites or word(image, pc-4) != 0x38800000 | key
                    or row.get("required_parent_model", model) != model):
                raise ValueError("small-carrier action key or required parent disagrees")
            require_link(image, pc, FACTORIES[family])
            # The second 0003 bomb depends on a nonzero runtime actor byte.
            # Keep its call and parent evidence without borrowing the normal queue.
            unresolved_queue = ("conditional-actor-state-unqualified" if model == 3 and key == 2 else
                                "alternate-spec-unqualified" if unresolved else None)
            queue = ({"status": unresolved_queue} if unresolved_queue else
                     queue_metadata(image, block, table, row["collision_group_selector"]))
            item = {
                "borg": f"gg4e08:{model:04x}", "profile": profile, "spec_record": observed["spec_record"],
                "site": address(pc), "key": key, "action_candidates": contexts, "queue": queue,
                "call_order_in_reviewed_arm": order, "release_condition": condition,
                "debit_in_reviewed_arm": {"site": address(debit), "channel": 0 if stationary else 2, "cost": 1}
                if debit is not None else None,
            }
            if model == 3 and key == 2:
                item["additional_guard"] = {"actor_byte": "0x7C8", "required": "nonzero"}
            if family == "type-49" and not unresolved_queue:
                item["followup_queue"] = queue_metadata(image, block, table, row["followup_group_selector"])
            results.append(item)

    if family == "type-15":
        for model, calls in NINJA_BOMBS.items():
            other = model == 0x000C
            join(model, "X", 2, 0x801A0414 if other else 0x80071598, 0x801A0458 if other else 0x800715F8,
                 calls, "reviewed-phase-1-cue-and-callback", debit=0x801A04B8 if other else 0x80071658,
                 condition={"phase": 1, "positive_actor_cue": "0x1CEF", "condition_scope": "paid callback only"})
        join(0x0101, "X-charge", 3, 0x80166E30, 0x80166FA8, CHARGED_BOMBS,
             "reviewed-phase-1-charged-arm", condition={"phase": 1, "positive_actor_cue": "0x1CF0",
                                                      "actor_state": 3, "opposite_ordinary_X_state": 2})
    elif family in ("type-49", "type-94"):
        for model, (kind, key, site) in worm.MODELS.items():
            if family != f"type-{kind}":
                continue
            if atlas["profiles"][by_model[model]["profile"]]["config"] not in worm.PROFILES:
                raise ValueError("unqualified Worm profile")
            if rows[key]["source_joint_selector"] != 12:
                raise ValueError("unqualified ordinary B Worm muzzle")
            join(model, "B-shot", 0, 0x80119250, 0x80119848, ((site, key),),
                 "reviewed-stationary-B-phase-2-cue-and-paid-dispatch", debit=0x80119868, stationary=True,
                 condition={"phase": 2, "positive_actor_cue": "0x1CEF", "cue_consumer_call": "8011942C"})
    return results


def recover(image, archive, atlas, selections):
    for kind, spans in (("text", EXTRA_TEXT + worm.TEXT), ("data", worm.DATA)):
        for at, size, digest in spans:
            if sha256(image.read(at, size, kind)).hexdigest() != digest:
                raise ValueError(f"small-carrier signature disagree at {address(at)}")
    for at, entry in TABLE_ENTRIES.items():
        if word(image, at, "data") != entry:
            raise ValueError(f"small-carrier dispatch entry disagree at {address(at)}")
    for site, target in SOURCE_CALLS.items():
        require_link(image, site, target)
    origins = origin_tables(image)
    inventory, candidates, sites, rows = {}, [], [], []
    for family, factory in FACTORIES.items():
        selector = selector_code(image, family)
        factory_rows = row_metadata(image, origins, family, selector)
        counts, callers, attached = candidate_census(
            image, archive, atlas, selections, origins, family, factory, factory_rows,
            dragon_selector if family == "type-47" else None)
        attached.extend(supplement(image, archive, atlas, selections, family, factory_rows, callers))
        if family == "type-15" and any(factory_rows[r["key"]]["required_parent_model"] != int(r["borg"][-4:], 16)
                                       for r in attached):
            raise ValueError("type-15 parent restriction does not match action candidate")
        mapped = {r["site"] for r in attached}
        counts.update(models_with_candidates=len({r["borg"] for r in attached}), joined_call_sites=len(mapped),
                      unjoined_call_sites=len(callers)-len(mapped), candidate_rows=len(attached),
                      joined_keys=len({r["key"] for r in attached}),
                      action_context_sets=sum(len(r["action_candidates"]) for r in attached),
                      queue_statuses=dict(Counter(r["queue"]["status"] for r in attached)))
        inventory[family] = {"factory": address(factory), "selector": selector, "counts": counts,
                             "row_domain": "reviewed prefix" if family in ("type-49", "type-94") else "bounded table"}
        rows.extend({"family": family, **r} for r in factory_rows)
        sites.extend({"family": family, **r} for r in callers)
        candidates.extend({"family": family, **r} for r in attached)
    return {
        "kind": "gf-retail-small-projectile-roster", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "code_atlas_sha256": ATLAS_SHA256,
        "standalone_installers_sha256": INSTALLERS_SHA256, "families": inventory,
        "counts": {"roster_profiles_examined": 206, "factory_rows": len(rows), "direct_call_sites": len(sites),
                   "joined_call_sites": len({r["site"] for r in candidates}), "candidate_rows": len(candidates),
                   "models_with_candidates": len({r["borg"] for r in candidates})},
        "factory_rows": rows, "call_sites": sites, "candidates": candidates,
        "notes": [
            "The selected profiles are from bounded installer execution. Factory candidate contexts and queues are static.",
            "Parent eligibility is a constructor operand, not a proof of an action or successful allocation.",
            "The 0101 type-15 four-call arm belongs to X-charge state 3. The debit at 80167138 is on the opposite state-2 arm.",
            "Queue powers describe direct opening shapes. Bomb areas, Dragon descendants and Worm poison effects are not inferred.",
            "Worm group 7 is a separate follow-up on type 49. Opening zero power does not promote its later power.",
            "This receipt does not change emitted gameplay or establish cue timing, a dynamic source choice, or native damage.",
        ],
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "atlas", "selections", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export_census(args.dol, args.afs, args.atlas, args.selections, args.output, recover), sort_keys=True))
