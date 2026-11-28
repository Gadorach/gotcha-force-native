"""Join original stage-contact correction to the later ordinary hit test."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze import retail_receiver_correction as correction


s = correction.s
pair = correction.moving.sweep.pair
melee = pair.melee
CORRECTION_SHA256 = "9e9d41a438f6f302553b7e478277d7e9985283c949dc4bd6219487ee5c65ff1f"
TEXT = (
    (0x8003D4A0, 580, "7e7780e5c0aa6e213afcfed5ebda79e0888da328b35a886528d518fa047b91d7"),
    (0x8002E1C8, 224, "470b12b4ca9eb8185cec3058d100094f89086e056a23748770ba2a97923e829b"),
    (0x80035E58, 3868, "3b12c61c320b7803115905ddcf9f965e0b0c8e571f59aa31ddbc5af7bb9d4678"),
    (0x80039AEC, 308, "34a98991b5f94e53f4d2db6d97ac25b4da1cf082066462ffda3a14ba4e9f1607"),
    (0x8003ED44, 768, "211cfa1a38e24afad3393a624d66655b96ea1a587a0c3d8c86d0ff7351ce6103"),
)
# Name, wall, initial/final pose, optional Tau center, corrected root, stage latch.
CASES = (
    ("empty", 0, 130, 130, None, 130, 0), ("just-outside", 1, 151, 151, None, 151, 0),
    ("tangent", 1, 150, 150, None, 150, 1), ("front-overlap", 1, 130, 130, None, 150, 1),
    ("on-plane", 1, 100, 100, None, 150, 1), ("approach", 1, 170, 130, None, 150, 1),
    ("cross-plane", 1, 170, 90, None, 150, 1),
    ("attack-left-empty", 0, 130, 130, 40, 130, 0), ("attack-left-wall", 1, 130, 130, 40, 150, 1),
    ("attack-right-empty", 0, 130, 130, 240, 130, 0), ("attack-right-wall", 1, 130, 130, 240, 150, 1),
    ("stationary-behind", 1, 90, 90, None, 90, 0),
)


def verify_source(image):
    correction.verify_source(image)
    s.verify_text(image, TEXT)
    for site, target in ((0x8002BDA8, 0x8002E1C8), (0x8002E278, 0x80035E58), (0x8003618C, 0x80039AEC),
                         (0x80036278, 0x80035CA4), (0x80036C5C, 0x8003ED44), (0x80036D24, 0x80035CA4)):
        s.link(image, site, target)


def source_inputs(image, afs):
    with s.open_regular(afs) as stream:
        stamp = s.archive_stamp(stream)
        s.verify_archive(stream)
        archive = s.RetailArchive(stream)
        sasuke = melee.opening.read_normal(archive, melee.opening.RECEIVER, melee.opening.RECEIVER_BLOCK_SHA256,
                                         melee.ACTOR_SHA256[melee.opening.RECEIVER])
        reviewed = s.read_metadata(s.MAPS / "reaction-melee-map.json", melee.MELEE_SHA256)
        tau, tau_info = melee.verify_route(image, archive, reviewed, melee.ROUTES[0])
        if s.archive_stamp(stream) != stamp:
            raise ValueError("stage correction source archive changed")
    body, _ = sasuke
    if (body[1] != 1 or body[11] != 11 or body[112:124] != bytes.fromhex("0080000800000000FFFF0000")
            or struct.unpack_from(">4f", body, 124) != (0, 50, 0, 50)
            or struct.unpack_from(">7f", body, 32+11*80+12) != (-12, 0, 0, 15, 0, 0, 7)):
        raise ValueError("stage correction normal source body shapes changed")
    return b"".join(sasuke)+tau, tau_info


def decode(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != 28:
        raise ValueError("stage correction report extent")
    pos = 0
    cases = []
    bits = correction.moving.sweep.pair.capsule.geometry.bits

    def take(tag, fields):
        nonlocal pos
        if lines[pos] != tag + ": " + " ".join(map(str, fields)):
            raise ValueError(f"stage correction changed at row {pos+1}: {tag}")
        pos += 1

    for i, (name, wall, begin, end, attack, corrected, latch) in enumerate(CASES):
        vector = corrected-end
        take("stage-correction", (i, wall, begin, end, *(bits(n) for n in (corrected, vector, end, corrected)), latch, latch, 180))
        endpoints = [end-15, end+15]
        take("stage-capsule", (i, *(bits(n) for n in (end, corrected, *endpoints, *endpoints))))
        ordinary = None
        if attack is not None:
            broad = abs(attack-corrected) <= 92
            hit = broad and abs(attack-corrected) < 92
            damage = 17 if hit else 0
            take("stage-correction-pair", (i, attack, int(broad), int(hit), 180-damage, int(hit)))
            ordinary = {"tau_center_x": attack, "broad_phase_accepted": broad, "sphere_capsule_kernel_reached": broad,
                        "scratch_endpoints_x": [x+vector for x in endpoints] if broad else None,
                        "hp_removed": damage, "hp_after": 180-damage, "accepted_token": 1 if hit else None}
        cases.append({
            "case": name, "wall_grid_supplied": bool(wall), "declared_pose_x_samples": [begin, end],
            "post_solver_root_x": corrected, "generated_correction_x": vector,
            "role08_wall_contact_flag": bool(latch), "owner_correction_flag": bool(latch),
            "sphere_and_capsule_broad_center_x": [corrected, corrected], "allocated_capsule_endpoints_x": endpoints,
            "hp_after_stage_only": 180, "ordinary_pair": ordinary,
        })
    return cases


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("stage correction output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_source(image)
    s.read_metadata(s.MAPS / "gg4e08-receiver-correction.json", CORRECTION_SHA256)
    payload, tau = source_inputs(image, afs)
    cases = decode(s.fixture_report(fixture, dol, module, payload=payload))
    raw = s.encode({
        "kind": "gf-retail-stage-correction", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "receiver_correction_sha256": CORRECTION_SHA256,
        "receiver": "gg4e08:000a", "stage_contact_sphere_descriptor": 1, "damage_capsule_descriptor": 11,
        "tau_pair_attack": tau, "grid_role": 1, "declared_wall_vertices": [[100, -200, -200], [100, 600, -200], [100, -200, 600]],
        "declared_wall_plane": [1, 0, 0, -100], "cases": cases,
        "notes": [
            "Three declared STIH blocks pass through the original relocator. The secondary block is empty or contains one declared wall triangle; this is not geometry from a retail arena.",
            "Sasuke's unedited role-08 sphere and ordinary capsule bind through a narrowed original body queue. The original stage contact pass generates the observed correction from those declared poses.",
            "The original stage pass writes root, correction and contact flags without touching HP. A separate bucket pass consumes the same vector for both broad centers without updating allocated capsule endpoints.",
            "In four original ordinary-pair probes the generated correction can admit or remove Tau's hit. The hit uses the corrected capsule scratch endpoint, not an edited allocated descriptor.",
            "No original stage model loop, linked actors, further contacts, hardware or native gameplay are qualified.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases),
            "ordinary_pair_cases": sum(row["ordinary_pair"] is not None for row in cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
