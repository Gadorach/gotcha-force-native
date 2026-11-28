"""Join an ordinary loaded Tau KO to archive handoff and replacement readiness."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
from subprocess import run

from tools.analyze import retail_body_replacement as replacement, retail_st42_wall as st42
from tools.analyze import retail_world_moving_receiver as moving


s = moving.s
PRIOR = {
    "gg4e08-world-moving-receiver.json": "1d892d90cd2a64904f7a42a01e832c059cf6349a290a14c00b39251a8188805f",
    "gg4e08-body-phase-exit.json": replacement.cleanup.EXIT_SHA256,
    "gg4e08-body-phase-cleanup.json": replacement.CLEANUP_SHA256,
    "gg4e08-body-replacement.json": "bec58b5e0f7d05188064bccc6e1013c033f4bdb34429844a3d8ca63fe898ca78",
    "gg4e08-replacement-entry.json": "a1e8c383eb3717c56969fdeef95d2894a87f686249861ccfacefa933464194f9",
}
TEXT = (
    (0x8003D344, 164, "f8a7485a7bb1eb37c6a02be76336a2fef43d8a348adbe9670469e6e57d4da06f"),
    (0x8005BBC0, 268, "8bbbe25fdc9ed379896e412fbcd66328cff259e960dc68fc5f0b86a904d79993"),
    (0x8018214C, 220, "e0996eb254bae537b09699eb277bae355de21bc2c2035d93c6dc9f0cae561a1c"),
    (0x80054CBC, 52, "5031b1cbdf20bc9464eef663b0778abe4c88b6c39931b9b0be6f0feabc0fdb26"),
)


def verify_source(image):
    moving.verify_source(image)
    replacement.verify_source(image)
    s.verify_text(image, TEXT)
    for site, target in ((0x8002F1D8, 0x8003D344), (0x8002F298, 0x8018214C),
                         (0x8002C6DC, 0x8005BBC0), (0x80054CD0, 0x80056900),
                         (0x80130580, 0x800E058C), (0x80041B98, 0x80197AD8),
                         (0x80041BA0, 0x80197AD8)):
        s.link(image, site, target)


def payload(afs):
    base = moving.tau.payload(afs)+st42.source_inputs(afs)[-sum(row[0] for row in st42.GRIDS):]
    roles = []
    with s.open_regular(afs) as stream:
        stamp = s.archive_stamp(stream)
        s.verify_archive(stream)
        member, digest = s.RetailArchive(stream).member(504, 0x0106)
        if digest != replacement.MEMBER_SHA256 or member.entries[1]["size"]:
            raise ValueError("queued normal Time Bomber member changed")
        for role, size, digest in replacement.ASSETS:
            data = member.read(role, size)[0]
            if len(data) != size or sha256(data).hexdigest() != digest:
                raise ValueError(f"queued Time Bomber role {role} changed")
            roles.append(data)
        if s.archive_stamp(stream) != stamp:
            raise ValueError("queued archive changed during extraction")
    return base+b"".join(roles)


def decode(text):
    if len(text) > 4096 or len(text.splitlines()) != 28:
        raise ValueError("loaded KO report extent")
    lines = iter(text.splitlines())

    def take(tag, *fields):
        if next(lines) != "world-ko-"+tag+": "+" ".join(str(x) for x in fields):
            raise ValueError(f"loaded KO changed at {tag}/{fields}")

    take("open", 174, 2, 41, 90)
    old = []
    for arm in (0, 1):
        before, requested, removed = (1, 14, 1) if arm else (60, 17, 17)
        take("writer", arm, 27, before, requested, removed)
        steps = []
        for visit in ((27, 28, 32, 63, 64, 65) if arm else (27, 28, 32)):
            phase = 1 if not arm else 2 if visit < 64 else 3
            receivers = 6 if not arm or visit == 27 else 0
            cells = 7 if arm and visit >= 63 else 0
            mask = "0443" if arm else "0000"
            owner = int(visit != 65)
            take("old", arm, visit, phase, mask, f"{receivers:04X}", cells, owner, 1, before-removed,
                 requested, requested, arm, 2)
            steps.append({
                "visit_after_scene_open": visit, "actor_phase": phase, "active_seats": 2, "hp": before-removed,
                "role_mask": mask, "ordinary_receiver_descriptor_mask": receivers,
                "old_shape_cells_returned_mask": cells, "old_archive_owner_bound": bool(owner), "seat_generation": 1,
                "attacker_damage_credit": requested, "receiver_damage_ledger": requested, "ko_loss_count": arm,
            })
        old.append({"case": "lethal" if arm else "survives", "writer": {
            "declared_current_hp": before, "unchanged_max_hp": 60, "requested_hp": requested, "removed_hp": removed,
            "force_after_writer": 300, "actor_phase_after_writer": 1, "archive_still_bound_after_writer": True,
        }, "world_boundaries": steps})

    take("request", 65, 504, 4, 1, 0, 1)
    wait = []
    for i in (1, 59, 60, 63):
        phase = 3 if i < 60 else 0
        take("wait", i, 1, phase, int(i < 60), max(0, 60-i), 1, "0807", "0443", 0, "0000", 0, 0, 1, 4)
        wait.append({"completed_dispatches_after_disc_cut": i, "actor_phase": phase, "wait_remaining": max(0, 60-i),
                     "archive_owner_bound": False, "processed_pad_read": False, "seat_active": True,
                     "seat_generation": 1, "old_ready_byte_still_set": True})
    take("delivery", 0, 1, 0, 0, 0, 1, "0807", "0443", 0, "0000", 1, 0, 1, 4)
    take("bound", 1, 16, 4, 10, 3, 48)
    restart = []
    for i, sub, timer, mask in ((1, 0, 0, "00EF"), (21, 0, 60, "00C3"), (80, 0, 1, "00C3"),
                                 (81, 1, 0, "00C3"), (82, 2, 0, "0000"), (83, 2, 0, "0000"),
                                 (139, 2, 0, "0000"), (140, 2, 0, "0000")):
        if i == 83:
            take("input", 83, 2, 10, 9)
        receivers = "3EF0" if i == 140 else "0000"
        take("restart", i, 1, 1, sub, timer, 2, "0106", mask, 220, receivers, 1, int(i >= 83), int(i >= 81), 4)
        restart.append({"visit_after_declared_delivery": i, "subphase": sub, "role_mask": mask,
                        "ready_byte": bool(i >= 81), "processed_pad_read": bool(i >= 83),
                        "ordinary_receiver_descriptor_mask": receivers})
    return old, wait, restart


def native_report(executable):
    p = run([Path(executable).resolve(strict=True)], capture_output=True, text=True, timeout=20)
    if (p.returncode or p.stderr or
            p.stdout != "native-body-exit: 0 17 12 48 12 23 24\nnative-body-exit: 1 17 1 0 1 17 17\n"):
        raise ValueError("native authored round/body contrast changed")
    return [
        {"case": "survives", "authored_start_hp": 60, "hp_write_update": 17, "credited_and_removed_hp": 12,
         "hp_after": 48, "attack_window_retired_update": 23, "round_ended": False},
        {"case": "lethal", "authored_start_hp": 1, "hp_write_update": 17, "credited_and_removed_hp": 1,
         "hp_after": 0, "attack_window_retired_update": 17, "round_ended": True, "actor_replaced": False},
    ]


def export(dol, afs, fixture, module, native, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("loaded KO output already exists")
    dol = Path(dol).resolve(strict=True)
    verify_source(s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256))
    for name, digest in PRIOR.items():
        s.read_metadata(s.MAPS/name, digest)
    text = s.fixture_report(fixture, dol, module, ("--world-ko",), payload(afs))
    old, wait, restart = decode(text)
    raw = s.encode({
        "kind": "gf-retail-world-ko-handoff", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "tau_member_sha256": moving.tau.MEMBER_SHA256,
        "replacement_member_sha256": replacement.MEMBER_SHA256, "stage_member_sha256": st42.MEMBER_SHA256,
        "prior_receipts": PRIOR, "source_trace_sha256": sha256(text.encode("ascii")).hexdigest(),
        "opening_visit": 174, "same_physical_seat": 1, "declared_side_1_queue": ["0807", "0106"],
        "old_actor_control": old, "handoff": {
            "request_visit_after_scene_open": 65, "member": 504, "old_view_roots": 2,
            "returned_view_wrappers": 4, "returned_old_archive_owner": True, "disk_call_executed": False,
            "old_queue_row_marked": True, "new_queue_row_selected": True, "cursor_wrapped_to": 0,
        }, "no_owner_wait": wait, "replacement": {
            "delivery_is_declared": True, "seat_generation": 2, "reallocated_same_physical_header": True,
            "hp": 220, "body_descriptors": 16, "view_roots": 4, "stock_channels": [10, 3],
            "history_words_cleared": 48, "opened_ordinary_receivers": 9,
            "first_paid_input_fork": {"visit": 83, "key": 2, "pose": 10, "stock_before": 10, "stock_after": 9,
                                      "bomb_factory_ran": False}, "restart_observations": restart,
        }, "authored_native_control": native_report(native),
        "notes": [
            "The same loaded original Tau contact setup runs on st42 grids. A two-row queue and an empty KO notification buffer are declared; the lethal fork changes only current HP, not max HP or the amount evaluator.",
            "The original dispatcher returns through the KO and the separate body unlink. The phase-three loader returns a loaded owner and four live wrappers before the real disc call. Shape cells, queue selection, and wrapper release are independent.",
            "The disc call is a hard cut, not a simulated successful return. New dispatcher visits start from that retained request state. Exact decompressed roles arrive by a declared handoff through 800412C4; no physical I/O or timing is inferred.",
            "The retained physical seat and header do not identify the new actor generation. Neutral startup, first paid input fork and first receiving list publication are observed separately. Native authored round termination is a contrast; no replacement admission is promoted.",
        ],
    }, expanded=("old_actor_control", "no_owner_wait", "authored_native_control"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as out:
        out.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "original_observations": len(text.splitlines())}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("dol", "afs", "fixture", "module", "native", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.native, args.output), sort_keys=True))
