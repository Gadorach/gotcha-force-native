"""Verify normal receiver tier capacities and original post-contact expiry."""
import argparse
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import re
import struct

from tools.analyze import damage_contact_tiers as source
from tools.analyze import retail_stock_startup as startup
from tools.analyze import retail_type41_parent_contact as parent
from tools.analyze.retail_stock_lifecycle import TEXT as CLOCK_TEXT, STARTUP_SHA256


PARENT_SHA256 = "2934423345d6d87e948edd1336fef00869c5c3bfec5a25678381a0c858c28a28"
CAPACITY_ROW = re.compile(r"tier-capacity: [0-9A-F]{4} [0-7] [0-9]{1,5} [0-9]{1,5}")
PAIR_ROW = re.compile(r"tier-contact:(?: [0-9]{1,3}){8}")
CASES = (
    ("first-charge", 0, 0, 500, 0, 17, 163, 400, 50, 0, 2),
    ("below-threshold", 49, 1, 500, 0, 14, 166, 400, 99, 1, 2),
    ("exact-threshold", 50, 1, 500, 0, 14, 166, 400, 0, 2, 2),
    ("discard-overflow", 99, 2, 150, 0, 8, 172, 50, 0, 3, 2),
    ("wear-depletes-gauge", 60, 2, 100, 0, 8, 172, 500, 50, 0, 166),
    ("wear-inhibited", 50, 1, 100, 128, 14, 166, 100, 0, 2, 130),
    ("tier-capped", 99, 63, 500, 0, 1, 179, 400, 0, 63, 2),
    ("scene-holds-hp", 50, 1, 100, 0, 14, 180, 500, 50, 0, 166),
)


def decode_capacity(report, sources):
    lines = report.splitlines()
    if len(report) > 8192 or len(lines) != len(sources):
        raise ValueError("receiver capacity report extent")
    rows = []
    for line, model in zip(lines, sources, strict=True):
        (cap,) = struct.unpack_from(">h", model.spec)
        if CAPACITY_ROW.fullmatch(line) is None or tuple(line.split()[1:]) != (
                f"{model.model:04X}", str(model.spec_index), str(cap), str(cap)):
            raise ValueError(f"receiver startup gauge changed: {model.id}")
        rows.append({"borg": model.id, "spec_record": model.spec_index, "capacity": cap, "initial_gauge": cap})
    counts = Counter(r["capacity"] for r in rows)
    if counts != {500: 198, 700: 7, 30000: 1}:
        raise ValueError("receiver capacity roster coverage changed")
    return rows, dict(sorted(counts.items()))


def decode_contacts(report):
    lines = report.splitlines()
    if len(report) > 1024 or len(lines) != 2*len(CASES):
        raise ValueError("tier contact report extent")
    rows = []
    for model in (0x0106, 0x0107):
        for i, (name, meter, tier, gauge, flags, request, hp, gauge_after, meter_after, tier_after, flags_after) in enumerate(CASES):
            line = lines[len(rows)]
            expected = (model & 255, i, request, hp, gauge_after, meter_after, tier_after, flags_after)
            if PAIR_ROW.fullmatch(line) is None or tuple(map(int, line.split()[1:])) != expected:
                raise ValueError(f"receiver tier or expiry behavior changed: {model:04X}/{name}")
            rows.append({
                "caster": f"gg4e08:{model:04x}", "area_key": 19+int(model == 0x0107), "case": name,
                "signed_hp_request": request, "hp_removed": 180-hp, "scene_hp_held": i == 7,
                "before_hp_writer": {"gauge": gauge, "meter": meter, "tier": tier, "flags": flags},
                "after_pair": {"gauge": gauge_after, "meter": meter_after, "tier": tier_after, "flags": flags_after},
                "timers_after_hit": {"gauge": 60, "tier": 60},
                "independent_expiry_checked": True, "half_step_expiry_visit": 120,
            })
    return rows


def export(dol, afs, startup_fixture, pair_fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("receiver tier clock output already exists")
    dol, module = (Path(p).resolve(strict=True) for p in (dol, module))
    image = startup.DolImage(startup.read_bounded(dol, 8 << 20), startup.SOURCE_SHA256)
    report = source.recover(image)
    startup.verify_text(image, (*startup.TEXT, *CLOCK_TEXT, *parent.child.TEXT))
    for address in (0x802C49D0, 0x802C49E8):
        raw = image.read(address, 24, "data")
        if raw[4:6] != bytes((100, 50)) or struct.unpack_from(">H", raw, 18)[0] & source.BYPASS:
            raise ValueError("scoped source tier charge, wear or bypass changed")
    startup.read_metadata(startup.MAPS / "gg4e08-stock-startup.json", STARTUP_SHA256)
    startup.read_metadata(startup.MAPS / "gg4e08-type41-parent-contact.json", PARENT_SHA256)
    installed = startup.read_metadata(startup.MAPS / "gg4e08-standalone-installers.json", startup.INSTALLERS_SHA256)
    with startup.open_regular(afs) as stream:
        stamp = startup.archive_stamp(stream)
        startup.verify_archive(stream)
        plan, models = startup.source_inputs(image, startup.RetailArchive(stream), installed)
        if startup.archive_stamp(stream) != stamp:
            raise ValueError("receiver capacity archive input changed")
    capacities, counts = decode_capacity(startup.fixture_report(
        startup_fixture, dol, module, ("--tier-capacity",), plan), models)
    child = parent.child
    payload = parent.bucket.qualified_payload(image, afs, (child.RECEIVER, child.RECEIVER_BLOCK_SHA256,
                                                            child.ACTOR_SHA256[child.RECEIVER]))
    contacts = decode_contacts(startup.fixture_report(pair_fixture, dol, module, ("--tiers",), payload))
    raw = startup.encode({
        "kind": "gf-retail-damage-tier-clock", "revision": 1, "source_dol_sha256": startup.SOURCE_SHA256,
        "source_afs_sha256": startup.AFS_SHA256, "stock_startup_sha256": STARTUP_SHA256,
        "type41_parent_contact_sha256": PARENT_SHA256,
        "capacity_counts": counts, "normal_receivers": capacities, "cases": contacts,
        "tier_factors": report["factors"],
        "clock_controls": {"skip_actor_flag": "01000000", "source_step_field": "1DCC", "declared_step": 0.5,
                           "refreshed_secondary_while_held": 0.03, "expiry_visit": 120},
        "notes": [
            "Normal capacity rows execute the original installer and level-1 constructor prefix. They do not admit actors or qualify variants.",
            "The actual common area pairs sample the old tier at the HP writer. Each surviving pair completes before the original wrapper ticks.",
            "Gauge wear and meter charge consume distinct fields. The declared near-expiry controls expire one without expiring the other.",
            "The control phase bit freezes both clocks. With that bit clear they subtract the declared raw lane factor even during the separate contact hold.",
            "No full actor/stage schedule, global animation timing, KO or native gameplay is established.",
        ],
    }, expanded=("normal_receivers", "cases"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "normal_receivers": len(capacities), "cases": len(contacts)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "startup_fixture", "pair_fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.startup_fixture, args.pair_fixture, args.module, args.output), sort_keys=True))
