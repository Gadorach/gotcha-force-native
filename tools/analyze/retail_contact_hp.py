"""Verify a scoped original melee contact through its amount and HP write."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze.damage_content_row import CLASS_ROOT
from tools.analyze.reaction_emit import qualified_hit_block
from tools.analyze.retail_action_input import AFS_SHA256, archive_stamp, verify_archive
from tools.analyze.retail_attack_transactions import decode_report
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_shape_sweep import CASES, source_operands
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import ACTOR_BYTES, RetailArchive, levels
from tools.extract.source_image import DolImage

MODEL = 0x000A
RECORD = 0x802D4190
HP_ROW = re.compile(r"contact-hp:(?: [0-9]{1,3}){15}")
INPUTS = tuple(case + (0, 180, 0, 0) for case in CASES) + (
    (81, 0, 0, 0, 1, 180, 0, 0),
    (81, 0, 0, 0, 0, 1, 0, 0),
    (81, 0, 0, 0, 0, 180, 7, 0),
    (81, 0, 0, 0, 0, 180, 7, 1),
)


def read_contact_hp(report):
    decode_report(report)  # The original carrier and pair fixtures remain prerequisites.
    rows = []
    for line in report.splitlines():
        if line.startswith("contact-hp:"):
            if not HP_ROW.fullmatch(line):
                raise ValueError("malformed HP fixture row")
            rows.append(tuple(map(int, line.split()[1:])))
    if tuple(row[:8] for row in rows) != INPUTS:
        raise ValueError("missing or reordered HP fixture cases")
    out = []
    for row in rows:
        distance, masked, inhibited, pair_gate, protect, initial, setting, source = row[:8]
        writer, amount, hp, previous, dealt, taken, zero = row[8:]
        expected_writer = not masked and distance <= 82 and not inhibited and not pair_gate
        expected_amount = (34 if initial == 1 else 40 if source else 35) if expected_writer else 0
        write = expected_writer and not protect
        expected_hp = max(0, initial - expected_amount) if write else initial
        expected_ledger = expected_amount if write else 0
        if ((writer, amount, hp, previous, dealt, taken, zero) !=
                (int(expected_writer), expected_amount, expected_hp, initial if write else 0,
                 expected_ledger, expected_ledger, int(write and expected_hp == 0))):
            raise ValueError("inconsistent original amount, HP, or ledger result")
        out.append({
            "distance": distance, "attack_masked": bool(masked), "receiver_inhibited": bool(inhibited),
            "receiver_pair_gate": bool(pair_gate), "scene_hp_protection": bool(protect),
            "settings_index_for_both_teams": setting, "receiver_controller_source": source,
            "initial_hp": initial, "hp_writer_reached": bool(writer), "requested_damage": amount,
            "remaining_hp": hp, "previous_hp_field": previous, "hp_removed": initial - hp,
            "attacker_ledger_delta": dealt, "receiver_ledger_delta": taken,
            "writer_reported_zero_hp": bool(zero),
        })
    return out


def factor_rows(image, spec):
    if len(spec) != ACTOR_BYTES or tuple(spec[156:159]) != (1, 1, 0):
        raise ValueError("Sasuke receiver spec extent or selectors")
    base, = struct.unpack(">H", image.read(RECORD, 2, "data"))
    if base != 40 or levels(image, MODEL)[0][0] != 180:
        raise ValueError("Sasuke source base amount or level-one HP")

    def word(at):
        return struct.unpack(">I", image.read(at, 4, "data"))[0]

    rows = []
    for controller_source in (0, 1):
        table = word(CLASS_ROOT + 4 * controller_source)
        row = word(table + 4 * spec[156])
        factor, = struct.unpack(">f", image.read(row + 7 * 4, 4, "data"))
        rows.append({"controller_source": controller_source, "receiver_class_factor": round(factor, 6)})
    return base, rows


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("contact HP output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        archive = RetailArchive(stream)
        block = qualified_hit_block(archive, MODEL)
        member, _ = archive.member(archive.names.index(b"pl000a.pzz"), MODEL)
        spec, _ = member.read(0, ACTOR_BYTES)
        if stamp != archive_stamp(stream):
            raise ValueError("contact source AFS changed during extraction")
    geometry = source_operands(block)
    base, rows = factor_rows(image, spec)
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)),
            "--sasuke-contact-hp"]
    result = run(argv, input=block + spec, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"contact HP fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    cases = read_contact_hp(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-contact-hp", "revision": 1,
        "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "scope": "exact Sasuke attack pair and same-model singleton body, declared ordinary actor/scene settings",
        "source": {"actor_spec_sha256": sha256(spec).hexdigest(),
                   "hit_block_sha256": geometry["attack"]["hit_block_sha256"],
                   "level_one_max_hp": 180, "attack_record": "802D4190", "raw_attack_power": base},
        "path": {"pair_loop": "8002DB58", "amount": "8002F108 -> 8003CD5C",
                 "write": "8002F1D8 -> 8003D344", "stop_after_writer_at": "8002F1DC"},
        "settings": {
            "attack_team": 0, "receiver_team": 1, "attacker_controller_source": 0,
            "receiver_role_bank_from": "actor+0x3E6 != 0", "team_settings_index_from": "actor+0x88",
            "controller_source_rows_at_setting_7": rows,
            "declared_rank_both": 0, "relative_multipliers": 1, "force_current_and_cap_per_team": 180,
        },
        "cases": cases,
        "notes": [
            "The original allocator, shape pass, pair loop, amount helpers and HP writer produce the result; no damage operand is supplied.",
            "The stopped original writer resumes with unchanged registers, stack and LR. Subsequent reaction and KO scheduling are not executed.",
            "The scene protection byte can preserve HP despite a positive computed amount and a call to the writer.",
            "Credit and receiver ledgers count requested damage, not the HP loss after the clamp.",
            "This does not qualify real scene generation, rank/mode scheduling, full body geometry or native move timing.",
        ],
    }
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode("ascii")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases),
            "hp_changes": sum(row["hp_removed"] > 0 for row in cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
