"""Verify the original type-11 segment's swept contact with and without a wall control."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import reaction_emit, retail_type32_roster as roster, retail_type32_shared as shared
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import address, read_bounded
from tools.analyze.retail_projectile_census import AFS_SHA256, SOURCE_SHA256, encode
from tools.analyze.retail_rifle_contact_hp import ACTOR_SHA256, SLICES
from tools.analyze.retail_shape_sweep import source_operands
from tools.analyze.retail_type0_contact import PAIR_TEXT, RECEIVER, RECEIVER_BLOCK_SHA256, read_normal
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive, levels
from tools.extract.source_image import DolImage


# Level-one HP, normal raw sphere radius, measured HP delta, normal actor record.
ACTORS = {
    0x0104: (300, 30, 2, "bdfc852e6a5bf511a4c6023c516bbc980f915c1f3c3d8d11d13ae470bde71eb6"),
    0x0505: (500, 50, 3, "ca0c1b314b348a051a2f031fe932c8d31cb0b2da05bd768a103683be317c7ee6"),
    0x0511: (250, 50, 1, "2f0f11ecc109322cc0a046be3fb35e5f60ff7fe7bc0a3fc15be4229479bed909"),
    0x0615: (200, 20, 1, "44def5423ee908d01fa73c62598d8c2837fa94291773bfcd923af83a1e5cb161"),
}
CASES = ("clear-hit", "clear-miss", "wall-hit", "wall-miss")
ROW = re.compile(r"type32-contact:(?: [0-9]{1,4}){8}(?: [0-9]{1,5}\.[0-9]{5}){5}")
TEXT = (
    (0x8003E978, 124, "bfb5cd0f3daca84c72cb691c8f7396b0ad585983c7d679e074cd39c5fdd24d41"),
    (0x80139C78, 40, "b70562df6290f4c6d1ccfa7b1974b2ea50b87837aef462d6cd49202330139d7a"),
    (0x80139CA0, 32, "e4b0283f1035b48abff245315c7b8ee9f52ad02e315e4f649343a294c7d49628"),
    (0x80139CC0, 84, "bf68009ae32e2b6b347abd46705eda63b5d3a12d962768f3ccc1bed0f12968f9"),
    *tuple((at, *SLICES[at]) for at in PAIR_TEXT),
)


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 2048 or len(lines) != len(shared.PILOTS)*len(CASES):
        raise ValueError("type-32 contact report extent")
    cases = []
    for i, line in enumerate(lines):
        p = shared.PILOTS[i//len(CASES)]
        case = i % len(CASES)
        if not ROW.fullmatch(line):
            raise ValueError("malformed type-32 contact report row")
        fields = line.split()[1:]
        hit, wall = case % 2 == 0, case >= 2
        _, raw, hp_loss, _ = ACTORS[p.model]
        damage = hp_loss if hit else 0
        values = tuple(map(int, fields[:8]))
        if values != (p.model, p.key, case, damage, 180-damage, int(hit), 16 if hit else 0, int(wall)):
            raise ValueError(f"type-32 contact result changed: {p.model:04X}/{CASES[case]}")
        radius, offset, near, far, midpoint = map(float, fields[8:])
        expected_radius = raw*p.scale/15
        origin_z = 300+p.offset[2]
        expected_near = origin_z+10*p.scale/15
        expected_far = origin_z+15110*p.scale*2/15
        if (abs(radius-expected_radius) > 0.001 or abs(offset-radius-(21 if hit else 23)) > 0.001
                or abs(near-expected_near) > 0.002 or abs(far-expected_far) > 0.002
                or abs((near+far)*0.5-midpoint) > 0.002 or not near < 1000 < midpoint < far):
            raise ValueError("type-32 contact narrow radius, sweep or wall inlet changed")
        cases.append({
            "borg": f"gg4e08:{p.model:04x}", "carrier_key": p.key, "segment_key": p.group, "case": CASES[case],
            "first_sample_z": near, "refreshed_second_sample_z": far, "receiver_center_z": midpoint,
            "registered_narrow_radius": radius, "receiver_offset_x": offset,
            "stage_query_hit_z": 1000 if wall else None,
            "credited_actor_amount_record": address(p.table+24*p.index) if hit else None,
            "hp_loss": damage, "receiver_hp": 180-damage, "receiver_token": values[5],
            "segment_contact_flags": values[6], "carrier_contact_flags": 0,
            "same_token_refused_on_repeat": hit, "segment_shape_unlinked": True, "segment_retired_before_carrier": True,
        })
    return cases


def qualified_inlet(image, archive):
    roster.verify_text(image, TEXT)
    roster.link(image, 0x80139C8C, 0x8008AFF0)
    attack_blocks, sources = shared.qualified_inlet(image, archive)
    body_block, defender_spec = read_normal(archive, RECEIVER, RECEIVER_BLOCK_SHA256, ACTOR_SHA256[RECEIVER])
    body = source_operands(body_block)["receiver"]
    if levels(image, RECEIVER)[0][0] != 180:
        raise ValueError("type-32 contact receiver HP changed")
    specs = []
    for p, source in zip(shared.PILOTS, sources, strict=True):
        hp, radius, _, digest = ACTORS[p.model]
        block, spec = read_normal(archive, p.model, p.digest, digest)
        step = reaction_emit.STEPS+reaction_emit.STEP_SIZE*p.descriptor
        if (struct.unpack_from(">BBHHhhH4f", block, step) != (0, 0, 1, p.index, 0, -1, 0, 0, 0, 0, radius)
                or levels(image, p.model)[0][0] != hp):
            raise ValueError("type-32 contact source radius or owner HP changed")
        specs.append(spec)
        source.update(level_one_hp=hp, raw_sphere_radius=radius, actor_spec_sha256=digest)
    return attack_blocks+body_block+b"".join(specs)+defender_spec, sources, body


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type-32 contact output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        inlet, sources, body = qualified_inlet(image, RetailArchive(stream))
        if stamp != archive_stamp(stream):
            raise ValueError("type-32 contact AFS changed during extraction")
    args = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--contact"]
    result = run(args, input=inlet, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"type-32 contact fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-type32-contact", "revision": 1, "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "sources": sources, "cases": cases, "receiver": {**body, "borg": "gg4e08:000a", "level_one_hp": 180},
        "entries": {"segment_transform": "80139CC0", "terrain_query": "8003E978", "pair_pass": "8002DB58",
                    "hp_writer": "8003D344", "segment_shape_unlink": "80139C8C", "segment_retirement": "80139CA0"},
        "notes": [
            "The model-relative carrier emits into one spare segment cell; its next request still refreshes the far endpoint.",
            "The isolated original type-11 transform runs before the class-4 pass. The receiver is stationary at the sweep midpoint offset.",
            "A separate source terrain query intersects the fixture-authored stage triangle at z=1000. This does not provide a result to the beam.",
            "The two receiver offsets straddle the sum of the published narrow radius and body radius by one unit.",
            "The segment, not the type-32 carrier, owns contact and retires separately; the actor gets HP credit.",
            "This does not qualify other keys, terrain or prop implementations, live match scheduling, or native gameplay.",
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
