"""Verify original sphere-to-capsule attack sweeps from consecutive pose samples."""
import argparse
from hashlib import sha256
import json
import math
from pathlib import Path
import struct

from tools.analyze import retail_capsule_pair as pair


s = pair.s
PAIR_SHA256 = "8360ca56e1db2fd9400d70651cfbdf02256680ce8be64f43b33bb9c4f53a7fec"
TEXT = (
    (0x80030500, 216, "7ac306d984c3e0161de2c58373ddafab7c4603b0cd4a0da50d522f84cf285169"),
    (0x80031E50, 224, "b9787b69550f502fe5cda0e433a80ee1b1eb949399575b921287b52d40a72d7b"),
    (0x80038C00, 2404, "d6806504398834dff05cb8b2febd87579743762e1deb808220dd548fb918c0ed"),
)
CENTER = (200, 150, 325)
CASES = (
    ("below-motion-threshold", False, 0, 0, .25, 0, None),
    ("exact-motion-threshold", False, 0, 0, .5, 0, None),
    ("above-motion-threshold", False, 0, 0, .75, 0, None),
    ("sphere-crossing", False, 0, -120, 120, 0, None),
    ("sphere-side-inside", False, 0, -120, 120, 91, None),
    ("sphere-side-tangent", False, 0, -120, 120, 92, None),
    ("sphere-side-outside", False, 0, -120, 120, 93, None),
    ("capsule-parallel-crossing", True, 0, -120, 120, 0, None),
    ("capsule-perpendicular-crossing", True, 2, -120, 120, 0, None),
    ("capsule-side-inside", True, 2, -120, 120, 76, None),
    ("capsule-side-tangent", True, 2, -120, 120, 77, None),
    ("capsule-side-outside", True, 2, -120, 120, 78, None),
    ("crossing-role-excluded", False, 0, -120, 120, 0, "role-01-mask"),
    ("crossing-late-excluded", False, 0, -120, 120, 0, "late-276"),
)


def decode(report):
    lines = report.splitlines()
    if len(report) > 12288 or len(lines) != 3*len(CASES):
        raise ValueError("sphere sweep report extent")
    rows = []
    bits = pair.capsule.geometry.bits
    for i, (name, capsule, axis, first, last, side, gate) in enumerate(CASES):
        start, end = list(CENTER), list(CENTER)
        start[axis] += first
        end[axis] += last
        start[1] += side
        end[1] += side
        samples = []
        for visit in range(3):
            travel = last-first if visit == 1 else 0
            swept = travel > .5
            center = [(a+b)/2 for a, b in zip(start, end, strict=True)] if swept else (end if visit else start)
            old = end if visit == 2 else start
            new = end if visit else start
            broad_field = 70 + (travel / 2 if swept else travel)
            receiver_broad = (484 if visit else 22) if capsule else 22
            published = not (gate and visit == 1)
            broad = published and math.dist(CENTER, center) <= broad_field+receiver_broad
            hit = broad and (i < 3 or (visit == 1 and side <= (77 if capsule else 92)))
            narrow = ("80031E50" if swept and capsule else "80030A3C" if swept or capsule else "80030500") if broad else None
            request = 17 if hit else 0
            fields = (i, visit, 2 if swept else 0, int(swept), bits(broad_field), bits(70),
                      *(bits(n) for n in (*center, *old, *old, *new)), int(published), narrow or "00000000",
                      int(broad), int(hit), int(hit), request, 180-request, int(hit))
            if lines[3*i+visit] != "sphere-motion: " + " ".join(map(str, fields)):
                raise ValueError(f"sphere sweep geometry, admission, or HP changed: {name}/{visit}")
            samples.append({
                "rebuild": visit+1, "published": published, "effective_shape": "capsule" if swept else "sphere",
                "motion_latch": swept, "core_radius": 70, "broad_field": broad_field, "center": center,
                "retained_previous_center": old, "stored_edges": [old, new], "receiver_broad_field": receiver_broad,
                "broad_accepted": broad if published else None, "narrow_kernel": narrow,
                "narrow_accepted": hit if broad else None, "kernel_operands_swapped": bool(narrow and swept and not capsule),
                "hp_writer_reached": hit, "hp_removed": request, "hp_after": 180-request,
                "accepted_token": 1 if hit else None,
            })
        rows.append({
            "case": name, "receiver_descriptor": 11 if capsule else 9, "receiver_shape": "capsule" if capsule else "sphere",
            "declared_motion_axis": ("x", "y", "z")[axis], "declared_side_offset": side, "middle_publication_gate": gate,
            "shape_kept_same_attack_token": 1, "samples": samples,
        })
    return rows


def verify_source(image):
    pair.verify_source(image)
    s.verify_text(image, TEXT)
    s.link(image, 0x80031E7C, 0x80038C00)
    for table_index, expected in ((0, 0), (2, 2), (16, 0x82), (18, 7)):
        if image.read(0x802CFF68+table_index, 1, "data") != bytes([expected]):
            raise ValueError("sphere sweep kernel selector or operand swap changed")
    if (image.read(0x802CFF2C, 4, "data") != bytes.fromhex("80030500")
            or image.read(0x802CFF48, 4, "data") != bytes.fromhex("80031E50")
            or image.read(0x80436F7C, 4, "data") != bytes.fromhex("3F000000")):
        raise ValueError("sphere sweep motion threshold or kernel changed")


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("sphere sweep output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_source(image)
    s.read_metadata(s.MAPS / "gg4e08-capsule-pair.json", PAIR_SHA256)
    prior = s.read_metadata(s.MAPS / "gg4e08-body-capsules.json", pair.CAPSULE_SHA256)
    payload, tau, _ = pair.source_inputs(image, afs, prior)
    sasuke = 7840+432
    shape = payload[sasuke+32+9*80:sasuke+32+10*80]
    if (shape[:4] != bytes.fromhex("00020002")
            or struct.unpack_from(">4f", shape, 12) != (0, -14, 0, 22)):
        raise ValueError("sphere sweep receiver sphere changed")
    rows = decode(s.fixture_report(fixture, dol, module, ("--moving-sphere",), payload))
    raw = s.encode({
        "kind": "gf-retail-sphere-sweep", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "capsule_pair_sha256": PAIR_SHA256, "attack": tau,
        "receiver": "gg4e08:000a", "receiver_center": CENTER, "motion_threshold": .5, "cases": rows,
        "notes": [
            "Tau's exact sphere command is bound once. The host changes only its declared matrix translation and corresponding root for the middle pose; the receiver remains stationary.",
            "The middle visit may publish the old-to-new center segment as an effective capsule without a new descriptor or token. A separate pair fork is taken from each rebuild, so a hit at one pose cannot block a later probe.",
            "The effective capsule's broad field is core radius plus half of the center displacement. Small moves remain a sphere with the full displacement added to the broad field.",
            "For the sampled sweeps, the side-tangent cases reach HP and one-unit-outside cases do not. This is not a universal tangency rule for either classifier.",
            "Original update history advances in both declared middle-publication refusals. Reopening the stationary endpoint does not replay the withheld sweep.",
            "No animation transition, moving receiver, hardware or native gameplay is qualified.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(rows),
            "rebuilds": sum(len(row["samples"]) for row in rows),
            "hp_writes": sum(sample["hp_writer_reached"] for row in rows for sample in row["samples"])}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
