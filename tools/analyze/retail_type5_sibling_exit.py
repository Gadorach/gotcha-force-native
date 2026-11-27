"""Replay the contact, zero-HP, expiry and bounds exit split on four parent types."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
from subprocess import run

from tools.analyze import retail_dragon_area as common_area, retail_type32_roster as beam
from tools.analyze import retail_type4_ancestors as parents, retail_type5_roster as area
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import SOURCE_SHA256, address, encode, read_metadata
from tools.extract.source_image import DolImage


CENSUS_SHA256 = "7a4b338af1d56c71121cc3cac6dba5842fb98cc29e4f8601c4f8d4443205fea3"
# Model, parent type and row, declared socket ordinal, area key.
PILOTS = ((0x0106, 33, 2, 2, 19), (0x0601, 52, 0, 0, 11), (0x0617, 52, 1, 2, 11),
          (0x0D00, 74, 0, 0, 6), (0x0D01, 77, 0, 0, 7), (0x0D05, 77, 1, 0, 7))
# Label, contact, zero HP, expired, outside bounds, spare descriptor cells.
CASES = (("latched", True, False, False, False, 2), ("latched-terminal", True, True, True, False, 2),
         ("latched-one-descriptor", True, True, True, False, 1), ("zero-hp", False, True, False, False, 2),
         ("expiry", False, False, True, False, 2), ("outside-terminal", False, True, True, True, 2))
ROW = re.compile(r"area-exit:(?: [0-9]{1,4}){11}")
TEXT = (
    (0x80083244, 128, "0d12380007430d1784fc3a5d0183bdd2c9b40b58252868175125d47bbd3e48c4"),
    (0x8001BDA8, 288, "01586cb4fa31c8c0f19871852fa315ee27958073bd45cff37c3f73355b129b00"),
    *common_area.TEXT[:1],
    *parents.TEXT,
)


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != len(PILOTS)*len(CASES):
        raise ValueError("area sibling report extent")
    cases = []
    for model, kind, row, ordinal, area_key in PILOTS:
        for scenario, (label, contact, zero_hp, expired, outside, descriptors) in enumerate(CASES):
            line = lines[len(cases)]
            if ROW.fullmatch(line) is None:
                raise ValueError("malformed area sibling report row")
            child_type = 0 if outside else 5 if contact else 4
            key = area_key if contact else 0
            allocated = bool(child_type) and (not contact or descriptors >= 2)
            free = descriptors - (2 if contact else 1)*int(allocated)
            offset = 0 if outside else 672 if contact and kind != 77 else 32
            expected = (model, scenario, kind, row, ordinal, child_type, key, allocated, free, 2, offset)
            if tuple(map(int, line.split()[1:])) != expected:
                raise ValueError(f"area sibling priority or position changed: {model:04X}/{label}")
            site = area.EXIT_PEERS[kind][1] if contact else parents.PARENTS[kind].sites[0 if zero_hp else 1]
            cases.append({
                "borg": f"gg4e08:{model:04x}", "parent_type": kind, "parent_row": row, "declared_ordinal": ordinal, "case": label,
                "contact_latch": contact, "zero_hp": zero_hp, "expired": expired, "outside_bounds": outside,
                "free_descriptors_before": descriptors, "free_descriptors_after": free, "parent_phase_after": 2,
                "request": {"site": address(site), "type": child_type, "key": key, "allocated": allocated,
                            "position_offset": f"0x{offset:X}", "position": [260, 120, 580] if offset == 672 else [200, 100, 500]}
                if child_type else None,
            })
    return cases


def check_census(image, census):
    links = [r for r in census["candidates"] if "branches" in r["descendant"]]
    if len(links) != 7:
        raise ValueError("area sibling census scope changed")
    for model, kind, row, _, key in PILOTS:
        matches = [r for r in links if r["borg"] == f"gg4e08:{model:04x}" and r["parent"]["type"] == kind]
        _, on_contact, on_stage, _, _, _, offset = area.EXIT_PEERS[kind]
        if not matches or any(
            r["parent"]["keys"] != [row] or r["parent"]["constructor"] != address(parents.PARENTS[kind].constructor)
            or r["descendant"]["key"] != key or r["descendant"]["sites"] != [address(on_contact), address(on_stage)]
            or [b["position_offset"] for b in r["descendant"]["branches"]] != [f"0x{offset:X}", "0x20"]
            for r in matches
        ):
            raise ValueError(f"area sibling key, parent or position source changed: {model:04X}")
        beam.link(image, on_contact, 0x80085E00)
        for site in parents.PARENTS[kind].sites:
            beam.link(image, site, 0x8001BDA8)


def export(dol, census, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("area sibling output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    beam.verify_text(image, TEXT)
    check_census(image, read_metadata(census, CENSUS_SHA256))
    args = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True))]
    proc = run(args, capture_output=True, timeout=45)
    if proc.returncode or proc.stderr:
        raise ValueError(f"area sibling fixture failed: {proc.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode_report(proc.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-type5-sibling-exits", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "type5_census_sha256": CENSUS_SHA256, "cases": cases,
        "notes": [
            "Original constructors allocate the parent; phase one, HP, timer, position and contact latch are declared inputs.",
            "Factory arguments and full allocation are observed from the same saved inlet, not as two game events.",
            "The contact latch wins over zero HP or expiry. Refusing the area does not fall back to type 4 even with its required pools available.",
            "Actor contact uses the saved point except for type 77, which uses the carrier root. The bounds guard is not a replay of the separate stage probe.",
            "No actor overlap, area opening, HP writer, live action or native gameplay runs in these cases.",
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
