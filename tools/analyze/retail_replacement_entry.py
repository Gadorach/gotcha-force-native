"""Verify the separate post-replacement gates for input and receiving publication."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze import retail_body_replacement as replacement


s, area = replacement.s, replacement.area
REPLACEMENT_SHA256 = "bec58b5e0f7d05188064bccc6e1013c033f4bdb34429844a3d8ca63fe898ca78"
TEXT = (
    (0x8005475C, 128, "1123e2ac001c1e70649a8db6558d85a462c2837f16c81a8c65bc00c567156b5e"),
    (0x800547DC, 1180, "41a9dbfa5c1cd37d956d837fab7a66c58a3f5b5aa5931f844cfb5c9cabf363cd"),
    (0x80054C78, 68, "81f97bf335f352727071f136d31de22a856e4d80a09d600ec17e9a8c0f4a8c45"),
    (0x80054CBC, 52, "5031b1cbdf20bc9464eef663b0778abe4c88b6c39931b9b0be6f0feabc0fdb26"),
    (0x80056900, 424, "52c4c7e19500682352bbf4503d8973d50f69fed24c5525d5b3ab40bba34bbfa2"),
    (0x8008BBC0, 16, "257d48a7d554a62bc9c18c1daf1fa73860c81815999016f64a0a7b01f7cda582"),
    (0x800685C0, 304, "ce30ce82c8a3802573cd9741dba970d10c207e00cb2307bc77d7031c1aae4758"),
    (0x8005568C, 1856, "0f964164be0f7581e45a26f8acd1113b414d13ecdbefeb94be72acde53c156ac"),
    (0x801304B8, 704, "75d567d05a4ea6ef6537d8d513e3f1f1822187128f2586e81b2cdf4cca442406"),
    (0x8008A2BC, 788, "2c811080af75624f6664c00f6e11b229fdb83bea44b41482b53b2ed14d2f0528"),
)


def fbits(value):
    return struct.pack(">f", value).hex().upper()


def decode(report):
    lines = report.splitlines()
    if len(report) > 8192 or len(lines) != 70:
        raise ValueError("normal replacement entry report extent")
    pos = 0
    cases = []

    def take(tag, fields):
        nonlocal pos
        if pos >= len(lines) or lines[pos] != tag + ": " + " ".join(map(str, fields)):
            raise ValueError(f"normal replacement entry changed at row {pos+1}: {tag}")
        pos += 1

    for caster, _, _, _ in area.bucket.MODELS:
        for phase in (4, 5):
            samples, presses = [], []
            actor_model = caster & 255

            def sample(visit, subphase, sequence, wait, mask, arrival, protection, polled, receivers, entering):
                take("replacement-entry", (actor_model, phase, visit, 1, subphase, sequence, wait,
                                           f"{mask:04X}", fbits(arrival), fbits(protection), polled,
                                           receivers, entering, 0))
                samples.append({
                    "visit": visit, "actor_subphase": subphase, "entry_sequence": sequence, "entry_wait": wait,
                    "role_mask": f"{mask:04X}", "arrival_timer": arrival, "receiving_guard": protection,
                    "processed_pad_reader_called": bool(polled), "ordinary_receiving_shapes": receivers,
                })

            def pulse(visit, *, suppressed=False, polled=False):
                paid = polled and not suppressed
                mask = 0 if (phase == 4 and visit >= 81) or visit == 162 else 0xC3
                take("replacement-pulse", (actor_model, phase, visit, int(suppressed), int(polled),
                                           "00000200" if paid else "00000000",
                                           "00000200" if paid else "00000000",
                                           "00000060" if paid else "00000000", int(paid),
                                           9 if paid else 10, f"{mask:04X}", "80130584" if paid else "80054144"))
                presses.append({
                    "visit": visit, "raw_pad_b": True, "scene_packet_suppressed": suppressed,
                    "processed_pad_reader_called": polled, "b_held": paid, "b_pressed": paid,
                    "conditioned_request_flags": "00000060" if paid else "00000000",
                    "channel_0_stock_after": 9 if paid else 10,
                    "type41_request": {"key": 2, "pose": 10, "factory_entry_cut": True} if paid else None,
                })

            sample(0, 0, 0, 0, 0xEF, 0, 0, 0, 0, 1)
            sample(1, 0, 1, 0, 0xEF, 19, 0, 0, 0, 1)
            sample(2, 0, 1, 0, 0xEF, 18, 0, 0, 0, 1)
            sample(20, 0, 3, 60, 0xC3, 0, 0, 0, 0, 1)
            pulse(21)
            guard = 59 if phase == 4 else 0
            sample(21, 0, 3, 59, 0xC3, 0, guard, 0, 0, 1)
            sample(79, 0, 3, 1, 0xC3, 0, guard, 0, 0, 1)
            pulse(80)
            sample(80, 1, 0, 0, 0xC3, 0, guard, 0, 0, 0)
            pulse(81)
            late = phase == 5
            if not late:
                sample(81, 2, 0, 0, 0, 0, 58, 0, 0, 0)
                take("replacement-guard", (actor_model, phase, 0, 9))
                pulse(82, polled=True)
                pulse(82, polled=True, suppressed=True)
                sample(82, 2, 0, 0, 0, 0, 57, 1, 0, 0)
                sample(138, 2, 0, 0, 0, 0, 1, 1, 0, 0)
                sample(139, 2, 0, 0, 0, 0, 0, 1, 9, 0)
                sample(160, 2, 0, 0, 0, 0, 0, 1, 9, 0)
            else:
                pulse(82)
                for visit in (82, 138, 139, 160):
                    sample(visit, 1, 0, 0, 0xC3, 0, 0, 0, 0, 0)
                sample(161, 2, 0, 0, 0, 0, 0, 0, 9, 0)
                pulse(162, polled=True)
            cases.append({
                "initial_attacker": f"gg4e08:{caster:04x}", "area_key": 19+int(caster == 0x0107),
                "initial_scene_state": phase, "scene_state_change": {"before_visit": 161, "to": 4} if late else None,
                "role_mask_opens": 161 if late else 81, "first_processed_pad_read": 162 if late else 82,
                "first_fresh_b_paid_request": 162 if late else 82, "first_ordinary_receiver_publication": 161 if late else 139,
                "only_receiving_guard_zeroed": None if late else {"visit": 81, "before": 0, "after": 9},
                "timeline": samples, "b_press_forks": presses,
            })
    if pos != len(lines):
        raise ValueError("extra normal replacement entry rows")
    return cases


def verify_source(image):
    replacement.verify_source(image)
    s.verify_text(image, TEXT)
    for site, target in ((0x80054BFC, 0x80055568), (0x80054CA8, 0x80055568), (0x80054CD0, 0x80056900),
                         (0x800569E4, 0x8008BBC0), (0x800590B4, 0x800685C0),
                         (0x8013055C, 0x8006DBE0), (0x80130580, 0x800E058C)):
        s.link(image, site, target)
    if (image.read(0x802D3274, 16, "data") != struct.pack(">4I", 0x800547DC, 0x80054C78, 0x80054CBC, 0x80054CF0)
            or image.read(0x8043727C, 4, "data") != struct.pack(">f", 60)
            or image.read(0x80437C40, 4, "data") != bytes(4)):
        raise ValueError("normal replacement entry selectors or timer bounds changed")


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("normal replacement entry output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_source(image)
    s.read_metadata(s.MAPS / "gg4e08-body-replacement.json", REPLACEMENT_SHA256)
    cases = decode(s.fixture_report(fixture, dol, module, ("--replacement-entry",), replacement.payload(image, afs)))
    raw = s.encode({
        "kind": "gf-retail-replacement-entry", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "replacement_sha256": REPLACEMENT_SHA256,
        "borg": "gg4e08:0106", "seat": 1, "receiving_steps": list(replacement.RECEIVING_STEPS),
        "cases": cases,
        "notes": [
            "The original normal replacement constructor runs before these isolated actor dispatch prefixes and receiving-list rebuilds.",
            "Scene state 4 or 5 and processed pad snapshots are declared; neither the enclosing scene scheduler nor physical pad polling is replayed.",
            "Neutral timelines keep stock at 10. Discarded fresh-B forks stop before factory allocation or playback; a paid request is not a placed bomb.",
            "The receiving role mask and the separate receiving guard both matter. The guard-only control alters no role bits.",
            "Transitioning from scene state 5 to 4 late opens receiving publication before the first B sample in that branch, unlike the state-4-from-start run.",
            "No input admission rule for other bodies, whole-match timing, audible playback or native gameplay is established.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
