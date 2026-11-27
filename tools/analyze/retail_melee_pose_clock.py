"""Replay two exact opcode-10 melee queues through both original body clocks."""
import argparse
from collections import Counter
from dataclasses import dataclass
from hashlib import sha256
import json
from pathlib import Path
import re

from tools.analyze import reaction_emit, reaction_installer_tables, reaction_melee
from tools.analyze import retail_contact_pose_clock as clock, retail_type32_roster as beam
from tools.analyze import retail_type76_opening as opening
from tools.analyze.gred_animation_controller import decode_program
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import AFS_SHA256, SOURCE_SHA256, address, encode, read_metadata
from tools.analyze.retail_rifle_contact_hp import ACTOR_SHA256
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive, levels
from tools.extract.source_image import DolImage


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"
MELEE_SHA256 = "6ad0e7dbaea6f413f9b78498e7b30c9bed8c676752170647f37318c5c95d6c19"
CLOCK_SHA256 = "169d0ae165d556f6388cc6f4c4abb8f2b8c327107022c0f24eb7991e4cb81c15"
ROW = re.compile(r"melee-clock:(?: [0-9]{1,4}){14}")
TEXT = (*opening.TEXT, (0x8004C6B4, 168, "1d5f88373d984b7b7afb6a80147ec28f2e39ae3061fce32f4b0a47cd47f5a4a3"))
CASES = ("lane-zero-not-admitted", "out-of-range", "all-shapes-overlap", "attacker-holds-longer",
         "receiver-holds-longer", "only-step-5-overlaps", "only-step-6-overlaps")


@dataclass(frozen=True)
class Route:
    model: int
    command: int
    table: int
    group: int
    index: int
    steps: tuple[int, ...]
    joints: tuple[int, ...]
    frame: int
    hp: int
    raw_power: int
    lost: int
    seed: int
    hit_sha: str
    spec_sha: str

    @property
    def amount(self):
        return self.table+24*self.index


ROUTES = (
    Route(0x0807, 0x80377E40, 0x80377AC0, 1, 1, (2,), (1,), 10, 60, 20, 20, 4,
          "8835c40673a51544702ca7770649c92744d7d12105cdc1ab9851e23b212cf12a",
          "6c9b35fad296fc38940fb4f9b163219327556289526d8864406b2f7fd77fc051"),
    Route(0x0001, 0x8031AAD4, 0x8031AF90, 4, 5, (6, 5), (0, 7), 9, 200, 40, 41, 8,
          "a0ca857b3eca287b90c4d32620de88a98264452863a475e1b457263c9711fc6a",
          "d4b8ab770e91620c870de76c5342424db68d5414606a8a4b8cbb3a8f45f2fe02"),
)


def decode(report):
    lines = report.splitlines()
    if len(report) > 2048 or len(lines) != 12:
        raise ValueError("melee clock report extent")
    cases = []
    for r in ROUTES:
        for i, name in enumerate(CASES[:5] if len(r.steps) == 1 else CASES):
            hit = i >= 2
            count = len(r.steps) if i else 0
            selected = (r.steps[0] if i == 6 else r.steps[-1]) if hit else 0
            before = (r.seed+2, 2) if i == 3 else (2, r.seed+2) if i == 4 else (0, 0)
            armed = tuple(max(n, r.seed) for n in before) if hit else before
            slow = tuple(max(0, n-1) for n in armed)
            lost = r.lost if hit else 0
            expected = (r.model, i, count, selected, r.seed, *before, *armed, *slow, lost, 180-lost, hit)
            line = lines[len(cases)]
            if ROW.fullmatch(line) is None or tuple(map(int, line.split()[1:])) != expected:
                raise ValueError(f"melee shape, timer, or HP changed: {r.model:04X}/{name}")
            cases.append({
                "borg": f"gg4e08:{r.model:04x}", "case": name, "published_attack_shapes": count,
                "selected_step": selected or None, "accepted_amount_record": address(r.amount) if hit else None,
                "source_seed": r.seed, "timers_before": list(before), "timers_at_amount_entry": list(armed) if hit else None,
                "timers_after_pair": list(armed), "slow_body_clock_visits": list(slow),
                "hp_lost": lost, "receiver_hp_after": 180-lost, "receiver_token": 1 if hit else None,
                "retained_pair_did_not_rearm_after_expiry": hit,
            })
    return cases


def census(image, reviewed):
    unique = {}
    references = kinds = 0
    for row in reviewed["pilots"].values():
        binding = row["binding"]
        kinds += bool(binding.get("continuation"))
        for contact in binding["contacts"]:
            for step in contact["steps"]:
                address_ = int(binding["table"], 16)+24*step["index"]
                amount = image.read(address_, 24, "data")
                unique[address_] = (amount[8], bool(int.from_bytes(amount[18:20], "big") & 0x0FC0))
                references += 1
    hist = Counter(unique.values())
    if (len(reviewed["pilots"]), kinds, references, hist) != (9, 3, 23, {(0, False): 2, (4, False): 7, (8, False): 1}):
        raise ValueError("reviewed melee seed census changed")
    return {"reviewed_borg_rows": 9, "opening_entries": 6, "continuations": 3, "step_references": references,
            "distinct_amount_records": len(unique), "seed_counts": {seed: count for (seed, _), count in hist.items()},
            "receiver_bypass_records": 0}


def verify_route(image, archive, reviewed, route):
    binding = reviewed["pilots"][f"{route.model:04x}"]["binding"]
    contact, = binding["contacts"]
    request = reaction_melee.request_lane(image, int(binding["request_site"], 16), binding["request_mask"])
    program = decode_program(lambda pc, n: image.read(pc, n, "data"), route.command, max_commands=32, max_span=128)
    raw = image.read(route.command, 4, "data")
    if (request is None or request["lane"] != 0 or request["request_mask"] != 15 or binding.get("continuation") or
        int(contact["selector_address"], 16) != route.command+1 or int(contact["program"], 16) != route.command or
        contact["queue_group"] != route.group or int(binding["table"], 16) != route.table or
        reaction_installer_tables.model_table(image, route.model)[1] != route.table or levels(image, route.model)[0][0] != route.hp or
        raw != bytes((10, route.group, 0, 255)) or
        not (events := reaction_melee.contact_events(program)) or events[0]["selector_address"] != address(route.command+1)):
        raise ValueError("melee clock opening or owner route changed")
    attack, actor = opening.read_normal(archive, route.model, route.hit_sha, route.spec_sha)
    if reaction_emit.read_group(attack, route.group) != [(step, route.index) for step in route.steps]:
        raise ValueError("melee clock group entries changed")
    for step, joint in zip(route.steps, route.joints, strict=True):
        at = reaction_emit.STEPS+reaction_emit.STEP_SIZE*step
        if (attack[at], attack[at+1], int.from_bytes(attack[at+2:at+4], "big"), int.from_bytes(attack[at+6:at+8], "big")) != (
            0, joint, 1, route.frame
        ):
            raise ValueError("melee clock shape payload changed")
    amount = image.read(route.amount, 24, "data")
    if int.from_bytes(amount[:2], "big") != route.raw_power or amount[8] != route.seed or int.from_bytes(amount[18:20], "big") & 0x0FC0:
        raise ValueError("melee clock amount record changed")
    return attack+actor, {
        "borg": f"gg4e08:{route.model:04x}", "opcode_address": address(route.command), "queue_group": route.group,
        "steps": list(route.steps), "matrix_joints": list(route.joints), "declared_pose_frame": route.frame,
        "source_amount_record": address(route.amount), "raw_power": route.raw_power, "owner_level_one_hp": route.hp,
        "source_seed": route.seed, "hit_block_sha256": route.hit_sha, "actor_spec_sha256": route.spec_sha,
    }


def qualified_inputs(image, afs):
    clock.verify_source(image)
    beam.verify_text(image, TEXT)
    beam.link(image, 0x8004C6F0, 0x8008AC80)
    beam.link(image, 0x8008ACE8, 0x80088AA0)
    read_metadata(MAPS / "gg4e08-contact-pose-clock.json", CLOCK_SHA256)
    reviewed = read_metadata(MAPS / "reaction-melee-map.json", MELEE_SHA256)
    sources, scoped = [], census(image, reviewed)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        archive = RetailArchive(stream)
        hit, actor = opening.read_normal(archive, opening.RECEIVER, opening.RECEIVER_BLOCK_SHA256, ACTOR_SHA256[opening.RECEIVER])
        payload = hit+actor
        for route in ROUTES:
            raw, source = verify_route(image, archive, reviewed, route)
            payload += raw
            sources.append(source)
        if stamp != archive_stamp(stream):
            raise ValueError("melee clock source AFS changed during extraction")
    return payload, sources, scoped


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("melee clock output already exists")
    dol, module = (Path(p).resolve(strict=True) for p in (dol, module))
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    payload, sources, scoped = qualified_inputs(image, afs)
    cases = decode(clock.fixture_report(fixture, dol, module, payload=payload))
    data = {
        "kind": "gf-retail-melee-pose-clock", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "reaction_melee_map_sha256": MELEE_SHA256, "projectile_pose_clock_sha256": CLOCK_SHA256,
        "entries": {"opcode_10": "8004C6B4", "attack_shape_bind": "8008AC80", "pair": "8002DB58",
                    "amount": "8003CD5C", "body_clock": "8005A378"},
        "scoped_melee_census": scoped, "sources": sources, "cases": cases,
        "notes": [
            "Exact first opcode bytes, hit blocks and normal stats run with declared joint matrices and pose frame.",
            "The original shape retains the executing actor as both attack object and credit; it is not an actor-owned projectile carrier.",
            "Timers and rate visits are in attacker, receiver order. Existing larger timers stay local to the body that held them.",
            "Equal-overlap priority for G-Red is scoped to the tested placement. Both steps share the same amount row and are individually tested.",
            "The retained pair is repeated after both timers expire, without publishing a new attack. It does not re-arm either timer.",
            "The reviewed census is static. This is not live B input, clip progression, whole-roster behavior, frame scheduling or native gameplay.",
        ],
    }
    raw = encode(data, expanded=("sources", "cases"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases), "accepted": sum(c["receiver_token"] == 1 for c in cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
