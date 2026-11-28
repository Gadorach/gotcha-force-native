"""Verify the admitted selector-1 heal effect against the replacement guard."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze import retail_contact_effect_source_art as art
from tools.analyze import retail_replacement_heal_tail as tail


s = tail.s
TAIL_SHA256 = "7f844cb98a66bf47d1ae5e7123db6b80adf048fba0d4dabdeacb6c3987e8548c"
ART_SHA256 = "025c745717b4c40844049721831a40d402bc82ad31d07fddc451187efb8eb86e"
TEXT = (
    (0x8010E2A8, 60, "3c84b4a7da1a71aa8f41e1ea6eb44f72a0e38abb343e2a2fd405b36e461ef940"),
    (0x8010E2E4, 492, "dcf49ab3346990eae1e335ce86ddb68ec3ca55ddb527524de1a3b8bd2de13053"),
    (0x8010E4D0, 208, "46b97d1fb61d1cfc1577cef53c2049935cde0f88f92f3917a8ddf327d17df104"),
    (0x8010E5A0, 68, "2f25935d4e6c25d2efdaf330ec3cdc0dd36f0e9e3e93c1f0b514977cefb7bdec"),
    (0x8010E5E4, 72, "fb0081142c2269bfa68762ffa42586230ddf56150a09a7b86798da21ea975b3f"),
    (0x8010EA30, 620, "dc904f8d22fdce6093bc1a7b3a8cbfe6762255ea71cd1b9cb39c85e7ccf2ba9a"),
    (0x8010EC9C, 492, "496c877b76a13b69b2af99fc51862c495ac8c50e8f794ef60d8cfc3905e1e854"),
    (0x80006DC8, 116, "a9d42d4b031609acdc4056ee2c21d561240c52207e9ba085f640a656a1129282"),
    (0x80007198, 212, "c5285fd127a1f512f1c162fa638dbea07019a652b42c8a1e275789d237d94c61"),
    (0x80089898, 228, "126a25b8262b9ec16da7e6121fd72318cce3c39faab481f718d2a559307c1299"),
)
SAMPLES = (0, 1, 15, 16, 17, 31, 32, 33, 57, 58, 64)


def decode(report):
    lines = report.splitlines()
    if len(report) > 16384 or len(lines) != 12*len(SAMPLES):
        raise ValueError("normal replacement heal effect report extent")
    pos = 0
    cases = []
    for earlier in (6, 7):
        for heal_visit in (81, 138, 139):
            initial_guard = 139-heal_visit
            for rule in (0, 1):
                samples = []
                for visit in SAMPLES:
                    released = rule == 1 and visit >= 17
                    retired = rule == 1 and visit >= 33
                    phase = 0 if not visit else 1 if not released else 2 if visit < 32 else 3
                    owner = 0 if released else 2
                    guard = max(0, initial_guard-visit)
                    opened = bool(initial_guard and not guard)
                    role = "0000" if opened else "0043"
                    receive = "3EF0" if opened else "0000"
                    pose = max(0, 15-visit)
                    payload = bool(visit and not retired)
                    fields = (earlier, heal_visit, rule, visit, role, struct.pack(">f", guard).hex().upper(),
                              pose, int(not retired), phase, owner, int(retired), int(retired), int(payload),
                              receive, receive, int(bool(visit)))
                    if lines[pos] != "heal-effect: " + " ".join(map(str, fields)):
                        raise ValueError(f"normal replacement heal effect changed at {earlier}/{heal_visit}/{rule}/{visit}")
                    pos += 1
                    samples.append({
                        "visit_after_heal": visit, "receiver_role_mask": role, "ordinary_guard": guard, "pose_timer": pose,
                        "effect_active": not retired, "effect_phase_field": phase, "owner_8c4": owner,
                        "heap_header_claimed": payload, "free_kind2_and_view_cells": [int(retired), int(retired)],
                        "receiver_counts_ordinary_and_alternate": [9, 9] if opened else [0, 0],
                    })
                cases.append({
                    "previous_ko_source": f"gg4e08:010{earlier}", "heal_on_replacement_visit": heal_visit,
                    "declared_owner_bit_clear_visit": 17 if rule else None, "active_bucket": 26,
                    "collected_common_model_record": 77, "heap_payload_bytes": 32*92,
                    "actor_and_effect_bucket_visits": 64, "effect_retired_visit": 33 if rule else None,
                    "body_receivers_reopened_visit": initial_guard or None, "samples": samples,
                })
    return cases


def verify_source(image):
    tail.verify_source(image)
    art.verify_source(image)
    s.verify_text(image, TEXT)
    for site, target in ((0x80089ABC, 0x80089898), (0x8010E3B0, 0x80006DC8), (0x8010E40C, 0x80007198),
                         (0x8010E4EC, 0x8010EC9C), (0x8010E5B4, 0x8010EC9C), (0x8010E604, 0x80197AD8),
                         (0x8010E614, 0x80088E50)):
        s.link(image, site, target)
    if (image.read(0x80325C10, 16, "data") != struct.pack(">4I", 0x8010E2E4, 0x8010E4D0, 0x8010E5A0, 0x8010E5E4)
            or image.read(0x80325BD0, 20, "data") != bytes.fromhex("004d0020004400234120000041cc000042aa0000")):
        raise ValueError("normal replacement heal effect phase table or model operands changed")


def payload(image, afs):
    s.read_metadata(s.MAPS / "gg4e08-replacement-heal-tail.json", TAIL_SHA256)
    s.read_metadata(s.MAPS / "gg4e08-contact-effect-source-art.json", ART_SHA256)
    base, _ = tail.contact.source_inputs(image, afs)
    view = next(raw for slot, raw in art.source_blocks(afs) if slot == 1)
    return base + view


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("normal replacement heal effect output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_source(image)
    cases = decode(s.fixture_report(fixture, dol, module, ("--replacement-heal-effect",), payload(image, afs)))
    raw = s.encode({
        "kind": "gf-retail-replacement-heal-effect", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "replacement_heal_tail_sha256": TAIL_SHA256,
        "contact_effect_source_art_sha256": ART_SHA256,
        "receiver": "gg4e08:0106", "nurse_request_hp": -50, "receiver_hp_after": 160,
        "common_view": {"member": art.art.EFFECT_MEMBER, "block": 1, "sha256": art.art.EFFECT_BLOCKS[1][1]},
        "effect_type": 21, "selector": 1, "owner_seat": 1, "cases": cases,
        "notes": [
            "The original Nurse pair writes HP and requests its effect, which allocates a kind-2 carrier and one view wrapper. The first bucket visit allocates type-3 heap storage and loads exact common model record 77.",
            "The original neutral replacement actor prefix is followed by its effect bucket and a list rebuild. This is not a full scene update or render path.",
            "Receiver publication follows the same ordinary-guard expiry as the no-effect replay. The live type-21 carrier and its owner bit do not prevent the guard from reopening both lists.",
            "The owner-bit-clear branch is a declared control, not a recovered input or natural bit writer. It stops new emissions and returns the exact heap payload, header, view wrapper and carrier without clearing the receiving role mask.",
            "Actor and common-archive owners stay distinct. No GX draw, audible playback, indefinite behavior, or native gameplay is qualified.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases),
            "actor_and_effect_bucket_visits": sum(row["actor_and_effect_bucket_visits"] for row in cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
