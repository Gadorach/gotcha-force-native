"""Verify original early counters, body-rate refresh, and the scheduled lower wait."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re

from tools.analyze import retail_contact_clock_consumers as consumers, retail_contact_pose_clock as clock
from tools.analyze import retail_melee_pose_clock as melee, retail_program_cues as cues, retail_type32_roster as beam
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import AFS_SHA256, INSTALLERS_SHA256, SOURCE_SHA256, encode, read_metadata
from tools.extract.source_image import DolImage


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"
CONSUMERS_SHA256 = "c3488c0f4d5055d6ac70cbaaba4cacd52d9d369157fa232820830f2ed99def20"
ROW = re.compile(r"frame-clock:(?: [0-9]{1,4}){5}(?: [0-9A-F]{8}){3} [0-9] [0-9A-F]{8} [01]")
TEXT = (
    (0x80055568, 292, "4ef4d2db70068dec76e280773b3d58c546452599966914bffd4f8cbaf56472c7"),
    (0x8005568C, 1856, "0f964164be0f7581e45a26f8acd1113b414d13ecdbefeb94be72acde53c156ac"),
    (0x8006F4F8, 276, "9bedc00559917b9e5fa76d02977c59be604bed473605fc394e3df7af750ee800"),
    (0x8005F554, 36, "68a2be2fe7c7f193b01d03ae977f0273e2c9d76d2fdce5bec7a0ac3f9b328c74"),
    (0x80059138, 4, "f332ea5b5437103cbb6f1508679da89eec9288ad775c96c439a17fccabe3de8e"),
    (0x8006F950, 4, "f332ea5b5437103cbb6f1508679da89eec9288ad775c96c439a17fccabe3de8e"),
    (0x8004CD24, *cues.SPANS[0x8004CD24]),
)


def decode(report):
    lines = report.splitlines()
    if len(report) > 2048 or len(lines) != 22:
        raise ValueError("contact frame prefix report extent")
    rows = []
    for route in melee.ROUTES:
        for scenario in (1, 2):
            timer = route.seed if scenario == 2 else 0
            incoming, counter, integer, wait = .375, 4., 8, 1.
            for visit in range(1, max(3, timer+2)+1):
                before = timer
                counter = consumers.f32(counter-incoming)
                if not before:
                    integer -= 1
                timer = max(0, before-1)
                outgoing = consumers.f32(.03) if timer else .375
                wait = max(0., consumers.f32(wait-outgoing))
                expected = [str(v) for v in (
                    route.model, scenario, visit, before, timer, consumers.bits(incoming), consumers.bits(outgoing),
                    consumers.bits(counter), integer, consumers.bits(wait), int(wait == 0.),
                )]
                line = lines[len(rows)]
                if ROW.fullmatch(line) is None or line.split()[1:] != expected:
                    raise ValueError(f"contact frame counter order changed: {route.model:04X}/{scenario}/{visit}")
                rows.append({
                    "attacking_borg": f"gg4e08:{route.model:04x}", "case": "accepted-hit" if scenario == 2 else "whiff-control",
                    "visit": visit, "receiver_hp": 180-route.lost if scenario == 2 else 180,
                    "timer_before": before, "timer_after": timer, "incoming_secondary_bits": consumers.bits(incoming),
                    "refreshed_secondary_bits": consumers.bits(outgoing),
                    "early_float_counter_bits": consumers.bits(counter), "early_integer_counter": integer,
                    "scheduled_lane_wait_bits": consumers.bits(wait), "scheduled_lane_complete": wait == 0.,
                })
                incoming = outgoing
    return rows


def verify_source(image):
    beam.verify_text(image, TEXT)
    for site, target in ((0x80055590, 0x8005568C), (0x80055598, 0x80059068), (0x80059118, 0x8005A378),
                         (0x8005F564, 0x8004CD24)):
        beam.link(image, site, target)
    for at, want in (
        (0x802D3300+4*9, 0x80059138), (0x802D36A8+4*3, 0x8005F554), (0x802D37A8, 0x8005F238),
        (0x802D3AD4+8, 0x8006F7C0), (0x802D3AD4+12, 0x8006F88C), (0x802D3AD4+20, 0x8006F950),
        (0x802BD766, 0x00090001),
    ):
        if int.from_bytes(image.read(at, 4, "data"), "big") != want:
            raise ValueError("contact frame original callback or wait selection changed")
    records = read_metadata(MAPS / "gg4e08-standalone-installers.json", INSTALLERS_SHA256)
    sasuke = [r for r in records["borgs"] if r["id"] == "gg4e08:000a"]
    if len(sasuke) != 1 or tuple(sasuke[0][k] for k in (
        "installer", "pre_callback", "post_callback", "config_lane_mask_seed"
    )) != ("8006F4F8", "8006F7C0", "8006F88C", 3):
        raise ValueError("contact frame normal Sasuke profile changed")


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("contact frame prefix output already exists")
    dol, module = (Path(p).resolve(strict=True) for p in (dol, module))
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    read_metadata(MAPS / "gg4e08-contact-clock-consumers.json", CONSUMERS_SHA256)
    read_metadata(MAPS / "gg4e08-melee-pose-clock.json", consumers.MELEE_REPLAY_SHA256)
    verify_source(image)
    payload, _, _ = melee.qualified_inputs(image, afs)
    rows = decode(clock.fixture_report(fixture, dol, module, ("--frame-prefix",), payload))
    data = {
        "kind": "gf-retail-contact-frame-prefix", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "contact_clock_consumers_sha256": CONSUMERS_SHA256,
        "melee_pose_clock_sha256": consumers.MELEE_REPLAY_SHA256, "standalone_installers_sha256": INSTALLERS_SHA256,
        "entries": {"sasuke_installer": "8006F4F8", "actor_update": "80055568", "early_counters": "8005568C",
                    "body_prepass": "80059068", "body_clock": "8005A378", "lower_slot_3": "8005F554",
                    "scheduled_wait": "8004CD24", "post_callback_cut": "8006F88C"},
        "fields": {"early_float_counter": "694", "early_integer_counter": "6B8",
                   "contact_timer": "71C", "secondary_rate": "1DC8"},
        "visits": rows,
        "notes": [
            "The normal Sasuke receiver is retained from the exact melee hit or whiff. The original class installer runs before the declared update prefix.",
            "Current and previous prepass mode 9, upper slot 0, lower slot 3, and the one-unit lane wait are declared; this does not infer them from live control or reaction selection.",
            "The declared scene byte +1829 suppresses the separate input-accumulation path.",
            "The original actor wrapper calls the early counters before refreshing the body clock. Its lower slot then consumes the refreshed rate.",
            "The early integer counter reads the timer before refresh; a timer of one keeps it held on the expiry visit, although the lane already uses the restored rate.",
            "The actual Sasuke post callback is the observation cut, after the lower VM visit but before world update.",
            "No complete actor frame, original controller polling, live phase order, world matrices or native gameplay are qualified.",
        ],
    }
    raw = encode(data, expanded=("visits",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "visits": len(rows),
            "rate_change_visits": sum(r["incoming_secondary_bits"] != r["refreshed_secondary_bits"] for r in rows)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
