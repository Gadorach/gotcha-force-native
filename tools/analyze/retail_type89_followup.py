"""Replay type-89 contact descendants through group-110 growth, HP, and retirement."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import retail_type89 as type89, retail_type89_area as key16
from tools.analyze import retail_type89_cast as cast, retail_type89_contact as contact
from tools.analyze import retail_bomb_area as area_lifetime, retail_dragon_area as area
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import address, read_bounded
from tools.analyze.retail_projectile_census import AFS_SHA256, SOURCE_SHA256, encode
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage


CASES = ("no-spare-shape", "original-receiver-pose", "receiver-offset-100")
ROW = re.compile(r"type89-followup:(?: [0-9]{1,4}){10}(?: [0-9]{1,3}\.[0-9]{4}){3}")
RECORD = 0x802C4988


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 2048 or len(lines) != len(cast.MODELS)*len(CASES):
        raise ValueError("type-89 follow-up report extent")
    result = []
    for i, line in enumerate(lines):
        if ROW.fullmatch(line) is None:
            raise ValueError("malformed type-89 follow-up row")
        model = cast.MODELS[i//len(CASES)]
        scenario = i % len(CASES)
        bound = scenario != 0
        direct = 17 if model < 0x050F else 8
        amount = 17 if bound else 0
        values = line.split()[1:]
        ints, radii = tuple(map(int, values[:10])), tuple(map(float, values[10:]))
        if (ints != (model, scenario, direct, amount, 180-direct-amount, 2 if bound else 0,
                     (34 if scenario == 1 else 36) if bound else 0, 50 if bound else 0, 83 if bound else 35, 44)
                or radii != ((3.1, 34.1 if scenario == 1 else 96.1, 387.5) if bound else (0, 0, 0))):
            raise ValueError(f"type-89 follow-up bind, growth, HP or retirement mismatch: {model:04X}/{CASES[scenario]}")
        _, _, _, _, hp, token, first, unlink, retire, parent_retire = ints
        result.append({
            "borg": f"gg4e08:{model:04x}", "case": CASES[scenario],
            "spare_shape_at_area_bind": bound, "body_center_distance_from_area": 100 if scenario == 2 else 22,
            "direct": {"bucket_visit": 32, "hp_loss": direct, "receiver_hp_after": 180-direct, "token": 1},
            "area": {
                "key": 16, "bind_attempt_bucket_visit": 33, "bound": bound, "initial_registry_count": 0,
                "token": token or None, "initial_bound_radius": radii[0] if bound else None,
                "hit": {"bucket_visit": first, "radius": radii[1], "common_amount_record": address(RECORD),
                        "same_token_refused_on_repeat": True} if bound else None,
                "hp_loss": amount, "receiver_hp_after": hp, "peak_radius": radii[2] if bound else None,
                "shape_unlink_bucket_visit": unlink or None, "carrier_and_descriptors_return_bucket_visit": retire,
            },
            "parent_return_bucket_visit": parent_retire, "received_and_credited_total": direct+amount,
        })
    return result


def qualified_inlet(image, archive):
    contact_input, sources, body = contact.qualified_inlet(image, archive)
    bank = key16.qualified_key16_bank(image, archive)
    type89.verify_text(image, tuple((at, *span) for at, span in area_lifetime.SLICES.items()))
    if (struct.unpack(">4I", image.read(area.PHASE_TABLE, 16, "data")) != area_lifetime.PHASES
            or struct.unpack(">3I", image.read(area.PHASE_TABLE+16, 12, "data")) != (0x800862B8, 0x80086350, 0x8008643C)):
        raise ValueError("type-89 area phase or active subphase table mismatch")
    table = image.read(area.TABLE, 27*36, "data")
    row = table[16*36:17*36]
    if (sha256(table).hexdigest() != area.TABLE_SHA or row[:2]+row[4:] != table[:2]+table[4:36]
            or int.from_bytes(row[2:4], "big") != 110
            or image.read(RECORD, 24, "data") != bytes.fromhex("001400646464000000000002000401000020000000200000")):
        raise ValueError("type-89 area key-16 curve or common amount record mismatch")
    return contact_input+bank, sources, body


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type-89 follow-up output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        inlet, sources, body = qualified_inlet(image, RetailArchive(stream))
        if stamp != archive_stamp(stream):
            raise ValueError("type-89 follow-up AFS changed during extraction")
    args = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--followup"]
    result = run(args, input=inlet, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"type-89 follow-up fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-type89-followup", "revision": 1,
        "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "sources": sources, "cases": cases,
        "receiver": {**body, "borg": "gg4e08:000a", "level_one_hp": 180},
        "area_source": {"key": 16, "group": 110, "descriptor": 150, "reaction_index": 23,
                        "common_amount_record": address(RECORD), "raw_power": 20, "raw_radius": 310,
                        "common_bank_sha256": area.COMMON_COLLISION_SHA},
        "notes": [
            "The area comes from accepted type-89 contact in the clear-stage replay. Bucket visits count from the carrier initializer.",
            "The same-pass area bind runs before the parent shape is returned. No spare means refusal, not deferred admission.",
            "The offset-pose case relocates the same receiver once between the direct hit and area bind. Its token history is untouched.",
            "The child uses a distinct token and common reaction record. It retains its shape after the parent object retires.",
            "No live mode scheduling, simultaneous recipients, visual playback, other area keys, or native gameplay are qualified.",
        ],
    }
    raw = encode(payload, expanded=("sources", "cases"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
