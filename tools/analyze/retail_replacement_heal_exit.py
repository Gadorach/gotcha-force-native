"""Join an original knockout to the retirement of the admitted Nurse effect."""
import argparse
from hashlib import sha256
import json
from pathlib import Path

from tools.analyze import retail_replacement_heal_effect as effect


s = effect.s
EFFECT_SHA256 = "92ede6a49254e3e5b4467f4722d17c8381d7247d67808eb07fbefff6870a12d6"
TEXT = (
    (0x8002BB14, 3712, "9aa80c3f876242ebee4cbf8afd5ede6938e90b767f8da7f4f8d9ab6d4fad1911"),
    (0x8005BBC0, 268, "8bbbe25fdc9ed379896e412fbcd66328cff259e960dc68fc5f0b86a904d79993"),
    (0x8005C290, 532, "7483434c1849ecd1a10b99e10c8b58f71daa7568f273ebf8f29f0be4dc2a8bc6"),
)
VISITS = (1, 2, 4)
LETHAL_VISITS = (*VISITS, 18, 19, 20)


def decode(report):
    lines = report.splitlines()
    if len(report) > 4096 or len(lines) != 26:
        raise ValueError("normal replacement heal exit report extent")
    pos = 0

    def take(tag, fields):
        nonlocal pos
        if lines[pos] != tag + ": " + " ".join(map(str, fields)):
            raise ValueError(f"normal replacement heal exit changed at row {pos+1}: {tag}")
        pos += 1

    cases = []
    for earlier in (6, 7):
        for lethal in (0, 1):
            hp, request, after = (7, 15, 0) if lethal else (160, 16, 144)
            next_handler, lr = ("8005BBC0", "8002C6E0") if lethal else ("8005C290", "8002C6C4")
            phase, owner, mask = (2, 0, "0443") if lethal else (1, 2, "0000")
            take("heal-exit-inlet", (earlier, lethal, hp, request, next_handler, lr, lethal, after, 2))
            take("heal-exit-contact", (earlier, lethal, "8005A580", "8002C870", lethal, after, phase,
                                       owner, 1, mask, 16+request, 4, request))
            rows = []
            for visit in LETHAL_VISITS if lethal else VISITS:
                effect_phase = (2 if visit < 19 else 3) if lethal else 1
                returned = int(lethal and visit == 20)
                receivers = 0 if lethal else 9
                take("heal-exit-tail", (earlier, lethal, visit, phase, owner, 1-returned, effect_phase, returned,
                                        returned, receivers, receivers))
                rows.append({
                    "followup_visit": visit, "actor_phase": phase, "owner_8c4": owner,
                    "effect_alive": not returned, "effect_phase_field": effect_phase,
                    "free_kind2_and_view_cells": [returned, returned],
                    "receiver_counts_ordinary_and_alternate": [receivers, receivers],
                })
            cases.append({
                "previous_ko_source": f"gg4e08:010{earlier}", "case": "declared-low-hp" if lethal else "retained-healed-hp",
                "hp_at_hit_inlet": hp, "hp_requested": request, "hp_after": after, "hp_removed": hp-after,
                "post_hp_handler": next_handler, "effect_owner_8c4_at_handler_entry": 2,
                "phase_after_handler": phase, "effect_owner_8c4_after_handler": owner, "role_mask_after_handler": mask,
                "receiver_damage_ledger_after": 16+request, "tau_opponent_credit": request, "accepted_tokens": [3, 4],
                "completed_actor_and_effect_visits": rows[-1]["followup_visit"],
                "effect_retired_visit": 20 if lethal else None, "samples": rows,
            })
    return cases


def verify_source(image):
    effect.verify_source(image)
    s.verify_text(image, TEXT)
    for site, target in ((0x8002C6C0, 0x8005C290), (0x8002C6DC, 0x8005BBC0), (0x8002C86C, 0x8005A580)):
        s.link(image, site, target)
    if image.read(0x8005BC00, 44, "text") != bytes.fromhex(
        "3800000238600000981f0018987f0019987f001a987f001b801f05e064008000901f05e0907f08c4b07f0710"
    ):
        raise ValueError("normal replacement heal exit phase or owner-clear instruction changed")


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("normal replacement heal exit output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_source(image)
    s.read_metadata(s.MAPS / "gg4e08-replacement-heal-effect.json", EFFECT_SHA256)
    cases = decode(s.fixture_report(fixture, dol, module, ("--replacement-heal-exit",), effect.payload(image, afs)))
    raw = s.encode({
        "kind": "gf-retail-replacement-heal-exit", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "replacement_heal_effect_sha256": EFFECT_SHA256,
        "healer": "gg4e08:0900", "receiver": "gg4e08:0106", "ordinary_probe": "gg4e08:0807",
        "tau_amount_record": "80377AD8", "receiver_guard_at_heal": 1, "heal_visit": 138, "tau_probe_visit": 139,
        "zero_hp_actor_entry": "8005BBC0", "original_owner_clear_site": "8005BC24", "cases": cases,
        "notes": [
            "The original Nurse request initializes type 21 and model record 77 on the next bucket visit. The original guard expires on that same actor visit, exposing both receiver lists.",
            "Independent forks offer the exact Tau opcode with a declared caster and pose. Only the low-HP control changes the healed actor's HP inlet. No fork edits its owner bit or receive-mask fields.",
            "The original collision walk reaches the HP writer and then either the nonlethal reaction or the zero-HP defeat helper. Both continue to the original presentation cut without drawing.",
            "The defeat helper itself puts the receiver in phase 2 and clears +0x8C4. Its already-initialized effect is still active at the cut and goes through separate bucket visits before returning the exact heap payload and pool cells.",
            "The lethal 15-HP request removes 7 HP; request ledgers and retained token history do not collapse to HP removed. The replacement and common archives are retained.",
            "This is a bounded original defeat path, not a census of natural bit writers, live inputs, later actor removal, hardware or native gameplay.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases),
            "followup_visits": sum(r["completed_actor_and_effect_visits"] for r in cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
