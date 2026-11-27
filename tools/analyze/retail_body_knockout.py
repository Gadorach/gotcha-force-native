"""Replay the normal body knockout continuation after an exact accepted area hit."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct

from tools.analyze import battle_hit_ledger as ledger
from tools.analyze import retail_type41_area_hp as area, retail_type41_parent_contact as parent
from tools.extract.retail_borg_roster import levels


s = area.s
LEDGER_SHA256 = "fa09f532ba4201f4632cc63ed586b1961cc12458a10c6b7ca00f45437c73269a"
TEXT = (
    (ledger.FUNCTION, ledger.TEXT_SIZE, ledger.TEXT_SHA256),
    (0x80055F90, 444, "e5c63b3dcbe89a313c0816f6589692220629ff56d813604552ac22677d83c1f1"),
    (0x80066168, 48, "7bbfae7ee17d25becbf96c3628bc6fa74ea5340cd55e6c8d90712cad132fafcd"),
    (0x8018214C, 220, "e0996eb254bae537b09699eb277bae355de21bc2c2035d93c6dc9f0cae561a1c"),
)
ROW = re.compile(r"body-knockout:(?: [0-9]{1,3}){19}")
COST = 150
# Case, actual HP, force, debit, reserve enabled, recipient eligible, scene held, same player, reserves.
CASES = (
    ("one-hp-survives", 18, 200, True, False, False, False, False, 0),
    ("exact-zero", 17, 200, True, False, False, False, False, 0),
    ("overkill-force-clamp", 7, 140, True, False, False, False, False, 0),
    ("reserve-without-force-debit", 7, 200, False, True, True, False, False, 2),
    ("reserve-recipient-ineligible", 7, 200, False, True, False, False, False, 2),
    ("scene-holds-hp", 7, 140, True, True, True, True, False, 2),
    ("same-player-slot-zero", 3, 200, True, False, False, False, True, 0),
    ("exact-force-zero-empty-reserve", 7, 150, True, True, True, False, False, 0),
)


def decode(report):
    lines = report.splitlines()
    if len(report) > 2048 or len(lines) != len(area.bucket.MODELS)*len(CASES):
        raise ValueError("normal body knockout report extent")
    rows = []
    for model, _, _, _ in area.bucket.MODELS:
        for i, (name, hp, force, debit, reserve, eligible, held, ally, reserves) in enumerate(CASES):
            request = 4 if ally else 17
            damage = 0 if held else request
            hp_after = max(0, hp-damage)
            ko = hp_after == 0
            credit = ko and not ally
            force_after = max(0, force-COST) if ko and debit else force
            used_reserve = bool(ko and reserve and eligible and reserves)
            pair = ko and (force_after == 0 or used_reserve)
            expected = (model & 255, i, request, hp_after, damage, force_after, reserves-used_reserve,
                        pair, ko, COST*ko, credit, COST*credit, 50+100*credit, credit, 1,
                        0 if ko else 50, 4*ko, credit, (4 if ally else 3)*ko)
            line = lines[len(rows)]
            if ROW.fullmatch(line) is None or tuple(map(int, line.split()[1:])) != expected:
                raise ValueError(f"normal body knockout request, force or attribution changed: {model:04X}/{name}")
            rows.append({
                "caster": f"gg4e08:{model:04x}", "area_key": 19+int(model == 0x0107), "case": name,
                "before_pair": {"hp": hp, "force": force, "debit_gate": debit, "reserve_gate": reserve,
                                "reserve_receiver_eligible": eligible, "reserves": reserves, "scene_hp_hold": held,
                                "receiving_player_slot": 0 if ally else 1},
                "hp_request": request, "hp_after": hp_after, "hp_removed": hp-hp_after,
                "damage_ledger_delta": damage, "knockout": ko, "force_after": force_after,
                "reserves_after": reserves-used_reserve, "scene_actor_pair_written": pair,
                "receiver_loss_count": int(ko), "receiver_loss_cost": COST*ko,
                "owner_credit_count": int(credit), "owner_credit_cost": COST*credit,
                "owner_slot_tally_delta": 50+100*credit, "receiver_last_knockout_owner_written": credit,
                "mode_zero_model_requests": int(credit), "receiver_phase_after": 1, "meter_after": 0 if ko else 50,
                "owner_ring_relations": [2 if ally else 1]+([4 if ally else 3] if ko else []),
                "same_player_observer_knockout_relation": 4 if ko else None,
            })
    return rows


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("normal body knockout output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    s.verify_text(image, (*area.TEXT, *TEXT))
    for site, target in ((0x8005601C, 0x80066168), (0x8002F1D8, 0x8003D344),
                         (0x8002F200, ledger.FUNCTION), (0x8002F298, 0x8018214C)):
        s.link(image, site, target)
    if ledger.recover(image) != s.read_metadata(s.MAPS / "battle-hit-ledger.json", LEDGER_SHA256):
        raise ValueError("normal body knockout static ledger fields changed")
    (root,) = struct.unpack(">I", image.read(0x802DD3B4, 4, "data"))
    if (struct.unpack(">h", image.read(root+10*12, 2, "data")) != (COST,) or
            levels(image, area.RECEIVER)[0][0] != 180):
        raise ValueError("Sasuke's original level-1 HP or cost changed")
    s.read_metadata(s.MAPS / "gg4e08-type41-area-hp.json", parent.AREA_SHA256)
    payload = area.bucket.qualified_payload(image, afs, (area.RECEIVER, area.RECEIVER_BLOCK_SHA256,
                                                         area.ACTOR_SHA256[area.RECEIVER]))
    rows = decode(s.fixture_report(fixture, dol, module, ("--knockout",), payload))
    raw = s.encode({
        "kind": "gf-retail-body-knockout", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "type41_area_hp_sha256": parent.AREA_SHA256,
        "battle_hit_ledger_sha256": LEDGER_SHA256, "receiver": "gg4e08:000a", "scene_mode": 0,
        "receiver_level_1_max_hp": 180, "receiver_cached_force_cost": COST, "receiver_bank": 0, "cases": rows,
        "notes": [
            "Both paid placements and isolated area children use the original type-41 and area routines. Actual current HP is declared after source HP and cost initialization; max HP and amount mirrors are not changed.",
            "A writer-cut fork proves damage totals can already contain the requested amount. Completing the writer does not consume force.",
            "A separate retained original pair completes the mode-zero knockout branch. A retained collision token does not repeat the writes.",
            "Force and reserve fields are declared scene inputs. A force clamp and a successful reserve debit can both write the scene actor pair; the write is not a scheduled scene change.",
            "The credited ordinary contact writes a ring entry and 50 tally units before the post-zero bookkeeping. The knockout emits its separate cost, ring and credit records.",
            "No respawn, actor retirement, surrounding match scheduler or native rule is qualified.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(rows), "knockouts": sum(r["knockout"] for r in rows)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
