"""Qualify stock ally admission by collision row, not by move name or carrier."""
import argparse
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze import reaction_attack_table, reaction_emit, reaction_installer_tables
from tools.analyze import retail_program_cues, retail_rifle_contact_hp as rifle, retail_type0_contact as type0
from tools.analyze import retail_worm_contact as worm, retail_world_team_pair as tau
from tools.analyze.gred_animation_controller import decode_program


s = tau.s
PRIOR = {
    "gg4e08-world-team-pair.json": "96611e18632969a3aa576a09c9b5b31e1af69b0bfbd2667c379a8730e06194f3",
    "gg4e08-worm-contact.json": "0151076d6f77f2b4aea53f65580ce6a7d10891b6605493f2505caebe9d415632",
    "gg4e08-type0-contact.json": "062a675dcf556775f610789cade7069fd0a355a9eab7828891c5fec0e0d3d337",
    "gg4e08-rifle-next-receiver.json": "6ebd399d481675d5f367d719dda04a3aea3c401a96f655c6845072c7ef5f35de",
    "gg4e08-rifle-contact-hp.json": "a027cff5da32df60f4f66d6c6b1dcb9168228a6304fe87129c273c7624fe748c",
    "gg4e08-program-cues.json": "fa2a0b4b706f64148184f67b3bf8e5e56974a949844f1a39e47786d7124993f8",
    "gg4e08-standalone-installers.json": retail_program_cues.INSTALLER_MAP_SHA256,
    "gg4e08-type0-roster.json": "14aa6bc3d10c9edfb7d11414a53c674dccb769e762a23ceb7c8d9718da4f8475",
    "gg4e08-type2-roster.json": "060278740ca523ef6eede5e34cf2ea0db1bce28e61ecd8f291d529f0888e47ff",
}


def verify_text(image):
    for name, spans in (("text", worm.worm.TEXT+tuple((at, *row) for at, row in
                         {**rifle.SLICES, **worm.REENTRY_SLICES}.items())+(
                             (0x8002FD7C, 832, "88d85748f4401cc2a4be18a7c321b2051d29f8e8d284525a611aa91cefa71247"),
                             (0x80030500, 216, "7ac306d984c3e0161de2c58373ddafab7c4603b0cd4a0da50d522f84cf285169"),
                         )), ("data", worm.worm.DATA)):
        for at, n, digest in spans:
            if sha256(image.read(at, n, name)).hexdigest() != digest:
                raise ValueError(f"relationship source span changed at {at:08X}")
    s.link(image, 0x8002DC58, 0x8002FD7C)
    s.link(image, 0x8002DD8C, 0x80030490)


def flag_counts(counts):
    return {f"{key:04X}": value for key, value in sorted(counts.items())}


def census(image, archive, prior):
    cues = prior["gg4e08-program-cues.json"]
    declared = {r["id"]: r for r in prior["gg4e08-standalone-installers.json"]["borgs"]}
    if len(cues["programs"]) != 3798 or len(declared) != 206 or len(cues["models"]) != 206:
        raise ValueError("relationship census inlet extent")
    programs = {}
    for key, indexed in cues["programs"].items():
        if indexed["status"] == "refused-program-extent":
            continue
        d = decode_program(lambda at, n: image.read(at, n, "data"), int(key, 16))
        if d["status"] != indexed["status"] or len(d["commands"]) != indexed["commands"]:
            raise ValueError("indexed program prefix changed")
        programs[key] = {(c["address"], bytes.fromhex(c["raw"])[1]) for c in d["commands"]
                         if c["opcode"] == 10 and 0 < bytes.fromhex(c["raw"])[1] < 128}
    events_seen, refused, flags = 0, 0, Counter()
    for row in cues["models"]:
        ident = row["id"]
        if declared[ident]["spec_record"]:
            continue
        model = int(ident.removeprefix("gg4e08:"), 16)
        _, table = reaction_installer_tables.model_table(image, model)
        if not table or declared[ident]["profile"] != row["profile"]:
            raise ValueError("direct command census changed its owner")
        roots = [cues["banks"][row[k]] for k in ("local_bank", "common_bank")]
        events = set().union(*(programs.get(p, set()) for b in roots for g in b.get("groups", ())
                               for p in g.get("choices", ())))
        if not events:
            continue
        block = reaction_emit.qualified_hit_block(archive, model)
        events_seen += len(events)
        for _, group in events:
            steps = reaction_emit.resolve_group(image, block, table, group)
            if steps is None:
                refused += 1
            else:
                flags.update(s["flags"] & 65535 for s in steps)
    if events_seen != 1244 or flags != {0: 10, 1: 2046, 3: 15, 33: 48, 257: 4}:
        raise ValueError("direct command relationship counts changed")
    result = {"indexed_programs": len(cues["programs"]), "normal_spec_zero_owners": sum(
                  not row["spec_record"] for row in declared.values()), "owner_command_edges": events_seen,
              "edges_without_qualified_steps": refused, "direct_flags_by_step_edge": flag_counts(flags)}
    for family, expected in (("type0", {0: 54, 1: 1}), ("type2", {0: 15, 1: 21})):
        f = Counter()
        for r in prior[f"gg4e08-{family}-roster.json"]["candidates"]:
            if r.get("spec_record", 0):
                continue
            _, table = reaction_installer_tables.model_table(image, int(r["borg"].removeprefix("gg4e08:"), 16))
            for step in r.get("queue", {}).get("steps", ()):
                f[reaction_attack_table.entry(image, table, step["reaction_index"])["flags"] & 65535] += 1
        if f != expected:
            raise ValueError(f"{family} candidate relationship counts changed")
        result[f"{family}_flags_by_qualified_step_edge"] = flag_counts(f)
    return result


def decode(text):
    if len(text) > 8192 or len(text.splitlines()) != 34:
        raise ValueError("contact relationship report extent")
    lines = iter(text.splitlines())

    def take(line):
        if next(lines) != line:
            raise ValueError(f"contact relationship observation changed: {line}")

    def relation(model, key, step, record, flags, token, amounts, lifecycle=None):
        cases = []
        for team, amount in zip((1, 0), amounts, strict=True):
            accepted = int(team == 1 or not flags & 0x80)
            token1, token2 = int(accepted or token == 2), 2 if token == 2 and accepted else 0
            opposing, allied = (amount, 0) if team else (0, amount)
            take(f"relation: {model:04X} {key} {step} {record:08X} {flags:04X} {token} {team} 1 "
                 f"{accepted} {accepted} {accepted} {amount} {180-amount} {token1} {token2} {opposing} {allied}")
            event = {"receiver_relation": "opponent" if team else "ally", "broad_overlap": True,
                     "relationship_admitted": bool(accepted), "narrow_attempts": accepted, "hp_writer_reached": bool(accepted),
                     "requested_and_removed_hp": amount, "receiver_hp_after": 180-amount,
                     "receiver_history_first_two_slots": [token1, token2],
                     "opponent_credit": opposing, "ally_credit": allied}
            if lifecycle:
                take(f"relation-life: {model:04X} {team} "+" ".join(map(str, lifecycle)))
                phase, stop, count, free, stock = lifecycle
                event["first_carrier_followup"] = {"phase": phase, "stopped": bool(stop), "attack_shapes": count,
                                                   "free_shape_slots": free, "stable_post_contact_stock": stock}
            cases.append(event)
        return {"model": f"{model:04X}", "key": key, "hit_descriptor": step, "attack_rule": f"{record:08X}",
                "attack_rule_flags": f"{flags:04X}", "carrier_token": token, "cases": cases}

    take(rifle.RELEASE)
    take(rifle.PUBLISHED)
    ordinary = [relation(0x0600, 21, 15, rifle.RECORD, 0, 1, (8, 2), (1, 0, 1, 0, 1))]
    for p, flags in zip(type0.PILOTS, (0, 0, 1), strict=True):
        ordinary.append(relation(p.model, p.key, p.step, p.record, flags, 1, (17, 4), (2, 1, 0, 1, 0)))

    tether = []
    for model, key, opening, radius in ((0x0501, 0, 0x8032BA58, 15), (0x050D, 1, 0x8032BB18, 7)):
        before = relation(model, key, 9, opening, 0, 1, (0, 0))
        take(f"worm-release: {model} 49 {key} 3 1 1 1 1 {opening:08X} 16 1 0 180")
        take(f"worm-contact: {model} 3 previous=180 ledger=0/0 cursor=1 radius={radius:.5f}")
        take(f"worm-rebind: {model} 1 1 1 1 10 1 0 1 1")
        take(f"worm-rebind: {model} 2 1 2 0 10 1 1 2 1")
        after = relation(model, key, 10, opening+24, 0xA1, 2, (1, 0))
        tether.append({"opening": before, "followup": after, "retained_carrier_on_visits": [1, 2],
                       "opening_shape_return_visit": 1, "followup_shape_rebind_visit": 2,
                       "followup_paid_again": False})
    return ordinary, tether


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("contact relationship output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_text(image)
    prior = {name: s.read_metadata(s.MAPS/name, digest) for name, digest in PRIOR.items()}
    with s.open_regular(afs) as stream:
        stamp = s.archive_stamp(stream)
        s.verify_archive(stream)
        archive = s.RetailArchive(stream)
        scope = census(image, archive, prior)
        type0_inlet = list(type0.read_normal(archive, type0.RECEIVER, type0.RECEIVER_BLOCK_SHA256,
                                             type0.ACTOR_SHA256[type0.RECEIVER]))
        rows = type0.carrier_rows(image, type0.origin_tables(image)["type-0"])
        for p in type0.PILOTS:
            block, spec = type0.read_normal(archive, p.model, p.block_digest, p.spec_digest)
            type0.qualify_pilot(image, p, block, rows)
            type0_inlet.extend((block, spec))
        worm_inlet, sources = worm.qualified_sources(image, archive)
        if stamp != s.archive_stamp(stream):
            raise ValueError("relationship source archive changed")
    traces = [s.fixture_report(fixture, dol, module, (f"--{mode}-relationship",), payload)
              for mode, payload in (("rifle", b"".join(rifle.load_contact_inputs(afs))),
                                    ("type0", b"".join(type0_inlet)), ("worm", worm_inlet))]
    ordinary, tether = decode("".join(traces))
    for case, source in zip(tether, sources[::2], strict=True):
        for phase in ("opening", "followup"):
            case[phase]["collision_group"] = source[phase]["group"]
            case[phase]["raw_power"] = source[phase]["raw_power"]
    raw = s.encode({
        "kind": "gf-retail-contact-relationship", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "prior_receipts": PRIOR, "static_census": scope,
        "source_trace_sha256": sha256("".join(traces).encode("ascii")).hexdigest(),
        "bounded_ordinary_projectiles": ordinary, "bounded_type49": tether,
        "native_policy_changed": False,
        "notes": [
            "The stock row, carrier, credited owner and geometry are the same within each pair of relationship cases. The fixture changes only the declared receiver team after publication.",
            "The type-49 follow-up uses the republished group 7 under a fresh token, from a retained opposing opening. The ally refusal is a disposable pair fork, not a full ally tether lifecycle.",
            "Only the original HP leaf returns on positive pairs. Carrier follow-up visits are isolated, not full scene updates. No late tether, poison, KO, or complete ordinary moveset is qualified.",
            "The static counts are descriptor edges in indexed command prefixes and qualified projectile candidates, not live, unique, or exhaustive moves. A stock exclusion row beyond this census is verified in type 49.",
        ],
    }, expanded=("bounded_ordinary_projectiles", "bounded_type49"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "original_observations": sum(len(t.splitlines()) for t in traces)}


if __name__ == "__main__":
    cli = argparse.ArgumentParser(description=__doc__)
    for arg in ("dol", "afs", "fixture", "module", "output"):
        cli.add_argument(arg, type=Path)
    args = cli.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
