"""Replay original scheduled exit from an accepted knockout through body phase 3."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze import retail_body_knockout as ko


s, area = ko.s, ko.area
KNOCKOUT_SHA256 = "6bd31799a39744b265800207eb9b0d3d1a3eaeb4414775280c7b90aa3c2c1b97"
TEXT = (
    (0x8002BB14, 3712, "9aa80c3f876242ebee4cbf8afd5ede6938e90b767f8da7f4f8d9ab6d4fad1911"),
    (0x8002CDA4, 160, "2dcecd916811217d411f21faca6a1ca97774fa3c65c5f3756bf92d230096a693"),
    (0x8005BBC0, 268, "8bbbe25fdc9ed379896e412fbcd66328cff259e960dc68fc5f0b86a904d79993"),
    (0x8005BCCC, 316, "0cb38f451f7b213dafb408bd28451c17f165629696ca719e2eb92370b8940d47"),
    (0x8005C028, 88, "caf6f4c4d4fc5e6b6431c076311982456d76481f6c033cf8092c3c964c68280d"),
    (0x8005BE08, 192, "c925bf6ea2c8c7da67cb580d5879650159f3724ad59b999ae063eb4dc6412143"),
    (0x8005BEC8, 164, "62da1f7efe58248504eae3ff150a16f14760b5f02fb3933cc48ccf994a04ffcb"),
    (0x8005BF6C, 128, "ef45e55b9b9ae63ed090d3720ccfd05ded5735593d1fd551726f96ed9dd739c8"),
    (0x8005BFEC, 60, "fc8dcea778ccb36327061a4133bda1ed5851b8f7c744ad115ce164be15b879de"),
    (0x80054070, 316, "349fd1d6d1088a9ba19eb03192f38515faa2652943f8d2edce8d84ead782be90"),
    (0x80054D54, 1016, "83a7e4896c9e1bdd1d84c315791fe4e108004501b8a663395568474895b54663"),
    (0x80055568, 292, "4ef4d2db70068dec76e280773b3d58c546452599966914bffd4f8cbaf56472c7"),
    (0x80065E44, 32, "f9ea65eb45ba47d111ad758a0311d5e17253cff53e5700705d25e1022f2fd738"),
    (0x8006F4F8, 276, "9bedc00559917b9e5fa76d02977c59be604bed473605fc394e3df7af750ee800"),
    (0x8008A2BC, 788, "2c811080af75624f6664c00f6e11b229fdb83bea44b41482b53b2ed14d2f0528"),
    (0x8008AFF0, 32, "8ea8cac06c446278fad05cfae1dbe3ee6b3a321f4bfd1168269ce88283bef951"),
    (0x800C1F70, 264, "b6f4850d80539b2e9640d64f5d3d52356880246808097892c0b51913055bd90a"),
)
CASES = (
    ("unmasked-lethal", 1, 0, 16, "8002C870"),
    ("masked-lethal", 1, 0, 16, "8002C870"),
    ("scene-holds-hp", 1, 1, 0, "8002C6C4"),
    ("retained-zero-without-fresh-hit", 0, 0, 0, "8002C870"),
    ("nonlethal", 180, 163, 17, "8002C6C4"),
)


def decode(report):
    rows = report.splitlines()
    if len(report) > 4096 or len(rows) != 46:
        raise ValueError("normal body phase exit report extent")
    pos = 0
    cases = []

    def take(tag, fields):
        nonlocal pos
        expected = tag + ": " + " ".join(map(str, fields))
        if rows[pos] != expected:
            raise ValueError(f"normal body phase exit changed at row {pos+1}: {tag}")
        pos += 1

    for model, _, _, _ in area.bucket.MODELS:
        caster = model & 255
        for i, (name, hp, hp_after, damage, boundary) in enumerate(CASES):
            exit_started, masked, retained = i < 2, i == 1, i == 3
            request = None if retained else 17 if hp == 180 else 16
            if not retained:
                take("body-exit-amount", (caster, i, hp, 140, request, 17, 17, 17))
            initial_timer = (30 if masked else 29) if exit_started else 0
            initial_bits = "41F00000" if masked else "41E80000" if exit_started else "00000000"
            take("body-exit", (caster, i, int(exit_started), 2 if exit_started else 1, hp_after,
                              17 if retained else damage, 2, int(not retained), 1 if exit_started else 3,
                              63 if exit_started else 0, 0, int(i == 0), initial_bits, boundary,
                              int(exit_started or retained), 0 if exit_started or retained else 140))
            row = {
                "caster": f"gg4e08:{model:04x}", "area_key": 19+(model == 0x0107), "case": name,
                "hp_before": hp, "hp_after_walk": hp_after, "hp_request": request,
                "damage_ledger_delta": damage, "force_before": 0 if retained else 140,
                "force_capacity": 200, "force_after": 0 if exit_started or retained else 140,
                "calculator_control": None if retained else {"hp_mirror": hp, "force_mirror": 140,
                    "hp_mirror_180_request": 17, "force_mirror_200_request": 17, "both_restored_request": 17},
                "fresh_receive_latch": not retained, "retained_token": 2, "dispatch_phase_after_walk": 2 if exit_started else 1,
                "phase2_initializer_reached": exit_started, "walk_frontier_lr": boundary,
            }
            cases.append(row)
            if not exit_started:
                continue
            take("body-exit-list", (caster, i, 0, 1))
            row.update({
                "receiver_mask_after_walk": "0443", "actor_phase_lane_slot": 63,
                "scene_active_mask_after_walk": 1, "exit_timer_after_walk": initial_timer,
                "phase_entry_once_flag_after_walk": int(not masked), "masked_actor_visits": 2 if masked else 0,
                "receive_registry_after_rebuild": 0, "receiving_shape_still_allocated_after_rebuild": True,
                "clock_samples": [],
            })
            for visit, before, after, phase, bits, requested, inactive_request in (
                (1, 0, 0, 2, "41E80000" if masked else "41E00000", 0, 0),
                (2, 0, 0, 2, "41E00000" if masked else "41D80000", 0, 0),
                (29+masked, 0, 1, 2, "00000000", 0, 0),
                (30+masked, 1, 2, 2, "41700000", 1, 0),
                (46+masked, 2, 3, 2, "BF800000", 0, 1),
                (47+masked, 3, 3, 3, "BF800000", 0, 1),
            ):
                take("body-exit-visit", (caster, i, visit, before, after, phase, 1, bits, inactive_request, 1, requested))
                row["clock_samples"].append({
                    "unmasked_visit": visit, "subphase_before": before, "subphase_after": after, "actor_phase": phase,
                    "timer_bits": bits, "body_remove_flag": bool(inactive_request), "actor_cell_active": True,
                    "type13_key0_factory_requested": bool(requested), "type13_allocated": False,
                })
    if pos != len(rows):
        raise ValueError("extra phase exit report rows")
    return cases


def verify_source(image):
    s.verify_text(image, (*area.TEXT, *TEXT))
    for site, target in ((0x8002BBE4, 0x8002CDA4), (0x8002C4CC, 0x8002DB58), (0x8002C6DC, 0x8005BBC0),
                         (0x8005BC88, 0x8005C028), (0x8005BCB4, 0x8005BCCC), (0x80054DA0, 0x80055568),
                         (0x80065E50, 0x8005BCCC), (0x8005BF00, 0x800C1F70), (0x8005BFC4, 0x8008AFF0)):
        s.link(image, site, target)
    for address, words in (
        (0x802D326C, (0x80054D54, 0x8005514C)),
        (0x802D3570, (0x8005BE08, 0x8005BEC8, 0x8005BF6C, 0x8005BFEC)),
        (0x802D3B10, (0x802D3FF8,)),
        (0x802D38A4, (0x80065E44,)),
        (0x80437428, (0x3F800000, 0, 0x40C00000, 0x41700000, 0x41F00000, 0x41A00000)),
    ):
        if image.read(address, 4*len(words), "data") != struct.pack(f">{len(words)}I", *words):
            raise ValueError(f"normal body exit selector or timer changed: {address:08X}")
    if image.read(0x802D3FF8+2*47, 2, "data") != b"\x00\x3f":
        raise ValueError("normal Sasuke exit lane selection changed")


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("normal body phase exit output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_source(image)
    s.read_metadata(s.MAPS / "gg4e08-body-knockout.json", KNOCKOUT_SHA256)
    payload = area.bucket.qualified_payload(image, afs, (area.RECEIVER, area.RECEIVER_BLOCK_SHA256,
                                                         area.ACTOR_SHA256[area.RECEIVER]))
    cases = decode(s.fixture_report(fixture, dol, module, ("--body-exit",), payload))
    raw = s.encode({
        "kind": "gf-retail-body-phase-exit", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "knockout_sha256": KNOCKOUT_SHA256, "receiver": "gg4e08:000a",
        "entries": {"collision_walk": "8002BB14", "phase2_initializer": "8005BBC0", "body_dispatch": "80054070",
                    "phase2_update": "80054D54", "exit_lane": "80065E44", "exit_subphases": "802D3570",
                    "not_entered_phase3_update": "8005514C"},
        "cases": cases,
        "notes": [
            "A side-by-side isolated pair is checked before replaying the original walk on the same published lists. The retained-zero control carries the isolated pair history into the walk.",
            "The exact receiver is installed in a declared contiguous scan, with the credited body outside that scan. The declared receiving Force is 140 of 200; the original walk refreshes both receiving mirrors.",
            "Only disposable amount-helper forks restore one or both receiving mirrors. The retained walk uses the refreshed source inputs.",
            "The scene actor mask bit inhibits the phase-2 lane; two masked visits are checked before clearing it. Unmasked visits are actor dispatch prefixes cut before presentation, not match frames.",
            "The next registry rebuild is a disposable fork. It omits the defeated receiver while its shape is still allocated.",
            "The effect pool is empty. A type-13 request, remove flag and phase 3 do not establish rendering, resource teardown, slot reuse, respawn or scene transitions.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases),
            "clock_samples": sum(len(case.get("clock_samples", ())) for case in cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
