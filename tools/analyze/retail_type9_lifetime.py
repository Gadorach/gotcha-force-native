"""Replay type-9 publication, shape return, and later carrier retirement."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re

from tools.analyze import retail_type9_roster as effect, retail_type32_roster as beam
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import AFS_SHA256, SOURCE_SHA256, address, encode
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage


CASES = (
    ("clock-one", 1.0, True, None, (30, 31, 32, 31, 31, 33, 33, 0x800FC9F4)),
    ("clock-half", 0.5, True, None, (30, 31, 32, 31, 31, 33, 33, 0x800FC9F4)),
    ("no-shape", 1.0, False, None, (0, 1, 2, 0, 0, 3, 3, 0)),
    ("owner-phase-two", 1.0, True, 2, (1, 0, 2, 2, 2, 3, 3, 0x80084090)),
)
ROW = re.compile(r"type9-lifetime:(?: [0-9]{1,2}){9} [0-9A-F]{8}")
TEXT = (
    (0x80089A94, 88, "dcb8bf7edce7313e0e258ee526af82153e5e35eac0987938a42cccc6a270dde4"),
    (0x8008997C, 280, "3d628d2b7b32b3cd2c7e0816370547898bb863673bd68351246c797720e57eab"),
    (0x80089898, 228, "126a25b8262b9ec16da7e6121fd72318cce3c39faab481f718d2a559307c1299"),
    (0x800FC9B4, 184, "51c050c251d13a91335e386a455f9f4dd09ed5cd87dc4a7dd697b3686dbef9d9"),
    (0x800FCA6C, 20, "9212e9caa3a25e54a968e0a58a25f7946023df4677e5701fd0ee94e4396d8819"),
    (0x800FCA80, 32, "3fc15c892abd48cc29c3df5e0eeb3ea6c7a2c00483d54c665819ad685bb9699d"),
    (0x80084000, 184, "bd6cd7c06f97c3beecc085fc63a52bb16cd991f35c49f0cca34b562a382a5138"),
    (0x8008AFF0, 32, "8ea8cac06c446278fad05cfae1dbe3ee6b3a321f4bfd1168269ce88283bef951"),
    (0x80088E50, 328, "c747b919f60eaad0d8b255db2b1fdc9ecb0bd22cea33c4930d1085dcdf34d3f4"),
    (0x80089814, 132, "1bb689f99364d17b27d89cf9d15ceb89ffde7be29dc90d4bba30b495975710dd"),
    (0x8008A65C, 996, "92fa6953e7aea5fe06460450ea539bf738406dc0632251b89ccba4c973b3adf4"),
    *tuple(s for s in effect.CONTACT_TEXT if s[0] in (0x80089554, 0x8008A28C, 0x80089AEC)),
)


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 2048 or len(lines) != effect.ROW_COUNT*len(CASES):
        raise ValueError("type-9 lifetime report extent")
    rows = []
    for i, line in enumerate(lines):
        key, scenario = divmod(i, len(CASES))
        name, clock, shape, owner_exit_visit, expected = CASES[scenario]
        if ROW.fullmatch(line) is None:
            raise ValueError("malformed type-9 lifetime row")
        values = line.split()[1:]
        raw = (*map(int, values[:-1]), int(values[-1], 16))
        if raw != (key, scenario, *expected):
            raise ValueError(f"type-9 lifetime, return order or exit site changed: {key}/{name}")
        _, _, published, phase2, phase3, unbind, shape_return, carrier_return, descriptor_return, site = raw
        rows.append({
            "key": key, "case": name, "owner_clock_step": clock, "spare_shape_cells": int(shape),
            "owner_phase_two_before_visit": owner_exit_visit, "published_attack_visits": published,
            "radius_during_publication": 150 if published else None, "token_during_publication": 1 if published else None,
            "first_post_visit_phase_two": phase2 or None, "first_post_visit_phase_three": phase3,
            "shape_unbind_requested_visit": unbind or None, "shape_unbind_site": address(site) if site else None,
            "shape_return_visit": shape_return or None, "carrier_return_visit": carrier_return,
            "descriptor_return_visit": descriptor_return,
        })
    return rows


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type-9 lifetime output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    beam.verify_text(image, TEXT)
    for site, target in ((0x80089AC4, 0x8008997C), (0x80089ACC, 0x80089898), (0x80084090, 0x8008AFF0),
                         (0x800FC9F4, 0x8008AFF0), (0x800FCA8C, 0x80088E50), (0x80088F78, 0x80089814)):
        beam.link(image, site, target)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        bank = effect.common_bank(RetailArchive(stream))
        if stamp != archive_stamp(stream):
            raise ValueError("type-9 lifetime AFS changed during extraction")
    _, _, _, collision, phase = effect.source_index(image, bank)
    if phase["configured_active_limit"] != 30:
        raise ValueError("type-9 lifetime active counter limit changed")
    args = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--lifetime"]
    cases = decode_report(effect.fixture_report(args, bank))
    payload = {
        "kind": "gf-retail-type9-lifetime", "revision": 1,
        "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "entries": {"bucket_visit": "80089A94", "factory": address(effect.EFFECT), "type9_update": "800FC7C0",
                    "phase_entries": phase["phase_entries"], "shape_unbind": "8008AFF0", "carrier_remove": "80088E50"},
        "inlet": {"declared_owner_model": "0D03", "actor_bucket_argument": 8, "allocated_carrier_bucket": 9,
                  "bind_group": collision["group"], "configured_active_limit": phase["configured_active_limit"]},
        "cases": cases,
        "notes": [
            "Visit 1 runs initialization in the original actor-relative bucket. Late transform and registry publication follow each visit.",
            "Counts and free-list entries identify the returned cells separately. The owner-exit case sets only the declared owner phase before visit 2.",
            "Phase labels are sampled after the visit; the owner guard can enter phase 2 and dispatch it before that sample.",
            "There is no recipient, damage or live action scheduling in these cases. Clock inputs do not identify wall time or frame rate.",
        ],
    }
    raw = encode(payload, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases), "visits": sum(r["carrier_return_visit"] for r in cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
