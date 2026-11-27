"""Replay original post-contact rates through four narrowly scoped consumers."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct

from tools.analyze import retail_contact_pose_clock as clock, retail_melee_pose_clock as melee
from tools.analyze import retail_program_cues as cues, retail_type32_roster as beam
from tools.analyze.gred_animation_controller import decode_program
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import (
    AFS_SHA256, ATLAS_SHA256, SOURCE_SHA256, address, callers, encode, read_metadata,
)
from tools.extract.source_image import DolImage


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"
MELEE_REPLAY_SHA256 = "7bb38ad95e772e1d0e3e3527a647d88319e757ef2695ce430694c1c7576af682"
PROGRAM_SHA256 = "fa2a0b4b706f64148184f67b3bf8e5e56974a949844f1a39e47786d7124993f8"
COMMANDS = (
    ("terminal-wait", 0x802BD65E, 0x802BD74A, 0x802BD766, "00090001"),
    ("relative-frame-clip", 0x802C2E60, 0x802C2F56, 0x802C2F56, "010003000000ffff"),
)
COUNTS = {0x800676D8: (4, 4), 0x80066E40: (45, 38), 0x8004CD24: (1128, 1053), 0x8004C298: (0, 0)}
ROW = re.compile(r"clock-consumers:(?: [0-9]{1,4}){4}(?: [0-9A-F]{8}){3}(?: [0-9]{1,4}){4}(?: [0-9A-F]{8}){2}")
TEXT = (
    (0x8004C050, *cues.SPANS[0x8004C050]), (0x8004CD24, *cues.SPANS[0x8004CD24]),
    (0x8004C298, 396, "42c560d248bf6cdff40bb3e0f93a73b6ce65af55eaef0fd4874a521daa6a2188"),
    (0x80049E58, 184, "373e73e020a9bdcb8900885a993e6efdf782aa8f79cb9b3a3a3bb02406f25b76"),
    (0x80066E40, 128, "807b93b68a6d9f8c01955cc3577a4b5919ee2b0ebd5b6b1bf4cef223cbb761ac"),
    (0x800676D8, 124, "e96e1cce4d0250a0fe62cd79f37735bd66a971bf1147ccd38f492d5feebace65"),
)


def bits(value):
    return struct.pack(">f", value).hex().upper()


def f32(value):
    return struct.unpack(">f", struct.pack(">f", value))[0]


def decode(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != 22:
        raise ValueError("contact rate consumer report extent")
    result = []
    for route in melee.ROUTES:
        for case in (1, 2):
            armed = route.seed if case == 2 else 0
            wait, gap, z = 1.0, 1024, 325
            for visit in range(1, max(3, armed+2)+1):
                left = max(0, armed-visit)
                p, s = (f32(.03), f32(.03)) if left else (1., .375)
                advance = 3 if left else 100
                z += advance
                turn = int(f32(f32(gap*f32(.04))*s))
                gap -= turn
                wait = max(0., f32(wait-s))
                done = wait == 0.
                sample = f32(6.+s)
                row = (route.model, case, visit, left, bits(p), bits(s), bits(wait), int(done), z, turn, gap,
                       bits(sample), bits(2.5))
                line = lines[len(result)]
                if ROW.fullmatch(line) is None or line.split()[1:] != [str(v) for v in row]:
                    raise ValueError(f"contact motion, wait, heading or clip changed: {route.model:04X}/{case}/{visit}")
                result.append({
                    "attacking_borg": f"gg4e08:{route.model:04x}", "case": "accepted-hit" if case == 2 else "whiff-control",
                    "receiver_hp": 180-route.lost if case == 2 else 180, "initial_timer": armed, "visit": visit,
                    "timer_after": left, "primary_bits": bits(p), "secondary_bits": bits(s),
                    "lane_wait_bits": bits(wait), "lane_complete": done,
                    "z_after_planar_advance": z, "planar_advance": advance, "heading_step": turn, "heading_gap_after": gap,
                    "relative_clip_frame_bits": bits(sample), "explicit_clip_frame_bits": bits(2.5),
                })
    return result


def scope(image, atlas, program_map):
    beam.verify_text(image, TEXT)
    for site, target in ((0x8004C3A4, 0x80048648), (0x800676D8+56, 0x80045204), (0x800676D8+76, 0x80045238)):
        beam.link(image, site, target)
    if image.read(cues.OPCODE_TABLE+4, 4, "data") != bytes.fromhex("8004C298"):
        raise ValueError("relative frame clip opcode dispatch changed")
    samples = []
    for name, bank, program, command, raw in COMMANDS:
        bank_id, program_id = address(bank), address(program)
        if program_id not in program_map["programs"] or not any(
            program_id in r.get("choices", ()) for r in program_map["banks"][bank_id]["groups"]
        ):
            raise ValueError("contact rate command is no longer in the indexed bank")
        decoded = decode_program(lambda pc, size: image.read(pc, size, "data"), program, max_commands=32, max_span=128)
        if decoded["status"] != "terminal_wait" or not any(
            row["address"] == command and row["raw"] == raw for row in decoded["commands"]
        ):
            raise ValueError("contact rate wait or clip command changed")
        samples.append({"probe": name, "source_bank": bank_id, "program": program_id, "command": address(command)})
    links = {}
    for entry, expected in COUNTS.items():
        found = callers(image, atlas, entry, 2048)
        counts = (len(found), len({row["function"] for row in found.values()}))
        if counts != expected:
            raise ValueError("contact rate direct call census changed")
        links[address(entry)] = {"direct_sites": counts[0], "parent_functions": counts[1]}
    return samples, links


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("contact rate consumer output already exists")
    dol, module = (Path(x).resolve(strict=True) for x in (dol, module))
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    read_metadata(MAPS / "gg4e08-melee-pose-clock.json", MELEE_REPLAY_SHA256)
    samples, links = scope(image, read_metadata(MAPS / "gg4e08-code-atlas.json", ATLAS_SHA256),
                          read_metadata(MAPS / "gg4e08-program-cues.json", PROGRAM_SHA256))
    payload, _, _ = melee.qualified_inputs(image, afs)
    rows = decode(clock.fixture_report(fixture, dol, module, ("--consumers",), payload))
    data = {
        "kind": "gf-retail-contact-clock-consumers", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "code_atlas_sha256": ATLAS_SHA256,
        "melee_pose_clock_sha256": MELEE_REPLAY_SHA256, "program_cues_sha256": PROGRAM_SHA256,
        "entries": {"planar_advance": "800676D8", "heading_step": "80066E40", "wait_install": "8004C050",
                    "lane_wait_update": "8004CD24", "clip_request": "8004C298", "clip_cut": "80048648"},
        "source_commands": samples, "direct_call_census": links, "visits": rows,
        "notes": [
            "The two exact melee hits and corresponding whiffs leave the normal Sasuke receiver in guest memory. Only its clock is visited.",
            "Movement speed, heading gap, lane mask and reference frame are declared. The original wait is installed by the VM from its indexed command.",
            "The wait and clip are source probe commands, not inferred choices of this receiver's controller.",
            "The planar helper reads the primary rate. Heading, the finite wait, and the relative-frame clip request read the secondary rate.",
            "The explicit frame bypasses the relative rate calculation. Both clip probes stop at the original request before sampling any pose.",
            "Direct call counts are static, not qualified call schedules. No live movement sweep, actor frame loop, pose playback or native gameplay runs.",
        ],
    }
    raw = encode(data, expanded=("source_commands", "visits"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "visits": len(rows), "slow_visits": sum(bool(r["timer_after"]) for r in rows)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
