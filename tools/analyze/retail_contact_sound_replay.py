"""Replay contact sound choice, counters, and independent presentation gates."""
import argparse
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import contact_sound as sound, retail_type32_roster as beam, retail_type9_roster as type9
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import AFS_SHA256, SOURCE_SHA256, encode, read_metadata
from tools.analyze.retail_rifle_contact_hp import SLICES
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"
SOUND_SHA256 = "a48eea4c85345436231bf45a6a5d5e52671b03400653472f0a3af41bfda930e6"
CENSUS_SHA256 = "0eb904daecf3a7188209b6ab8d5804460dc78e5cc2da9a690def380e4f22271c"
CODES = (*sound.CODES, 0xC0, 0xC8)
BANDS = (0, 0, 1, 1, 2, 2, 3)
COUNTER_CASES = (
    ("wrap", 0x84, 0, 4, 255, 0, 0, 4, 57),
    ("shared-partition", 0x84, 0, 4, 0, 1, 0, 4, 56),
    ("held-gate", 0x84, 0, 4, 1, 1, 4, 4, 0),
    ("separate-partition", 0x84, 1, 17, 0, 1, 0, 4, 56),
    ("four-resets", 0x84, 0, 4, 1, 2, 0, 4, 57),
    ("special-cross-row", 0xC8, 0, 22, 0, 1, 0, 4, 92),
    ("ordinary-alias", 0x88, 1, 22, 1, 2, 0, 4, 67),
    ("silent", 0xFF, 1, 255, 0, 0, 0, 0, 0),
)
BAND_ROW = re.compile(r"impact-band:(?: [0-9]{1,3}){6}")
COUNTER_ROW = re.compile(r"impact-counter:(?: [0-9]{1,3}){8}")
PAIR_ROW = re.compile(r"impact-sound-pair:(?: [0-9]{1,3}){7}")
TEXT = (
    *sound.TEXT,
    (0x800EFB3C, 200, "166caf117dce26eea64c8dcca8c88f971b0926375bcf721e41062a840cf426ba"),
    (0x8002CDA4, 160, "2dcecd916811217d411f21faca6a1ca97774fa3c65c5f3756bf92d230096a693"),
    (0x8002E2A8, *SLICES[0x8002E2A8]),
)


def f32_successor(value):
    bits, = struct.unpack(">I", struct.pack(">f", value))
    return struct.unpack(">f", struct.pack(">I", bits+1))[0]


SIZES = (0.0, 1.5, f32_successor(1.5), 2.5, f32_successor(2.5), 3.5, f32_successor(3.5))


def decode_helper(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != len(CODES)*len(SIZES)+len(COUNTER_CASES):
        raise ValueError("contact sound helper report extent")
    bands = []
    for code in CODES:
        for sample, size in enumerate(SIZES):
            line = lines[len(bands)]
            if BAND_ROW.fullmatch(line) is None:
                raise ValueError("malformed contact sound band row")
            band = BANDS[sample] if code & 0x80 else 0
            lane = (code & 0x3C)+band
            special = bool(code & 0x40)
            slot, tone = lane+12*special, 48+2*lane+24*special
            if tuple(map(int, line.split()[1:])) != (code, sample, slot, tone, 4, 1):
                raise ValueError(f"contact sound threshold, tone or counter changed: {code:02X}/{sample}")
            bands.append({"code": code, "receiver_size": size, "size_band_used": band, "counter_byte": slot,
                          "tone_id": tone, "sound_gate_after": 4, "counter_after": 1, "special_group": special})
    counters = []
    for i, (name, code, partition, slot, before, after, gate_before, gate_after, tone) in enumerate(COUNTER_CASES):
        line = lines[len(bands)+i]
        if (COUNTER_ROW.fullmatch(line) is None or tuple(map(int, line.split()[1:])) !=
                (i, code, slot, before, after, gate_before, gate_after, tone)):
            raise ValueError(f"contact sound sharing, parity or held gate changed: {name}")
        counters.append({"case": name, "code": code, "receiver_partition": partition,
                         "counter_byte": slot if slot != 255 else None, "counter_before": before if slot != 255 else None,
                         "counter_after": after if slot != 255 else None, "tone_id": tone or None,
                         "sound_gate_before": gate_before, "sound_gate_after": gate_after})
    return bands, counters


def decode_pairs(report):
    lines = report.splitlines()
    if len(report) > 512 or len(lines) != 4:
        raise ValueError("contact sound pair report extent")
    rows = []
    for i, line in enumerate(lines):
        sound_held, visual_held = bool(i&1), bool(i&2)
        if (PAIR_ROW.fullmatch(line) is None or tuple(map(int, line.split()[1:])) !=
                (i, not sound_held, not visual_held, 4, 4, 163, 1)):
            raise ValueError(f"contact sound/visual gate independence changed: {i}")
        rows.append({"sound_gate_before": 4 if sound_held else 0, "visual_gate_before": 4 if visual_held else 0,
                     "sound_gate_after": 4, "visual_gate_after": 4, "sound_counter_delta": int(not sound_held),
                     "visual_requested": not visual_held, "visual_allocated": False,
                     "hp_lost": 17, "receiver_hp_after": 163, "accepted_token": 1})
    return rows


def source_codes(image, census):
    actor, common, special_records = Counter(), Counter(), []
    for field, hist in (("actor_amount_records", actor), ("common_amount_records", common)):
        for scoped in census[field]:
            raw = image.read(int(scoped["record"], 16), 24, "data")
            if struct.unpack_from(">H", raw)[0] != scoped["raw_power"] or raw[9] != scoped["visual_selector_byte"]:
                raise ValueError("contact sound amount record no longer matches the scoped census")
            hist[raw[10]] += 1
            if field == "actor_amount_records" and raw[10] not in sound.CODES:
                special_records.append({"record": scoped["record"], "code": raw[10], "references": scoped["references"]})
    if actor != {0: 1, 128: 34, 136: 33, 192: 2, 200: 3} or common != {0: 22}:
        raise ValueError("contact sound scoped code coverage changed")
    return {"actor_code_counts": dict(actor), "common_code_counts": dict(common),
            "actor_records_outside_native_ordinary_codes": special_records}


def fixture_report(fixture, dol, module, options=(), payload=b""):
    args = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), *options]
    proc = run(args, input=payload, capture_output=True, timeout=45)
    if proc.returncode or proc.stderr:
        raise ValueError(f"contact sound fixture failed: {proc.stderr.decode('utf-8', 'replace').strip()}")
    return proc.stdout.decode("ascii")


def export(dol, afs, sound_fixture, type9_fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("contact sound replay output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    beam.verify_text(image, TEXT)
    if sound.recover(image) != read_metadata(MAPS / "contact-sound.json", SOUND_SHA256):
        raise ValueError("contact sound static lookup receipt changed")
    for pc, target in ((0x8002F894, 0x8002FC18), (0x8002FCA8, 0x800F088C),
                       (0x800F090C, 0x800F04B4), (0x800F050C, 0x800EFB3C)):
        beam.link(image, pc, target)
    if image.read(0x802C4838+10, 1, "data") != b"\0":
        raise ValueError("contact sound paired common code changed")
    codes = source_codes(image, read_metadata(MAPS / "gg4e08-contact-effect-census.json", CENSUS_SHA256))
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        archive = RetailArchive(stream)
        bank = type9.common_bank(archive)
        _, _, _, common, _ = type9.source_index(image, bank)
        body, _ = type9.contact_inlet(image, archive)
        if stamp != archive_stamp(stream):
            raise ValueError("contact sound replay AFS changed during extraction")
    bands, counters = decode_helper(fixture_report(sound_fixture, dol, module))
    pairs = decode_pairs(fixture_report(type9_fixture, dol, module, ("--impact-sound",), bank+body))
    data = {
        "kind": "gf-retail-contact-sound-replay", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "contact_sound_sha256": SOUND_SHA256, "contact_effect_census_sha256": CENSUS_SHA256,
        "scoped_source_codes": codes, "band_cases": bands, "counter_cases": counters, "pair_cases": pairs,
        "paired_source": {"type": 9, "key": 4, "amount_record": common["amount_record"], "contact_code": 0,
                          "scope": "standalone type-9 effect hits stationary Sasuke"},
        "notes": [
            "Band tests supply the original receiver size field; the tests immediately above thresholds use the next binary32 value.",
            "The disabled mixer path leaves the selected ID in r3. The fixture observes that register and the actual counter byte, not playback.",
            "Receiver partitions are declared source bytes. Special-group slot offsets can alias an ordinary slot in the next partition.",
            "The accepted-pair replay holds the sound and visual gates independently and leaves the visual class-2 pool empty.",
            "The isolated special-code probes do not enable native special bindings or qualify their live contact routes, mixing, or output.",
        ],
    }
    raw = encode(data, expanded=("band_cases", "counter_cases", "pair_cases"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "band_cases": len(bands), "counter_cases": len(counters), "pair_cases": len(pairs)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "sound_fixture", "type9_fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.sound_fixture, args.type9_fixture, args.module, args.output), sort_keys=True))
