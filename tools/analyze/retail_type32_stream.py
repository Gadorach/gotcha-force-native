"""Replay repeated type-11 emission, pool pressure, and per-token HP in the original bucket."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
from subprocess import run

from tools.analyze import retail_type32_contact as contact, retail_type32_roster as roster, retail_type32_shared as shared
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import AFS_SHA256, SOURCE_SHA256, encode
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage


POOLS = ((1, 1), (3, 3), (4, 1), (4, 2), (4, 3))
VISITS = 7
ROW = re.compile(r"type32-stream:(?: [0-9]{1,4}){13}")
TEXT = (
    (0x80089A94, 88, "dcb8bf7edce7313e0e258ee526af82153e5e35eac0987938a42cccc6a270dde4"),
    (0x8008997C, 280, "3d628d2b7b32b3cd2c7e0816370547898bb863673bd68351246c797720e57eab"),
    (0x80089898, 228, "126a25b8262b9ec16da7e6121fd72318cce3c39faab481f718d2a559307c1299"),
    (0x80088E50, 328, "c747b919f60eaad0d8b255db2b1fdc9ecb0bd22cea33c4930d1085dcdf34d3f4"),
)


def expected_steps(carriers, shapes):
    """Expected order for these fixture cases, not a runtime scheduler."""
    active = []
    token = 0
    for visit in range(1, VISITS+1):
        requested = visit < VISITS
        allocated = requested and len(active) < carriers
        free_shapes_before = shapes-sum(bool(key) for _, key in active)
        if allocated:
            active.insert(0, (0, 0))
        bound = False
        sweep = 0
        survivors = []
        for phase, key in active:
            if phase == 0:
                if free_shapes_before:
                    token += 1
                    bound = True
                    survivors.append((1, token))
                else:
                    survivors.append((2, 0))
            elif phase == 1:
                sweep = key
                survivors.append((2, key))
            elif phase == 2:
                survivors.append((3, 0))
        active = survivors
        registered = sum(bool(key) for _, key in active)
        yield requested, allocated, bound, registered, sweep, carriers-len(active), shapes-registered


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 8192 or len(lines) != len(shared.PILOTS)*len(POOLS)*VISITS:
        raise ValueError("type-32 stream report extent")
    cases = []
    i = 0
    for pilot in shared.PILOTS:
        for scenario, (carriers, shapes) in enumerate(POOLS):
            hp = 180
            visits = []
            for n, step in enumerate(expected_steps(carriers, shapes), 1):
                line = lines[i]
                i += 1
                if not ROW.fullmatch(line):
                    raise ValueError("malformed type-32 stream row")
                request, created, bound, registered, token, free_c, free_s = step
                damage = contact.ACTORS[pilot.model][2] if token else 0
                hp -= damage
                phase = 1 if request else 2
                expected = (pilot.model, scenario, n, request, created, bound, registered, token, damage, hp, free_c, free_s, phase)
                if tuple(map(int, line.split()[1:])) != expected:
                    raise ValueError(f"type-32 stream order, token or HP mismatch: {pilot.model:04X}/{carriers}/{shapes}/{n}")
                visits.append({
                    "active_visit": n, "request": request, "segment_allocated": created, "opening_shape_bound": bound,
                    "registry_count": registered, "landed_segment_token": token or None, "hp_loss": damage,
                    "receiver_hp": hp, "free_carrier_cells_after": free_c, "free_shape_cells_after": free_s, "parent_phase": phase,
                })
            cases.append({
                "borg": f"gg4e08:{pilot.model:04x}", "carrier_key": pilot.key, "segment_key": pilot.group,
                "spare_carriers_at_start": carriers, "attacker_shapes_at_start": shapes, "visits": visits,
                "hp_lost": 180-hp, "landed_visits": [v["active_visit"] for v in visits if v["landed_segment_token"]],
            })
    return cases


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type-32 stream output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    roster.verify_text(image, TEXT)
    roster.link(image, 0x80089AC4, 0x8008997C)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        inlet, sources, body = contact.qualified_inlet(image, RetailArchive(stream))
        if stamp != archive_stamp(stream):
            raise ValueError("type-32 stream AFS changed during extraction")
    args = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--stream"]
    result = run(args, input=inlet, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"type-32 stream fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-type32-stream", "revision": 1, "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "sources": sources, "cases": cases, "receiver": {**body, "borg": "gg4e08:000a", "level_one_hp": 180},
        "entries": {"bucket": "80089A94", "actor_relative_walk": "8008997C", "segment_request": "800E20C0",
                    "segment_transform": "80139CC0", "pair_pass": "8002DB58"},
        "notes": [
            "A supplied open producer byte and unit owner clock allow six bucket visits, then the byte closes before visit seven.",
            "The bucket runs both the carrier and its children; the late transform and class-4 publication are run separately.",
            "The receiver stays at z=1000 off the initial near spheres but on the tested sweeps. Its body has a separate slot.",
            "Creation and first shape binding occur before older segments return their cells in that visit.",
            "The receiver retains each consumed token. Resolution is retried after every hit without another HP write.",
            "Neither actor is scheduled beyond the declared bucket and pair passes, so these totals are not whole-move damage.",
        ],
    }
    raw = encode(payload, expanded=("sources", "cases"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases), "visits": len(cases)*VISITS}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
