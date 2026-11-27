"""Join ordinary Ninja floor crossings to area growth and original HP writes."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
from subprocess import run

from tools.analyze import retail_bomb_floor as floor
from tools.analyze.retail_bomb_area import AMOUNT
from tools.analyze.retail_code_atlas import address, read_bounded
from tools.analyze.retail_type0_contact import RECEIVER_BLOCK_SHA256, read_normal
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import RetailArchive, levels
from tools.extract.source_image import DolImage


SPEC_SHA256 = "bd5c6a7611df76737868e6d34580ff4e8dc464e288d23340c753e3f45cfec6c5"
PILOT_HP = (150, 180)
CASES = ((0, 56), (1, 56), (1, 57))
ROW = re.compile(r"bomb-floor-contact:(?: [0-9]{1,3}){10}")


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 1024 or len(lines) != len(floor.PILOTS)*len(CASES):
        raise ValueError("bomb floor-contact report extent")
    pilots = []
    width = len(CASES)
    for i, model in enumerate(floor.PILOTS):
        cases = []
        for line, (spare, offset) in zip(lines[i*width:(i+1)*width], CASES, strict=True):
            if ROW.fullmatch(line) is None:
                raise ValueError("malformed bomb floor-contact row")
            first_writer = (36 if offset == 56 else 37) if spare else 0
            expected = (model, spare, offset, 35, first_writer,
                        163 if first_writer == 36 else 180, 163 if spare else 180,
                        2 if spare else 0, (38-first_writer) if spare else 0, 1)
            values = tuple(map(int, line.split()[1:]))
            if values != expected:
                raise ValueError(f"bomb floor-contact token, HP or ordering mismatch: {model:04X}/{spare}/{offset}")
            _, _, _, impact, writer, early_hp, late_hp, token, duplicates, stock = values
            samples = []
            for visit, hp, radius in ((36, early_hp, 34.1), (37, late_hp, 65.1)):
                samples.append({
                    "visit": visit, "registered_area_radius": radius if spare else None, "receiver_hp": hp,
                    "amount_record": address(AMOUNT) if writer == visit else None,
                    "hp_written": writer == visit, "damage": 17 if writer == visit else 0,
                })
            cases.append({
                "shape_cells_free_at_area_bind": spare, "receiver_center_offset_x": offset,
                "impact": {"visit": impact, "receiver_hp": 180, "opening_attack_registered": True,
                           "opening_attack_hit": False, "receiver_hit_tokens": []},
                "growth_visits": samples, "first_hp_write_visit": writer or None,
                "receiver_hit_tokens_after_area": [token] if token else [],
                "duplicate_checks_without_hp_write": duplicates, "stock_after": stock,
                "credited_and_received_damage": 180-late_hp,
            })
        pilots.append({"borg": f"gg4e08:{model:04x}", "cases": cases})
    return pilots


def qualified_inlet(image, archive):
    inlet, common, pilots = floor.qualified_inlet(image, archive)
    specs = []
    for model, hp, pilot in zip(floor.PILOTS, PILOT_HP, pilots, strict=True):
        _, spec = read_normal(archive, model, RECEIVER_BLOCK_SHA256, SPEC_SHA256)
        if levels(image, model)[0][0] != hp:
            raise ValueError(f"bomb floor-contact actor HP mismatch: {model:04X}")
        pilot.update(actor_level_one_hp=hp, actor_spec_sha256=SPEC_SHA256)
        specs.append(spec)
    return inlet+b"".join(specs), common, pilots


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("bomb floor contact output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), floor.SOURCE_SHA256)
    with open_regular(afs) as stream:
        stamp = floor.archive_stamp(stream)
        floor.verify_archive(stream)
        inlet, common, source = qualified_inlet(image, RetailArchive(stream))
        if stamp != floor.archive_stamp(stream):
            raise ValueError("bomb floor-contact AFS changed during extraction")
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True)), "--floor-contact"]
    result = run(argv, input=inlet, capture_output=True, timeout=45)
    if result.returncode or result.stderr:
        raise ValueError(f"bomb floor-contact fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    pilots = decode_report(result.stdout.decode("ascii"))
    receipt = {
        "kind": "gf-retail-bomb-floor-contact", "revision": 1, "source_dol_sha256": floor.SOURCE_SHA256,
        "source_afs_sha256": floor.AFS_SHA256, "common_collision": common,
        "area": {"type": 5, "key": 0, "raw_power": 20, "pair_loop": "8002DB58", "hp_writer": "8003D344"},
        "pilots": [{**pilot, "source": origin} for pilot, origin in zip(pilots, source, strict=True)],
        "receiver": {"borg": "gg4e08:000a", "body_step": 9, "radius": 22, "initial_hp": 180,
                     "normal_hit_block_sha256": RECEIVER_BLOCK_SHA256, "normal_actor_spec_sha256": SPEC_SHA256},
        "damage_inlets": {"actor_banks": [0, 0], "teams": [0, 1], "team_settings_rows": [0, 0], "team_force_caps": [100, 100]},
        "notes": [
            "The original paid release and triangle crossing are replayed without a receiver; its pose is declared at the corrected point plus the named X offset.",
            "The receiver participates in the original pair loop first with the still-registered parent on the impact visit, then with any area on growth visits.",
            "Only the original HP writer returns; the caller stopped at that boundary is not resumed.",
            "The amount, token history, recipient and credit ledgers are from the child. No parent hit was counted.",
            "These are bounded passes on one floor triangle and one receiver, not live scheduling, moving-body sweeps, or native gameplay.",
        ],
    }
    raw = (json.dumps(receipt, indent=2, sort_keys=True) + "\n").encode("ascii")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "pilots": len(pilots), "cases": len(pilots)*len(CASES)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
