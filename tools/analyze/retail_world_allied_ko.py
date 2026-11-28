"""Compare a loaded Tau's allied and opposing KOs, including the result pass."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct
from subprocess import run

from tools.analyze import retail_world_match_end as end, retail_world_team_pair as pair


s = pair.s
PRIOR = {
    "gg4e08-world-team-pair.json": "96611e18632969a3aa576a09c9b5b31e1af69b0bfbd2667c379a8730e06194f3",
    "gg4e08-world-match-end-frontier.json": "296e2694bb21e161f68f245ed1b573fc504fdc199b90089f3af1fe2ecde4c899",
    "gg4e08-contact-relationship.json": "dd37541211030ac4f8f175b620ae0572b5f069bc029b7e9c2527e3f220314010",
    "gg4e08-body-knockout.json": "6bd31799a39744b265800207eb9b0d3d1a3eaeb4414775280c7b90aa3c2c1b97",
}
TEXT = (
    (0x8002BB14, 3712, "9aa80c3f876242ebee4cbf8afd5ede6938e90b767f8da7f4f8d9ab6d4fad1911"),
    (0x8003CD5C, 1512, "7c752eea0cdf04fbe970796f80ffe1bd98ea2ef6edebea6e33cd1d3462355792"),
    (0x8002F8DC, 828, "a43da7a452462e71cf27cdfe9965001b9dfb306fdc5c693d6df8f4d2df14bb00"),
)
BANDS = ((1, 23, 14, 3), (24, 29, 15, 3), (30, 34, 16, 4), (35, 60, 17, 4))


def verify_source(image):
    end.verify_source(image)
    pair.verify_source(image)
    s.verify_text(image, TEXT)
    for site, target in ((0x8002C4CC, 0x8002DB58), (0x8002F108, 0x8003CD5C), (0x8002F1D8, 0x8003D344)):
        s.link(image, site, target)
    if (struct.unpack(">f", image.read(0x80437024, 4, "data")) != (.25,)
            or struct.unpack(">f", image.read(0x80436F78, 4, "data")) != (1.,)
            or struct.unpack(">H", image.read(0x80377AD8, 2, "data")) != (20,)
            or struct.unpack(">H", image.read(0x80377AE8, 2, "data")) != (1,)):
        raise ValueError("allied KO amount operands changed")


def decode(text):
    if len(text) > 8192 or len(text.splitlines()) != 76:
        raise ValueError("allied KO report extent")
    lines = iter(text.splitlines())

    def take(tag, *fields):
        if next(lines) != "ally-ko-"+tag+": "+" ".join(str(x) for x in fields):
            raise ValueError(f"allied KO changed at {tag}/{fields[:4]}")

    take("open", 174, 26, 3, 90)
    bands = []
    for lo, hi, opponent, ally in BANDS:
        for hp in range(lo, hi+1):
            take("amount", hp, hp, opponent, ally, opponent)
        bands.append({"current_hp_and_refreshed_mirror": [lo, hi], "opponent": opponent, "ally": ally,
                      "ally_with_0x1000_override": opponent})
    take("stale", 1, 60, 17, 4, 17)

    cases = []
    for arm in range(5):
        victim = 1+(arm-1)%2 if arm else None
        opponent_ko, ally_ko = victim == 1, victim == 2
        exhausted = arm >= 3
        pool = 90 if exhausted else 180
        req1, req2 = (14 if opponent_ko else 17), (3 if ally_ko else 4)
        hp1, hp2 = (1 if opponent_ko else 60), (1 if ally_ko else 60)
        force = [pool-90*ally_ko, pool-90*opponent_ko]
        result = (1 if opponent_ko else 2) if exhausted else 0
        snapshots = []
        bodies = (
            (60, 1, "0000", 0, 90*opponent_ko, 0, 90*ally_ko, int(opponent_ko), 0, int(ally_ko)),
            (0 if opponent_ko else 43, 2 if opponent_ko else 1, "0443" if opponent_ko else "0000", req1,
             0, 90*opponent_ko, 0, 0, int(opponent_ko), 0),
            (0 if ally_ko else 56, 2 if ally_ko else 1, "0443" if ally_ko else "0000", req2,
             0, 90*ally_ko, 0, 0, int(ally_ko), 0),
        )
        writes = [(1, req1, hp1, pool, pool), (2, req2, hp2, pool, pool-90*opponent_ko)]
        ring = [1, 3, 2] if opponent_ko else [1, 2, 4] if ally_ko else [1, 2]
        for visit in (27,) if exhausted else (27, 28, 32, 35):
            hits = writes if visit == 27 else []
            receivers, strikes = (4 if victim and visit >= 28 else 6), int(visit < 32)
            take("visit", arm, visit, 3 if exhausted else 2, result, *force, receivers, strikes, len(hits),
                 *(v for hit in hits for v in hit), *(v for body in bodies for v in body),
                 req1, req2, int(exhausted), (victim+1 if exhausted else 0), int(opponent_ko),
                 int(result == 1), int(result == 2), int(opponent_ko), 0, len(ring), *ring, *([0] if not victim else []))
            snapshots.append({
                "visit": visit, "world_dispatch_returned": True, "scene_handler": 3 if exhausted else 2,
                "force_after": force, "scene_result_mask": result, "winning_team": result-1 if result else None,
                "receivers_published": receivers, "strikes_published": strikes,
                "hp_writes": [{"seat": i, "requested": amount, "removed": min(amount, hp), "hp_before": hp,
                               "live_force_before": [f0, f1], "force_mirrors": [pool, pool]}
                              for i, amount, hp, f0, f1 in hits],
                "hp_by_seat": [row[0] for row in bodies], "phase_by_seat": [row[1] for row in bodies],
                "role_masks_by_seat": [row[2] for row in bodies], "damage_taken_by_seat": [row[3] for row in bodies],
                "opponent_credit": req1, "ally_credit": req2, "opponent_ko_credit_cost": 90*opponent_ko,
                "opponent_ko_credit_count": int(opponent_ko), "own_team_ko_observer_cost": 90*ally_ko,
                "own_team_ko_observer_count": int(ally_ko), "victim_ko_cost": 90 if victim else 0,
                "victim_ko_loss_count": int(bool(victim)), "credited_ko_notification_count": int(opponent_ko),
                "victim_ko_credit_owner_seat": 0 if opponent_ko else None, "attacker_event_ring_types": ring,
                "exhaustion_pair": [0, victim] if exhausted else None, "all_original_archives_still_bound": True,
            })
        cases.append({"initial_force": [pool, pool], "declared_one_hp_seat": victim,
                      "receiver_relationship": ("ally" if ally_ko else "opponent") if victim else None,
                      "completed_visits": snapshots})
    return bands, cases


def native_report(executable):
    p = run([Path(executable).resolve(strict=True), "--allied-ko"], capture_output=True, text=True, timeout=20)
    if (p.returncode or p.stderr or
            p.stdout != "native-allied-ko: 1 60 0 1 90 89 1\nnative-allied-ko: 2 1 48 12 90 78 0\n"):
        raise ValueError("authored allied-KO control changed")
    return [
        {"declared_one_hp_seat": 1, "ally_hp_after": 60, "enemy_hp_after": 0, "credited_hp_removed": 1,
         "force_after": [90, 89], "terminal": True},
        {"declared_one_hp_seat": 2, "ally_hp_after": 1, "enemy_hp_after": 48, "credited_hp_removed": 12,
         "force_after": [90, 78], "terminal": False},
    ]


def export(dol, afs, fixture, module, native, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("allied KO output already exists")
    dol = Path(dol).resolve(strict=True)
    verify_source(s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256))
    for name, digest in PRIOR.items():
        s.read_metadata(s.MAPS/name, digest)
    moving = pair.moving
    st42 = moving.st42
    payload = moving.tau.payload(afs)+st42.source_inputs(afs)[-sum(row[0] for row in st42.GRIDS):]
    text = s.fixture_report(fixture, dol, module, ("--world-allied-ko",), payload)
    bands, cases = decode(text)
    raw = s.encode({
        "kind": "gf-retail-world-allied-ko", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "tau_member_sha256": moving.tau.MEMBER_SHA256,
        "stage_member_sha256": st42.MEMBER_SHA256, "prior_receipts": PRIOR,
        "source_trace_sha256": sha256(text.encode("ascii")).hexdigest(), "opening_visit": 174,
        "teams_by_seat": [0, 1, 0], "cached_force_cost_per_actor": 90, "amount_probe": {
            "raw_power": 20, "ally_multiplier_before_minimum_and_truncation": .25,
            "bypass_amount_rule_bit": "1000", "fixed_body_and_geometry_seat": 2,
            "refreshed_hp_bands": bands,
            "late_hp_edit": {"current_hp": 1, "stale_hp_mirror": 60, "opponent": 17, "ally": 4},
        }, "joined_world_cases": cases, "authored_native_control": native_report(native),
        "notes": [
            "Materialized three normal Taus with original resources, declared processed input and relocated st42 grids. Both recipients are hit at visit 27. Force debit is enabled on both teams with the declared budget and no reserve spend.",
            "The amount-only sweep updates current HP before scene mirror refresh, or after it in the stale control. Team relabel and the 0x1000 flag edit are contained to disposable amount calls. Joined world runs keep stock rules and team labels.",
            "The source ending selector follows the pair pass. An exhausted team loses with either relationship while its actors are still allocated. This is not an ending return, archive handoff, save, or general friendly-fire implementation.",
            "The authored native control skips the ally and tracks HP removed in its Force ledger. No engine or package defaults change.",
        ],
    }, expanded=("joined_world_cases", "authored_native_control"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "original_observations": len(text.splitlines())}


if __name__ == "__main__":
    cli = argparse.ArgumentParser(description=__doc__)
    for name in ("dol", "afs", "fixture", "module", "native", "output"):
        cli.add_argument(name, type=Path)
    args = cli.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.native, args.output), sort_keys=True))
