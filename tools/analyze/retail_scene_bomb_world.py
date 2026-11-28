"""Replay Time Bomber's first paid B through the original coordinator and natural expiry."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze import retail_dual_scene_entry as dual
from tools.analyze import retail_type41_bucket as bucket


s = dual.s
DUAL_SHA256 = "7caed283865bedafac11776243ef37f7800aa698e369b78ab87cacaa4a27295b"
BUCKET_SHA256 = "a72911d4e47c272804df9dc76d5e2c17d38dcc2717da4542a5bdaac5db469d05"
TEXT = (
    (0x800E058C, 312, "d1760c991f463afadd23c634578ce59b8a4a3404894b6fc37e83ca143d2cd320"),
    (0x800E075C, 724, "55161c2dab0ed1277642e2f123c8ccb69dd84dc7be712059a2feb2484a32cb7a"),
    (0x800E0DB8, 52, "425fdd6dc72abe7f77b9b981af3c6b976156c65c1f01f2e00a7b9f6a26969d89"),
    (0x800E0E68, 40, "5d0cc6b3acf58489f594ed308c36e6785ba4118e96616314decd7f8097646d65"),
    (0x8000BDA4, 336, "4a1f60baa37fd87e9b9bd776d5fcb4eb01e684984ee553bcbf5ce87b451ff701"),
    (0x80040B94, 464, "d86872d1ddde0d509084f7daad8fed3d8e8e8d2758584cd8cd8c04579478968e"),
    (0x800EFF30, 468, "795964d3f0af7aad82b5209636797f8b0dd96b739eb564853ba7063e7ab2541d"),
    (0x80087F20, 4, "f332ea5b5437103cbb6f1508679da89eec9288ad775c96c439a17fccabe3de8e"),
    (0x80036E24, 1216, "1e95c5b5b01321a06768d975a6776e8d00821b1cb0b8100003d4e9f13cf20d77"),
)
CASES = ("neutral", "no-carrier", "no-shape", "natural-expiry")
ROOTS = {
    1: ("4235AC02", "4310671D", "41216E0C"),
    2: ("4235ACF8", "4310671D", "416FD472"),
    3: ("4235ADE9", "4310671D", "419E509F"),
    178: ("4235C40E", "43106715", "430496E7"),
    179: ("4235C40E", "431066DE", "430496E7"),
    180: ("4235C40E", "43106526", "430496E7"),
}


def decode(report):
    probe = dual.scene.Report(report, 44)
    cases = []
    for delay in (0, 10):
        for i, name in enumerate(CASES):
            paid, has_slot, timed = i > 0, i >= 2, i == 3
            if paid:
                probe.take("dual-world-pay", (delay, i, 9))
            samples = []
            for visit in ((1, 2, 3, 178, 179, 180, 181, 182, 183) if timed else (1, 2, 3)):
                terminal = timed and visit == 181
                if terminal:
                    probe.take("dual-world-terminal", (delay, i, visit, 1, "800E0B4C", 19, 1))
                carrier = has_slot and visit < (183 if timed else 3)
                published = timed and visit <= 180
                phase = (1 if published else 2 if terminal else 3) if timed else (
                    (2 if visit == 1 else 3) if has_slot else 0)
                stock = 9 if (timed and visit <= 181) or (i == 2 and visit == 1) else 10
                remaining = 181-visit if published else 0 if timed else 180 if has_slot else 0
                root = ROOTS[min(visit, 180)] if timed else ROOTS[1] if has_slot else ("00000000",)*3
                call = int((paid and visit == 1) or terminal)
                probe.take("dual-world-bomb", (delay, i, visit, call, stock, int(carrier), phase, int(published),
                                              int(published), 18+int(published), int(timed), 180, 220, remaining, *root))
                samples.append({
                    "visit_from_press": visit, "stock_after_world": stock, "parent_carrier_active": carrier,
                    "parent_phase_field": phase if has_slot else None,
                    "parent_shape_published_both_roles": published, "total_attack_shapes": int(published),
                    "total_ordinary_receivers": 18+int(published), "recorded_parent_token": 1 if timed else None,
                    "hp_by_seat": [180, 220], "parent_lifetime_field": remaining if has_slot else None,
                    "parent_root_bits": list(root) if has_slot else None,
                    "terminal_request": {"key": 19, "caller": "800E0B4C", "area_carrier_allocated": False} if terminal else None,
                })
            cases.append({
                "entry_hold_visits": delay, "case": name, "press_in_entry_visit": 175+delay if paid else None,
                "channel_0_stock_at_factory_entry": 9 if paid else None,
                "carrier_cells_offered": int(has_slot), "shape_cells_offered": int(timed),
                "completed_coordinator_visits": 183 if timed else 3, "actor_damage_ledger_changed": False,
                "stock_return_visit": 182 if timed else 2 if has_slot else 1 if paid else None,
                "carrier_retired_visit": 183 if timed else 3 if has_slot else None, "samples": samples,
            })
    if probe.pos != len(probe.lines):
        raise ValueError("extra world bomb report rows")
    return cases


def verify_source(image):
    dual.verify_source(image)
    s.verify_text(image, (*bucket.TEXT, *TEXT))
    for site, target in ((0x800E05C0, 0x80088AA0), (0x800E09A8, 0x80006FB4), (0x800E09EC, 0x8008AC80),
                         (0x800E0B4C, 0x80085E00), (0x800E0DD8, 0x800E0E68), (0x800E0E7C, 0x8006DE10),
                         (0x800294A4, 0x80089A94), (0x800294F4, 0x8002BB14),
                         (0x800294F8, 0x80036E24), (0x80029530, 0x8000BDA4),
                         (0x80029544, 0x80040B94), (0x80029548, 0x800EFF30)):
        s.link(image, site, target)
    if (image.read(0x80433918, 8, "data") != struct.pack(">2I", 0x80087D2C, 0x80087F20)
            or image.read(0x80316EF8+10*28, 4, "data") != bytes.fromhex("03 08 00 01")
            or image.read(0x80317064+2*20, 20, "data") != bytes.fromhex(
                "0f 00 00 b4 00 01 01 02 40 a0 00 00 bd cc cc cd 00 00 00 13")):
        raise ValueError("world bomb stage, pose or lifetime operands changed")


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("world bomb output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_source(image)
    s.read_metadata(s.MAPS / "gg4e08-dual-scene-entry.json", DUAL_SHA256)
    s.read_metadata(s.MAPS / "gg4e08-type41-bucket.json", BUCKET_SHA256)
    cases = decode(s.fixture_report(fixture, dol, module, ("--world-bomb",), dual.payload(afs)))
    raw = s.encode({
        "kind": "gf-retail-scene-bomb-world", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "dual_scene_entry_sha256": DUAL_SHA256, "type41_bucket_sha256": BUCKET_SHA256,
        "attacker": "gg4e08:0106", "other_actor": "gg4e08:000a", "owner_seat": 1,
        "parent_type": 41, "parent_key": 2, "parent_pose": 10, "parent_bucket": 10,
        "parent_shape_group": 15, "view_record": 3, "stage_dispatch_state": 1, "cases": cases,
        "notes": [
            "The retained first-input inlets come from the two loaded-actor entry prefixes. The collision grid is declared empty and initialized; stage dispatch 1 uses its original no-op handler.",
            "Each B fork checks its one-stock debit at the original factory entry separately from the retained complete coordinator calls.",
            "The retained calls execute the actor pass, live carrier buckets, collision lists, original collision walk and the coordinator tail. The three observed scene counters each advance by one.",
            "A successful parent keeps the normal paying actor and clones view record 3 from its source archive. The shape in both registries is a single original shape, not two independently owned hits.",
            "The tested successful initializer chooses 180 lifetime units. No timer or position is overwritten for expiry; the only kind-3 cell is the parent, so the key-19 request cannot allocate a child.",
            "These complete source callbacks use declared resources and muted audio; the startup prefixes do not replay the entire preceding world. No live arena, original hardware, general damage timing or native gameplay is qualified.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases),
            "coordinator_visits": sum(case["completed_coordinator_visits"] for case in cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
