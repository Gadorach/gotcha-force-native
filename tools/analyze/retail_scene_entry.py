"""Replay the original scene readiness scan, countdown and actor handoff."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze import retail_replacement_entry as entry


s, area = entry.s, entry.area
ENTRY_SHA256 = "a1e8c383eb3717c56969fdeef95d2894a87f686249861ccfacefa933464194f9"
TEXT = (
    (0x80028264, 92, "2963c21df457f779bb828307c1e65896e4adc63f1be017def76375c6150752bd"),
    (0x800288FC, 256, "86717406bfd4338d87bc1a357d6f051f6c36b0f82ca46c3b115e6daa0321c9fe"),
    (0x800289FC, 1896, "2656010b540078a686e4696822d983832c9d5f55dd7b2a01b1c52a7836c54ce5"),
    (0x80029164, 180, "eb13594f85edfd5251a3bc25811178b991506f480306f82922d332a5c4bac378"),
    (0x80029218, 368, "33ac965ef10300c520ec86b4efc0f621309e9537208fa6d4a409c8049e3f63b5"),
    (0x80029388, 84, "01c798ad521e46d52f217127c7f8de95a3e32cc4a7eaa07d6b252661158bf90f"),
    (0x800293DC, 44, "76c8fbe6d6de6ff2b70902def62220b11580af4f37f9b19a92591bf475e08ac7"),
    (0x80029408, 44, "5bdf670ba9a310c7766e39eefa0c46286cd68c072abfb663f64af1e032006439"),
    (0x80029434, 320, "6b951fb1955f3acb931beff0831af002d9b6ff9b108a15796a3114b5020b1b6c"),
    (0x800529EC, 16, "e66d02faa7bc7948ed4effca13d0faad3e88254cd76d86d2c21b3ed448b6b337"),
    (0x80053B30, 1344, "8c5a0b248d9fa0b131b7ff20346ee5af01ea3453ae1c8c9a18012ff8839a6890"),
    (0x80053A2C, 104, "1a8719769697cc0f7a8a1a711983175c6b9823e6c5e080885fa83dd1430cb177"),
)
HANDLERS = ("80029218", "80029388", "800293DC", "80029408")
CLOCKS = (
    ("short", ((1, 0, 1, 0), (2, 1, 2, 90), (91, 2, 2, 1), (92, 2, 3, 0), (93, 3, 3, 0))),
    ("extended", ((1, 0, 1, 120), (2, 1, 1, 119), (120, 1, 1, 1), (121, 1, 2, 90),
                  (210, 2, 2, 1), (211, 2, 3, 0), (212, 3, 3, 0))),
    ("skip-delays", ((1, 0, 3, 0), (2, 3, 3, 0))),
)


class Report:
    def __init__(self, report, limit):
        self.lines = report.splitlines()
        if len(report) > 4096 or len(self.lines) != limit:
            raise ValueError("normal scene entry report extent")
        self.pos = 0

    def take(self, tag, fields):
        expected = tag + ": " + " ".join(map(str, fields))
        if self.pos >= len(self.lines) or self.lines[self.pos] != expected:
            raise ValueError(f"normal scene entry changed at row {self.pos+1}: {tag}")
        self.pos += 1


def decode_scene(report):
    probe = Report(report, 27)
    seats, clocks = [], []
    for i in range(6):
        probe.take("scene-seat", (i, 0, HANDLERS[0], 3, 0, 37, 1))
        probe.take("scene-seat", (i, 1, HANDLERS[0], 3, 1, 0, 1))
        seats.append({
            "seat": i, "active_unready_leaves_substep": 0, "inactive_unready_allows_substep": 1,
            "ready_actors_have_zero_hp": True, "status_during_scan": 3,
        })
    for i, (name, visits) in enumerate(CLOCKS):
        samples = []
        for visit, handler, substep, timer in visits:
            status = 4 if handler == 3 else 3
            probe.take("scene-clock", (i, visit, HANDLERS[handler], status, substep, timer, int(status == 3)))
            samples.append({"visit": visit, "handler": HANDLERS[handler], "scene_substep": substep,
                            "timer": timer, "status_before_actor_pass": status, "hud_byte_13": int(status == 3)})
        clocks.append({"case": name, "scene_status_four_visit": visits[-1][0], "samples": samples})
    probe.take("scene-ending", (0, 1, "800289FC", 5, 1, 1, 1))
    return seats, clocks


def decode_bridge(report):
    probe = Report(report, 28)
    rows = []
    for caster, _, _, _ in area.bucket.MODELS:
        samples, pulses = [], []
        for n, handler, substep, timer in (
            (1, 0, 0, 0), (20, 0, 0, 0), (79, 0, 0, 0), (80, 0, 0, 0), (81, 0, 1, 0), (82, 1, 2, 90),
            (83, 2, 2, 89), (171, 2, 2, 1), (172, 2, 3, 0), (173, 3, 3, 0), (174, 3, 3, 0),
        ):
            early = n < 173
            colmask = "00EF" if n == 1 else "00C3" if early else "0000"
            polled = n == 174
            if n >= 172:
                probe.take("scene-bridge-b", (caster & 255, n, int(polled), 9 if polled else 10, colmask,
                                             "80130584" if polled else "80054144"))
                pulses.append({"visit": n, "channel_0_stock_after": 9 if polled else 10,
                               "factory_request": {"key": 2, "pose": 10, "allocation_ran": False} if polled else None})
            probe.take("scene-bridge", (caster & 255, n, int(n > 80), int(n >= 80), HANDLERS[handler],
                                       3 if early else 4, substep, timer, 0 if n < 80 else 1 if early else 2,
                                       colmask, "00000000", int(polled), 0 if early else 9))
            samples.append({
                "visit": n, "ready_before_coordinator": n > 80, "ready_after_actor": n >= 80,
                "scene_handler": HANDLERS[handler], "scene_substep": substep, "scene_timer": timer,
                "scene_status": 3 if early else 4, "actor_subphase": 0 if n < 80 else 1 if early else 2,
                "role_mask": colmask, "receiving_guard": 0, "processed_pad_read": polled,
                "ordinary_receiving_shapes_after_rebuild": 0 if early else 9,
            })
        rows.append({
            "initial_attacker": f"gg4e08:{caster:04x}", "area_key": 19+int(caster == 0x0107),
            "replacement": "gg4e08:0106", "physical_seat": 1,
            "actor_marks_ready": 80, "scene_observes_ready": 81, "scene_writes_four": 173,
            "ordinary_receiving_publishes": 173, "first_processed_pad_read": 174,
            "samples": samples, "fresh_b_forks": pulses,
        })
    return rows


def verify_source(image):
    entry.verify_source(image)
    s.verify_text(image, TEXT)
    for start, values in (
        (0x802CFDE8, (0x800288FC, 0x800289FC)),
        (0x802CFE3C, tuple(int(handler, 16) for handler in HANDLERS)),
    ):
        if image.read(start, 4*len(values), "data") != struct.pack(f">{len(values)}I", *values):
            raise ValueError("normal scene entry source table changed")
    for site, target in ((0x80028918, 0x80029164), (0x800291C0, 0x80029434),
                         (0x800290F8, 0x80029434), (0x80029420, 0x800529EC),
                         (0x80029458, 0x80053B30), (0x80029488, 0x80053A2C), (0x80053A58, 0x80054070)):
        s.link(image, site, target)


def export(dol, afs, scene_fixture, body_fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("normal scene entry output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_source(image)
    s.read_metadata(s.MAPS / "gg4e08-replacement-entry.json", ENTRY_SHA256)
    seats, clocks = decode_scene(s.fixture_report(scene_fixture, dol, module))
    bodies = decode_bridge(s.fixture_report(body_fixture, dol, module, ("--replacement-scene",),
                                            entry.replacement.payload(image, afs)))
    raw = s.encode({
        "kind": "gf-retail-scene-entry", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "replacement_entry_sha256": ENTRY_SHA256,
        "readiness_seats": seats, "scene_clocks": clocks, "replacement_handoffs": bodies,
        "ending_control": {"outer_step": 3, "inner_step": 1, "timer_before": 2, "timer_after": 1,
                           "status_before_actor_pass": 5},
        "notes": [
            "The standalone coordinator fixtures use declared physical actor slots and stop at the original actor pass entry; the readiness scan is not an HP or resource-owner check.",
            "The extended and skip-delay countdown controls are not assigned to the loaded replacement actor. No presentation objects are offered.",
            "The replacement traces run from the original scene dispatcher through the actual body update, cut before actor presentation. The receiving registry pass is invoked separately.",
            "The loaded actor writes readiness after the blocking scan. The original scene coordinator sees that write on its next visit and later sets status 4 before dispatching the actor.",
            "The mode-0 B forks supply processed pad data and stop at the factory or presentation entry. They do not qualify physical polling or bomb placement.",
            "The ending control is a separate declared scene inlet; no reversal back from ending state 5, complete match frames, or native gameplay is established.",
        ],
    }, expanded=("readiness_seats", "scene_clocks", "replacement_handoffs"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "readiness_seats": len(seats), "clocks": len(clocks), "handoffs": len(bodies)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "scene_fixture", "body_fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.scene_fixture, args.body_fixture, args.module, args.output),
                     sort_keys=True))
