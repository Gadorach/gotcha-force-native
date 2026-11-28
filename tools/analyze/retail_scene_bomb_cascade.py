"""Verify the next area after Time Bomber's first blast destroys a second bomb."""
import argparse
from hashlib import sha256
import json
from pathlib import Path

from tools.analyze import retail_scene_bomb_reaction as reaction


s = reaction.s
REACTION_SHA256 = "7b40e71f64b24f0faec1f75b04bb73a8b1c632b647c2894324dcc2791f7311ef"
TEXT = (
    (0x80088AA0, 416, "800882bf558ba7e7f9e955e1884be71e0c9aad8870807bf7dc0876f56baaeab6"),
    (0x8008AFF0, 32, "8ea8cac06c446278fad05cfae1dbe3ee6b3a321f4bfd1168269ce88283bef951"),
    (0x80083244, 128, "0d12380007430d1784fc3a5d0183bdd2c9b40b58252868175125d47bbd3e48c4"),
)
CASES = ("no-spare-carrier", "spare-carrier-no-spare-shape", "spare-carrier-and-shape")


def decode(report):
    lines = report.splitlines()
    if len(report) > 8192 or len(lines) != 66:
        raise ValueError("scene bomb cascade report extent")
    pos = 0

    def take(tag, fields):
        nonlocal pos
        if lines[pos] != tag + ": " + " ".join(map(str, fields)):
            raise ValueError(f"scene bomb cascade changed at row {pos+1}: {tag}")
        pos += 1

    cases = []
    for delay in (0, 10):
        for capacity, name in enumerate(CASES):
            has_cell, admitted = capacity > 0, capacity == 2
            opponent_visit = 190 if delay == 0 else 191
            visits = (186, 187, 188, 189, 198, 231)
            if admitted:
                visits = visits[:4] + (opponent_visit, 198, 204, 231, 235, 236, 237)
            samples = []
            for visit in visits:
                if visit == 187:
                    take("cascade-request", (delay, capacity, visit, "800E0B18", 19, 180, int(has_cell)))
                    if has_cell:
                        take("cascade-init", (delay, capacity, visit, int(admitted)))
                parent = visit <= 188
                old = visit <= 230
                new = has_cell and 187 <= visit <= (236 if admitted else 188)
                parent_phase = 1 if visit == 186 else 2 if visit == 187 else 3
                old_phase = 1 if visit <= 228 else 2 if visit == 229 else 3
                if not has_cell or visit == 186:
                    new_phase = 0
                elif not admitted:
                    new_phase = 2 if visit == 187 else 3
                else:
                    new_phase = 1 if visit <= 234 else 2 if visit == 235 else 3
                parent_shape, old_shape = visit == 186, visit <= 197
                bound = admitted and 187 <= visit <= 203
                published = bound and visit >= 188
                token = 4 if admitted and visit >= 187 else 0
                attacks = int(parent_shape) + int(old_shape) + int(published)
                receivers = 18 + int(parent_shape)
                self_hit, opponent_hit = admitted and visit >= 188, admitted and visit >= opponent_visit
                hp = [146 if opponent_hit else 163, 212 if self_hit else 216]
                stock, credit = (9 if visit <= 187 else 10), (34 if opponent_hit else 17)
                free_carriers = 2+int(has_cell)-parent-old-new
                free_shapes = 2+int(admitted)-parent_shape-old_shape-bound
                # Type-3 allocation consumes view cells even if later area shape binding is refused.
                free_views = 18-2*old-2*new-parent
                take("cascade", (delay, capacity, visit, stock, int(parent), parent_phase, int(old), old_phase,
                                 int(new), new_phase, int(bound), token, attacks, receivers, *hp, credit,
                                 4*int(opponent_hit), 4*int(self_hit), free_carriers, free_shapes, free_views))
                contact = None
                if visit == 186 or (admitted and visit in (188, opponent_visit)):
                    carrier = visit == 186
                    receiver, before, request = (
                        (reaction.BOMB, 1, 4) if carrier else
                        (reaction.TIME, 216, 4) if visit == 188 else (reaction.SASUKE, 163, 17))
                    take("cascade-hp", (delay, capacity, visit, receiver, request, before))
                    contact = {
                        "source_token": 2 if carrier else 4,
                        "receiver": "second-bomb" if carrier else "seat-1" if receiver == reaction.TIME else "seat-0",
                        "receiver_tier_at_amount": None if carrier else 0,
                        "hp_before": before, "hp_requested": request, "hp_removed": min(before, request),
                    }
                samples.append({
                    "visit_from_first_press": visit, "stock": stock, "second_bomb_active": parent,
                    "second_bomb_phase_field": parent_phase, "old_area_active": old,
                    "old_area_phase_field": old_phase, "new_area_active": new, "new_area_phase_field": new_phase or None,
                    "new_area_shape_bound": bound, "new_area_shape_published": published, "recorded_new_area_token": token or None,
                    "total_attacks": attacks, "ordinary_receivers": receivers, "hp_by_seat": hp,
                    "owner_opponent_credit": credit, "owner_ally_credit": 0,
                    "received_damage_ledgers_by_seat": [credit, 8 if self_hit else 4],
                    "accepted_tokens_by_seat": [[2, 4] if hit else [2] for hit in (opponent_hit, self_hit)],
                    "free_carrier_cells": free_carriers, "free_shape_cells": free_shapes, "free_view_cells": free_views,
                    "hp_write": contact,
                })
            cases.append({
                "entry_hold_visits": delay, "case": name, "second_press_visit": 186,
                "extra_carrier_cells": int(has_cell), "extra_shape_cells": int(admitted),
                "request_visit": 187, "second_bomb_lifetime_at_request": 180, "new_area_carrier_allocated": has_cell,
                "free_shape_cells_at_initializer": int(admitted) if has_cell else None,
                "second_bomb_shape_return_visit": 187, "second_bomb_stock_return_visit": 188,
                "second_bomb_carrier_return_visit": 189, "old_area_shape_return_visit": 198,
                "old_area_carrier_return_visit": 231, "new_area_first_publish_visit": 188 if admitted else None,
                "new_area_shape_return_visit": 204 if admitted else None,
                "new_area_carrier_return_visit": 237 if admitted else 189 if has_cell else None,
                "completed_fork_visits": visits[-1]-185, "samples": samples,
            })
    if pos != len(lines):
        raise ValueError("extra scene bomb cascade rows")
    return cases


def verify_source(image):
    reaction.verify_source(image)
    s.verify_text(image, TEXT)
    for site, target in ((0x800E0AE0, 0x80083244), (0x800E0B08, 0x800E0D7C), (0x800E0B18, 0x80085E00),
                         (0x800E0D98, 0x8008AFF0)):
        s.link(image, site, target)


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("scene bomb cascade output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_source(image)
    s.read_metadata(s.MAPS / "gg4e08-scene-bomb-reaction.json", REACTION_SHA256)
    cases = decode(s.fixture_report(fixture, dol, module, ("--world-cascade",), reaction.area.payload(image, afs)))
    raw = s.encode({
        "kind": "gf-retail-scene-bomb-cascade", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "scene_bomb_reaction_sha256": REACTION_SHA256,
        "actors_by_seat": ["gg4e08:000a", "gg4e08:0106"], "credit_owner_seat": 1, "second_bomb_token": 3,
        "area_type": 5, "area_key": 19, "common_group": 179, "common_descriptor": 246, "amount_record": reaction.AREA,
        "old_area_token": 2, "new_area_token_if_admitted": 4, "zero_hp_terminal_site": "800E0B18",
        "prefix_coordinator_visits_per_startup_schedule": 185, "cases": cases,
        "notes": [
            "The empty-grid source replay retains the initial area hit and forks just before a new seat-1 B on visit 186. Extra cells, if any, are appended to the original free lists before that press.",
            "The first area requests 4 HP from the new bomb and removes its only 1 HP. The original next visit reaches the zero-HP branch with a nonexpired timer.",
            "Carrier allocation and shape binding are separate. At the second area's initializer both prior shapes still occupy their cells; the new shape requires an additional free slot.",
            "The two areas keep the same actor credit and amount record but different carrier and shape cells, and tokens 2 and 4. A stored parent address is not a lifetime identity: the original bomb cell was reused.",
            "With both resources available, token 4 reaches both actors again, each from tier 0. The self hit changes only received damage, not the owner's ally or opponent credit.",
            "The fork stops after both area carriers retire. The replay does not qualify other setups, further B presses, rendering, hardware, or native gameplay.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases),
            "shared_prefix_visits": 370, "completed_fork_visits": sum(r["completed_fork_visits"] for r in cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
