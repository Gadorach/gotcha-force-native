"""Replay model-relative type-32 aim, request cadence, and type-11 endpoint reads."""
import argparse
from dataclasses import dataclass
from hashlib import sha256
import json
import math
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import reaction_emit, reaction_installer_tables
from tools.analyze import retail_type32_bind as borrowed, retail_type32_roster as roster
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import address, read_bounded
from tools.analyze.retail_projectile_census import AFS_SHA256, SOURCE_SHA256, encode
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage


@dataclass(frozen=True)
class Pilot:
    model: int
    key: int
    group: int
    descriptor: int
    index: int
    table: int
    power: int
    offset: tuple[int, int, int]
    scale: float
    aim: bool
    digest: str


PILOTS = (
    Pilot(0x0104, 1, 10, 9, 9, 0x803214E0, 3, (0, 25, 350), 3.0, True,
          "74ac700aa52dd84e1c5486a58fa51b2f5a3d29f95a3f7c2135c0eb3e72ad27f7"),
    Pilot(0x0505, 16, 1, 1, 1, 0x803928D8, 4, (0, 0, 200), 5.0, False,
          "a06f57db65b5aac679ad3ee62e7682580d47dd09e689d8ea86df7737c1095439"),
    Pilot(0x0511, 19, 1, 1, 1, 0x80392908, 2, (0, 0, 100), 2.5, False,
          "f24fea7959f3f717da5188f67031cddc48d7ddfdb3f5ac41ce9464101788483b"),
    Pilot(0x0615, 15, 27, 34, 31, 0x80365EC0, 1, (0, 40, 10), 1.0, True,
          "da9d7d33b1c8b66f93c0beb7572e5140f82ae120558e5f908398526fbd37e4c3"),
)
CASES = ("no-target", "copied-target-moves", "copied-target-outside-gate", "half-rate")
ROW = re.compile(r"type32-shared:(?: [0-9]{1,4}){6}(?: -?[0-9]{1,5}\.[0-9]{5}){12}")
TEXT = (
    (0x800E245C, 256, "6bd4d1caf1f3423082b8856061480840989b51635ed38410e9bfabdf02cdbe3f"),
    (0x800E255C, 184, "9a4375cf0b210ab01fd532cde037bb4b2dd2a0e480c35dbc34e3efe5423ef849"),
    *tuple(span for span in borrowed.TEXT if span[0] in (0x80006DC8, 0x80088C40, 0x80139B80, 0x80139C50)),
)


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != len(PILOTS)*len(CASES):
        raise ValueError("type-32 shared report extent")
    result = []
    for i, line in enumerate(lines):
        pilot = PILOTS[i//len(CASES)]
        case = i % len(CASES)
        if not ROW.fullmatch(line):
            raise ValueError("malformed type-32 shared row")
        words = line.split()[1:]
        if tuple(map(int, words[:6])) != (pilot.model, pilot.key, case, pilot.group, 2, int(pilot.key != 1)):
            raise ValueError(f"type-32 shared group or request mismatch: {pilot.model:04X}")
        operands = list(map(float, words[6:]))
        coordinates = [operands[j:j+3] for j in range(0, len(operands), 3)]
        corrected = pilot.aim and case == 1
        second_visit = 3 if case == 3 else 2
        origin = [a+b for a, b in zip((100, 200, 300), pilot.offset, strict=True)]
        for j, measured in enumerate(coordinates):
            visit = second_visit if j >= 2 else 1
            length = (15110 if j % 2 else 10)*pilot.scale*visit/15
            magnitude = math.hypot(300, 2000)
            dx = (300 if j < 2 else -300)/magnitude if corrected else 0
            dz = 2000/magnitude if corrected else 1
            expected = (origin[0]+dx*length, origin[1], origin[2]+dz*length)
            if any(abs(a-b) > 0.005 for a, b in zip(measured, expected, strict=True)):
                raise ValueError(f"type-32 shared refreshed endpoint mismatch: {pilot.model:04X}/{CASES[case]}")
        result.append({
            "borg": f"gg4e08:{pilot.model:04x}", "carrier_key": pilot.key, "case": CASES[case],
            "owner_clock_units_per_visit": 0.5 if case == 3 else 1,
            "copied_target_changes_direction": corrected, "empty_scratch_request": pilot.key != 1,
            "emission_active_visits": [1, second_visit], "gate_closed_on_active_visit": second_visit+1,
            "first_request": {"near": coordinates[0], "far": coordinates[1]},
            "second_request": {"near": coordinates[2], "far": coordinates[3], "carrier_available": False},
            "child": {"group": pilot.group, "first_sample": coordinates[0], "next_sample": coordinates[3]},
        })
    return result


def qualified_inlet(image, archive):
    _, rows = roster.row_index(image)
    roster.verify_text(image, TEXT)
    roster.link(image, 0x800E1C40, 0x80006DC8)
    if (struct.unpack(">2I", image.read(0x80339660, 8, "data")) != (0x80139BBC, 0x80139C50)
            or struct.unpack(">7f", image.read(0x8043EA20-23640, 28, "data"))[0:2] != (0, 15)
            or struct.unpack(">4f", image.read(0x8043EA20-23628, 16, "data")) != (1, -1, 10, 15100)):
        raise ValueError("type-32 child phase or shared active operands changed")
    sources, blocks = [], []
    for pilot in PILOTS:
        row = rows[roster.SHARED, pilot.key]
        data = image.read(int(row["row"], 16), 32, "data")
        scratch_selector, scale = struct.unpack_from(">hf", data, 6)
        dot_gate, = struct.unpack_from(">f", data, 28)
        block = reaction_emit.qualified_hit_block(archive, pilot.model)
        if (sha256(block).hexdigest() != pilot.digest
                or reaction_installer_tables.model_table(image, pilot.model)[1] != pilot.table
                or reaction_emit.read_group(block, pilot.group) != [(pilot.descriptor, pilot.index)]
                or image.read(pilot.table+24*pilot.index, 2, "data") != struct.pack(">H", pilot.power)
                or row["collision_path"]["selector"] != pilot.group or row["source_socket_offset"] != pilot.offset
                or scratch_selector != (-1 if pilot.key == 1 else 85)
                or scale != pilot.scale or data[28:32] != bytes.fromhex("3f6c8356" if pilot.aim else "3f800000")):
            raise ValueError(f"type-32 shared exact input mismatch: {pilot.model:04X}")
        blocks.append(block)
        sources.append({"borg": f"gg4e08:{pilot.model:04x}", "carrier_key": pilot.key, "row": row["row"],
                        "socket": row["source_socket_selector"], "socket_offset": pilot.offset,
                        "scale_operand": scale, "target_dot_gate": dot_gate, "scratch_selector": scratch_selector,
                        "descriptor": pilot.descriptor, "reaction_index": pilot.index, "reaction_table": address(pilot.table),
                        "raw_power": pilot.power, "hit_block_sha256": pilot.digest})
    return b"".join(blocks), sources


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type-32 shared output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        inlet, sources = qualified_inlet(image, RetailArchive(stream))
        if stamp != archive_stamp(stream):
            raise ValueError("type-32 shared AFS changed during extraction")
    args = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--shared"]
    result = run(args, input=inlet, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"type-32 shared fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-type32-shared", "revision": 1, "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "sources": sources, "cases": cases,
        "entries": {"factory": "800E19A8", "carrier_update": "800E1AB4", "model_relative_active": "800E1CD8",
                    "scratch_request": "800E1C40", "segment_request": "800E20C0", "segment_update": "80139B80"},
        "notes": [
            "The shared factory and initializer return normally with an empty render record and no scratch allocation.",
            "Socket matrices, the positive producer byte, and target positions are declared inputs. The first active update is visit 1.",
            "The original target pointer is copied at construction; changing the owner's pointer does not retarget the carrier.",
            "The original active callback can still sample the old target's changed position when its dot gate passes.",
            "The second request is observed without a second free carrier. The first segment samples the updated far endpoint.",
            "No terrain or receiver pass runs. Occlusion, paid admission, collision, HP, and native gameplay remain unqualified.",
        ],
    }
    raw = encode(payload, expanded=("sources", "cases"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
