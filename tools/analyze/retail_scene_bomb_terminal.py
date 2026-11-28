"""Pin Time Bomber's outgoing-hit versus zero-HP terminal arbitration."""
import argparse
from hashlib import sha256
import json
from pathlib import Path

from tools.analyze import retail_scene_bomb_cascade as cascade


r = cascade.reaction
s = cascade.s
CASCADE_SHA256 = "e7ab99d8cdff4e9fec41bb51ccd5ac9dba5e6e2f750f89f799aff7223c41101a"
RULES = ("retained", "clear-outgoing", "restore-carrier-hp", "clear-outgoing-and-restore-carrier-hp")
OUTGOING, ZERO_HP = "800E0AD4", "800E0B18"


def decode(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != 43:
        raise ValueError("scene bomb terminal report extent")
    pos = 0

    def take(tag, fields):
        nonlocal pos
        if lines[pos] != tag + ": " + " ".join(map(str, fields)):
            raise ValueError(f"scene bomb terminal changed at row {pos+1}: {tag}")
        pos += 1

    cases = []
    for delay in (0, 10):
        landed = delay == 10
        credit, hp, damage = (40, 140, 14) if landed else (17, 163, 17)
        initial_hits = [(r.SASUKE, 26, 180), (r.SASUKE, 14, 154)] if landed else [(r.SASUKE, 17, 180)]
        initial_hits += [(r.BOMB, 4, 1), (r.TIME, 4, 220)]
        take("terminal-inlet", (delay, len(initial_hits), 16 if landed else 0, 1, 0, 180, 3, hp, 216, credit))
        contacts = []
        for i, (receiver, request, before) in enumerate(initial_hits):
            take("terminal-inlet-hp", (delay, i, receiver, request, before))
            contacts.append({
                "source_token": 3 if landed and i == 0 else 2,
                "receiver": {r.SASUKE: "seat-0", r.TIME: "seat-1", r.BOMB: "second-bomb"}[receiver],
                "hp_before": before, "hp_requested": request, "hp_removed": min(before, request),
            })
        controls = []
        for rule, label in enumerate(RULES if landed else RULES[:1]):
            outgoing = 16 if landed and not (rule & 1) else 0
            bomb_hp = int(bool(rule & 2))
            site = OUTGOING if outgoing else ZERO_HP if not bomb_hp else None
            exits = site is not None
            take("terminal-choice", (delay, rule, "80085E00" if exits else "00000000", int(exits),
                                     site or "00000000", 19 if exits else 0, bomb_hp, outgoing))
            samples = []
            for visit in range(186, 191 if exits else 187):
                refunded = exits and visit >= 187
                stock = 10 if refunded else 9
                active = visit <= 187
                phase = 1 if not exits else 2 if visit == 186 else 3
                lifetime = 180 if exits else 179
                attacks = 1 if exits and visit == 186 else 2
                hits = 2 if visit == 187 else 0
                landed_again = visit >= 187
                new_hp = [hp-damage, 212] if landed_again else [hp, 216]
                take("terminal-world", (delay, rule, visit, stock, int(active), phase, lifetime, int(exits),
                                        int(exits), int(exits), 4 if exits else 0, attacks, *new_hp, hits))
                followups = []
                if hits:
                    for i, (receiver, request, before, tier) in enumerate(
                        ((r.SASUKE, damage, hp, int(landed)), (r.TIME, 4, 216, 0))
                    ):
                        take("terminal-world-hp", (delay, rule, visit, i, receiver, request, before))
                        followups.append({
                            "receiver_seat": i, "source_token": 4, "receiver_tier_at_amount": tier,
                            "hp_before": before, "hp_requested": request, "hp_removed": request,
                        })
                samples.append({
                    "visit_from_first_press": visit, "stock": stock, "second_bomb_active": active,
                    "second_bomb_phase_field": phase, "second_bomb_lifetime_field": lifetime,
                    "new_area_shape_bound": exits, "new_area_shape_published": exits and visit >= 187,
                    "new_area_token": 4 if exits else None, "attack_shapes": attacks, "hp_by_seat": new_hp,
                    "owner_opponent_credit": credit+(damage if landed_again else 0), "owner_ally_credit": 0,
                    "received_damage_ledgers_by_seat": [credit+(damage if landed_again else 0), 8 if landed_again else 4],
                    "hp_writes_in_order": followups,
                })
            controls.append({
                "case": label, "changed_operands": (["outgoing_latch"] if rule & 1 else []) + (["carrier_hp"] if rule & 2 else []),
                "outgoing_latch_at_inlet": outgoing, "incoming_latch_at_inlet": 1, "carrier_hp_at_inlet": bomb_hp,
                "request_site": site, "request_key": 19 if exits else None,
                "stock_return_visit": 187 if exits else None, "samples": samples,
            })
        cases.append({
            "entry_hold_visits": delay, "second_press_visit": 185, "second_bomb_token": 3,
            "recorded_second_bomb_hp": 0, "recorded_outgoing_latch": 16 if landed else 0, "recorded_incoming_latch": 1,
            "recorded_fighter_hp": [hp, 216], "second_press_hp_writes_in_order": contacts, "controls": controls,
        })
    return cases


def verify_source(image):
    cascade.verify_source(image)
    for site, target in ((0x800E0AC4, 0x800E0D7C), (0x800E0AD4, 0x80085E00)):
        s.link(image, site, target)


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("scene bomb terminal output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_source(image)
    s.read_metadata(s.MAPS / "gg4e08-scene-bomb-cascade.json", CASCADE_SHA256)
    cases = decode(s.fixture_report(fixture, dol, module, ("--world-terminal",), r.area.payload(image, afs)))
    raw = s.encode({
        "kind": "gf-retail-scene-bomb-terminal", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "scene_bomb_cascade_sha256": CASCADE_SHA256,
        "credit_owner_seat": 1, "actors_by_seat": ["gg4e08:000a", "gg4e08:0106"],
        "carrier_type": 41, "parent_key": 2, "area_key": 19, "area_amount_record": r.AREA,
        "outgoing_latch_offset": "1D9", "incoming_latch_offset": "1DA", "carrier_hp_offset": "1C6",
        "cases": cases,
        "notes": [
            "The second B is placed on visit 185 rather than 186. With both resources offered, the 10-visit startup hold produces both an outgoing hit and zero carrier HP before the next bucket visit.",
            "Only the retained case reads unmodified post-hit state. The three declared controls edit the outgoing latch, carrier HP, or both after placement; no actor HP, token history, or tier is edited.",
            "The two exit sites both request key 19. The retained mixed branch follows the outgoing latch; clearing it leaves the zero-HP path. Restoring carrier HP alone still follows the outgoing latch.",
            "With neither operand present, the bomb stays active and advances its lifetime. The set incoming marker alone does not select a terminal request in this inlet.",
            "Both exiting branches bind the same common record and publish a fresh token. Matching follow-up amounts in the controlled schedule do not establish identical arena coverage or qualify Remote Bomber's command route.",
            "The replay uses an initialized empty stage; no hardware, later inputs, or native gameplay is qualified.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "second_press_cases": len(cases),
            "terminal_forks": sum(len(r["controls"]) for r in cases),
            "completed_followup_visits": sum(len(f["samples"]) for r in cases for f in r["controls"])}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
