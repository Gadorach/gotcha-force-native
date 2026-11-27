"""Join paid normal type-41 placement to the original bucket and stock return."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re

from tools.analyze import retail_stock_returns as returns
from tools.analyze import retail_type5_roster as area
from tools.analyze.retail_type76_opening import read_normal


s = returns.s
STOCK_RETURNS_SHA256 = "3446ac9aaed274350a66bf7180f91bd3b8f6f6c8c56413e72ede6328a9a6fb1e"
TYPE5_SHA256 = "7a4b338af1d56c71121cc3cac6dba5842fb98cc29e4f8601c4f8d4443205fea3"
HIT_SHA256 = "8aa36502a67ebb5a77d0f6812470b8d0bb244dd0592198805bb126e0b2cc3e8d"
MODELS = (
    (0x0106, "p018", 10, "69919a580073d32e8db9bdbccaed60fa81687a5fc7c58af87fda3ea6bb0ae26f"),
    (0x0107, "p019", 8, "3ec8e4ba94797d595faf173bc260bae088c3b48b27e640a8b2f5cba421c66506"),
)
ROW = re.compile(r"type41-bucket:(?: [0-9]{1,3}){16}")
TEXT = (
    (0x801301F8, 312, "8548e29bc78b5dbd1c5887a257532e44933c3ecd5f8d9d82ebd5828f1333e546"),
    (0x800E06C4, 152, "f90da84bdd8ba79421c8d38a358a6070503670ef90de1e3b22af3c00504840c0"),
    (0x800E0A30, 844, "8103e7ed14df49e0061911901e2b20264cea236f557e9b85918a6a3c62a226d5"),
    (0x800E0D7C, 60, "edad68f6cc5ba4ba1120df585ae3537ae640e0f9d157746e70fba77a45586c2c"),
    (0x800E0DEC, 32, "6276650ffb82fee84ea5b99bddd573b25692d28909d955e18f523ff80b6f9184"),
    (0x80089A94, 88, "dcb8bf7edce7313e0e258ee526af82153e5e35eac0987938a42cccc6a270dde4"),
    (0x8008997C, 280, "3d628d2b7b32b3cd2c7e0816370547898bb863673bd68351246c797720e57eab"),
)
# (name, model, reverse carrier cells, initialized count, requested area,
#  area allocated, spare child shape bound, returned descriptors in the final state).
CASES = (
    ("time-carrier-refused", 0x0106, 0, 0, 0, 0, 0, 1),
    ("remote-carrier-refused", 0x0107, 0, 0, 0, 0, 0, 1),
    ("time-descriptor-refused", 0x0106, 0, 0, 0, 0, 0, 0),
    ("remote-descriptor-refused", 0x0107, 0, 0, 0, 0, 0, 0),
    ("time-opening-shape-refused", 0x0106, 0, 1, 0, 0, 0, 1),
    ("remote-opening-shape-refused", 0x0107, 0, 1, 0, 0, 0, 1),
    ("time-expiry-area", 0x0106, 0, 1, 19, 1, 0, 3),
    ("time-expiry-area-descriptor-refused", 0x0106, 0, 1, 19, 0, 0, 2),
    ("remote-bucket-first-cell", 0x0107, 0, 2, 20, 1, 0, 3),
    ("remote-bucket-no-area-cell", 0x0107, 1, 2, 20, 0, 0, 3),
    ("remote-bucket-no-area-descriptors", 0x0107, 0, 2, 20, 0, 0, 2),
    ("remote-bucket-reversed-cells", 0x0107, 1, 2, 20, 1, 0, 3),
    ("remote-owner-invalid-phase", 0x0107, 0, 1, 0, 0, 0, 1),
    ("remote-command-at-stock-cap", 0x0107, 0, 0, 0, 0, 0, 0),
    ("remote-bucket-child-shape-admitted", 0x0107, 0, 2, 20, 1, 1, 1),
    ("time-expiry-child-shape-admitted", 0x0106, 0, 1, 19, 1, 1, 1),
)


def verify_source(image, installed, stock_receipt, area_receipt):
    for site, target in ((0x8013055C, 0x8006DBE0), (0x801305AC, 0x8006DBE0), (0x80130580, 0x800E058C),
                         (0x801305E8, 0x800E058C), (0x800E09EC, 0x8008AC80), (0x80089AC4, 0x8008997C),
                         (0x800E0AAC, 0x80085E00), (0x800E0B4C, 0x80085E00), (0x800E0DD8, 0x800E0E68),
                         (0x800E0E7C, 0x8006DE10)):
        s.link(image, site, target)
    if any(returns.word(image, 0x803170B4+4*i, "data") != target for i, target in
           enumerate((0x800E075C, 0x800E0A30, 0x800E0DB8, 0x800E0DEC))):
        raise ValueError("type-41 phase dispatch changed")
    poses = image.read(0x80316EF8+10*28, 3*28, "data")
    if sha256(poses).hexdigest() != "626decd3f643a53dd17bf25e01a3cf382591e9cefd81be5d84bf39637c10d6a5":
        raise ValueError("type-41 exact initialization pose records changed")
    indexed = {r["id"]: r for r in installed["borgs"]}
    for model, profile, cap, _ in MODELS:
        r = indexed[f"gg4e08:{model:04x}"]
        if ((r["profile"], r["spec_record"], r["installer"], r["post_callback"]) != (profile, 0, "801301F8", "801304B8") or
            s.levels(image, model)[0][:5] != (220, cap, cap, 2, 0)):
            raise ValueError("exact type-41 profile or normal stock changed")
    by_key = {r["key"]: r["return_channel"] for r in stock_receipt["type41_derived_selector_scope"]}
    descendants = {r["key"]: r["common_group"] for r in area_receipt["factory_rows"]}
    if by_key != {0: None, 1: None, 2: 0, 3: 0} or (descendants[19], descendants[20]) != (179, 180):
        raise ValueError("type-41 prior stock or child qualification changed")


def decode(report):
    lines = report.splitlines()
    if len(report) > 1536 or len(lines) != len(CASES):
        raise ValueError("type-41 bucket report extent")
    rows = []
    for i, (name, model, reverse, count, key, area_made, area_bound, free) in enumerate(CASES):
        cap = 10 if model == 0x0106 else 8
        lifetime = (188 if model == 0x0106 else 608) if count else 0
        sibling_lifetime = 625 if count == 2 else 0
        after_pay = cap-count
        after_exit = cap if i == 12 else after_pay
        final = after_pay+int(count > 0)
        latch = int(key == 19)
        phase, returned = (3, 1) if count else (0, 0)
        expected = (i, model, cap, reverse, lifetime, sibling_lifetime, after_pay, after_exit,
                    final, latch, key, area_made, phase, returned, free, area_bound)
        if ROW.fullmatch(lines[i]) is None or tuple(map(int, lines[i].split()[1:])) != expected:
            raise ValueError(f"type-41 stock, order, lifetime or child changed: {name}")
        rows.append({
            "case": name, "borg": f"gg4e08:{model:04x}", "capacity": cap, "stock_after_placement": after_pay,
            "stock_after_terminal_visit": after_exit, "stock_after_final_cleanup": final,
            "initialized_countdown_seeds": [lifetime, sibling_lifetime][:count],
            "first_carrier_cell": reverse if count else None, "initialized_carriers": count,
            "one_sibling_still_active": count == 2, "owner_command_latch_after_terminal_visit": latch,
            "requested_area_key": key or None, "area_allocated": bool(area_made), "area_bound_shape": bool(area_bound),
            "phase_after_return": phase if count else None, "lifecycle_stock_writer_calls": returned,
            "placement_refusal_writer": "800E0E7C" if i < 4 else None, "free_descriptors_after_cleanup": free,
        })
    return rows


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type-41 bucket output already exists")
    dol, module = (Path(p).resolve(strict=True) for p in (dol, module))
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    s.verify_text(image, (*s.TEXT, *returns.TEXT, *TEXT))
    verify_source(image, s.read_metadata(s.MAPS / "gg4e08-standalone-installers.json", s.INSTALLERS_SHA256),
                  s.read_metadata(s.MAPS / "gg4e08-stock-returns.json", STOCK_RETURNS_SHA256),
                  s.read_metadata(s.MAPS / "gg4e08-type5-roster.json", TYPE5_SHA256))
    with s.open_regular(afs) as stream:
        stamp = s.archive_stamp(stream)
        s.verify_archive(stream)
        archive = s.RetailArchive(stream)
        common = area.common_bank(archive)
        hits = []
        specs = []
        for model, _, _, digest in MODELS:
            hit, spec = read_normal(archive, model, HIT_SHA256, digest)
            hits.append(hit)
            specs.append(spec)
        if hits[0] != hits[1] or s.archive_stamp(stream) != stamp:
            raise ValueError("type-41 exact shared hit block or archive input changed")
    rows = decode(s.fixture_report(fixture, dol, module, payload=common+hits[0]+b"".join(specs)))
    raw = s.encode({
        "kind": "gf-retail-type41-bucket", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "stock_returns_sha256": STOCK_RETURNS_SHA256, "type5_roster_sha256": TYPE5_SHA256,
        "cases": rows,
        "notes": [
            "The original installed owner post hook runs with declared processed inputs and placement eligibility. There is no controller or cooldown timing replay.",
            "Time passes factory key 2, pose 10; Remote passes key 3 and selects poses 12 then 11 from its post-debit parity.",
            "Initialization performs original queue/return-channel selection. Material records are empty observation inlets, not render assets.",
            "The original bucket walks both allocated Remote carriers in two cell orders. Roots and the retained sibling substate are declared.",
            "Time's expiry uses an explicit near-expiry value after recording the original randomized countdown seed. Seeds are not live lifetime measurements.",
            "Child allocation, shape bind and the phase-2 stock writer are separate. No area HP, full scene scheduling, render/audio playback or native gameplay is qualified.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(rows)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
