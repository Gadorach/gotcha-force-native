"""Compare exact normal Blue Striker's unit-rate boost prefixes with native cruise."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze.retail_code_atlas import read_bounded
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage


MODEL, MEMBER = 0x0D00, 672
MEMBER_SHA256 = "eba3a267ada2a759640e0c47a94687ed721703c18bdb9d78b669014865e2b4f4"
RECORD_SHA256 = "fa220945c0852ba65835ebd497636417f26c1c03bf476e2dc51b72beb6c57ca6"
EVIDENCE = "docs/api-map/gg4e08-blue-striker-boost.json"
EVIDENCE_SHA256 = "157fed34030b613b4e755f105759123712821d8567472efedfb29c6951c6adcf"
OLD = (240, 240, 12, 12, 8, 4)
SCOPED = (240, 10000, 50, 50, 16, 31)
KIND = "gf-retail-blue-striker-boost"
TEXT = (
    (0x800F7580, 224, "db68393ccc31e1daaaf1a3face68f75d1cba91d0814f46bb6499699caba5e83f"),
    (0x8005614C, 52, "3ad751d94fd5b75a4ced6d2a1719b43ff420497090fec94a773a4799f48b5e4d"),
    (0x80056180, 312, "105bfba36f7609d8ecc447ebbbfbfa55bb82e52eeaee98cd9c0c8e09b0420494"),
    (0x80055568, 292, "4ef4d2db70068dec76e280773b3d58c546452599966914bffd4f8cbaf56472c7"),
    (0x8005568C, 1856, "0f964164be0f7581e45a26f8acd1113b414d13ecdbefeb94be72acde53c156ac"),
    (0x800633C8, 636, "44a8bb2556386c798f1faf61939dca491f42738cf5347a311fcdee9951b44e5a"),
    (0x80063644, 716, "2a9ad4407f89b7b1fea3700c2a380bec1594348dc6424f658bd9ae0a2c614d11"),
    (0x800664A0, 144, "0c6f2d5c5eefca037394bae294df3ae24761eebb49e8d19b6bf71f3e75dd34aa"),
    (0x80069860, 96, "71628a78b0b29ebc97f6aeb1f4d4c045c56e95a82b799f61025e39a8664c1972"),
    (0x800698C0, 88, "4ce45736ac0ec00239a4b89edf9da9fdce9374ab99cc6927d24b8c4e212a5107"),
    (0x8006A750, 100, "0512ba29e89a10d5dc9786ce20496633ed39f7b752b7c0313fa5afa0673e7233"),
    (0x800F0104, 272, "5d114b7dd7dc8252d474b0681d7b6fa1db756427eb0d67c68db3e00a318ec5b4"),
)
EXTENTS = (49, 54, 250)
FIELDS = ("visit", "held", "pulse", "selector_before", "latch_before", "meter_before", "delay_before",
          "selector_after", "meter_after", "delay_after", "latch_after", "remaining_clock")


def verify_source(image):
    from tools.analyze.retail_type32_roster import link, verify_text
    from tools.extract.retail_borg_roster import source_table

    if image.sha256 != SOURCE_SHA256 or image.read(0x8031CD78, 18).hex() != "0f03030007000000000000000d0e0e020107":
        raise ValueError("unqualified normal Blue Striker source or config")
    verify_text(image, TEXT)
    for site, target in ((0x80056160, 0x80056180), (0x80055588, 0x80055DCC), (0x80055590, 0x8005568C),
                         (0x80055598, 0x80059068), (0x80063448, 0x80066C58), (0x8006365C, 0x800664A0),
                         (0x80063678, 0x800F0104), (0x80063760, 0x80066C58),
                         (0x800698AC, 0x8006A750), (0x80069904, 0x8006A750)):
        link(image, site, target)
    if (image.read(source_table(image, 0x802D0584, MODEL), 4) != struct.pack(">I", MEMBER)
            or image.read(source_table(image, 0x802D3224, MODEL), 4) != struct.pack(">I", 0x800F7580)):
        raise ValueError("normal Blue Striker member or installer mapping changed")


def verify_record(record):
    if (len(record) != 432 or sha256(record).hexdigest() != RECORD_SHA256
            or struct.unpack_from(">7h", record, 4) != (10000, 16, 50, 0, 0, 0, 50)
            or struct.unpack_from(">f", record, 0x64) != (30.0,)):
        raise ValueError("unqualified Blue Striker row or meter/clock operands")


def payload(path):
    from tools.analyze.retail_action_input import archive_stamp, verify_archive
    from tools.extract.retail_borg_placeholders import open_regular
    from tools.extract.retail_borg_roster import RetailArchive

    with open_regular(path) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        member, digest = RetailArchive(stream).member(MEMBER, MODEL)
        record = member.read(0, 432)[0]
        if stamp != archive_stamp(stream) or digest != MEMBER_SHA256:
            raise ValueError("normal Blue Striker archive identity changed")
    verify_record(record)
    return record


def decode(text):
    lines = text.splitlines()
    if len(text) > 24000 or len(lines) != sum(EXTENTS):
        raise ValueError("Blue Striker source trace extent")
    rows = []
    for route, extent in enumerate(EXTENTS):
        samples, prior = [], (27, 0, 10000, 0)
        for visit in range(1, extent+1):
            line = lines[sum(EXTENTS[:route])+visit-1]
            if re.fullmatch(r"blue-boost:(?: [0-9]{1,5}){13}", line) is None:
                raise ValueError("malformed Blue Striker boost sample")
            r, v, held, pulse, old, ready, before, old_delay, next_, after, delay, next_ready, clock = map(int, line.split()[1:])
            down = int(visit == 1 if route == 0 else visit < 37 if route == 1 else visit <= 201 or visit >= 218)
            if ((r, v, held, pulse) != (route, visit, down, int(visit == 1 or route == 2 and visit == 218))
                    or (old, ready, before, old_delay) != prior or next_ not in (27, 28)
                    or after > 10000 or delay > 16 or next_ready > 1 or clock > 30):
                raise ValueError("Blue Striker input/resource timeline changed")
            prior = next_, next_ready, after, delay
            samples.append(dict(zip(FIELDS, (v, held, pulse, old, ready, before, old_delay, next_, after, delay,
                                             next_ready, clock), strict=True)))
        rows.append(samples)
    return rows


def summarize(rows):
    sample = lambda route, visit: rows[route][visit-1]
    edges = []
    for route, (name, exit_) in enumerate((("early-release", 32), ("late-release", 37), ("held-depletion-and-repress", 201))):
        opening, latch, exit_row = (sample(route, v) for v in (1, 31, exit_))
        if (opening["meter_after"], opening["remaining_clock"], latch["meter_after"], latch["latch_after"],
                latch["remaining_clock"], exit_row["selector_after"], exit_row["delay_after"]) != (9950, 30, 8450, 1, 0, 27, 16):
            raise ValueError("Blue Striker seed, latch or ordinary exit changed")
        for wait in range(1, 17):
            row = sample(route, exit_+wait)
            if row["meter_after"] != exit_row["meter_after"] or row["delay_after"] != 16-wait:
                raise ValueError("Blue Striker delay-only pass changed")
        key = (1, 2, 30, 31, exit_, exit_+1, exit_+16, exit_+17)
        if route == 2:
            key += (219, 248, 249)
            refire = sample(route, 218)
            if ((refire["meter_before"], refire["meter_after"], refire["selector_after"],
                    refire["remaining_clock"], refire["delay_after"]) != (0, 0, 28, 30, 16)
                    or sample(route, 249)["selector_after"] != 27):
                raise ValueError("Blue Striker refill-funded re-entry or empty early hold changed")
        elif sample(route, exit_+17)["meter_after"] != exit_row["meter_after"]+50:
            raise ValueError("Blue Striker first delayed refill changed")
        edges.append({"case": name, "samples": [sample(route, v) for v in key]})
    return edges


def export(dol, afs, fixture, module, native, output):
    from tools.analyze.retail_action_input import AFS_SHA256
    from tools.analyze.retail_contact_pose_clock import fixture_report
    from tools.analyze.retail_projectile_census import encode

    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("Blue Striker output already exists")
    dol = Path(dol).resolve(strict=True)
    verify_source(DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256))
    original = fixture_report(fixture, dol, module, payload=payload(afs))
    rows = decode(original)
    p = run([Path(native).resolve(strict=True)], capture_output=True, text=True, timeout=20)
    controls = "blue-control: 240 4 5 14\nblue-control: 10000 30 31 48\n"
    if p.returncode or p.stderr or p.stdout != original+controls:
        raise ValueError("Blue Striker source/native meter trace or defaults disagree")
    raw = encode({
        "kind": KIND, "revision": 1, "borg": "gg4e08:0d00", "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "member": MEMBER, "member_sha256": MEMBER_SHA256,
        "record": 0, "record_sha256": RECORD_SHA256, "declared_rate": 1,
        "source_clock_seed": 30, "native_minimum_handler_visits": 31,
        "source_native_matched_visits": sum(EXTENTS), "normalized_trace_sha256": sha256(original.encode("ascii")).hexdigest(),
        "native_controls": [{"capacity": 240, "minimum": 4, "exit_visit": 5, "first_refill_visit": 14},
                            {"capacity": 10000, "minimum": 30, "exit_visit": 31, "first_refill_visit": 48}],
        "traces": summarize(rows),
        "notes": [
            "Original class installer and resource startup select normal record zero. Scoped visits join pre-input bookkeeping, a conditioned selection leaf and the lower handler prefix, not a scheduled actor frame.",
            "The original resource rate is declared as one and the held/pulse samples are supplied. An already-held cue slot avoids playback. Heading, travel, floor contact, earlier action owners and the HUD are not part of the comparison.",
            "The native authored age counts the entry handler visit. Its projected remaining clock on this one unit-rate route is 31 minus the elapsed boost age. Do not copy 30 literally into the authored minimum.",
            "The existing cruise controller matches debit, latch and refill ordering with the scoped row, while altitude, speed, turn, controls and combat remain authored.",
        ],
    }, expanded=("traces", "native_controls"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as f:
        f.write(raw)
    return {"matched_visits": sum(EXTENTS), "sha256": sha256(raw).hexdigest()}


def qualify(image, path=None):
    raw = read_bounded(path if path is not None else Path(__file__).resolve().parents[2]/EVIDENCE, 32 << 10)
    if sha256(raw).hexdigest() != EVIDENCE_SHA256:
        raise ValueError("unqualified Blue Striker boost receipt")
    verify_source(image)
    evidence = json.loads(raw)
    if (evidence["kind"], evidence["revision"], evidence["borg"], evidence["source_dol_sha256"],
            evidence["member_sha256"], evidence["record_sha256"], evidence["declared_rate"],
            evidence["source_clock_seed"], evidence["native_minimum_handler_visits"],
            evidence["source_native_matched_visits"]) != (
            KIND, 1, "gg4e08:0d00", SOURCE_SHA256, MEMBER_SHA256, RECORD_SHA256, 1, 30, 31, sum(EXTENTS)):
        raise ValueError("Blue Striker receipt scope changed")
    return {"source_evidence": EVIDENCE, "source_evidence_sha256": EVIDENCE_SHA256,
            "source_clock_seed": 30, "native_minimum_handler_visits": 31, "declared_rate": 1,
            "retail_behavior_verified": False}


def row(fields):
    return ("cruise-boost="+"|".join(map(str, fields))+"\n").encode("ascii")


def bind(model, digest, record, gameplay, receipt, proof):
    if model != MODEL:
        return gameplay
    verify_record(record)
    if (digest != MEMBER_SHA256 or not receipt or receipt.get("source_level") != 1
            or receipt.get("retail_behavior_verified") is not False or receipt.get("native_cruise") != OLD
            or receipt.get("source_charge_factory") != {"key": 20, "site": "800F80CC"}
            or "blue_striker_boost_test" in receipt):
        raise ValueError("Blue Striker boost opt-in needs the exact normal authored cruise")
    lines = gameplay.splitlines(keepends=True)
    if (any(lines.count(line) != 1 for line in (b"variant=normal\n", b"body-record=0\n",
                                                b"body-motion=static-cruise\n", row(OLD)))
            or sum(line.startswith(b"cruise-boost=") for line in lines) != 1):
        raise ValueError("Blue Striker boost opt-in cannot replace other forms or policies")
    receipt["native_cruise"] = SCOPED
    receipt["blue_striker_boost_test"] = dict(proof)
    return b"".join(row(SCOPED) if line == row(OLD) else line for line in lines)


if __name__ == "__main__":
    cli = argparse.ArgumentParser(description=__doc__)
    for arg in ("dol", "afs", "fixture", "module", "native", "output"):
        cli.add_argument(arg, type=Path)
    opts = cli.parse_args()
    print(json.dumps(export(opts.dol, opts.afs, opts.fixture, opts.module, opts.native, opts.output), sort_keys=True))
