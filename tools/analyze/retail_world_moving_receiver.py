"""Join the original loaded Tau pose, receiver list, world pair and HP boundaries."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import retail_melee_interruption as tau, retail_st42_wall as st42


s = tau.s
PRIOR = {
    "gg4e08-dual-scene-entry.json": tau.DUAL_SHA256,
    "gg4e08-melee-interruption.json": "7debf8c3fdccc5a7a3a4cbe9671908941d89da56895c9d4b46a0b6446a53fb02",
    "gg4e08-moving-receiver.json": "9a160963eb3dfab7d57f15aa216e1b741a3b0e733d785a314e56802518164892",
}
TEXT = (
    (0x80054070, 316, "349fd1d6d1088a9ba19eb03192f38515faa2652943f8d2edce8d84ead782be90"),
    (0x800465FC, 156, "7ed4ac7350606317cb9c955ccda304c0989e84dfa9b16052184c41b33a5be24f"),
    (0x8008A28C, 48, "94cb040d0d0bd380e6ab2d81aec5d82583aaf212f52408a7050212f7e39acc08"),
    (0x80089AEC, 276, "03e4a562cd8742937f8f85f602f0f3a3a682457f8f5887accc0b15cbf24f4802"),
    (0x8002DB58, 944, "4aa097fc70080cd5b6c849d879cd20b39efebb359b80fb70b6a02c486ba38238"),
    (0x80030490, 112, "a395c9c1f3da5874d3ddfc9b2a710408a80620ba356a16412fc1c5aa1d8b8716"),
    (0x80030500, 216, "7ac306d984c3e0161de2c58373ddafab7c4603b0cd4a0da50d522f84cf285169"),
    (0x80039F6C, 240, "ea15e86d0428a669821b13e9d6131da6e5c4947d84970604cea6527186912e23"),
)


def verify_source(image):
    tau.verify_source(image)
    s.verify_text(image, TEXT)
    for site, target in ((0x80054160, 0x800465FC), (0x800294EC, 0x8008A28C),
                         (0x800294F0, 0x80089AEC), (0x800294F4, 0x8002BB14),
                         (0x8002DC58, 0x8002FD7C), (0x8002DCEC, 0x80039F6C),
                         (0x8002DD8C, 0x80030490)):
        s.link(image, site, target)


def positions(fields):
    values = list(struct.iter_unpack(">3f", bytes.fromhex("".join(fields))))
    return [list(row) for row in values]


def decode(text):
    if len(text) > 8192 or len(text.splitlines()) != 23:
        raise ValueError("world moving-receiver report extent")
    lines = iter(text.splitlines())
    if next(lines) != "world-receiver-open: 174 60 60":
        raise ValueError("world moving-receiver opening changed")
    cases = []
    word = r" [A-F0-9]{8}"
    number = r" [0-9]{1,3}"

    def row(tag, arm, visit, suffix):
        text = next(lines)
        if not re.fullmatch(rf"world-receiver-{tag}: {arm} {visit}"+suffix, text):
            raise ValueError(f"malformed world moving-receiver observation: {tag}/{arm}/{visit}")
        return text.split()[3:]

    for arm in (0, 1):
        samples, ends = [], []
        for visit in (26, 27):
            phase, published, clip, *xyz = row("pose", arm, visit, number*2+word*7)
            clip = struct.unpack(">f", bytes.fromhex(clip))[0]
            before, after = positions(xyz)
            if (int(phase), int(published), clip) != (13, int(visit==27), visit-22) or before == after:
                raise ValueError("receiver no longer moves in the common original phase")
            shapes = []
            for d in (1, 2):
                desc, slot, *xyz = row("shape", arm, visit, number*2+word*9)
                matrix, previous, current = positions(xyz)
                if (int(desc), int(slot)) != (d, d+3) or previous == current:
                    raise ValueError("receiver shape/matrix owner or publication changed")
                if samples and previous != samples[0]["shapes"][d-1]["current_center"]:
                    raise ValueError("receiver no longer retains its real previous published pose")
                shapes.append({"descriptor": d, "cell_ordinal": int(slot), "actor_matrix_translation": matrix,
                               "previous_center": previous, "current_center": current})
            samples.append({"visit_after_opening": visit, "receiver_selector": int(phase),
                            "receiver_lower_pose_frame": clip, "root_before_actor_visit": before,
                            "root_after_actor_visit": after, "published_attacks": int(published), "shapes": shapes})
        slot, kind, *xyz = row("pair", arm, 27, number*2+word*12)
        correction, attack_center, *centers = positions(xyz)
        if (int(slot), int(kind)) != (6, 0) or correction != [0, 0, 0] or centers != [
                shape["current_center"] for shape in samples[-1]["shapes"]]:
            raise ValueError("original pair no longer consumes the published current centers")
        eligible, broad, tried, accepted, fork_hp = map(int, row("narrow", arm, 27, number*5))
        if (eligible, broad, tried, accepted, fork_hp) != ((3, 1, 1, 0, 60) if arm else (3, 3, 2, 2, 43)):
            raise ValueError("world moving-receiver contact fork changed")
        for visit in (27, 28, 32):
            writes, hp, token, credit, removed, attacks, returned = map(int, row("end", arm, visit, number*7))
            if (writes, hp, token, credit, removed, attacks, returned) != (
                    int(not arm and visit==27), 60 if arm else 43, 0 if arm else 1, 0 if arm else 17,
                    0 if arm else 17, int(visit < 32), int(visit==32)):
                raise ValueError("joined world HP/credit/retirement result changed")
            ends.append({"visit_after_opening": visit, "hp_writes": writes, "receiver_hp": hp,
                         "receiver_token": token, "attacker_credit_ledger": credit, "receiver_damage_ledger": removed,
                         "published_attacks": attacks, "same_attack_cell_returned": bool(returned)})
        cases.append({
            "case": "moving-miss" if arm else "moving-hit", "receiver_processed_stick_x": -63 if arm else 63,
            "poses_and_registry": samples, "ordinary_pair_on_visit": 27,
            "attack_cell_ordinal": int(slot), "attack_shape_kind": "sphere", "attack_center_at_pair": attack_center,
            "receiver_owner_correction_at_pair": correction, "receiver_centers_at_pair": centers,
            "relationship_eligible_receivers_mask": eligible, "broad_accepted_mask": broad, "narrow_attempted_mask": tried,
            "narrow_accepted_mask": accepted, "pair_only_fork_receiver_hp": fork_hp, "complete_world": ends,
        })
    return cases


def native_report(executable):
    p = run([Path(executable).resolve(strict=True)], capture_output=True, timeout=20, text=True)
    if (p.returncode or p.stderr or p.stdout !=
            "native-receiver: 0 17 128.50 123.50 1 1 1 48\nnative-receiver: 1 17 165.50 170.50 1 0 0 60\n"):
        raise ValueError("authored native moving-receiver control changed")
    return [
        {"case": "moving-hit", "native_window_armed_update": 17, "position_z_before": 128.5, "position_z_at_contact": 123.5,
         "stale_would_overlap": True, "current_overlaps": True, "hp_removed": 12},
        {"case": "moving-miss", "native_window_armed_update": 17, "position_z_before": 165.5, "position_z_at_contact": 170.5,
         "stale_would_overlap": True, "current_overlaps": False, "hp_removed": 0},
    ]


def export(dol, afs, fixture, module, native, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("world moving-receiver output already exists")
    dol = Path(dol).resolve(strict=True)
    verify_source(s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256))
    for name, digest in PRIOR.items():
        s.read_metadata(s.MAPS/name, digest)
    payload = tau.payload(afs)+st42.source_inputs(afs)[-sum(row[0] for row in st42.GRIDS):]
    text = s.fixture_report(fixture, dol, module, ("--world-moving-receiver",), payload)
    raw = s.encode({
        "kind": "gf-retail-world-moving-receiver", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "tau_member_sha256": tau.MEMBER_SHA256,
        "stage_member": st42.MEMBER, "stage_member_sha256": st42.MEMBER_SHA256, "prior_receipts": PRIOR,
        "source_trace_sha256": sha256(text.encode("ascii")).hexdigest(), "original_opening_visit": 174,
        "shared_full_world_visits_after_opening": 21, "receiver_direction_start_visit": 22,
        "original_cases": decode(text), "authored_native_controls": native_report(native),
        "notes": [
            "Both normal Taus load before the scoped full dispatcher visits. Scene slot placement and processed pads are declared; st42 static grids are real. Stage world state is the no-op branch; this does not include stage props, hardware input or presentation devices.",
            "The paired cases share the first 21 world visits including the processed B packet. Opposite receiver flicks change the two actors' subsequent motion. The receiver is not staged by replacing a root, matrix or shape after opening.",
            "Boundary forks observe original actor publication, then original registry rebuild, then the real pair inlet. The complete scene returns naturally through the world tail and records HP writes separately. Slot ordinals refer to the original bounded pool, not authorities outside their owners.",
            "The native control uses the authored Tau-shaped capsule/body proxy with movement before contact, not skeletal retail geometry, original damage or source visit equivalence.",
        ],
    }, expanded=("original_cases", "authored_native_controls"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as file:
        file.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "original_observations": len(text.splitlines())}


if __name__ == "__main__":
    cli = argparse.ArgumentParser(description=__doc__)
    for name in ("dol", "afs", "fixture", "module", "native", "output"):
        cli.add_argument(name, type=Path)
    args = cli.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.native, args.output), sort_keys=True))
