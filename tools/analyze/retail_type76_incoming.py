"""Replay an incoming type-9 hit against the normal type-76 carrier receiver."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
from subprocess import run

from tools.analyze import retail_type32_roster as beam, retail_type76_opening as opening
from tools.analyze import retail_type9_lifetime as lifetime, retail_type9_roster as effect
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import AFS_SHA256, SOURCE_SHA256, address, encode
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive
from tools.extract.source_image import DolImage


CASES = (
    ("attack-slot-only", 1, 0, False, False),
    ("receiver-present-miss", 2, 400, False, False),
    ("lethal-effect-admitted", 2, 0, True, True),
    ("lethal-no-effect-cell", 2, 0, False, True),
    ("lethal-no-effect-descriptor", 2, 0, True, False),
)
ROW = re.compile(r"type76-incoming:(?: [0-9]{1,4}){14}")
TEXT = (
    (0x8001BDA8, 288, "01586cb4fa31c8c0f19871852fa315ee27958073bd45cff37c3f73355b129b00"),
    (0x800FD338, 12, "e84eeb834936daad4d3bbdaff4f1d2bedaaaf1739dd55c7761e90b0b31092e08"),
    (0x800FD344, 32, "b5e8660cced3595cf8f1124132f7e191c5dfd6a87081c078617ea5e4a7286409"),
    *tuple(s for s in lifetime.TEXT if s[0] in (0x80088E50, 0x80089814)),
)


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != len(opening.PILOTS)*2*len(CASES):
        raise ValueError("type-76 incoming report extent")
    rows = []
    for model, key, _, _, _, sockets, *_ in opening.PILOTS:
        for ordinal, socket in enumerate(sockets):
            for i, (label, slots, offset, fx_cell, fx_descriptor) in enumerate(CASES):
                line = lines[len(rows)]
                if ROW.fullmatch(line) is None:
                    raise ValueError("malformed type-76 incoming row")
                hit = slots == 2 and offset == 0
                amount, hp, token = (17, 0, 2) if hit else (0, 10, 0)
                allocated = hit and fx_cell and fx_descriptor
                values = tuple(map(int, line.split()[1:]))
                if values != (model, ordinal, i, slots, offset, amount, hp, token, 64 if hit else 0,
                              hit, hit, allocated, 2 if hit else 0, hit):
                    raise ValueError(f"type-76 incoming writer, latch or resource mismatch: {model:04X}/{ordinal}/{label}")
                rows.append({
                    "borg": f"gg4e08:{model:04x}", "parent_key": key, "ordinal": ordinal, "socket": socket, "case": label,
                    "carrier_shape_slots": slots, "incoming_x_offset": offset, "receiver_published": slots == 2,
                    "amount_record_reached": address(effect.AMOUNT_RECORD) if hit else None,
                    "writer_amount": amount, "carrier_hp_before": 10, "carrier_hp_after": hp, "carrier_hp_lost": 10-hp,
                    "carrier_history_token": token or None, "carrier_outgoing_latch": 0, "carrier_receive_flags": int(hit),
                    "incoming_contact_flags": 64 if hit else 0, "pilot_hp_after": 150, "sasuke_hp_after": 180,
                    "fighter_damage_ledger_deltas": {"pilot_taken": 0, "pilot_dealt": 0, "sasuke_taken": 0, "sasuke_dealt": 0},
                    "zero_hp_exit": {"type": 4, "key": 0, "effect_cell_supplied": fx_cell,
                                     "effect_descriptor_supplied": fx_descriptor, "allocated": allocated,
                                     "shape_return_visit_after_hit": 1, "carrier_and_descriptor_return_visit_after_hit": 3}
                    if hit else None,
                })
    return rows


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type-76 incoming output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    beam.verify_text(image, TEXT)
    for site, target in ((0x800FD144, 0x8001BDA8), (0x8001BDE0, 0x80088AA0), (0x800FD350, 0x80088E50)):
        beam.link(image, site, target)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        archive = RetailArchive(stream)
        bank = effect.common_bank(archive)
        _, _, _, shape, _ = effect.source_index(image, bank)
        inlet, sources, body = opening.qualified_inlet(image, archive)
        if stamp != archive_stamp(stream):
            raise ValueError("type-76 incoming AFS changed during extraction")
    args = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--incoming"]
    result = run(args, input=bank+inlet, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"type-76 incoming fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-type76-incoming", "revision": 1, "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "sources": sources, "cases": cases,
        "incoming": {"type": 9, "key": 4, "credited_actor": "gg4e08:000a", "constructor_supplied": True,
                     "common_shape": shape, "sasuke_body": body},
        "entries": {"zero_hp_request": "800FD144", "type4_factory": "8001BDA8", "type4_allocator": "8001BDE0",
                    "type76_carrier_remove": "800FD344"},
        "notes": [
            "Type-76 parents follow the paid opening replay with declared pose and stock. The opposing type-9 source is created directly, not from a Sasuke action.",
            "Sasuke's body is 800 units away on X. The two tested type-76 receivers stay level with the incoming shape.",
            "The original resolver and HP writer run through the write only. A hit sets incoming flags and reduces carrier HP without changing either fighter's HP or damage ledger.",
            "The two exit observations branch from one saved inlet. The zero-HP callback requests type 4 even if it cannot allocate, not the outgoing type-8 child.",
            "Parent retirement is replayed through the third follow-up. The type-4 object does not initialize, and no live reactions, controller timing or native gameplay are qualified.",
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
