"""Resolve Ultimate Cannon's type-71 segment through the original damage and HP writes."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import reaction_emit, reaction_installer_tables, retail_cannon_beam as cannon
from tools.analyze import retail_type11_roster as segments, retail_type32_contact as contact, retail_type32_roster as beam
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import address, read_bounded
from tools.analyze.retail_projectile_census import AFS_SHA256, SOURCE_SHA256, encode
from tools.analyze.retail_rifle_contact_hp import ACTOR_SHA256
from tools.analyze.retail_shape_sweep import source_operands
from tools.analyze.retail_type0_contact import RECEIVER, RECEIVER_BLOCK_SHA256, read_normal
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive, levels
from tools.extract.source_image import DolImage


ACTOR_DIGEST = "ff711ea5a9cfcff90cf4b08a16ddb0c11063b1027f2bc31fb06b7c424d0906d8"
BLOCK_DIGEST = "65ffc7333822b099e3416fdcfa839971ef725cb3caa956362389efd0a5b2ad53"
ACTOR_TABLE, AMOUNT_RECORD = 0x802FC350, 0x802FC380
CASES = ("open-hit", "open-miss", "closing-hit", "no-segment-cell", "no-shape-cell")
ROW = re.compile(r"type71-contact:(?: [0-9]{1,5}){10}(?: [0-9]{1,5}\.[0-9]{5}){3}")
TEXT = (
    (0x800EC394, 272, "7627cb0bc6f1772baad22a30d404e5657e6b449494b1c1bea319fce48982c2e0"),
    (0x800EC50C, 256, "fff9e1cb0b88c225f1cac572cda8047b572a2772557b956663e233f76eccd646"),
    (0x800ECBC8, 240, "d3ab31ff638d3775e4ea71159c927e34e30dd420cd61dddba2a206ff73cd5950"),
    *tuple(span for span in segments.TEXT if span[0] in (0x800EC2EC, 0x800EC4A4)),
    *tuple(span for span in beam.TEXT if span[0] in (beam.SEGMENT, beam.SEGMENT_BIND)),
    *tuple(span for span in contact.TEXT if span[0] != 0x8003E978),
)


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 1024 or len(lines) != len(CASES):
        raise ValueError("type-71 contact report extent")
    results = []
    for i, (case, line) in enumerate(zip(CASES, lines, strict=True)):
        if ROW.fullmatch(line) is None:
            raise ValueError("malformed type-71 contact row")
        words = line.split()[1:]
        created, bound, hit = i != 3, i < 3, i in (0, 2)
        amount = 4307 if hit else 0
        phase = (2 if i == 2 else 1) if bound else 0
        hp = 180 if i == 1 else 0
        if tuple(map(int, words[:10])) != (i, created, bound, phase, amount, hp, amount, amount,
                                           int(hit), 16 if hit else 0):
            raise ValueError(f"type-71 contact or overkill ledger mismatch: {case}")
        values = list(map(float, words[10:]))
        if (bound and any(abs(a-b) > 0.001 for a, b in zip(values, (133.33334, 1033.63867, 3362.01221), strict=True))
                or not bound and any(values)):
            raise ValueError("type-71 segment radius or endpoints changed")
        results.append({
            "case": case, "first_segment_requested": True, "segment_created": created, "shape_bound": bound,
            "second_segment_requested": True if bound else None, "parent_phase_at_second_request": phase or None,
            "segment_geometry": {"narrow_radius": values[0], "near_z": values[1], "far_z": values[2]} if bound else None,
            "receiver": {
                "hp_before": 180, "hp_after": hp, "hp_loss": 180-hp,
                "hp_writer_amount": amount if hit else None,
                "credited_damage_ledger_delta": amount, "received_damage_ledger_delta": amount,
                "segment_token": 1 if hit else None, "duplicate_token_refused": hit,
            } if bound else None,
        })
    return results


def qualified_inlet(image, archive):
    beam.verify_text(image, TEXT+cannon.TEXT)
    for pc, target in ((0x800B0FF4, cannon.CARRIER), (0x800B13EC, cannon.CARRIER),
                       (0x800ECBF4, 0x80006DC8), (0x800EC75C, beam.SEGMENT)):
        beam.link(image, pc, target)
    if (struct.unpack(">4I", image.read(0x8031B200, 16, "data")) != (0x800EC394, 0x800EC4A4, 0x800EC77C, 0x800EC838)
            or struct.unpack(">2I", image.read(0x80434408, 8, "data")) != (0x800EC50C, 0x800EC60C)
            or struct.unpack(">2h3f", image.read(0x8031B1C0, 16, "data")) != (2, 2, 0, 0, 585.5)
            or image.read(AMOUNT_RECORD, 24, "data") != bytes.fromhex("138801f46464000001018802000404001001000000180000")):
        raise ValueError("type-71 carrier row, dispatch or amount record changed")
    attack, owner_spec = read_normal(archive, cannon.MODEL, BLOCK_DIGEST, ACTOR_DIGEST)
    body, target_spec = read_normal(archive, RECEIVER, RECEIVER_BLOCK_SHA256, ACTOR_SHA256[RECEIVER])
    stage = reaction_emit.STEPS+3*reaction_emit.STEP_SIZE
    if (reaction_emit.read_group(attack, 2) != [(3, 2)]
            or struct.unpack_from(">BBHHhhH4f", attack, stage) != (0, 0, 0x1001, 2, 0, -1, 0, 0, 0, 0, 2000)
            or reaction_installer_tables.model_table(image, cannon.MODEL)[1] != ACTOR_TABLE
            or (levels(image, cannon.MODEL)[0][0], levels(image, RECEIVER)[0][0]) != (500, 180)):
        raise ValueError("type-71 normal actor source changed")
    return attack+body+owner_spec+target_spec, source_operands(body)["receiver"]


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type-71 contact output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        inlet, body = qualified_inlet(image, RetailArchive(stream))
        if stamp != archive_stamp(stream):
            raise ValueError("type-71 contact AFS changed during extraction")
    args = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--cannon"]
    result = run(args, input=inlet, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"type-71 contact fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-type71-contact", "revision": 1, "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "source": {"borg": "gg4e08:0c04", "level_one_hp": 500, "factory": address(cannon.CARRIER),
                   "key": 0, "actor_spec_sha256": ACTOR_DIGEST, "hit_block_sha256": BLOCK_DIGEST,
                   "parent_active_call": "800EC75C", "segment_key": 2, "descriptor": 3, "raw_radius": 2000,
                   "reaction_index": 2, "amount_record": address(AMOUNT_RECORD), "raw_power": 5000},
        "receiver": {**body, "borg": "gg4e08:000a", "level_one_hp": 180},
        "cases": cases,
        "notes": [
            "The type-71 factory runs directly; the actor launch and debit schedule are not replayed.",
            "The original initializer handles empty effect scratch and empty render records; socket pose and owner gate are declared.",
            "Owner byte zero remains in warmup. Byte one enters emission. Byte two in the emitting leaf sets phase two but still requests once.",
            "This replay observes the amount and finishes only the original HP writer. KO presentation and scheduling are outside scope.",
            "No native gameplay, live-match timing, other ranks, or whole-beam damage is qualified.",
        ],
    }
    raw = encode(payload, expanded=("cases",))
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
