"""Join the late actor eligibility byte to the next melee shape publication."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct

from tools.analyze import retail_contact_pose_clock as clock, retail_melee_pose_clock as melee
from tools.analyze import retail_type32_roster as beam
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_contact_clock_consumers import MELEE_REPLAY_SHA256
from tools.analyze.retail_projectile_census import (
    AFS_SHA256, ATLAS_SHA256, INSTALLERS_SHA256, SOURCE_SHA256, address, encode, read_metadata,
)
from tools.extract.source_image import DolImage


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"
PREFIX_SHA256 = "1e1f95f75fe1c24f47286fdc645446c71cedceb6591bd7b52ec87a5a63738e0a"
AMOUNT_ROW = re.compile(r"frame-amount:(?: [0-9]{1,4}){4}")
ADMISSION_ROW = re.compile(r"frame-admission:(?: [0-9]{1,4}){11}")
ACTIVE_LOSS = {0x0807: 17, 1: 35}
INSTALLED = {
    0x0807: ("801A04F0", "801A05F0", "801A05F4", "801A0610", 0x80377808),
    1: ("800EA500", "800EA61C", "800EA6A0", "800EA6B8", 0x8031B068),
}
TEXT = (
    (0x80055568, 292, "4ef4d2db70068dec76e280773b3d58c546452599966914bffd4f8cbaf56472c7"),
    (0x80055EE4, 172, "f9d106d64326e759fd9b72a619c6bde22a12cc53c0c1a7da4ae9e8f1d40c0ca2"),
    (0x8003CC24, 312, "7fcf39fdb005df817b0de481e6860f17a2a8bb8c0d9fc7b82cb11882b2c9b415"),
    (0x801A04F0, 252, "21ce77a7dc7fffbe340cf0de925f8bbe68c29aa3a9dd0b243cf8522cd1cd2b92"),
    (0x800EA500, 252, "e70e8739559718c93f54da18005abab9c99859091f913aa667ad6b1709ab40f5"),
    (0x801A05F4, 24, "bd21c94a26c1012a3df64a5d311c9c12c29b173b32dcf41834e2f572ce01899c"),
    (0x800EA6A0, 20, "f10e2e6980bd76ebbc8f3cddcbd0b703db1620afe5c1d3b3b959ec741ee464e6"),
)
USES = {
    0x8003CC88: ("8003CC24", 0x98030276, "separate store, not replayed"),
    0x80055F64: ("80055EE4", 0x98030276, "late close"),
    0x80055F70: ("80055EE4", 0x98030276, "late reopen"),
    0x8008A454: ("8008A2BC", 0x88050276, "ordinary attack-list admission"),
    0x8008A5A4: ("8008A2BC", 0x88050276, "other list, not replayed"),
}


def decode(report):
    lines = report.splitlines()
    if len(lines) != 8 or len(report) > 1024:
        raise ValueError("frame admission report extent")
    rows = []
    for i, route in enumerate(melee.ROUTES):
        count, active = len(route.steps), ACTIVE_LOSS[route.model]
        key = f"gg4e08:{route.model:04x}"
        controls, sequence = [], []
        for phase, lost in enumerate((route.lost, active)):
            line = lines[4*i+phase]
            if AMOUNT_ROW.fullmatch(line) is None or tuple(map(int, line.split()[1:])) != (route.model, phase, lost, 180-lost):
                raise ValueError(f"frame admission phase-only amount changed: {key}/{phase}")
            controls.append({"declared_actor_phase": phase, "hp_lost": lost, "receiver_hp_after": 180-lost})
        for step, name in enumerate(("closed", "reopened")):
            closed = step == 0
            before, after = (count, 0) if closed else (0, count)
            lost = 0 if closed else active
            line = lines[4*i+2+step]
            expected = (route.model, step, closed, step, closed, before, after, count, 1, step, 180-lost)
            if ADMISSION_ROW.fullmatch(line) is None or tuple(map(int, line.split()[1:])) != expected:
                raise ValueError(f"frame admission publication or clock ownership changed: {key}/{name}")
            sequence.append({
                "case": name, "source_bit_set": closed, "admission_byte_before": step,
                "admission_byte_after_frame": int(closed), "attack_count_after_frame_before_list_rebuild": before,
                "attack_count_after_list_rebuild": after, "receiver_shapes_after_list_rebuild": 1,
                "retained_melee_cells": count, "unchanged_owner_token": 1,
                "amount_calls": step, "hp_lost": lost, "receiver_hp_after": 180-lost,
                "body_timers_after_pair": [0, 0] if closed else [route.seed, route.seed],
            })
        rows.append({"borg": key, "amount_record": address(route.amount), "phase_only_controls": controls,
                     "retained_queue_sequence": sequence})
    return rows


def verify_source(image, atlas):
    beam.verify_text(image, TEXT)
    beam.link(image, 0x80055638, 0x80055EE4)
    if (beam.word(image, 0x8003CE2C), beam.word(image, 0x8003CE30), beam.word(image, 0x80055F58)) != (
        0x881F0018, 0x2C000001, 0x54000085
    ):
        raise ValueError("actor phase or outgoing eligibility guard changed")
    installed = read_metadata(MAPS / "gg4e08-standalone-installers.json", INSTALLERS_SHA256)
    for model, (install, pre, post, aux, table) in INSTALLED.items():
        row, = [r for r in installed["borgs"] if r["id"] == f"gg4e08:{model:04x}"]
        table_pre, table_post, _, table_aux = map(address, struct.unpack(">4I", image.read(table+8, 16, "data")))
        if ((row["installer"], row["pre_callback"], row["post_callback"]) != (install, pre, post) or
                (table_pre, table_post, table_aux) != (pre, post, aux)):
            raise ValueError("actor frame installer callbacks changed")
    found = {}
    for entry, span in atlas["functions"].items():
        start = int(entry, 16)
        for i, (word,) in enumerate(struct.iter_unpack(">I", image.read(start, span["size"], "text"))):
            if word & 0xFFFF == 0x276 and word >> 26 in (34, 35, 38, 39):
                found[start+4*i] = (entry, word)
    if found != {pc: row[:2] for pc, row in USES.items()}:
        raise ValueError("explicit attack eligibility byte access census changed")
    return [{"site": address(pc), "function": row[0], "scope": row[2]} for pc, row in sorted(USES.items())]


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("actor frame admission output already exists")
    dol, module = (Path(p).resolve(strict=True) for p in (dol, module))
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    read_metadata(MAPS / "gg4e08-melee-pose-clock.json", MELEE_REPLAY_SHA256)
    read_metadata(MAPS / "gg4e08-contact-frame-prefix.json", PREFIX_SHA256)
    uses = verify_source(image, read_metadata(MAPS / "gg4e08-code-atlas.json", ATLAS_SHA256))
    payload, _, _ = melee.qualified_inputs(image, afs)
    rows = decode(clock.fixture_report(fixture, dol, module, ("--frame-admission",), payload))
    data = {
        "kind": "gf-retail-actor-frame-admission", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "code_atlas_sha256": ATLAS_SHA256,
        "melee_pose_clock_sha256": MELEE_REPLAY_SHA256, "contact_frame_prefix_sha256": PREFIX_SHA256,
        "entries": {"actor_wrapper": "80055568", "late_flag_project": "80055EE4", "shape_publish": "8008A2BC",
                    "pair": "8002DB58", "amount_actor_phase_read": "8003CE2C"},
        "fields": {"source_flags": "5E0", "closure_bit": "20000000", "attack_admission_byte": "276"},
        "explicit_eligibility_byte_accesses": uses, "cases": rows,
        "notes": [
            "The complete original actor wrapper runs with exact Tau and G-Red class callbacks, declared neutral lanes and a scoped stage.",
            "A stale byte is deliberately opposite the source bit. The actor tail rewrites the byte but does not retroactively remove entries from the already published list.",
            "The next original list rebuild omits the attack shapes while closed and re-admits the same cells and owner token after reopening.",
            "The actor phase is held at one during the retained sequence. Separate phase-only forks reproduce the different HP amounts without changing admission.",
            "The explicit displacement census is not proof about indirect aliases or other collision roles.",
            "Neither the origin of the status bit in a live match nor a full scene frame, animation pose, guard semantics or native gameplay is qualified.",
        ],
    }
    raw = encode(data, expanded=("explicit_eligibility_byte_accesses", "cases"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "routes": len(rows), "actor_visits": 4, "phase_only_controls": 4}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
