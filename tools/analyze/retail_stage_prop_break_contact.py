"""Replay Tau's original attack command through stage prop HP, break and recovery."""
import argparse
from hashlib import sha256
import json
from pathlib import Path

from tools.analyze import retail_melee_pose_clock as melee
from tools.analyze import retail_stage_prop_recovery as recovery


prop = recovery.prop
body = recovery.body
RECOVERY_SHA256 = "65007c5b48ea9144c8be213e69afe7bd5c0f3b6edc1db4bddb6540b020167b4d"
TEXT = (
    (0x8002DB58, 944, "4aa097fc70080cd5b6c849d879cd20b39efebb359b80fb70b6a02c486ba38238"),
    (0x8002E2A8, 5672, "3f79bf55474f2fdc4d39b4436d481b713312b1477289db13deb902ffa26c0f1e"),
    (0x8003CD5C, 1512, "7c752eea0cdf04fbe970796f80ffe1bd98ea2ef6edebea6e33cd1d3462355792"),
    (0x8003D344, 164, "f8a7485a7bb1eb37c6a02be76336a2fef43d8a348adbe9670469e6e57d4da06f"),
    (0x8004C6B4, 168, "1d5f88373d984b7b7afb6a80147ec28f2e39ae3061fce32f4b0a47cd47f5a4a3"),
    (0x8008AC80, 400, "88d16df3bda187ed3cb011269a356a7d0646ca1c946dcd194664a5abf4a317d0"),
)
CASES = (("horizontal-miss", 600, 275), ("capsule-retained-token", 0, 275), ("plane-fresh-tokens", 0, 520))


def source_inputs(image, afs):
    recovery.verify_source(image)
    body.verify_text(image, TEXT)
    for site, target in ((0x8004C6F0, 0x8008AC80), (0x8008ACE8, 0x80088AA0), (0x8002DED4, 0x8002E2A8),
                         (0x8002F108, 0x8003CD5C), (0x8002F1D8, 0x8003D344)):
        body.link(image, site, target)
    reviewed = body.read_metadata(body.MAPS / "reaction-melee-map.json", melee.MELEE_SHA256)
    with body.open_regular(afs) as stream:
        stamp = body.archive_stamp(stream)
        body.verify_archive(stream)
        archive = body.RetailArchive(stream)
        tau, tau_info = melee.verify_route(image, archive, reviewed, melee.ROUTES[0])
        if body.archive_stamp(stream) != stamp:
            raise ValueError("stage prop attack archive changed")
    return prop.source_inputs(afs)+tau, tau_info


def decode(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != 30:
        raise ValueError("stage prop break contact report extent")
    pos = 0

    def take(tag, fields, name):
        nonlocal pos
        if lines[pos] != tag + ": " + " ".join(map(str, fields)):
            raise ValueError(f"stage prop break contact changed: {name}/{tag}")
        pos += 1

    cases = []
    for i, (name, dx, y) in enumerate(CASES):
        hp, previous_hp, cursor = 100, 100, 0
        offers = []
        tokens = (1, 1, 2, 3, 4, 5, 6) if i == 2 else (1, 1)
        for visit, token in enumerate(tokens):
            hit = i != 0 and visit != 1
            request = 17 if hit else 0
            before = hp
            hp = max(0, hp-request)
            if hit:
                previous_hp, cursor = before, token
            amount = "80377AD8" if hit else None
            writer = "8002F1D8" if hit else None
            cell = i-1 if hit else None
            take("stage-prop-hit", (i, visit, token, before, int(hit), amount or "00000000", writer or "00000000",
                 request, hp, previous_hp, int(cursor > 0), cursor, 0, int(cursor > 0),
                 255 if cell is None else cell, 1, 1, 0), name)
            offers.append({
                "offer": visit, "attack_token": token, "amount_record": amount, "hp_writer_call_site": writer,
                "accepted_prop_cell": cell, "accepted_prop_descriptor": 236+cell if cell is not None else None,
                "requested_damage": request, "hp_before": before, "hp_after": hp, "hp_removed": before-hp,
                "previous_hp_field": previous_hp, "retained_tokens": list(range(1, cursor+1)),
                "caster_damage_credit": 0, "prop_damage_taken_credit": 0,
                "prop_hit_flag": bool(cursor), "plane_still_published": True, "actor_support_still_latched": True,
            })
        held = None
        if i != 2:
            take("stage-prop-hold", (i, hp, 0, 0, 1, 0), name)
            held = {"hp": hp, "subphase": 0, "break_timer": 0, "plane_published": True, "break_cue_requests": 0}
        cases.append({"case": name, "declared_attack_center_x_offset": dx, "declared_attack_center_y": y,
                      "offers": offers, "positive_hp_bucket_visit": held})

    checkpoints, carry = recovery.decode("\n".join(lines[pos:-1]), from_contact=True)
    pos = len(lines)-1
    take("stage-prop-attack-return", (6, 0, 1, 100), "retained-attack-return")
    return cases, checkpoints, carry


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("stage prop break contact output already exists")
    dol = Path(dol).resolve(strict=True)
    image = body.DolImage(body.read_bounded(dol, 8 << 20), body.SOURCE_SHA256)
    body.read_metadata(body.MAPS / "gg4e08-stage-prop-support.json", recovery.PROP_SHA256)
    prior = body.read_metadata(body.MAPS / "gg4e08-stage-prop-recovery.json", RECOVERY_SHA256)
    payload, tau = source_inputs(image, afs)
    cases, checkpoints, carry = decode(body.fixture_report(fixture, dol, module, ("--break-contact",), payload))
    if checkpoints != prior["checkpoints"][1:] or carry != prior["rising_carry_fork"]:
        raise ValueError("attack-driven and declared prop recovery observations disagree")
    raw = body.encode({
        "kind": "gf-retail-stage-prop-break-contact", "revision": 1, "source_dol_sha256": body.SOURCE_SHA256,
        "source_afs_sha256": body.AFS_SHA256, "stage_prop_recovery_sha256": RECOVERY_SHA256,
        "tau_attack": tau, "prop_owner_bucket": 40, "prop_owner_kind": 22, "prop_hp_supplied": False,
        "ordinary_pair_entry": "8002DB58", "hp_writer": "8003D344", "break_callback": "80081DE4",
        "post_break_observations_match_recovery_receipt": True, "post_break_bucket_visits": 664,
        "break_collision_cells_returned": 2, "break_cue": 115, "rebind_visit_after_break": 600,
        "rise_steps_to_settle": 61, "rising_carry_fork": carry,
        "retained_attack_return": {"last_token": 6, "published_after_close": 0, "cells_returned_on_close": 1,
                                   "prop_hp_after_close": 100},
        "cases": cases,
        "notes": [
            "One original Tau opcode-10 command is offered at declared rigid poses against the constructed stage prop. This does not qualify the full input-to-attack path or an arena tick.",
            "The two overlap heights use different descriptors on one owner. A repeated visit with the same retained shape and token adds no HP write. Fresh commands use the same cell only after original close and return; their tokens and accepted history entries are distinct.",
            "The ordinary amount and HP writer remove 100 HP in six writes without supplying prop HP. The last request is 17 but removes 15. The observed caster dealt/ally and prop taken credit fields stay zero; no Force-pool rules are inferred.",
            "Zero HP leaves the plane and support latch present until later passes. The next prop bucket/shape visit returns the two prop cells and requests cue 115, and the separate collision walk clears active support. All subsequent recovery observations match the independent declared-zero-HP receipt.",
            "The last Tau allocation is retained while only the bounded prop recovery advances. It is separately closed and returned afterward. The attack pose, rendering, audible playback, hardware, native gameplay, and other prop/attack routes are not inferred from this join.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases),
            "contact_offers": sum(len(r["offers"]) for r in cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
