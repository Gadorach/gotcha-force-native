"""Continue four scoped area keys through original growth, HP and retirement."""
import argparse
from hashlib import sha256
import json
import math
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import retail_dragon_area as common, retail_type32_roster as beam
from tools.analyze import retail_type5_roster as area, retail_type5_sibling_exit as exits
from tools.analyze import retail_type9_lifetime as bucket
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import AFS_SHA256, SOURCE_SHA256, address, encode, read_metadata
from tools.analyze.retail_rifle_contact_hp import ACTOR_SHA256, SLICES
from tools.analyze.retail_shape_sweep import source_operands
from tools.analyze.retail_type0_contact import PAIR_TEXT, RECEIVER, RECEIVER_BLOCK_SHA256, read_normal
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import ACTOR_BYTES, RetailArchive, levels
from tools.extract.source_image import DolImage


ACTORS = {
    0x0106: (220, "69919a580073d32e8db9bdbccaed60fa81687a5fc7c58af87fda3ea6bb0ae26f"),
    0x0601: (550, "7e32390f0e19ed7903bb549247cb791fabeaddcc54b6ea2ffe0ae04f9f207c5d"),
    0x0617: (550, "25fe9a8f4e6d0412aa26860238d9589888f17c5fef202aa1fd4a65729f4b2491"),
    0x0D00: (150, "fa220945c0852ba65835ebd497636417f26c1c03bf476e2dc51b72beb6c57ca6"),
    0x0D01: (150, "f3a4ff1a78b3bbc60f92d0ac7d29efff275e1d85c056bb341263700ab23fe843"),
    0x0D05: (150, "ac4170fd453bf06f235f121ba397d3d5f1469de879c261b20c9d42d6ace2e953"),
}
GROUPS = {6: (82, 120, 7), 7: (83, 121, 8), 11: (104, 143, 18), 19: (179, 246, 26)}
CASES = (("shape-refused", 22), ("near", 22), ("middle", 70), ("wide", 90), ("cap-split", 420))
ROW = re.compile(r"area-followup:(?: [0-9]{1,4}){12}(?: [0-9]{1,4}\.[0-9]{5}){4}")
TEXT = (
    *common.TEXT,
    *tuple((at, *SLICES[at]) for at in PAIR_TEXT),
    *bucket.TEXT,
    (0x800862B8, 152, "375e4c6c4edbe572e062666f083c96e75a54e2dc539c8ee70e66593203d2a991"),
    (0x80086698, 148, "7deb9912e26b4ac903b405de36cb586c4868ec7c74871cc4d4c4c470edb877d2"),
    (0x800864E4, 32, "3a1f67e030b60f5a034b1475777e047d9dd71efc025f953dd4b7af7f8677fc51"),
)


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != len(exits.PILOTS)*len(CASES):
        raise ValueError("area follow-up report extent")
    cases = []
    for model, kind, row, ordinal, key in exits.PILOTS:
        fast = key == 7
        group, _, index = GROUPS[key]
        for i, (label, offset) in enumerate(CASES):
            line = lines[len(cases)]
            if ROW.fullmatch(line) is None:
                raise ValueError("malformed area follow-up row")
            fields = line.split()[1:]
            admitted = i != 0
            first = ((0, 2, 2, 3, 8) if fast else (0, 2, 3, 4, 0))[i]
            unlink = (22 if fast else 18) if admitted else 0
            retire = (55 if fast else 51) if admitted else 3
            damage = 17 if first else 0
            if tuple(map(int, fields[:12])) != (model, i, key, offset, 2*int(admitted), first, damage, 180-damage,
                                                 admitted, 2, unlink, retire):
                raise ValueError(f"area follow-up admission, time or HP mismatch: {model:04X}/{label}")
            radii = tuple(map(float, fields[12:]))
            growth = 62 if fast else 31
            expected = (3.1, 3.1+growth, 3.1+(first-1)*growth if first else 0, 1240 if fast else 387.5) if admitted else (0,)*4
            if any(not math.isclose(actual, want, abs_tol=0.001) for actual, want in zip(radii, expected, strict=True)):
                raise ValueError(f"area follow-up radius mismatch: {model:04X}/{label}")
            cases.append({
                "borg": f"gg4e08:{model:04x}", "parent_type": kind, "parent_row": row, "declared_ordinal": ordinal, "case": label,
                "area_key": key, "common_group": group, "shape_bound": admitted, "target_x_offset": offset,
                "area_origin": [200, 100, 500] if kind == 77 else [260, 120, 580],
                "opening_radius": radii[0] if admitted else None, "peak_radius": radii[3] if admitted else None,
                "first_published_visit": 2 if admitted else None, "first_published_radius": radii[1] if admitted else None,
                "accepted_token": 1 if first else None, "first_hit_visit": first or None,
                "radius_at_first_hit": radii[2] if first else None, "common_amount_record": address(area.REACTIONS+24*index),
                "hp_loss": damage, "receiver_hp_after": 180-damage, "observed_area_token": 1 if admitted else None,
                "parent_return_visit": 2, "area_shape_return_visit": unlink or None, "area_carrier_return_visit": retire,
            })
    return cases


def qualified_inlet(image, archive, census):
    exits.check_census(image, census)
    beam.verify_text(image, exits.TEXT+TEXT)
    beam.link(image, 0x800860E0, 0x8008AE3C)
    beam.link(image, 0x8002F108, 0x8003CD5C)
    beam.link(image, 0x8002F1D8, 0x8003D344)
    if struct.unpack(">3I", image.read(common.PHASE_TABLE, 12, "data")) != common.PHASES:
        raise ValueError("area follow-up phase dispatch changed")
    bank = area.common_bank(archive)
    rows, shapes = area.area_rows(image, bank)
    if rows != census["factory_rows"] or shapes != census["common_bank"]["groups"]:
        raise ValueError("area follow-up common bank does not match the census")
    groups = {s["group"]: s for s in shapes}
    for key, (group, descriptor, index) in GROUPS.items():
        shape = groups[group]
        if (rows[key]["common_group"] != group or shape["descriptor"] != descriptor or shape["reaction_index"] != index
                or shape["raw_power"] != 20):
            raise ValueError("area follow-up qualified group or raw power changed")
    body, defender = read_normal(archive, RECEIVER, RECEIVER_BLOCK_SHA256, ACTOR_SHA256[RECEIVER])
    if levels(image, RECEIVER)[0][0] != 180:
        raise ValueError("area follow-up receiver HP changed")
    specs, sources = [], []
    for model, _, _, _, _ in exits.PILOTS:
        hp, digest = ACTORS[model]
        filename = f"pl{model:04x}.pzz".encode("ascii")
        if archive.names.count(filename) != 1:
            raise ValueError("area follow-up normal model member is not unique")
        member, _ = archive.member(archive.names.index(filename), model)
        spec, _ = member.read(0, ACTOR_BYTES)
        if len(spec) != ACTOR_BYTES or sha256(spec).hexdigest() != digest or levels(image, model)[0][0] != hp:
            raise ValueError(f"area follow-up normal model spec or HP changed: {model:04X}")
        specs.append(spec)
        sources.append({"borg": f"gg4e08:{model:04x}", "level_one_hp": hp, "actor_spec_sha256": digest})
    return bank+body+defender+b"".join(specs), sources, source_operands(body)["receiver"]


def export(dol, afs, census, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("area follow-up output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    census = read_metadata(census, exits.CENSUS_SHA256)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        inlet, sources, body = qualified_inlet(image, RetailArchive(stream), census)
        if stamp != archive_stamp(stream):
            raise ValueError("area follow-up AFS changed during extraction")
    args = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--follow"]
    proc = run(args, input=inlet, capture_output=True, timeout=45)
    if proc.returncode or proc.stderr:
        raise ValueError(f"area follow-up fixture failed: {proc.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode_report(proc.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-type5-sibling-followup", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "type5_census_sha256": exits.CENSUS_SHA256, "cases": cases, "sources": sources,
        "receiver": {**body, "borg": "gg4e08:000a", "level_one_hp": 180},
        "notes": [
            "The original parent allocates the child from a declared latch and contact point; parent collision and live admission do not run.",
            "Visit 1 is the first original bucket visit after area allocation. It initializes the area before its first attack publication.",
            "A shape slot offered after initial refusal does not cause a retry. Parent return does not remove an admitted area, and parent storage is not reused.",
            "An accepted pair finishes the original HP writer before re-entry. The retained token refuses repeat damage; the live reaction tail is not run.",
            "The stationary receiver, actor state, and empty render records are declared inlets. No native gameplay, live timing, or whole-move damage is qualified.",
        ],
    }
    raw = encode(payload, expanded=("cases", "sources"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "census", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.census, args.fixture, args.module, args.output), sort_keys=True))
