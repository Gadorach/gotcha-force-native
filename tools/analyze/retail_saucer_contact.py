"""Replay normal Saucer carrier HP and the next callback's independent descendants."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import reaction_emit, reaction_installer_tables, retail_dragon_area as area
from tools.analyze import retail_type32_contact as contact
from tools.analyze import retail_type32_roster as beam, retail_type32_terrain as terrain
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import AFS_SHA256, SOURCE_SHA256, encode
from tools.analyze.retail_rifle_contact_hp import ACTOR_SHA256
from tools.analyze.retail_shape_sweep import source_operands
from tools.analyze.retail_type0_contact import RECEIVER, RECEIVER_BLOCK_SHA256, read_normal
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive, levels
from tools.extract.source_image import DolImage


MODEL, ACTOR_TABLE, AMOUNT_RECORD = 0x0D03, 0x80362C28, 0x80362C40
ACTOR_DIGEST = "acd4ce064b1311db2f8c456f6e7341b6efc2d77b3e522a526e1d7fed7415b855"
CASES = (
    ("clear-miss", False, False, 2, 3), ("clear-hit", True, False, 2, 3),
    ("floor-miss", False, True, 2, 3), ("floor-and-hit", True, True, 2, 3),
    ("floor-hit-one-carrier", True, True, 1, 3), ("floor-hit-two-descriptors", True, True, 2, 2),
)
ROW = re.compile(r"saucer-contact:(?: [0-9]{1,4}){8}(?: -?[0-9]{1,5}\.[0-9]{5}){6}")
TEXT = (
    (0x801DF5EC, 92, "ae122cc067ec5653c98256dd818cfaf6cfb1f4a93cbb40fcfaa18edca6da2148"),
    (0x80048288, 452, "6eb193f0ca045bdfc543485297398860cffc66b4f14f8ae2d426352bccaedac9"),
    *tuple(span for span in contact.TEXT if span[0] not in (0x8003E978, 0x80139C78, 0x80139CA0, 0x80139CC0)),
    *tuple(span for span in area.TEXT if span[0] == 0x80085E00),
)


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 2048 or len(lines) != len(CASES):
        raise ValueError("Saucer contact report extent")
    results = []
    for i, ((label, hit, floor, carriers, descriptors), line) in enumerate(zip(CASES, lines, strict=True)):
        if ROW.fullmatch(line) is None:
            raise ValueError("malformed Saucer contact row")
        values = line.split()[1:]
        made_area = hit and carriers > int(floor) and descriptors >= 2+int(floor)
        damage = 26 if hit else 0
        if tuple(map(int, values[:8])) != (i, damage, 180-damage, int(hit), floor, floor, hit, made_area):
            raise ValueError(f"Saucer carrier HP or sibling request mismatch: {label}")
        coords = list(map(float, values[8:]))
        center, event = coords[:3], coords[3:]
        if center != [160, 200, 900] or event != (center if hit else [0, 0, 0]):
            raise ValueError("Saucer contact point does not match the admitted actor geometry")
        results.append({
            "case": label, "spare_carriers_on_followup": carriers, "spare_descriptors_on_followup": descriptors,
            "published_shape_center": center, "saved_actor_contact_point": event if hit else None,
            "hp_loss": damage, "receiver_hp_after": 180-damage, "carrier_token": 1 if hit else None,
            "duplicate_token_refused": hit,
            "followup_allocation_order": (["terrain-effect"] if floor else []) + (["actor-contact-area"] if made_area else []),
            "terrain_effect": {"type": 9, "key": 4, "allocated": True,
                               "origin": terrain.CROSSING, "parent": "credited-actor"} if floor else None,
            "actor_contact_area": {"key": 0, "allocated": made_area, "origin": event,
                                   "parent": "type-32-carrier"} if hit else None,
        })
    return results


def qualified_inlet(image, archive):
    _, checked = terrain.qualified_inlet(image, archive)
    beam.verify_text(image, TEXT)
    for site, entry in ((0x801DF600, 0x80048288), (0x801DF8CC, 0x800FC6A0), (0x801DF8E8, 0x80085E00)):
        beam.link(image, site, entry)
    source = checked[0]
    attack, spec = read_normal(archive, MODEL, source["hit_block_sha256"], ACTOR_DIGEST)
    body, defender = read_normal(archive, RECEIVER, RECEIVER_BLOCK_SHA256, ACTOR_SHA256[RECEIVER])
    delta = reaction_emit.STEPS+reaction_emit.STEP_SIZE
    if (struct.unpack_from(">BBHHhhH7f", attack, delta) != (2, 0, 1, 1, 0, -1, 0, 0, 0, 5, 0, 0, 5, 30)
            or reaction_installer_tables.model_table(image, MODEL)[1] != ACTOR_TABLE
            or image.read(AMOUNT_RECORD, 24, "data").hex() != "001e00646464000004088802000600000001000000180100"
            or (levels(image, MODEL)[0][0], levels(image, RECEIVER)[0][0]) != (250, 180)):
        raise ValueError("Saucer normal actor scope or carrier shape changed")
    source.update(level_one_hp=250, actor_spec_sha256=ACTOR_DIGEST, amount_record=f"{AMOUNT_RECORD:08X}",
                  raw_power=30, raw_shape_kind=2, raw_radius=30)
    return attack+body+spec+defender, source, source_operands(body)["receiver"]


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("Saucer contact output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        inlet, source, body = qualified_inlet(image, RetailArchive(stream))
        if stamp != archive_stamp(stream):
            raise ValueError("Saucer contact AFS changed during extraction")
    args = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--saucer-contact"]
    result = run(args, input=inlet, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"Saucer contact fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-saucer-contact", "revision": 1, "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "source": source, "cases": cases, "receiver": {**body, "borg": "gg4e08:000a", "level_one_hp": 180},
        "entries": {"carrier_transform": "801DF5EC", "pair_pass": "8002DB58", "hp_writer": "8003D344",
                    "floor_effect_request": "801DF8CC", "actor_contact_area_request": "801DF8E8"},
        "notes": [
            "The type-32 key-8 parent uses supplied scratch and a declared active subphase. The original carrier shape is published and resolved.",
            "The original HP writer updates Sasuke on the clear-stage pose. No contact latch or saved actor point is seeded.",
            "On the next callback the owner supplies another endpoint. The floor and actor-contact paths use different positions and parents.",
            "The source requests are observed from the same saved inlet. For the floor-and-hit cases type 9 allocates before type 5.",
            "Neither descendant is initialized here. Live entry timing, descendant damage and native gameplay remain unqualified.",
        ],
    }
    raw = encode(payload, expanded=("cases",))
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
