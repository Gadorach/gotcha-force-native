"""Verify the original Machine Red rifle's first carrier and attack-shape passes."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze.reaction_emit import HIT_BYTES, QUEUE, STEPS, STEP_SIZE, qualified_hit_block
from tools.analyze.retail_action_input import AFS_SHA256, archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import address, read_bounded
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage

SLICES = {
    0x80089A94: (88, "dcb8bf7edce7313e0e258ee526af82153e5e35eac0987938a42cccc6a270dde4"),
    0x8008997C: (280, "3d628d2b7b32b3cd2c7e0816370547898bb863673bd68351246c797720e57eab"),
    0x800C3CDC: (80, "8e10b3ca6fe1b434a8fd38f0d68cba22066ed681c1b1e3b7be71782025117be7"),
    0x800C3D2C: (860, "e53cd894e0eb38d5ae2e60d624b3bb74a8372b3bb561d6ef5287bdecf69e9c6a"),
    0x8008AC80: (400, "88d16df3bda187ed3cb011269a356a7d0646ca1c946dcd194664a5abf4a317d0"),
    0x80089AEC: (276, "03e4a562cd8742937f8f85f602f0f3a3a682457f8f5887accc0b15cbf24f4802"),
    0x8008A65C: (996, "92fa6953e7aea5fe06460450ea539bf738406dc0632251b89ccba4c973b3adf4"),
    0x8008A2BC: (788, "2c811080af75624f6664c00f6e11b229fdb83bea44b41482b53b2ed14d2f0528"),
}
HIT_BLOCK_SHA256 = "89e4a919e5989aad5b2fb36985d55859b8cfdf9387bec0149265cbf716b9aec3"
RELEASE = "rifle: 2 1 1 1 1 0 1 1 9 16 2 21 0"
BUCKET = re.compile(r"rifle-bucket:(?: [0-9]{1,3}){9}(?: -?[0-9]{1,4}\.[0-9]{5}){7}")
CASES = (
    ("scene-slot-inhibited", 0, 1, 0),
    ("empty-shape-pool", 1, 0, 0),
    ("attack-publication-suppressed", 1, 1, 1),
    ("attack-published", 1, 1, 0),
)


def source_shape(block):
    if len(block) != HIT_BYTES or sha256(block).hexdigest() != HIT_BLOCK_SHA256:
        raise ValueError("rifle hit block identity mismatch")
    if struct.unpack_from(">4b", block, QUEUE) != (15, -1, -1, -1):
        raise ValueError("rifle group zero does not select a singleton")
    step = STEPS + 15 * STEP_SIZE
    kind, flags, tuning = struct.unpack_from(">BxHH", block, step)
    center = struct.unpack_from(">3f", block, step + 12)
    radius, = struct.unpack_from(">f", block, step + 24)
    if (kind, flags, tuning, center, radius) != (0, 1, 0, (0, 0, 0), 20):
        raise ValueError("rifle descriptor 15 disagrees")
    return {
        "hit_block_sha256": HIT_BLOCK_SHA256, "group": 0, "descriptor": 15, "shape_kind": "sphere",
        "flags": flags, "tuning_index": tuning, "local_center": list(center), "base_radius": radius,
    }


def decode_report(report):
    if len(report) > 2048:
        raise ValueError("rifle bucket report extent")
    lines = report.splitlines()
    if len(lines) != 2 * len(CASES):
        raise ValueError("incomplete rifle bucket report")
    observations = []
    for i, (name, admit, pool, suppress) in enumerate(CASES):
        if lines[2*i] != RELEASE or BUCKET.fullmatch(lines[2*i+1]) is None:
            raise ValueError(f"malformed or unpaid rifle bucket observation: {name}")
        fields = lines[2*i+1].split()[1:]
        seen_admit, seen_pool, seen_suppress, phase, timer, audio_cut, bucket, local_phase, registry = map(int, fields[:9])
        center = list(map(float, fields[9:12]))
        velocity = list(map(float, fields[12:15]))
        radius = float(fields[15])
        launched = bool(admit and pool)
        if (seen_admit, seen_pool, seen_suppress, phase, timer, audio_cut, bucket, local_phase, registry) != (
            admit, pool, suppress, 1 if launched else 2 if admit else 0, 300 if admit else 0,
            int(launched), 49 if launched else 0, int(launched), int(launched and not suppress),
        ):
            raise ValueError(f"rifle bucket state disagrees: {name}")
        expected = (-149.4, 191, 365, 0, 0, 80, 32.46753) if launched else (0,) * 7
        if any(abs(a - b) > 0.0001 for a, b in zip((*center, *velocity, radius), expected, strict=True)):
            raise ValueError(f"rifle shape or launch geometry disagrees: {name}")
        observations.append({
            "case": name, "scene_slot_inhibited": not admit, "free_shape_cell": bool(pool),
            "carrier_attack_suppressed": bool(suppress), "stock_after": 1, "carrier_phase": phase,
            "carrier_timer": timer, "late_list_inserted": bool(audio_cut),
            "shape": {
                "bucket": bucket, "local_phase": local_phase, "center": center, "radius": radius,
                "initial_endpoints_match_center": True,
            } if launched else None,
            "launch_velocity": velocity if launched else None, "attack_registry_count": registry,
        })
    return observations


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("rifle bucket output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    for entry, (size, digest) in SLICES.items():
        if sha256(image.read(entry, size, "text")).hexdigest() != digest:
            raise ValueError(f"rifle bucket source signature mismatch at {address(entry)}")
    if struct.unpack(">I", image.read(0x803051E4, 4)) != (0x800C3D2C,):
        raise ValueError("rifle callback's phase zero target disagrees")
    muzzle = 0x803036CC
    _, joint, group = struct.unpack(">BBB", image.read(muzzle, 3))
    origin = struct.unpack(">3f", image.read(muzzle+4, 12))
    speed, = struct.unpack(">f", image.read(muzzle+44, 4))
    if (joint, group, origin[1:], speed) != (7, 0, (-9, 64), 80) or abs(origin[0] + 249.4) > 0.0001:
        raise ValueError("rifle muzzle, speed, or group source disagrees")
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        block = qualified_hit_block(RetailArchive(stream), 0x0600)
        if stamp != archive_stamp(stream):
            raise ValueError("rifle source AFS changed during extraction")
    shape = source_shape(block)
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--rifle-bucket"]
    result = run(argv, input=block, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"rifle bucket fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    observations = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-rifle-bucket", "revision": 1, "borg": "gg4e08:0600",
        "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "dispatch": {"bucket_pass": "80089A94", "bucket_pass_argument": 8, "actor_slot": 0, "carrier_bucket": 9,
                     "carrier_callback": "800C3CDC", "launch": "800C3D2C", "late_list_insert": "80089100",
                     "stop_before_audio": "800F036C", "class4_pass": "80089AEC", "shape_callback": "8008A65C",
                     "attack_registry": "803C477C"},
        "rifle": {"type": 2, "key": 21, "muzzle_row": address(muzzle), "matrix_key": joint,
                  "local_offset": list(origin), "speed_operand": speed, "source_shape": shape},
        "declared": {"key_7_translation": [100, 200, 300], "credit_forward": [0, 0, 1],
                     "launch_target": None, "stock_before_release": 2, "class2_effect_pool_cells": 0,
                     "visual_payload": None},
        "cases": observations,
        "notes": [
            "This joins the installed action, VM cue and paid original allocation to original bucket traversal on supplied scratch state.",
            "The successful bucket pass stops before sound after the original late insert. The class-4 pass then runs independently without resuming that stack.",
            "Pool capacity, owner model mirror and pose, null launch target, and missing visual payload are declared fixture inputs.",
            "No carrier, shape or attack-registry pointer is seeded. No contact consumer, HP path, retirement or running match is qualified.",
        ],
    }
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode("ascii")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(observations)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
