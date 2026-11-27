"""Replay sibling stage probes and the wall-effect/area descriptor competition."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
from subprocess import run

from tools.analyze import retail_type32_roster as beam, retail_type5_sibling_exit as exits
from tools.analyze.retail_type5_roster import EXIT_PEERS
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import SOURCE_SHA256, address, encode, read_metadata
from tools.extract.source_image import DolImage


# Label, stage triangle, contact latch, clipped endpoint, spare class-2 cell, descriptors.
CASES = (
    ("clear", False, False, False, True, 2),
    ("wall-three-descriptors", True, False, False, True, 3),
    ("wall-two-descriptors", True, False, False, True, 2),
    ("wall-contact-latched", True, True, False, True, 2),
    ("wall-outside-clip", True, False, True, True, 2),
    ("wall-no-class2-two-descriptors", True, False, False, False, 2),
    ("wall-no-class2-one-descriptor", True, False, False, False, 1),
)
STAGE_CALLS = {33: 0x800FEC68, 52: 0x80129488, 74: 0x800F9928, 77: 0x800FB730}
ROW = re.compile(r"area-stage:(?: [0-9]{1,4}){13}")
TEXT = (
    *exits.TEXT,
    (0x80084170, 192, "9820c165d09f46a0c71df798be6438f031c2ca9a1876c801a879562257fe9e76"),
    (0x80084230, 516, "616e17f25d355f351726fd8477f741b9f996e208c76047a388747c6f997cb682"),
    (0x8003E978, 124, "bfb5cd0f3daca84c72cb691c8f7396b0ad585983c7d679e074cd39c5fdd24d41"),
    (0x80019550, 76, "ba4e00a2bbd856928326f87a46b029a0fb26319ac6de39f8a776a1199df467f1"),
    (0x8001959C, 308, "8c9a81ff9a8f4589b8b3cc525efd66e18b7f53f6c4829778ccd2cf6b9f43b2cc"),
)


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != len(exits.PILOTS)*len(CASES):
        raise ValueError("area sibling stage report extent")
    cases = []
    for model, kind, row, ordinal, key in exits.PILOTS:
        for i, (label, wall, latched, clipped, effect_cell, descriptors) in enumerate(CASES):
            line = lines[len(cases)]
            if ROW.fullmatch(line) is None:
                raise ValueError("malformed area sibling stage row")
            crossed = wall and not latched and not clipped
            asked = crossed or latched
            effect = crossed and effect_cell
            made_area = asked and descriptors >= 2+effect
            probe_z = 0 if latched else 950 if clipped else 1100
            root_z = 900 if latched else 1000 if crossed else 1100
            area_z = 1000 if crossed else (900 if kind == 77 else 880) if latched else 0
            after = descriptors-int(effect)-2*int(made_area)
            expected = (model, i, kind, row, probe_z, crossed, root_z, crossed, effect, asked, made_area, area_z, after)
            if tuple(map(int, line.split()[1:])) != expected:
                raise ValueError(f"area sibling stage clip, effect or area mismatch: {model:04X}/{label}")
            contact_site, stage_site = EXIT_PEERS[kind][1:3]
            cases.append({
                "borg": f"gg4e08:{model:04x}", "parent_type": kind, "parent_row": row, "declared_ordinal": ordinal,
                "case": label, "wall_enabled": wall, "contact_latch": latched, "class2_cell_supplied": effect_cell,
                "free_descriptors_before": descriptors, "free_descriptors_after": after,
                "probe": {"near": [200, 100, 900], "far": [200, 100, probe_z],
                          "intersection": [200, 100, 1000] if crossed else None} if probe_z else None,
                "carrier_root_after": [200, 100, root_z], "parent_phase_after": 2 if asked else 1,
                "wall_effect": {"allocated": effect, "position": [200, 100, 1000]} if crossed else None,
                "area": {"site": address(contact_site if latched else stage_site), "key": key, "allocated": made_area,
                         "position": [260, 120, area_z] if latched and kind != 77 else [200, 100, area_z]} if asked else None,
            })
    return cases


def export(dol, census, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("area sibling stage output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    beam.verify_text(image, TEXT)
    exits.check_census(image, read_metadata(census, exits.CENSUS_SHA256))
    for site, target in (
        *((site, 0x80084170) for site in STAGE_CALLS.values()),
        (0x800841DC, 0x8003E978), (0x8008420C, 0x80019550), (0x80019588, 0x8001959C), (0x800195D4, 0x8008893C),
        *((info[2], 0x80085E00) for info in EXIT_PEERS.values()),
    ):
        beam.link(image, site, target)
    if image.read(0x802C7ED0, 4, "data") != b"\x00\x00\x00\x01":
        raise ValueError("area sibling stage wall effect row changed")
    args = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--stage"]
    proc = run(args, capture_output=True, timeout=45)
    if proc.returncode or proc.stderr:
        raise ValueError(f"area sibling stage fixture failed: {proc.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode_report(proc.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-type5-sibling-stage", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "type5_census_sha256": exits.CENSUS_SHA256, "cases": cases,
        "wall_effect": {"request_entry": "80019550", "factory": "8001959C", "object_tag": 0, "key": 0,
                        "update": "800196D0", "class": 2, "fixed_bucket": 31, "descriptor_cells": 1},
        "notes": [
            "The original parent is allocated, then supplied active phase, position, and forward movement for one visit.",
            "A real source wall probe checks the observed endpoints; the independent kernel control is not fed back to the parent.",
            "Only the probe endpoint is clipped when the boundary excludes the wall. With a crossing the source moves the carrier root to the intersection.",
            "The wall helper tries a fixed-bucket effect before the type-5 request. This is not the zero-HP branch's type-4 effect or a projectile route.",
            "A contact latch bypasses movement and the wall probe. All children stop at allocation; no HP, lifetime, stage equivalence or native gameplay is claimed.",
        ],
    }
    raw = encode(payload, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "census", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.census, args.fixture, args.module, args.output), sort_keys=True))
