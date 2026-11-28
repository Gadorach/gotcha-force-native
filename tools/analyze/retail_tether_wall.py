"""Join original type-25 mask production to wall transfer and later damage."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze import retail_type25_link as tether
from tools.analyze.reaction_emit import HIT_BYTES
from tools.analyze.retail_movement_operands import ACTOR_BYTES


s = tether.s
stage = tether.link.walk.stage
TYPE25_SHA256 = "799f18c5621fb1a3ede60cba8db43a2f3969931b8a8db566554b206f6b65cd40"
# Case, wall, carrier link subphase, timer, second reciprocal mask, made motion requests, Tau supplied.
CASES = (
    ("stretched-empty", False, 1, 598, True, True, False),
    ("before-pull-wall", True, 1, 599, False, False, False),
    ("stretched-wall", True, 1, 598, True, True, False),
    ("expired-wall", True, 2, 0, False, True, False),
    ("tau-stretched-empty", False, 1, 598, True, True, True),
    ("tau-stretched-wall", True, 1, 598, True, True, True),
)


def source_inputs(image, afs):
    stride = HIT_BYTES + ACTOR_BYTES
    tethers = tether.source_inputs(afs)
    stage_pair, tau = stage.source_inputs(image, afs)
    if (len(tethers) != 3*stride or len(stage_pair) != 2*stride
            or tethers[2*stride:] != stage_pair[:stride]):
        raise ValueError("tether wall normal body inputs disagree")
    owner_capsule = 32+11*80
    if (11 not in tethers[:32].split(b"\xff", 1)[0]
            or tethers[owner_capsule:owner_capsule+12] != bytes.fromhex("0206000200000000FFFF0000")
            or struct.unpack_from(">7f", tethers, owner_capsule+12) != (-20, 0, -2, 15, 0, 0, 14)):
        raise ValueError("tether wall lost the normal Chainsaw capsule descriptor")
    return tethers[:stride] + stage_pair, tau


def decode(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != 8:
        raise ValueError("tether wall report extent")
    bits = stage.pair.capsule.geometry.bits
    pos = 0
    cases = []

    def take(tag, fields, name):
        nonlocal pos
        if lines[pos] != tag + ": " + " ".join(map(str, fields)):
            raise ValueError(f"tether wall changed: {name}/{tag}")
        pos += 1

    for i, (name, wall, phase, timer, pulled, requested, attack) in enumerate(CASES):
        declared = [200, 830]
        endpoints = [185, 215]
        e = [2, 1] if phase == 1 else [0, 0]
        f = [2, 1] if pulled else [0, 0]
        requests = [int(requested)]*2
        stage_only = [0, 20*wall]
        transfer = wall and pulled
        writer_site = "8002CC34" if transfer else None
        corrections = [20*transfer, stage_only[1]]
        roots = [x+c for x, c in zip(declared, corrections)]
        flags = [bool(v) for v in corrections]
        hit = attack and not transfer
        damage = 17*hit
        take("tether-wall", (i, phase, timer, *e, *f, *requests, *(bits(x) for x in (*declared, *declared, *stage_only)),
             int(transfer), writer_site or "00000000", *(bits(x) for x in (*roots, *corrections, *roots, *endpoints)),
             *(int(v) for v in flags), 250-damage, 180, 1), name)
        pair = None
        if attack:
            scratch = [x+corrections[0] for x in endpoints]
            take("tether-wall-pair", (i, "8003D344" if hit else "8005A580", *(bits(x) for x in scratch),
                 damage, 2*hit, 60), name)
            pair = {
                "tau_attack_center": [105, 100, 200], "tau_attack_token": 2,
                "original_narrow_kernel": "80030A3C", "chainsaw_scratch_capsule_endpoints_x": scratch,
                "hp_writer_reached": hit, "hp_removed": damage, "chainsaw_hp_after": 250-damage,
                "chainsaw_accepted_token": 2 if hit else None, "tau_hp_after": 60,
            }
        cases.append({
            "case": name, "wall_grid_supplied": wall, "carrier_link_subphase": phase, "carrier_timer": timer,
            "produced_masks_71e_owner_target": e, "produced_masks_71f_owner_target": f,
            "recorded_motion_request_counts_owner_target": requests,
            "pre_walk_roots_x_owner_target": declared, "stage_only_corrections_x_owner_target": stage_only,
            "transfer_writer_call_site": writer_site, "post_walk_root_x_owner_target": roots,
            "post_walk_correction_x_owner_target": corrections, "post_walk_correction_flags_owner_target": flags,
            "pre_walk_broad_centers_x_owner_target": declared, "post_walk_broad_centers_x_owner_target": roots,
            "allocated_chainsaw_capsule_endpoints_x": endpoints, "chainsaw_hp_after": 250-damage,
            "sasuke_hp_after": 180, "sasuke_retained_tether_token": 1, "tau_pair": pair,
        })
    return cases


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("tether wall output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    tether.verify_source(image)
    s.read_metadata(s.MAPS / "gg4e08-type25-link.json", TYPE25_SHA256)
    s.read_metadata(s.MAPS / "gg4e08-linked-correction.json", tether.LINK_SHA256)
    payload, tau = source_inputs(image, afs)
    cases = decode(s.fixture_report(fixture, dol, module, ("--wall-join",), payload))
    raw = s.encode({
        "kind": "gf-retail-tether-wall", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "type25_link_sha256": TYPE25_SHA256,
        "linked_correction_sha256": tether.LINK_SHA256, "tau_pair_attack": tau,
        "tether_owner": "gg4e08:0201", "tether_key": 0, "tether_target": "gg4e08:000a",
        "chainsaw_body_capsule_descriptor": 11, "sasuke_stage_sphere_descriptor": 1,
        "declared_wall_plane": [1, 0, 0, -800], "stage_pass": "8002E1C8",
        "propagation_pass": "8002CB20", "collision_walk": "8002BB14",
        "cases": cases,
        "notes": [
            "The exact original carrier factory, shape bind and zero-HP pair produce Sasuke's token 1 and retain the normal target. Only original callbacks write the actor link masks.",
            "All collision probes use the same declared final rigid poses. A second shape rebuild holds the pose so the wall sphere is stationary rather than sweeping from the first contact. The declared STIH wall is not a retail arena.",
            "The stage sphere belongs to Sasuke. Chainsaw Knight has an ordinary capsule but does not present a stage shape in this test. His direct stage correction remains zero; only the original propagation pass moves him.",
            "The before-pull and expiry controls let the wall move Sasuke alone; the stretched wall case lets the same vector reach Chainsaw Knight. The empty-grid control has both link masks but no displacement.",
            "Tau publishes a separate attack under token 2 after the tether shape has closed. Both comparisons reach the original capsule narrow kernel. The empty-grid case completes the 17-HP writer without resuming its caller; the wall case reaches the presentation frontier without an HP write.",
            "Tether movement requests are observed but not integrated. No full actor/controller tick, live stage, matching hardware, rendering, playback or native gameplay is qualified.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases),
            "transfer_calls": sum(row["transfer_writer_call_site"] is not None for row in cases),
            "tau_hp_writes": sum(bool(row["tau_pair"] and row["tau_pair"]["hp_writer_reached"]) for row in cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
