"""Verify original stage prop break, timed rebind, rise and support reacquisition."""
import argparse
from hashlib import sha256
import json
from pathlib import Path

from tools.analyze import retail_stage_prop_support as prop


body = prop.body
stage = prop.stage
PROP_SHA256 = "2d32bbd79433faf770f5ac9c737c7ca508edee672c764b470e366da7b1d4ed89"
TEXT = (
    (0x80081DE4, 532, "5f6f2e6b59b66e0b1e5a6fcf51f7a2ef38c1467601e63bbb51eac2613ba3b236"),
    (0x80082268, 204, "7d45fc5ee6791ebc1407bcea3868260b579fa1f9ac6866a6e8c4425b25c1316a"),
    (0x8008278C, 52, "c5fa004e0301c064186b40309cdf68803aeac1026031aab00db06cd24dac6fc6"),
    (0x800827C0, 52, "11c63b6ddd3198ae429955626e0dc2f7190bcbfce51693ebaa94df728843fa31"),
    (0x8001BDA8, 288, "01586cb4fa31c8c0f19871852fa315ee27958073bd45cff37c3f73355b129b00"),
    (0x800F0468, 76, "e8114bdc8b81875035d1be868d02a148b48949a44cbc769e9c123e5711ef45f9"),
    (0x800EFB3C, 200, "166caf117dce26eea64c8dcca8c88f971b0926375bcf721e41062a840cf426ba"),
)
MARKS = (
    ("supported-before-break", 0), ("break", 1), ("collision-after-release", 2),
    ("first-wait", 3), ("last-wait", 601), ("collision-rebound", 602), ("first-rise", 603),
    ("second-rise", 604), ("rise-30", 632), ("rise-58", 660), ("rise-59", 661), ("rise-60", 662),
    ("settled", 663), ("held-1", 664), ("held-2", 665), ("held-3", 666),
)


def verify_source(image):
    prop.verify_source(image)
    body.verify_text(image, TEXT)
    for site, target in ((0x80081E68, 0x800827C0), (0x80081EDC, 0x8008AFF0), (0x80081F00, 0x8001BDA8),
                         (0x80081F08, 0x80082268), (0x80081F18, 0x8008278C), (0x80081F48, 0x800F0468),
                         (0x80081F8C, 0x8008AE10)):
        body.link(image, site, target)
    if image.read(0x80437AB8, 8, "data") != bytes.fromhex("42C8000042700000"):
        raise ValueError("stage prop original rise operands changed")


def decode(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != len(MARKS)+1:
        raise ValueError("stage prop recovery report extent")
    bits = stage.walk.stage.pair.capsule.geometry.bits
    offset0 = -stage.f32(100+stage.scalar("444064A4"))
    speed = stage.f32(-offset0/60)
    rise = [offset0]
    for _ in range(64):
        rise.append(min(0, stage.f32(rise[-1]+speed)))
    snapshots = []
    pos = 0
    carry = None
    for label, mark in MARKS:
        dead = 1 <= mark < 602
        delay = max(0, mark-2)
        step = max(0, delay-600)
        rebound = mark >= 602
        timer = 600-delay if dead else 0
        plane = 255 if dead else 0 if rebound else 1
        local_y = rise[step] if rebound else 0
        moving = rebound and local_y < 0
        y = stage.f32(10+local_y)
        plane_y = None if dead else stage.f32(stage.f32(10+offset0)+630) if mark == 603 else stage.f32(y+630)
        flag_281 = int(not moving or step == 0)
        active_support = mark < 2
        previous_support = not active_support
        fields = (mark, 1, int(dead), 0 if dead else 100, timer, 2 if dead else 0, 1 if dead else 3,
                  int(not dead), plane, flag_281, bits(y), bits(local_y), bits(speed if moving else 0),
                  bits(plane_y or 0), "00000001" if dead or moving else "00000000",
                  int(active_support), int(previous_support), 1)
        if lines[pos] != "stage-prop-recovery: " + " ".join(map(str, fields)):
            raise ValueError(f"stage prop recovery changed: {label}")
        pos += 1
        snapshots.append({
            "checkpoint": label, "marker": mark, "bucket_visits_after_break": delay, "prop_phase": 1,
            "prop_subphase": int(dead), "hp": 0 if dead else 100, "timer": timer,
            "shape_free_count": 2 if dead else 0, "ordinary_receiver_entries": 1 if dead else 3,
            "live_plane_cell": None if dead else plane, "owner_flag_281": bool(flag_281),
            "world_y_bits": bits(y), "rise_offset_bits": bits(local_y), "rise_rate_bits": bits(speed if moving else 0),
            "allocated_plane_y_bits": bits(plane_y) if plane_y is not None else None,
            "type4_placement_mask": 1 if dead or moving else 0,
            "actor_active_support": active_support, "actor_previous_support": previous_support,
            "actor_retained_shape_cell": 1,
        })
        if mark == 602:
            fields = ("1", "0", "C456E4A4", "C4534519", "C3659290", "C3659290",
                      "44408000", "C2B3251E", "C29628C8", "C29628C8")
            if lines[pos] != "stage-prop-rise-carry: " + " ".join(fields):
                raise ValueError("stage prop original rising carry changed")
            pos += 1
            carry = {
                "retired_plane_cell": 1, "reused_old_plane_cell_descriptor": 236, "reacquired_plane_cell": 0,
                "owner_y_before_after_bits": list(fields[2:4]), "plane_center_y_before_after_bits": list(fields[4:6]),
                "saved_local_anchor_y_bits": fields[6], "actor_y_before_carry_bits": fields[7],
                "support_query_y_after_first_rise_bits": fields[8], "actor_y_after_carry_bits": fields[9],
                "actor_update_stop_before": "80059E58",
            }
    return snapshots, carry


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("stage prop recovery output already exists")
    dol = Path(dol).resolve(strict=True)
    image = body.DolImage(body.read_bounded(dol, 8 << 20), body.SOURCE_SHA256)
    verify_source(image)
    body.read_metadata(body.MAPS / "gg4e08-stage-prop-support.json", PROP_SHA256)
    rows, carry = decode(body.fixture_report(fixture, dol, module, ("--recovery",), prop.source_inputs(afs)))
    raw = body.encode({
        "kind": "gf-retail-stage-prop-recovery", "revision": 1, "source_dol_sha256": body.SOURCE_SHA256,
        "source_afs_sha256": body.AFS_SHA256, "stage_prop_support_sha256": PROP_SHA256,
        "prop_owner_bucket": 40, "prop_owner_kind": 22, "declared_destruction_hp": 0,
        "destruction_cue": 115, "audible_playback_qualified": False,
        "destruction_handler": "80081DE4", "shape_close_site": "80081EDC", "rebind_site": "80081F8C",
        "bucket_visits_after_break": 664, "rebind_visit_after_break": 600, "rise_steps_to_settle": 61,
        "checkpoints": rows, "rising_carry_fork": carry,
        "notes": [
            "The original prop and the actor's first support contact come from the qualified side construction. Zero HP is a declared trigger, not a damage or attack replay.",
            "The break callback requests its positional cue and releases the original prop collision cells on the same bucket/shape visit. The next original collision pass clears active support while retaining the old address. Empty effect pools and a muted audio inlet keep graphics and playback outside the test.",
            "All 600 waiting visits use the original bucket callback. The edge restores HP and rebinds both shapes. The old plane address now names the capsule, not a retained support plane. The same original owner and registered world callback produce the rise.",
            "All rise updates are observed through the original bucket/world/shape sequence. The world moves on the first rise step while the allocated plane center holds; the next visit updates it. Sixty rises leave a small residual offset, and the sixty-first settles exactly.",
            "In a separate fork on rebind, the actor gets a declared rigid pose and acquires the new plane through original collision. The next automatic rise drives original actor carry and its supported height query, without a host-provided prop motion vector.",
            "No full scene tick, full actor motion, rendered recovery, attack/destruction admission, audible playback, hardware, or native gameplay is established.",
        ],
    }, expanded=("checkpoints",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "checkpoints": len(rows), "bucket_visits_after_break": 664,
            "rise_steps_to_settle": 61}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
