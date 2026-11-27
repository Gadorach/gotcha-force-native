"""Replay the floor area's and sibling segment's same-bucket bind and separate HP writes."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import retail_bomb_area as area_lifetime, retail_dragon_area as area
from tools.analyze import retail_sweep_area_routes as sweep, retail_type5_roster as area_bank
from tools.analyze import retail_type32_contact as contact, retail_type32_roster as beam, retail_type32_stream as bucket
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import address, read_bounded
from tools.analyze.retail_projectile_census import AFS_SHA256, SOURCE_SHA256, encode
from tools.analyze.retail_rifle_contact_hp import ACTOR_SHA256
from tools.analyze.retail_shape_sweep import source_operands
from tools.analyze.retail_type0_contact import RECEIVER, RECEIVER_BLOCK_SHA256, read_normal
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive, levels
from tools.extract.source_image import DolImage


ACTOR_DIGESTS = {
    0x0E00: "2cc8fbb0682121f654e24d88fff8e33d54ae6c7941ca2cb09c9c11332b5f85bb",
    0x0E05: "4b2cbdd149c7d39ef6687b5606fb85c0aee7c83825e3a232938a6b189debe154",
    0x0E03: "51044a7ab58f2228a0ec75827a77257df658f4a825d503644308cbe18eedb832",
}
CASES = (("no-shapes", 2, 0), ("one-shape", 2, 1), ("two-shapes", 2, 2), ("one-carrier", 1, 1))
AREA_RECORD = 0x802C4778
ROW = re.compile(r"sweep-followup:(?: [0-9]{1,4}){11}(?: [0-9]{1,3}\.[0-9]{5}){2}")


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 2048 or len(lines) != len(sweep.PILOTS)*len(CASES):
        raise ValueError("sweep follow-up report extent")
    result = []
    for i, line in enumerate(lines):
        model, row, _, group, _, index, table, _ = sweep.PILOTS[i//len(CASES)]
        scenario = i % len(CASES)
        label, carriers, shapes = CASES[scenario]
        if ROW.fullmatch(line) is None:
            raise ValueError("malformed sweep follow-up row")
        values = line.split()[1:]
        segment_bound = carriers == 2 and shapes >= 1
        area_bound = shapes > int(segment_bound)
        area_token = int(area_bound)
        segment_token = 1 + area_token if segment_bound else 0
        segment_amount, area_amount = 26*int(segment_bound), 17*int(area_bound)
        hp = 180-segment_amount-area_amount
        registered = int(segment_bound)+int(area_bound)
        if tuple(map(int, values[:11])) != (model, scenario, segment_bound, area_bound,
                                             int(segment_bound), registered, segment_token, area_token,
                                             segment_amount, area_amount, hp):
            raise ValueError(f"sweep follow-up shape priority, token or HP mismatch: {model:04X}/{label}")
        radii = tuple(map(float, values[11:]))
        if radii != ((3.1, 34.1) if area_bound else (0.0, 0.0)):
            raise ValueError("sweep follow-up area growth mismatch")
        hit_order = (["area"] if area_bound else [])+(["segment"] if segment_bound else [])
        result.append({
            "borg": f"gg4e08:{model:04x}", "parent_row": row, "case": label,
            "carrier_cells_at_crossing": carriers, "attacker_shape_cells_at_crossing": shapes,
            "allocation_order": ["area", "segment"] if carriers == 2 else ["area"],
            "shape_admission_order": hit_order[::-1],
            "crossing_visit": 34 if model == 0x0E03 else 35,
            "opening_attack_registry_count": int(segment_bound),
            "area": {"key": 26, "shape_bound": area_bound, "token": area_token or None,
                     "radii": list(radii) if area_bound else None, "hp_loss": area_amount,
                     "common_amount_record": address(AREA_RECORD)},
            "segment": {"key": group, "shape_bound": segment_bound, "token": segment_token or None,
                        "hp_loss": segment_amount, "actor_amount_record": address(table+index*24)},
            "next_registry_order": hit_order, "hp_writer_order": hit_order,
            "receiver_hp_after": hp, "receiver_hp_lost": 180-hp, "same_registry_refused_after_hits": bool(hit_order),
        })
    return result


def qualified_inlet(image, archive):
    attack_input, sources = sweep.qualified_inlet(image, archive)
    beam.verify_text(image, bucket.TEXT+tuple(span for span in contact.TEXT if span[0] != 0x8003E978))
    beam.verify_text(image, tuple((at, *span) for at, span in area_lifetime.SLICES.items()))
    beam.link(image, 0x80089AC4, 0x8008997C)
    beam.link(image, 0x800860E0, 0x8008AE3C)
    common = area_bank.common_bank(archive)
    rows = image.read(area.TABLE, 27*36, "data")
    off = 0x400+4*80
    if (sha256(rows).hexdigest() != area.TABLE_SHA or rows[26*36:27*36] != rows[:36]
            or common[12:16] != bytes((4, 255, 255, 255))
            or struct.unpack_from(">BBHHhhH4f", common, off) != (0, 0, 1, 1, 0, -1, 0, 0, 0, 0, 310)
            or image.read(AREA_RECORD, 2, "data") != struct.pack(">H", 20)):
        raise ValueError("key-26 common area source or curve changed")
    body, defender = read_normal(archive, RECEIVER, RECEIVER_BLOCK_SHA256, ACTOR_SHA256[RECEIVER])
    if levels(image, RECEIVER)[0][0] != 180:
        raise ValueError("sweep follow-up receiver HP changed")
    specs = []
    for (model, _, _, _, _, index, table, digest), source in zip(sweep.PILOTS, sources, strict=True):
        actor_digest = ACTOR_DIGESTS[model]
        _, spec = read_normal(archive, model, digest, actor_digest)
        if levels(image, model)[0][0] != 2000 or image.read(table+index*24, 2, "data") != struct.pack(">H", 30):
            raise ValueError(f"sweep follow-up actor HP or raw segment power changed: {model:04X}")
        source.update(level_one_hp=2000, segment_raw_power=30, actor_spec_sha256=actor_digest)
        specs.append(spec)
    return attack_input+body+b"".join(specs)+defender+common, sources, source_operands(body)["receiver"]


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("sweep follow-up output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        inlet, sources, body = qualified_inlet(image, RetailArchive(stream))
        if stamp != archive_stamp(stream):
            raise ValueError("sweep follow-up AFS changed during extraction")
    args = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--followup"]
    result = run(args, input=inlet, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"sweep follow-up fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-sweep-area-followup", "revision": 1,
        "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "sources": sources, "cases": cases, "receiver": {**body, "borg": "gg4e08:000a", "level_one_hp": 180},
        "area_source": {"key": 26, "group": 3, "descriptor": 4, "raw_power": 20, "raw_radius": 310,
                        "amount_record": address(AREA_RECORD), "common_bank_sha256": area.COMMON_COLLISION_SHA},
        "notes": [
            "The saved pre-crossing inlet continues through the original actor-relative bucket, where both allocated children may initialize.",
            "The receiver has a distinct body slot and a declared center 22 units above the source floor intersection.",
            "The floor allocates first, but a segment inserted before it attempts its shape first.",
            "HP is sampled through the original writer, then the same registry is resolved again. A continuous pair-pass tail or live reaction update is not replayed.",
            "The floor and sweep each keep their own token and amount record. No native or live timing equivalence is qualified.",
        ],
    }
    raw = encode(payload, expanded=("sources", "cases"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
