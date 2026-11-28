"""Replay receiver-list reopening after the original Nurse contact."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze import retail_replacement_contact as contact


s = contact.s
CONTACT_SHA256 = "6128b8453d8c77685cad65e99c81e143cb303394843f25d07f2961f279f63903"
CASES = (
    (81, 0, 58, None), (81, 1, 0, "clear-guard-before-update"),
    (138, 0, 1, None), (139, 0, 0, None), (139, 1, 1, "declare-one-unit-guard"),
)
SAMPLES = (0, 1, 14, 15, 57, 58, 64)
ONE, SLOW = "3F800000", "3CF5C28F"


def decode(report):
    lines = report.splitlines()
    if len(report) > 8192 or len(lines) != 80:
        raise ValueError("normal replacement heal tail report extent")
    rows = []
    pos = 0

    def take(tag, fields):
        nonlocal pos
        if lines[pos] != tag + ": " + " ".join(map(str, fields)):
            raise ValueError(f"normal replacement heal tail changed at row {pos+1}: {tag}")
        pos += 1

    for earlier in (6, 7):
        for visit, rule, guard, changed in CASES:
            mask_at_factory = "0000" if guard == 1 else "0043"
            take("heal-tail-b", (earlier, visit, rule, 1, mask_at_factory, 9))
            samples = []
            for follow in SAMPLES:
                left = max(0, guard-follow)
                pose = max(0, 15-follow)
                opened = bool(guard and not left)
                mask = "0000" if opened else "0043"
                receive = "3EF0" if opened else "0000"
                secondary = SLOW if follow and pose else ONE
                take("heal-tail", (earlier, visit, rule, follow, mask, struct.pack(">f", left).hex().upper(),
                                   pose, ONE, secondary, receive, receive, int(bool(follow))))
                samples.append({
                    "actor_visits_after_heal": follow, "ordinary_guard": left, "role_mask": mask, "pose_timer": pose,
                    "raw_rate_1dcc_bits": ONE, "secondary_rate_bits": secondary,
                    "ordinary_and_alternate_receiving_counts": [9, 9] if opened else [0, 0],
                    "processed_pad_seen": follow > 0, "hp": 160, "retained_accept_token": 3,
                })
            rows.append({
                "previous_ko_source": f"gg4e08:010{earlier}", "heal_on_replacement_visit": visit,
                "declared_control": changed, "original_guard_at_heal": 139-visit,
                "guard_at_start_of_followup": guard, "role_mask_at_start_of_followup": "0043",
                "role_mask_cleared_after_actor_visits": guard or None, "neutral_actor_visits": 64,
                "fresh_b_fork": {"after_heal_actor_visit": 1, "reader_calls": 1, "role_mask_at_factory": mask_at_factory,
                                 "stock_at_factory": 9, "key": 2, "pose": 10, "factory_entry_only": True},
                "samples": samples,
            })
    return rows


def verify_source(image):
    contact.verify_source(image)
    s.verify_text(image, (
        (0x8010E158, 336, "630ae855e62c4bfed07ba235fd131082388627d9e0693f77e06046235a17da81"),
        (0x8005A378, 520, "b6a3dfe9d8bf1b34fb4d470444385f1bbdafe4ba0596786d67699ca9e92ecd2f"),
    ))
    s.link(image, 0x8010E19C, 0x80088AA0)
    s.link(image, 0x80059118, 0x8005A378)
    if (image.read(0x80325C00, 8, "data") != bytes.fromhex("0101000080325BD0")
            or image.read(0x80055BE8, 0x38, "text") != bytes.fromhex(
                "c05f0720c0228830fc0208404081002cc01f1dccec020028d01f0720c01f0720fc000840"
                "4c40138240820010d03f072038000000b01f0272"
            )):
        raise ValueError("normal replacement heal tail effect or guard operands changed")


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("normal replacement heal tail output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_source(image)
    s.read_metadata(s.MAPS / "gg4e08-replacement-contact.json", CONTACT_SHA256)
    payload, _ = contact.source_inputs(image, afs)
    rows = decode(s.fixture_report(fixture, dol, module, ("--replacement-heal-tail",), payload))
    raw = s.encode({
        "kind": "gf-retail-replacement-heal-tail", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "replacement_contact_sha256": CONTACT_SHA256,
        "receiver": "gg4e08:0106", "probe_borg": "gg4e08:0900", "probe_relation": "opponent",
        "nurse_request_hp": -50, "receiver_hp_after": 160, "effect_request": {
            "factory": "8010E158", "caller": "8002F6DC", "selector": 1, "free_kind2_cells": 0,
            "carrier_allocated": False,
        }, "cases": rows,
        "notes": [
            "These are follow-up visits of the original normal replacement actor under declared scene state 4, cut before presentation and followed by the original registry rebuild.",
            "The source Nurse HP caller heals a declared 110 HP inlet to 160, requests the type-21 effect, then writes the role mask. No kind-2 cell is provided; the admitted effect lifecycle is outside this receipt.",
            "On the ordinary guard's positive-to-zero edge the original body prepass clears the role mask. The guard uses raw +0x1DCC, not the secondary rate slowed by the separate 15-visit pose timer.",
            "Zeroing the guard ahead of the prepass bypasses that edge. An already-zero guard leaves the Nurse mask set in the tested neutral window; the one-unit declared control restores the edge without changing the pose timer.",
            "Fresh B forks reach the original paid factory while the role mask is still closed, but are discarded before allocation. No continuing battle input, effect admission, hardware or native gameplay is qualified.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "followup_cases": len(rows),
            "completed_actor_prefixes": sum(row["neutral_actor_visits"] for row in rows)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
