"""Fix the original ordinary pair's time asymmetry with moving receiver probes."""
import argparse
from hashlib import sha256
import json
from pathlib import Path

from tools.analyze import retail_sphere_sweep as sweep


s = sweep.s
SWEEP_SHA256 = "9a2de04c06fa8fc1beef8ca4e2146809e2449cff4d630ae6c2be6cac12627653"
TEXT = (
    (0x8003A0C8, 4, "f332ea5b5437103cbb6f1508679da89eec9288ad775c96c439a17fccabe3de8e"),
    (0x8003A260, 88, "6f8cf252f6237e52b78b40e508797ccfc2ff65215783cbdf8b67f59154eb0ef0"),
)
CASES = (
    ("receiver-crosses-stationary-attack", 0, 0, -120, 120),
    ("opposite-crossing-ends-in-sweep", -120, 120, 120, -120),
    ("opposite-crossing-ends-outside-sweep", -120, 120, 120, -360),
    ("parallel-final-pose-in-sweep", -120, 120, -240, 0),
    ("parallel-final-pose-outside-sweep", -120, 120, 120, 360),
)


def decode(report):
    lines = report.splitlines()
    if len(report) > 8192 or len(lines) != 30:
        raise ValueError("moving receiver report extent")
    pos = 0
    cases = []
    bits = sweep.pair.capsule.geometry.bits
    for capsule in (0, 1):
        for i, (name, a, b, c, d) in enumerate(CASES):
            samples = []
            core = 7 if capsule else 22
            for visit in range(3):
                mover = visit == 1 and a != b
                a_current = 200 + ((a+b)/2 if mover else a if visit == 0 else b)
                r_current = 200 + (c if visit == 0 else d)
                a_previous = 200+(a if visit < 2 else b)
                r_previous = 200+(c if visit < 2 else d)
                a_broad = 190 if mover else 70
                r_travel = abs(d-c) if visit == 1 else 0
                # Keep the observed original binary32 result of the paired
                # distance/square sequence for the long offset, not a rounded rational.
                r_broad = 22+r_travel
                if capsule and visit:
                    r_broad = 252004.03125 if i == 2 and visit == 1 else r_broad**2
                broad = abs(a_current-r_current) <= a_broad+r_broad
                kernel = ("80031E50" if mover and capsule else "80030A3C" if mover or capsule else "80030500") if broad else None
                accepted = broad and visit == 1 and i in (1, 3)
                damage = 17 if accepted else 0
                ends = [r_current-15, r_current+15, r_previous-15, r_previous+15] if capsule else [0]*4
                fields = (capsule, i, visit, 2*int(mover), capsule*2, int(mover), 0,
                          *(bits(n) for n in (a_current, a_previous, a_broad, r_current, r_previous, r_broad, core, *ends)),
                          kernel or "00000000", int(broad), int(accepted), int(accepted), damage, 180-damage, int(accepted))
                if lines[pos] != "moving-receiver: " + " ".join(map(str, fields)):
                    raise ValueError(f"moving receiver geometry, narrow test or HP changed: {capsule}/{name}/{visit}")
                pos += 1
                samples.append({
                    "rebuild": visit+1, "attack_effective_shape": "capsule" if mover else "sphere",
                    "attack_center_x": a_current, "attack_previous_center_x": a_previous, "attack_broad_field": a_broad,
                    "receiver_center_x": r_current, "receiver_previous_center_x": r_previous,
                    "receiver_endpoints_x": ends[:2] if capsule else None,
                    "receiver_previous_endpoints_x": ends[2:] if capsule else None,
                    "receiver_core_radius": core, "receiver_broad_field": r_broad, "receiver_motion_sweep_latch": False,
                    "broad_accepted": broad, "narrow_kernel": kernel, "narrow_accepted": accepted if broad else None,
                    "kernel_received_current_geometry": True if broad else None,
                    "hp_removed": damage, "hp_after": 180-damage, "accepted_token": 1 if accepted else None,
                })
            first, last = a-c, b-d
            cases.append({
                "case": name, "receiver_descriptor": 11 if capsule else 9, "receiver_shape": "capsule" if capsule else "sphere",
                "attack_x_offsets": [a, b], "receiver_x_offsets": [c, d],
                "linear_simultaneous_reference_min_center_separation": 0 if first*last <= 0 else min(abs(first), abs(last)),
                "samples": samples,
            })
    return cases


def verify_source(image):
    sweep.verify_source(image)
    s.verify_text(image, TEXT)
    for site, target in ((0x8003A28C, 0x8020BA54), (0x8003A29C, 0x8020BA54)):
        s.link(image, site, target)
    if (image.read(0x802CFF98, 4, "data") != bytes.fromhex("8003A0C8")
            or image.read(0x802CFFA0, 4, "data") != bytes.fromhex("8003A260")):
        raise ValueError("moving receiver narrow preparation table changed")


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("moving receiver output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_source(image)
    s.read_metadata(s.MAPS / "gg4e08-sphere-sweep.json", SWEEP_SHA256)
    payload, tau = sweep.source_inputs(image, afs)
    cases = decode(s.fixture_report(fixture, dol, module, ("--moving-receiver",), payload))
    raw = s.encode({
        "kind": "gf-retail-moving-receiver", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "sphere_sweep_sha256": SWEEP_SHA256, "attack": tau, "receiver": "gg4e08:000a",
        "center_origin_x": 200, "capsule_pre_narrow_owner_offset": "1CC", "declared_pre_narrow_owner_offset": [0, 0, 0],
        "cases": cases,
        "notes": [
            "Both shapes bind once. The host changes their declared matrix translations and corresponding roots on the second rebuild; the third rebuild holds the endpoint pose.",
            "The receiver retains history and widens its broad field without setting the attack motion latch. Scratch operands at the original narrow kernel match the receiver's current center and, for the capsule, its current endpoints.",
            "The synchronized linear point-path distance is a comparison, not a recovered game algorithm. In these inlets a simultaneous crossing can miss outside the attack sweep's final receiver overlap, and parallel paths can hit despite remaining 120 units apart.",
            "The capsule scratch path can add an owner offset before narrow-phase comparison. That offset is zero here; no general moving physics or behavior under a nonzero correction is qualified.",
            "No live animation sampling, special attack flags outside the selected command, alternate list, stage, hardware or native gameplay is qualified.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases),
            "rebuilds": sum(len(row["samples"]) for row in cases),
            "hp_writes": sum(sample["accepted_token"] is not None for row in cases for sample in row["samples"])}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
