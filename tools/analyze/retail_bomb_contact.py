"""Verify direct type-15 contact and separately observe follow-up allocation."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import reaction_emit, retail_bomb_first_bind as first_bind
from tools.analyze.retail_code_atlas import address, read_bounded
from tools.analyze.retail_rifle_contact_hp import ACTOR_SHA256, SLICES as CONTACT_SLICES
from tools.analyze.retail_shape_sweep import source_operands as body_operands
from tools.analyze.retail_type0_contact import PAIR_TEXT, RECEIVER, read_normal
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive, levels
from tools.extract.source_image import DolImage


SLICES = {
    **{entry: CONTACT_SLICES[entry] for entry in PAIR_TEXT},
    0x80085E00: (296, "8e4e9e754e10d0a9aa506f32a6ddd8b707fa79eb6eda278760bd6bcf4a03d3e4"),
    0x8001BDA8: (288, "01586cb4fa31c8c0f19871852fa315ee27958073bd45cff37c3f73355b129b00"),
}
# Model: key, max HP, radius, HP delta, child selector byte, child class/type/bucket,
# descriptor cost and the exact level-one normal actor spec.
CONTACT = {
    0x0101: (8, 200, 20, 17, 255, 2, 4, 25, 1, "6d8e870169ef6c72939dbb49f1338ba36b7d86ff29e24e214ee46029b3852b5b"),
    0x000A: (3, 180, 10, 8, 0, 3, 5, 9, 2, ACTOR_SHA256[0x000A]),
}
CASES = ("miss", "duplicate-token", "contact-child-published", "contact-child-cell-empty",
         "contact-child-descriptors-short")
ROW = re.compile(r"bomb-contact:(?: [0-9]{1,3}){5} [0-9A-F]{8}(?: [0-9]{1,3}){11}")


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 2048 or len(lines) != len(CONTACT)*len(CASES):
        raise ValueError("bomb contact report extent")
    result = []
    for i, (model, _, _, index, table, _, _) in enumerate(first_bind.PILOTS):
        key, _, _, damage, _, cls, kind, bucket, cost, _ = CONTACT[model]
        rows = []
        for case, line in enumerate(lines[i*len(CASES):(i+1)*len(CASES)]):
            if ROW.fullmatch(line) is None:
                raise ValueError("malformed bomb contact row")
            hit, spawn = case >= 2, case == 2
            record = table+24*index if hit else 0
            amount = damage if hit else 0
            fields = line.split()[1:]
            values = tuple(map(int, fields[:5]))+(int(fields[5], 16),)+tuple(map(int, fields[6:]))
            if values != (
                model, case, key, 16 if hit else 0, int(case != 0), record, int(hit), amount, 180-amount,
                2 if hit else 1, 598, int(not hit), 3 if hit else 0, int(hit), kind if spawn else 0, 0, 1,
            ):
                raise ValueError(f"bomb direct contact or allocation outcome disagrees: {model:04X}/{CASES[case]}")
            rows.append({
                "case": CASES[case], "body_center_offset_x": 256 if case == 0 else 0,
                "seeded_duplicate_token": case == 1, "opening_record": address(record) if hit else None,
                "contact_flags": values[3], "receiver_token": values[4], "receiver_token_cursor": values[6],
                "hp_writer_reached": hit, "requested_damage": amount, "credited_and_received_damage": amount,
                "receiver_hp": values[8], "followup": {
                    "phase": values[9], "timer": values[10], "remaining_registered_attack_count": values[11],
                    "child_request": hit, "declared_free_child_cells": int(case != 3),
                    "declared_free_child_descriptors": cost-int(case == 4),
                    "child_published": {"class": cls, "type": kind, "key": 0, "active_bucket": bucket,
                                        "descriptor_cells": cost, "bound_shapes": 0} if spawn else None,
                },
                "parent_carrier_and_descriptor_return_followup_visit": values[12] or None,
                "stock_after": values[16],
            })
        result.append({"borg": f"gg4e08:{model:04x}", "key": key, "cases": rows})
    return result


def qualify(image, archive):
    block_inlet, sources = first_bind.qualified_inlet(image, archive)
    for at, (size, digest) in SLICES.items():
        if sha256(image.read(at, size, "text")).hexdigest() != digest:
            raise ValueError(f"bomb contact source signature mismatch: {address(at)}")
    specs, body = [], None
    for i, (model, _, step, _, _, _, block_digest) in enumerate(first_bind.PILOTS):
        key, hp, radius, _, selector, _, _, _, _, spec_digest = CONTACT[model]
        block, spec = read_normal(archive, model, block_digest, spec_digest)
        if (block != block_inlet[7840*i:7840*(i+1)] or levels(image, model)[0][0] != hp or
                struct.unpack_from(">4f", block, reaction_emit.STEPS+step*reaction_emit.STEP_SIZE+12) != (0, 0, 0, radius) or
                image.read(0x802D6D68+56*key+45, 1, "data") != bytes([selector])):
            raise ValueError(f"bomb contact shape, HP or child selector mismatch: {model:04X}")
        specs.append(spec)
        if model == RECEIVER:
            body = body_operands(block)["receiver"]
        sources[i].update({"actor_spec_sha256": spec_digest, "actor_level_one_hp": hp, "opening_radius": radius,
                           "contact_child_selector_signed": selector if selector < 128 else selector-256,
                           "contact_child_entry": "8001BDA8" if selector == 255 else "80085E00",
                           "contact_child_call": "8007E298" if selector == 255 else "8007E284"})
    if body is None:
        raise ValueError("bomb contact receiver missing")
    return block_inlet+b"".join(specs), sources, body


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("bomb contact output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), first_bind.SOURCE_SHA256)
    with open_regular(afs) as stream:
        stamp = first_bind.archive_stamp(stream)
        first_bind.verify_archive(stream)
        inlet, source, body = qualify(image, RetailArchive(stream))
        if stamp != first_bind.archive_stamp(stream):
            raise ValueError("bomb contact AFS changed during extraction")
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--contact"]
    result = run(argv, input=inlet, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"bomb contact fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-bomb-contact", "revision": 1, "source_dol_sha256": first_bind.SOURCE_SHA256,
        "source_afs_sha256": first_bind.AFS_SHA256, "pair_loop": "8002DB58", "hp_writer": "8003D344",
        "receiver": {"borg": f"gg4e08:{RECEIVER:04x}", "initial_hp": 180,
                     "actor_spec_sha256": ACTOR_SHA256[RECEIVER], **body},
        "damage_inlets": {"actor_banks": [0, 0], "teams": [0, 1], "team_settings_rows": [0, 0], "team_force_caps": [100, 100]},
        "pilots": [{**r, "source": s} for r, s in zip(cases, source, strict=True)],
        "notes": [
            "The declared pools admit only key 8 from Powered Gunman's four requests. Sasuke uses ordinary key 3.",
            "The exact normal receiver descriptor is registered from a singleton runtime list at a declared rigid pose.",
            "Only the original HP writer completes in the replay; its stopped caller is never resumed.",
            "The isolated bucket-9 follow-up visits stop before Sasuke's child callback; Gunman's separate bucket 25 is not visited.",
            "Child allocation is separate from contact and supplies no proof of child binding, blast damage or live timing.",
        ],
    }
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode("ascii")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "pilots": len(cases), "cases": len(cases)*len(CASES)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
