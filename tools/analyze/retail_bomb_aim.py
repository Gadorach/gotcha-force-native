"""Verify the separate aim-point, launch gate, and in-flight type-15 operands."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import reaction_emit, reaction_installer_tables, retail_bomb_x_cue as cue
from tools.analyze.retail_action_input import AFS_SHA256, archive_stamp, verify_archive
from tools.analyze.retail_bomb_first_bind import SLICES as FIRST_BIND_SLICES
from tools.analyze.retail_code_atlas import address, read_bounded
from tools.analyze.retail_small_projectile_roster import NINJA_BOMBS
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage


PILOTS = {
    0: (0x802D4160, "8d90bfdd7a4d598b4c0869fff93364b785d4cc98c876d56c99ce44fc86579414"),
    3: (0x8033B198, "787114791beb040ca95a23bc15b0e98a23c170ed775cd41eee3a9e7ffec6e70c"),
    10: (0x802D4160, "8d90bfdd7a4d598b4c0869fff93364b785d4cc98c876d56c99ce44fc86579414"),
    12: (0x80376960, "0e834cbf67276635456f28fd2e5c1a82ce6eaa69f693a9a8cce6a43e282daa4f"),
}
SLICES = {
    **{pc: FIRST_BIND_SLICES[pc] for pc in (0x80089A94, 0x80089898, 0x8008997C, 0x8007DCEC,
                                           0x8007DD84, 0x8007E200, 0x8008AC80)},
    0x8006BDA8: (472, "175c8cf36b7dc4bb7bc5c76ce86651b067ff08ff5cfdc0eb0c394411acb28dc9"),
    0x80047AA4: (404, "d1034c09b0f68dabce25aa8dc2fd55eb99b31ee2e017cf379fad8fa7c23eb779"),
}
BODY = (0.0, 0.0, 40.0)
DIAGONAL = (23.97356, 1.87707, 31.96474)
REFERENCE_POINT = (465, 200, 700)
# Label, copied references, live launch gate, tracking after four visits,
# remaining refit countdown, launch origin X, launch vector, gravity after visit four.
CASES = (
    ("no-target-with-leftover-cache", 0, 0, 0, 4, 165, BODY, .3),
    ("cached-diagonal-point", 1, 1, 1, 4, 165, DIAGONAL, .3003559),
    ("owner-cache-overwritten-before-launch", 1, 1, 1, 4, 165, DIAGONAL, .3003559),
    ("launch-gate-added-after-release", 0, 1, 0, 4, 165, DIAGONAL, .3),
    ("launch-gate-removed-after-release", 1, 0, 1, 4, 165, BODY, .15),
    ("high-point-pitch-cap", 1, 1, 1, 4, 165, (16.72096, 28.69424, 22.29461), .15),
    ("point-below-and-left", 1, 1, 1, 4, 165, (-22.24162, -15.02868, 29.65550), .2625512),
    ("validity-lost-then-restored", 1, 1, 0, 3, 165, DIAGONAL, .3),
    ("tracking-point-moved-before-refit", 1, 1, 1, 4, 165, DIAGONAL, .15),
    ("later-socket-shift", 1, 1, 1, 4, 215, (21.17917, 1.77059, 33.88668), .3003326),
)
ROW = re.compile(r"bomb-aim:(?: [0-9]{1,3}){9}(?: -?[0-9]{1,2}\.[0-9]{5}){3} [0-9]\.[0-9]{7}")


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != len(PILOTS)*len(CASES):
        raise ValueError("bomb aim report extent")
    shared = []
    for rank, model in enumerate(PILOTS):
        for index, (name, copied, gate, tracking, clock, x, velocity, gravity) in enumerate(CASES):
            line = lines[rank*len(CASES)+index]
            if ROW.fullmatch(line) is None:
                raise ValueError("malformed bomb aim row")
            tokens = line.split()[1:]
            counts, numeric = tuple(map(int, tokens[:9])), tuple(map(float, tokens[9:]))
            if counts != (model, index, copied, gate, tracking, clock, x, 200, 300) or numeric != (*velocity, gravity):
                raise ValueError(f"bomb aim result mismatch: {model:04X}/{name}")
            if rank == 0:
                point = (465, 9000, 700) if index == 5 else (-135, -30, 700) if index == 6 else REFERENCE_POINT
                shared.append({
                    "case": name, "point_copied_at_release": list(point),
                    "original_target_refresh_used": bool(copied), "target_references_copied_at_release": bool(copied),
                    "live_caster_gate_at_launch": bool(gate), "key_11_origin_at_launch": list(counts[6:9]),
                    "velocity_before_first_active_visit": list(numeric[:3]),
                    "fourth_active_visit": {"tracking_reference_retained": bool(tracking),
                                            "refit_countdown_after": clock, "gravity": numeric[3]},
                })
    return shared


def qualified_inlet(image, archive):
    cue.qualify(image)
    for pc, (size, digest) in SLICES.items():
        if sha256(image.read(pc, size, "text")).hexdigest() != digest:
            raise ValueError(f"bomb aim source signature mismatch at {address(pc)}")
    if image.read(0x804379CC, 4, "data") != bytes.fromhex("3f4ccccd"):
        raise ValueError("bomb aim pitch cap identity")
    sources, blocks = [], []
    for model, (table, digest) in PILOTS.items():
        block = reaction_emit.qualified_hit_block(archive, model)
        release, key = NINJA_BOMBS[model][0]
        row = 0x802D6D68+56*key
        if (sha256(block).hexdigest() != digest or reaction_emit.read_group(block, 1) != [(1, 1)]
                or reaction_installer_tables.model_table(image, model)[1] != table
                or struct.unpack(">fI", image.read(row+16, 8, "data")) != (40, 4)
                or image.read(row+28, 16, "data") != bytes.fromhex("0000000040f000003e19999a3e99999a")):
            raise ValueError(f"bomb aim operands or normal hit block mismatch: {model:04X}")
        blocks.append(block)
        sources.append({
            "borg": f"gg4e08:{model:04x}", "release_site": address(release), "key": key, "factory_row": address(row),
            "group": 1, "reaction_table": address(table), "normal_hit_block_sha256": digest,
        })
    return b"".join(blocks), sources


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("bomb aim output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        inlet, sources = qualified_inlet(image, RetailArchive(stream))
        if stamp != archive_stamp(stream):
            raise ValueError("bomb aim AFS changed during extraction")
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--aim"]
    result = run(argv, input=inlet, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"bomb aim fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-bomb-aim", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "pilots": sources, "common_cases_verified_for_each_pilot": cases,
        "entries": {"declared_target_refresh": "8006BDA8", "factory": "8007DB5C", "initializer": "8007DD84",
                    "matrix_update_call_before_first_active_visit": "8007E1D0", "active_visit": "8007E200"},
        "operands": {"launch_speed": 40, "initial_gravity": .3, "refit_interval": 4,
                     "gravity_limits": [0, 7.5], "max_gravity_change_per_refit": .15, "maximum_upward_pitch_rad": .8},
        "notes": [
            "Each fixture starts at the paid release leaf. With both selected references declared, the original refresh populates the owner's aim and validity references and caches the first reference's point.",
            "The constructor copies the cached point and both target references. The initializer tests the caster's live aim reference, but reads the carrier's stored point and the later supplied key-11 pose.",
            "In flight the separate validity reference can clear tracking without choosing a new target. Gravity can refit using the tracked live point without changing horizontal heading.",
            "The first three active visits retain 0.3 gravity. The receipt observes the fourth, not a whole-flight intercept.",
            "Target selection, matrices, and pose are declared; no global scheduler, moving-body hit, or native launch rule is established.",
        ],
    }
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode("ascii")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "pilots": len(sources), "cases": len(cases)*len(sources)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
