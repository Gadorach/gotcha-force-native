"""Verify collision reset and the original separate role-0x80 contact list."""
import argparse
from collections import Counter
from dataclasses import dataclass
from hashlib import sha256
import json
from pathlib import Path
import re
import struct

from tools.analyze import reaction_emit, reaction_installer_tables, retail_contact_pose_clock as clock
from tools.analyze import retail_type32_roster as beam, retail_type76_opening as opening
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import (
    AFS_SHA256, ATLAS_SHA256, INSTALLERS_SHA256, SOURCE_SHA256, address, callers, encode, read_metadata,
)
from tools.analyze.retail_rifle_contact_hp import ACTOR_SHA256
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive, levels
from tools.extract.source_image import DolImage


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"
ADMISSION_SHA256 = "dac326de0ac4633a3e955a42aba28f78b0b903f300f584ad4854d475d40410cf"
GROUP_26_SHA256 = "3b90b8b956dd79eb54568792b69f02885d5c9e51bf1839a7f1c0f092b9e4fc5a"
RESET_ROW = re.compile(r"alternate-reset:(?: [0-9]{1,3}){7}")
PAIR_ROW = re.compile(r"alternate-pair:(?: [0-9]{1,4}){13}")
TEXT = (
    *opening.TEXT,
    (0x8003CC24, 312, "7fcf39fdb005df817b0de481e6860f17a2a8bb8c0d9fc7b82cb11882b2c9b415"),
    (0x80055EE4, 172, "f9d106d64326e759fd9b72a619c6bde22a12cc53c0c1a7da4ae9e8f1d40c0ca2"),
    (0x8002DF08, 704, "63d85aedf020cb22b956652399476a95e802f67787b1d813ca9f2aa30f2e8de4"),
)


@dataclass(frozen=True)
class Route:
    model: int
    table: int
    hp: int
    raw_power: int
    seed: int
    lost: int
    hit_sha256: str
    spec_sha256: str

    @property
    def record(self):
        return self.table+4*24


ROUTES = (
    Route(0x0600, 0x8030A398, 500, 30, 8, 26, "89e4a919e5989aad5b2fb36985d55859b8cfdf9387bec0149265cbf716b9aec3",
          ACTOR_SHA256[0x0600]),
    Route(0x0401, 0x802F5B38, 350, 8, 4, 7, "4f271a265e9f6f141cf64084fab824acd5a88b06daf9b4384204bbf83bb2c349",
          "a2baafb3f6c7d35bf006a87a98a18985694a5e41e2172489cd71852207e7e28a"),
)
CASES = ("reset-default", "late-close-only", "attack-mask-only", "receiver-mask-only", "both-roles-admitted")


def decode(report):
    lines = report.splitlines()
    if len(report) > 2048 or len(lines) != 12:
        raise ValueError("alternate contact report extent")
    resets = []
    for i, slot in enumerate((0, 5)):
        line = lines[i]
        if RESET_ROW.fullmatch(line) is None or tuple(map(int, line.split()[1:])) != (slot, 48, 239, 0, 9, 3, 123):
            raise ValueError("original contact reset defaults changed")
        resets.append({"declared_actor_slot": slot, "rebound_slot_hit_bank": True, "history_entries_cleared": 48,
                       "role_exclusion_mask_after": 239, "late_eligibility_byte_after": 0,
                       "body_timer_preserved": 9, "visual_gate_preserved": 3, "hp_preserved": 123})
    pairs = []
    for route in ROUTES:
        for i, name in enumerate(CASES):
            attack_mask, late, receiver_mask = (239 if i in (0, 2) else 111), int(i == 1), 2 if i == 3 else 0
            attack, receiver = i >= 3, i != 3
            hit = attack and receiver
            lost = route.lost if hit else 0
            timer = route.seed if hit else 0
            line = lines[len(pairs)+2]
            expected = (route.model, i, attack_mask, late, receiver_mask, 0, attack, receiver, 1, hit, lost, 180-lost, timer)
            if PAIR_ROW.fullmatch(line) is None or tuple(map(int, line.split()[1:])) != expected:
                raise ValueError(f"alternate contact publication or amount changed: {route.model:04X}/{name}")
            pairs.append({
                "borg": f"gg4e08:{route.model:04x}", "case": name,
                "attacker_role_mask": attack_mask, "late_eligibility_byte": late, "receiver_role_mask": receiver_mask,
                "ordinary_attack_count": 0, "alternate_attack_count": int(attack), "alternate_receiver_count": int(receiver),
                "retained_owner_token": 1, "ordinary_pair_amount_calls": 0, "alternate_pair_amount_calls": int(hit),
                "accepted_amount_record": address(route.record) if hit else None, "hp_lost": lost, "receiver_hp_after": 180-lost,
                "body_timers_after_pair": [timer, timer], "retained_pair_refused": hit,
            })
    return resets, pairs


def census(archive, installed):
    listed, referenced = [], 0
    for row in installed["borgs"]:
        model = int(row["id"].split(":")[1], 16)
        hit = reaction_emit.qualified_hit_block(archive, model)
        groups = {}
        for group in range(32):
            for step, _ in reaction_emit.read_group(hit, group):
                groups.setdefault(step, []).append(group)
        referenced += len(groups)
        for step, uses in sorted(groups.items()):
            at = reaction_emit.STEPS+step*reaction_emit.STEP_SIZE
            raw = hit[at:at+80]
            mask = struct.unpack_from(">H", raw, 2)[0]
            if not mask & 0x80:
                continue
            if mask != 0x80 or len(uses) != 1:
                raise ValueError("alternate contact shape role or group cardinality changed")
            listed.append({"borg": row["id"], "group": uses[0], "step": step, "role_mask": mask,
                           "shape_sha256": sha256(raw).hexdigest()})
    groups = Counter(r["group"] for r in listed)
    if (len(installed["borgs"]), referenced, len(listed), len({r["borg"] for r in listed}),
        len({r["shape_sha256"] for r in listed}), groups) != (206, 4397, 60, 55, 11, {3: 9, 4: 5, 26: 46}):
        raise ValueError("alternate contact indexed source coverage changed")
    if any((r["step"], r["shape_sha256"]) != (44, GROUP_26_SHA256) for r in listed if r["group"] == 26):
        raise ValueError("alternate contact group-26 descriptor cohort changed")
    return listed, {"normal_hit_blocks": 206, "queue_referenced_descriptors": referenced, "alternate_descriptors": 60,
                    "alternate_borgs": 55, "distinct_alternate_shape_bytes": 11, "groups": dict(groups),
                    "identical_group_26_descriptors": 46}


def qualified_inputs(image, afs, installed):
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        archive = RetailArchive(stream)
        hit, body = opening.read_normal(archive, opening.RECEIVER, opening.RECEIVER_BLOCK_SHA256, ACTOR_SHA256[opening.RECEIVER])
        if struct.unpack_from(">H", hit, 32+9*80+2)[0] != 2 or levels(image, opening.RECEIVER)[0][0] != 180:
            raise ValueError("alternate contact receiving shape changed")
        payload = hit+body
        sources = []
        for r in ROUTES:
            hit, owner = opening.read_normal(archive, r.model, r.hit_sha256, r.spec_sha256)
            raw = image.read(r.record, 24, "data")
            if (reaction_emit.read_group(hit, 26) != [(44, 4)] or
                sha256(hit[0xAA0+44*80:0xAA0+45*80]).hexdigest() != GROUP_26_SHA256 or
                reaction_installer_tables.model_table(image, r.model)[1] != r.table or levels(image, r.model)[0][0] != r.hp or
                int.from_bytes(raw[:2], "big") != r.raw_power or raw[8] != r.seed):
                raise ValueError("alternate contact exact route changed")
            payload += hit+owner
            sources.append({"borg": f"gg4e08:{r.model:04x}", "group": 26, "step": 44, "amount_record": address(r.record),
                            "role_mask": 128, "radius": 10, "source_base": r.raw_power, "source_timer_seed": r.seed,
                            "level_one_hp": r.hp, "hit_block_sha256": r.hit_sha256, "actor_spec_sha256": r.spec_sha256})
        rows, stats = census(archive, installed)
        if archive_stamp(stream) != stamp:
            raise ValueError("alternate contact source archive changed")
    return payload, sources, rows, stats


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("alternate contact output already exists")
    dol, module = (Path(p).resolve(strict=True) for p in (dol, module))
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    beam.verify_text(image, TEXT)
    for site, target in ((0x8002C4C8, 0x8002DF08), (0x8002E084, 0x8002E2A8), (0x8002E194, 0x8002E2A8)):
        beam.link(image, site, target)
    read_metadata(MAPS / "gg4e08-actor-frame-admission.json", ADMISSION_SHA256)
    atlas = read_metadata(MAPS / "gg4e08-code-atlas.json", ATLAS_SHA256)
    sites = callers(image, atlas, 0x8003CC24, 128)
    if sorted(sites) != [0x80054444, 0x80057CA8, 0x80058314]:
        raise ValueError("collision reset caller census changed")
    payload, sources, rows, stats = qualified_inputs(image, afs, read_metadata(
        MAPS / "gg4e08-standalone-installers.json", INSTALLERS_SHA256))
    resets, pairs = decode(clock.fixture_report(fixture, dol, module, payload=payload))
    data = {
        "kind": "gf-retail-alternate-contact", "revision": 1, "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "code_atlas_sha256": ATLAS_SHA256, "standalone_installers_sha256": INSTALLERS_SHA256,
        "actor_frame_admission_sha256": ADMISSION_SHA256,
        "entries": {"collision_reset": "8003CC24", "alternate_publish": "8008A5A4", "alternate_pass": "8002DF08",
                    "ordinary_pair": "8002DB58", "direct_queue_bind": "8008AC80"},
        "reset_calls": list(sites.values()), "census": stats, "alternate_shape_rows": rows, "sources": sources,
        "reset_cases": resets, "pair_cases": pairs,
        "notes": [
            "Reset probes run before any shape is allocated. They do not claim that resetting an active pair is safe or replay any outer caller.",
            "The normal shape census reads group references and original descriptor bytes. It does not identify controller commands or infer active route selection.",
            "Machine Red and Drill Robot use the same exact group-26 shape bytes in the direct bind, not necessarily the same amount record or live action.",
            "Either the role-0x80 exclusion mask or the late outgoing admission byte suppresses alternate attack publication. The receiver's separate role-0x02 mask removes the target.",
            "The ordinary pair pass does not consume these shapes. The alternate pass does, after both lists admit them, and refuses the same retained token.",
            "The role masks are declared for the controls. No live guard, group-26 trigger, outer reset callers, full scene frame or native behavior is qualified.",
        ],
    }
    raw = encode(data, expanded=("reset_calls", "alternate_shape_rows", "reset_cases", "pair_cases"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "reset_cases": len(resets), "pair_cases": len(pairs),
            "alternate_descriptors": len(rows)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
