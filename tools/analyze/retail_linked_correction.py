"""Verify original stage correction transfer under declared reciprocal actor links."""
import argparse
from hashlib import sha256
import json
from pathlib import Path

from tools.analyze import retail_stage_collision_walk as walk


s = walk.s
WALK_SHA256 = "8c2b53a3ccb59edfb8bcc192b56613f246dc46196e725ee00cc01830fcb16b8b"
TEXT = ((0x8002CB20, 344, "c8809b685fad83bde8b7379680d8c9809b605c66f65bc2a882718f440d913b09"),)
# Name, wall, declared mask selection, +204 bits, reversed seats, target wall
# sphere present, Tau present, expected transfer site.
CASES = (
    ("empty-grid", False, 15, 0, False, False, False, None),
    ("unlinked", True, 0, 0, False, False, False, None),
    ("source-71e-missing", True, 14, 0, False, False, False, None),
    ("target-71e-missing", True, 13, 0, False, False, False, None),
    ("source-71f-missing", True, 11, 0, False, False, False, None),
    ("target-71f-missing", True, 7, 0, False, False, False, None),
    ("reciprocal", True, 15, 0, False, False, False, "8002CC08"),
    ("threshold-predecessor", True, 15, 0x47434FFF, False, False, False, "8002CC08"),
    ("threshold-equal", True, 15, 0x47435000, False, False, False, None),
    ("threshold-successor", True, 15, 0x47435001, False, False, False, None),
    ("reversed-seats", True, 15, 0, True, False, False, "8002CC34"),
    ("both-corrected", True, 15, 0, False, True, False, None),
    ("pair-unlinked", True, 0, 0, False, False, True, None),
    ("pair-linked", True, 15, 0, False, False, True, "8002CC08"),
)


def verify_source(image):
    walk.verify_source(image)
    s.verify_text(image, TEXT)
    for site, target in ((0x8002BDAC, 0x8002CB20), (0x8002CC08, 0x80035CA4), (0x8002CC34, 0x80035CA4)):
        s.link(image, site, target)
    if image.read(0x80436F90, 4, "data") != bytes.fromhex("47435000"):
        raise ValueError("linked correction comparison constant changed")


def decode(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != len(CASES)+2:
        raise ValueError("linked correction report extent")
    pos = 0
    cases = []
    bits = walk.stage.pair.capsule.geometry.bits

    def take(tag, fields, name):
        nonlocal pos
        if lines[pos] != tag + ": " + " ".join(map(str, fields)):
            raise ValueError(f"linked correction changed: {name}/{tag}")
        pos += 1

    for i, (name, wall, masks, scalar, reverse, both, attack, site) in enumerate(CASES):
        source_x, target_x = 130, 140 if both else 250
        source_stage, target_stage = 20 if wall else 0, 10 if both else 0
        target_correction = source_stage if site else target_stage
        roots = [source_x+source_stage, target_x+target_correction]
        endpoints = [target_x-15, target_x+15]
        latches = [bool(source_stage), bool(target_correction)]
        take("linked-correction", (i, bits(source_stage), bits(target_stage), int(site is not None), site or "00000000",
             *(bits(x) for x in (source_x, target_x, *roots, source_stage, target_correction, *roots, *endpoints)),
             *(int(v) for v in latches)), name)
        source_seat, target_seat = (2, 1) if reverse else (1, 2)
        pair = None
        if attack:
            hit = site is not None
            scratch = [x+target_correction for x in endpoints] if hit else None
            hp = 163 if hit else 180
            take("linked-pair", (i, int(hit), *(bits(x) for x in (scratch or [0, 0])), int(hit), hp, int(hit)), name)
            pair = {"tau_center_x": 360, "narrow_capsule_kernel": "80030A3C" if hit else None,
                    "target_scratch_capsule_endpoints_x": scratch, "target_hp_removed": 17 if hit else 0,
                    "target_hp_after": hp, "source_hp_after": 180, "target_accepted_token": 1 if hit else None}
        cases.append({
            "case": name, "wall_grid_supplied": wall, "source_seat": source_seat, "target_seat": target_seat,
            "declared_source_masks_71e_71f": [1 << target_seat if masks & bit else 0 for bit in (1, 4)],
            "declared_target_masks_71e_71f": [1 << source_seat if masks & bit else 0 for bit in (2, 8)],
            "declared_target_204_bits": f"{scalar:08X}", "declared_roots_x": [source_x, target_x],
            "target_has_wall_sphere": both, "stage_only_corrections_x": [source_stage, target_stage],
            "original_transfer_call_site": site, "post_walk_roots_x": roots,
            "post_walk_corrections_x": [source_stage, target_correction], "post_walk_correction_flags": latches,
            "pre_center_pass_capsule_centers_x": [source_x, target_x], "post_walk_capsule_centers_x": roots,
            "target_allocated_capsule_endpoints_x": endpoints, "ordinary_pair": pair,
        })
    return cases


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("linked correction output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_source(image)
    s.read_metadata(s.MAPS / "gg4e08-stage-collision-walk.json", WALK_SHA256)
    payload, tau = walk.stage.source_inputs(image, afs)
    cases = decode(s.fixture_report(fixture, dol, module, ("--linked-correction",), payload))
    raw = s.encode({
        "kind": "gf-retail-linked-correction", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "stage_collision_walk_sha256": WALK_SHA256,
        "body_model_both_seats": "gg4e08:000a", "tau_pair_attack": tau,
        "propagation": "8002CB20", "propagation_call_site": "8002BDAC", "correction_writer": "80035CA4",
        "target_204_upper_bound_exclusive": 50000, "stop_before_presentation": "8005A580",
        "cases": cases,
        "notes": [
            "Both source and target use narrowed original normal Sasuke queues and declared rigid poses. The source always has a role-08 sphere. The target has only the ordinary capsule except in the both-corrected control.",
            "The original stage solver generates the vectors. The host declares only the reciprocal actor masks and target +204 scalar, not the corrections or the source of a live link.",
            "An isolated fork captures original stage output and the propagation writer call. The enclosing original collision walk reproduces the final roots, vectors and capsule centers, with presentation not entered.",
            "The target can receive the correction only with all four declared bits in these probes. Clearing any bit, reaching the exclusive scalar threshold, or already having its own wall correction prevents transfer.",
            "The transferred vector moves the target capsule's broad center; allocated endpoints do not move. The matched Tau probe without transfer misses, while the transferred target reaches the kernel with shifted scratch endpoints and loses 17 HP under token 1. The source body loses no HP.",
            "This does not establish which retail actions create or maintain the masks, live link lifetime, multi-actor chains, hardware or a native gameplay rule.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases),
            "transfer_calls": sum(row["original_transfer_call_site"] is not None for row in cases),
            "pair_cases": sum(row["ordinary_pair"] is not None for row in cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
