"""Compare overlap, team eligibility and damage on a loaded Tau close-B strike."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
from subprocess import run

from tools.analyze import retail_world_moving_receiver as moving


s = moving.s
PRIOR = {
    "gg4e08-world-moving-receiver.json": "1d892d90cd2a64904f7a42a01e832c059cf6349a290a14c00b39251a8188805f",
}
TEXT = (
    (0x800536B0, 248, "99f5f9fd9611f8b4e1ff792fa40de5f9d4e047e024e846ae148937434cb6b645"),
    (0x800541AC, 1456, "b4d201a64a05727912a7435ecce9280e7f9a22075fae0afa2e59b9470f211748"),
    (0x8002FD7C, 832, "88d85748f4401cc2a4be18a7c321b2051d29f8e8d284525a611aa91cefa71247"),
    (0x8002E2A8, 5672, "3f79bf55474f2fdc4d39b4436d481b713312b1477289db13deb902ffa26c0f1e"),
)


def verify_source(image):
    moving.verify_source(image)
    s.verify_text(image, TEXT)
    for site, target in ((0x8002DC58, 0x8002FD7C), (0x8002DDA8, 0x8002E2A8), (0x8002F1D8, 0x8003D344)):
        s.link(image, site, target)


def decode(text):
    if len(text) > 4096 or len(text.splitlines()) != 22:
        raise ValueError("world team-pair report extent")
    lines = iter(text.splitlines())

    def take(tag, *fields):
        if next(lines) != "world-team-"+tag+": "+" ".join(str(x) for x in fields):
            raise ValueError(f"world team-pair changed at {tag}/{fields}")

    def result(tag, ally, n, writes, opponents, allies, published, retained=None):
        hp = [60, 60, 60]
        history = [0, 0, 0]
        taken = [0, 0, 0]
        for seat, amount in (writes if retained is None else retained):
            hp[seat] -= amount
            history[seat] = 1
            taken[seat] = amount
        take(tag, ally, n, len(writes), *(v for pair in writes for v in pair),
             *(v for group in zip(hp, history, taken) for v in group), opponents, allies, published)
        return {"hp_writes": [{"recipient": seat, "requested": amount, "removed": amount} for seat, amount in writes],
                "hp_by_seat": hp, "accepted_token_by_seat": history, "damage_taken_by_seat": taken,
                "attacker_opponent_damage_credit": opponents, "attacker_ally_damage_credit": allies,
                "published_attacks": published}

    cases = []
    for ally in (1, 2):
        take("open", 174, ally, 240, 3, 6)
        take("pose", ally, 27, "021")
        if ally == 1:
            controls = [("unmodified", 51, 60, 32, [(2, 17)], 17, 0)]
        else:
            controls = [
                ("unmodified", 63, 60, 40, [(1, 17), (2, 4)], 17, 4),
                ("seat_2_declared_opponent", 63, 60, 40, [(1, 17), (2, 17)], 34, 0),
                ("seat_1_declared_ally", 63, 60, 40, [(1, 4), (2, 4)], 0, 8),
                ("same_team_exclusion_bit_set", 63, 12, 8, [(1, 17)], 17, 0),
            ]
        variants = []
        for i, (name, broad, eligible, narrow, writes, opponents, allies) in enumerate(controls):
            take("pair", ally, i, broad, eligible, narrow, 9)
            variants.append({
                "case": name, "geometry_overlap_mask": broad, "eligible_mask": eligible, "narrow_attempt_mask": narrow,
                **result("fork", ally, i, writes, opponents, allies, 1),
            })
        _, _, _, _, writes, opponents, allies = controls[0]
        visits = [
            {"visit": n,
             **result("visit", ally, n, writes if n == 27 else [], opponents, allies, int(n != 32), retained=writes)}
            for n in (27, 28, 32)
        ]
        take("return", ally, 32, 9)
        cases.append({
            "same_team_seat": ally, "opposing_seat": 3-ally, "pose_publisher_order": [0, 2, 1],
            "pair_visit": 27, "strike_cell_ordinal": 9,
            "mask_bit_order": [[seat, descriptor] for seat in range(3) for descriptor in (1, 2)],
            "pair_controls": variants, "completed_world_observations": visits, "same_strike_cell_returned_visit": 32,
        })
    return cases


def native_report(executable):
    p = run([Path(executable).resolve(strict=True)], capture_output=True, text=True, timeout=20)
    if (p.returncode or p.stderr or
            p.stdout != "native-team-pair: 1 2 17 60 48 12 23\nnative-team-pair: 2 1 17 60 48 12 23\n"):
        raise ValueError("authored native team comparison changed")
    return [{"same_team_seat": ally, "opposing_seat": 3-ally, "both_proxies_overlap": True,
             "hp_write_update": 17, "ally_hp_after": 60, "opponent_hp_after": 48, "opponent_credit": 12,
             "attack_window_retired_update": 23} for ally in (1, 2)]


def export(dol, afs, fixture, module, native, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("world team-pair output already exists")
    dol = Path(dol).resolve(strict=True)
    verify_source(s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256))
    for name, digest in PRIOR.items():
        s.read_metadata(s.MAPS/name, digest)
    st42 = moving.st42
    payload = moving.tau.payload(afs)+st42.source_inputs(afs)[-sum(row[0] for row in st42.GRIDS):]
    text = s.fixture_report(fixture, dol, module, ("--world-team-pair",), payload)
    raw = s.encode({
        "kind": "gf-retail-world-team-pair", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "tau_member_sha256": moving.tau.MEMBER_SHA256,
        "stage_member_sha256": st42.MEMBER_SHA256, "prior_receipts": PRIOR,
        "source_trace_sha256": sha256(text.encode("ascii")).hexdigest(), "opening_visit": 174,
        "stock_model": "0807", "variant": "normal", "separately_bound_tau_views": 3,
        "stock_attack_rule_flags": "0001", "counterfactual_same_team_rejection_flag": "0080",
        "original_cases": decode(text), "authored_native_controls": native_report(native),
        "notes": [
            "Three independently loaded normal Taus share the original dispatcher, on relocated st42 grids and the no-op stage handler. The fixture declares three processed pad channels and two teams.",
            "The original body and attack poses are not patched after opening. Both receivers use the same stick input, but team and seat assignment do not guarantee identical original motion or overlap.",
            "Pair-only controls start from the already published visit-27 geometry. Relabel one team byte, or opt into the original exclusion bit in a separate copy. No control result is fed into the returning world visits.",
            "The native proxy check is an authored contrast. It keeps enemy-only melee. This is not a match for retail's same-team hit, multi-recipient custody, or damage ledgers.",
        ],
    }, expanded=("original_cases", "authored_native_controls"))
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
