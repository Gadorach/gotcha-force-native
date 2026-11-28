"""Trace a loaded one-row Tau KO through Force, result selection and retirement."""
import argparse
from hashlib import sha256
import json
from pathlib import Path

from tools.analyze import retail_world_body_exit as ko


s, moving = ko.s, ko.moving
PRIOR = {
    "gg4e08-world-ko-handoff.json": "ccdd39596629ed41d71caa2d3540b5a380a581bcae10abb5e59d8c11a2f71408",
}
TEXT = (
    (0x80029164, 180, "eb13594f85edfd5251a3bc25811178b991506f480306f82922d332a5c4bac378"),
    (0x800297C8, 912, "8f3accbb373edc251c6a980747e2de5091953d21020ef0ae2c8eb7310cc5e7b6"),
    (0x800289FC, 1896, "2656010b540078a686e4696822d983832c9d5f55dd7b2a01b1c52a7836c54ce5"),
    (0x80011BCC, 80, "227be8ef88e9644a84e2f1907e19d8fa1bc74e0bf0b377857066f2bead4f2dc0"),
    (0x800F0F84, 692, "0f73168764d12249d7cfd4d641e20ef4aa256cd93ee2b7771fb63743dbfa9def"),
)


def verify_source(image):
    ko.verify_source(image)
    s.verify_text(image, TEXT)
    for site, target in ((0x800291C0, 0x80029434), (0x800291C8, 0x800297C8),
                         (0x80028A80, 0x80011BCC), (0x80011C08, 0x800F03C0),
                         (0x800F0400, 0x800EFC04), (0x800EFC90, 0x800F0F84)):
        s.link(image, site, target)


def decode(text):
    if len(text) > 4096 or len(text.splitlines()) != 15:
        raise ValueError("one-row KO report extent")
    lines = iter(text.splitlines())

    def take(tag, *fields):
        if next(lines) != "world-end-"+tag+": "+" ".join(str(x) for x in fields):
            raise ValueError(f"one-row KO changed at {tag}/{fields}")

    take("open", 174, 90, 90, 1)
    cases = []
    for arm, label, visits in (
        (0, "survives", (27, 28, 35)),
        (2, "lethal_without_force_debit", (27, 28, 35, 63, 64, 65, 124, 125, 130)),
        (1, "lethal_exhausts_force", (27,)),
    ):
        steps = []
        for visit in visits:
            phase = 1 if not arm else 2 if visit < 64 else 3
            active = visit < 125
            sub = 2 if not arm or not active else int(visit >= 65)
            timer = 125-visit if arm == 2 and 65 <= visit < 125 else 0
            requested = (14 if arm else 17) if visit == 27 else 0
            hp, force, result = (0 if arm else 43), (0 if arm == 1 else 90), int(arm == 1)
            handler, receivers, cells = 2+result, (6 if not arm or visit == 27 else 0), (7 if arm and visit >= 63 else 0)
            take("step", arm, visit, handler, result, force, hp, requested, int(active), 1+int(active),
                 phase, sub, timer, f"{receivers:04X}", cells, 1)
            steps.append({
                "visit_after_scene_open": visit, "scene_handler": handler, "result_mask": result,
                "side_1_force": force, "receiver_hp": hp, "hp_write_request": requested, "receiver_active": active,
                "active_seats": 1+int(active), "receiver_phase": phase, "receiver_subphase": sub, "receiver_clock": timer,
                "ordinary_receiver_descriptor_mask": receivers, "old_shape_cells_returned_mask": cells,
                "old_archive_owner_and_wrappers_retained": True,
            })
        cases.append({"case": label, "declared_current_hp": 1 if arm else 60, "force_debit_enabled": arm != 2,
                      "hp_removed": 1 if arm else 17, "force_at_hp_leaf_return": 90, "result_at_hp_leaf_return": 0,
                      "completed_dispatches": steps})
    take("frontier", 28, 3, 1, 5, 90, 1, 2, 1, "800F0FDC", "800EFC94", "00000766", "803D3E44", 118)
    return cases, {
        "visit_after_scene_open": 28, "scene_handler": 3, "ending_step": 1, "actor_pass_status": 5, "ending_clock": 90,
        "result_mask": 1, "receiver_phase": 2, "old_archive_owner_still_bound": True,
        "pc": "800F0FDC", "lr": "800EFC94", "refused_external_read": "00000766",
        "missing_audio_table_pointer_at": "803D3E44", "audio_entry_index": 118,
        "ending_dispatch_return_observed": False, "match_end_return_observed": False, "save_behavior_qualified": False,
    }


def export(dol, afs, fixture, module, native, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("one-row KO output already exists")
    dol = Path(dol).resolve(strict=True)
    verify_source(s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256))
    for name, digest in PRIOR.items():
        s.read_metadata(s.MAPS/name, digest)
    st42 = moving.st42
    payload = moving.tau.payload(afs)+st42.source_inputs(afs)[-sum(row[0] for row in st42.GRIDS):]
    text = s.fixture_report(fixture, dol, module, ("--world-match-end",), payload)
    cases, frontier = decode(text)
    raw = s.encode({
        "kind": "gf-retail-world-match-end-frontier", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "tau_member_sha256": moving.tau.MEMBER_SHA256,
        "stage_member_sha256": st42.MEMBER_SHA256, "prior_receipts": PRIOR,
        "source_trace_sha256": sha256(text.encode("ascii")).hexdigest(), "opening_visit": 174,
        "declared_side_1_queue": ["0807"], "declared_side_1_force": 90, "original_receiver_force_cost": 90,
        "original_cases": cases, "ending_frontier": frontier, "authored_native_control": ko.native_report(native),
        "notes": [
            "The single-row queue has no reserve actor or next archive. The common prefix declares a team Force budget of 90. The lethal arms differ only in the Force-debit gate; each requests 14, removes one HP, and credits 14.",
            "The same returning combat dispatcher can select a result while the defeated actor and archive are still bound. In the no-debit arm, inactive-seat state does not return the old archive or select a result through visit 130.",
            "The fixture refuses the exact missing audio table read. No data is returned to the original code; the first ending dispatch does not return. No match-end return, additional cleanup, or save handling is claimed.",
            "The authored native terminal contrast exposes a portable win only after its immediate team KO, not an emulation of the original Force or ending schedule.",
        ],
    }, expanded=("original_cases", "authored_native_control"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "original_observations": len(text.splitlines())}


if __name__ == "__main__":
    cli = argparse.ArgumentParser(description=__doc__)
    for name in ("dol", "afs", "fixture", "module", "native", "output"):
        cli.add_argument(name, type=Path)
    args = cli.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.native, args.output), sort_keys=True))
