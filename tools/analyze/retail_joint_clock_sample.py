"""Replay the original SDK sampler on one isolated Machine Red joint and exact track."""
import argparse
from hashlib import sha256
import json
import math
from pathlib import Path
import re
import struct

from tools.analyze import retail_contact_clock_consumers as consumers, retail_contact_pose_clock as clock
from tools.analyze import retail_contact_effect_source_art as art, retail_type32_roster as beam
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import AFS_SHA256, SOURCE_SHA256, encode, read_metadata
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import BLOCK_LIMIT, RetailArchive, bank_clips, logical_joints, wrapper_model
from tools.extract.source_image import DolImage


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"
CONSUMERS_SHA256 = "c3488c0f4d5055d6ac70cbaaba4cacd52d9d369157fa232820830f2ed99def20"
ARCS = {
    "wrapper": "2362a75513d8f9748b047d0430a39ca4b5571256e81e7282290ebf988c137b21",
    "body": "81b09b93b64617f8792141bccbaf7fe201f052d8d3953a775ab6f382b41216fb",
    "motion": "bbaeaba76e322bce48e307c7695bb43c026d008a4fa3102369ad010c1addc78f",
    "clip": "d414a1cb532d6dc3d12bf429882d839ee4819743c148c20cb3ec2056512d3150",
}
ROW = re.compile(r"joint-clock:(?: [0-9]){3}(?: [0-9A-F]{8}){9}")
TEXT = (
    *tuple(r for r in art.TEXT if r[0] != 0x80040EB4),
    *tuple(r for r in consumers.TEXT if r[0] == 0x8004C298),
    (0x8024DC04, 320, "91e13b973a46a09e0e49268446047c63d3b1de70f9c0e74ef181575d0b7a81e4"),
    (0x8024D838, 136, "4fcc4a7ac908096598b2516a7afb5ab4b6f7a567de11185047a886322257eba6"),
    (0x80243FC0, 16, "57f7c52fe8f057dc353df840ce0de461adcdd8b40ad51ab82845ffec728ef8b5"),
    (0x8024EA64, 356, "a172b47a101e2f4eddda11943eab568dce973a8f77fa4aeb9f70f440cbaa0d38"),
    (0x80243BD0, 440, "dd5555ed929b4f8b047ff6d91a36f2c304089c4a7d51d1e265adb959931298e8"),
    (0x8024A198, 112, "bcf7c4c9b6022403f2418af170535e1b0e4ac28f37b73cea489370398cb7344d"),
    (0x80249AC4, 1748, "ab8ed02d2a35df72c8d07efb7ea76805a6f6edc9cb203d53768ba0c9640195af"),
    (0x8004B76C, 392, "b1add0ae0c57f064da9c8f145855c05634c964bc248cb4a38e236da629ef83d2"),
    (0x8025ACFC, 328, "bd4ffdacfc871236f778f38d80b703de8faae90f86d4fa215edee3da470cc368"),
    (0x8025983C, 136, "32ea344ae9fcb34441fb16ae74102f636195181176fddacfd7baf078df4d12c7"),
    (0x8024A208, 552, "795a2105d508f343ce69b86ab838dbe2f8083211b3cda6088f73f5093b4e7fa3"),
    (0x80243D88, 296, "769dc38459343e81e3bcf1ffb3d20900ec338358d74b3ccbd13d71d86156178c"),
    (0x802488B8, 164, "b5f850795387a7fabffdbbfd933a642b698c0c058cc5f24a373814ad790feeab"),
)


def verify_source(image):
    clock.verify_source(image)
    beam.verify_text(image, TEXT)
    for pc, target in (
        (0x8004C3A4, 0x80048648), (0x8024DC50, 0x80243D88), (0x8024D86C, 0x80243B90),
        (0x80243BBC, 0x802488B8), (0x8024EB88, 0x80243BD0), (0x80243CF4, 0x8024A198),
        (0x8024A1D8, 0x80249AC4), (0x8004B864, 0x8025ACFC),
    ):
        beam.link(image, pc, target)
    if image.read(0x802C2F56, 8, "data") != bytes.fromhex("010003000000ffff"):
        raise ValueError("joint clock probe opcode changed")


def exact_archives(afs):
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        archive = RetailArchive(stream)
        if archive.names[575] != b"pl0600.pzz" or archive.names.count(b"pl0600.pzz") != 1:
            raise ValueError("joint clock member changed")
        bank, _ = archive.member(575, 0x0600)
        motion, _ = bank.read(3, BLOCK_LIMIT)
        wrapper, _ = bank.read(4, BLOCK_LIMIT)
        if archive_stamp(stream) != stamp:
            raise ValueError("joint clock AFS changed during extraction")
    prefix, body = wrapper_model(wrapper)
    hits = [pos for group, choice, pos in bank_clips(motion) if (group, choice) == (0, 9)]
    if hits != [0x13FC0]:
        raise ValueError("joint clock clip slot changed")
    start = hits[0]
    size, = struct.unpack_from(">I", motion, start)
    clip = motion[start:start+size]
    if {k: sha256(blob).hexdigest() for k, blob in (
        ("wrapper", wrapper), ("body", body), ("motion", motion), ("clip", clip),
    )} != ARCS:
        raise ValueError("joint clock archive identity changed")
    bones = logical_joints(prefix, body)
    if len(bones) != 32 or bones[2]["descriptor_offset"] != 0x13A88 or bones[2]["child_next_parent"] != (3, 13, 1):
        raise ValueError("joint clock original logical joint changed")
    blob = memoryview(clip)[32:32+5612]
    if struct.unpack_from(">IfII", blob, 0x1A8) != (0, 24., 0x16C, 0):
        raise ValueError("joint clock AObj descriptor changed")
    table = [
        (0x16C, (0x180, 29, 0., 1, 46, 46, 0, 0xEC)),
        (0x180, (0x194, 45, 0., 2, 0, 0, 0, 0x10C)),
        (0x194, (0, 45, 0., 3, 0, 0, 0, 0x13C)),
    ]
    tracks = []
    for off, expected in table:
        actual = struct.unpack_from(">IIfBBBBI", blob, off)
        if actual != expected or actual[-1]+actual[1] > len(blob):
            raise ValueError("joint clock FObj descriptor changed")
        tracks.append({"descriptor_offset": off, "channel": actual[3], "stream_offset": actual[-1], "stream_bytes": actual[1]})
    return body+clip, tracks


def decode(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != 16:
        raise ValueError("joint clock report extent")
    rows = []
    cases = ("relative-clear", "relative-seed-4", "explicit-clear", "explicit-seed-4")
    for i, name in enumerate(cases):
        time = None
        seed = 4 if i & 1 else 0
        for visit in range(1, 5):
            remaining = max(0, seed-visit)
            rate = consumers.f32(.03) if remaining else .375
            time = (2.5 if i & 2 else consumers.f32(6.+rate)) if time is None else consumers.f32(time+rate)
            line = lines[len(rows)]
            fields = line.split()
            expected = [str(i), str(visit), str(remaining), consumers.bits(rate), consumers.bits(time)]
            if ROW.fullmatch(line) is None or fields[1:6] != expected:
                raise ValueError(f"joint clock request or sample time changed: {name}/{visit}")
            values = struct.unpack(">7f", bytes.fromhex("".join(fields[6:])))
            if not all(math.isfinite(v) for v in values) or abs(sum(q*q for q in values[3:])-1) > 0.0001:
                raise ValueError("joint clock non-finite angles or non-unit orientation")
            rows.append({
                "case": name, "declared_timer_seed": seed, "visit": visit, "body_timer_after": remaining,
                "aobj_rate_bits": fields[4], "sample_frame_bits": fields[5], "euler_bits": fields[6:9],
                "local_quaternion_bits": fields[9:13],
            })

    def pose(i):
        return rows[i]["euler_bits"], rows[i]["local_quaternion_bits"]

    if pose(0) == pose(4) or pose(8) != pose(12) or pose(9) == pose(13):
        raise ValueError("joint clock frame differences did not carry through the observed pose")
    return rows


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("joint clock output already exists")
    dol, module = (Path(p).resolve(strict=True) for p in (dol, module))
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    read_metadata(MAPS / "gg4e08-contact-clock-consumers.json", CONSUMERS_SHA256)
    verify_source(image)
    payload, tracks = exact_archives(afs)
    rows = decode(clock.fixture_report(fixture, dol, module, payload=payload))
    data = {
        "kind": "gf-retail-joint-clock-sample", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "contact_clock_consumers_sha256": CONSUMERS_SHA256,
        "source_joint": {"borg": "gg4e08:0600", "afs_member": 575, "archive_sha256": ARCS, "record": 0,
                         "logical_joint": 2, "joint_desc_offset": 0x13A88, "source_links": [3, 13, 1],
                         "isolated_instance": True, "source_flags": "10000000", "loaded_flags": "10000040"},
        "track": {"clip_slot": [0, 9], "archive_offset": 0x13FC0, "animjoint": "pl0600_b02_animjoint",
                  "animjoint_offset": 0x136C, "aobj_desc_offset": 0x1A8, "end_frame": 24, "channels": tracks},
        "entries": {"archive_relocate": "8025B00C", "joint_load": "8024F834", "track_attach": "8024DC04",
                    "frame_reset": "8024D838", "rate_write": "80243FC0", "joint_sample": "8024EA64",
                    "actor_local_publish": "8004B76C", "quaternion_math": "8025ACFC"},
        "samples": rows,
        "notes": [
            "Original heap, archive relocation, joint class construction, and track attachment run. Only the child and sibling fields of a copied key-2 descriptor are cleared to isolate this instance.",
            "The motion archive and three Euler track streams are unchanged; none of their values are supplied by a host sampler.",
            "The body clock timer is declared. The indexed relative-frame probe supplies the request frame but does not select Machine Red's clip.",
            "Rate assignment does not advance the frame. The first joint sample uses the requested frame; subsequent samples consume the current secondary rate.",
            "The original local publish copies translation and computes the quaternion. The scoped output is not a world matrix, live skeleton, boot return, or native gameplay.",
        ],
    }
    raw = encode(data, expanded=("samples",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "samples": len(rows), "channels": len(tracks)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
