"""Verify original scene input-mask and rank producers, without booting a match."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
from subprocess import run

from tools.analyze.retail_code_atlas import address, read_bounded
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage

SLICES = {
    0x80018494: (484, "3cc28ad3723935ac4fa7ec7c2c0beba07216cd40bdb2392b93d8f86a2c3aef98"),
    0x80173680: (156, "b505feb0e9fc6440aa5c72eba7295f1ee93a36b14bd129cbb5d357f4df7ac452"),
    0x8019052C: (180, "b28f783638cb53694cb3ecd5ad1d211d2999657c52ddb7eb0595e99ab739aa71"),
}
SLOT_CASES = tuple((slot, port, enabled) for slot in (0, 3) for port in (2, 3) for enabled in (0, 1))
PORT_CASES = (
    (0, 1, 2, (2, 2, 2, 0)),
    (1, 3, 2, (0, 2, 2, 0)),
    (1, 3, 4, (0, 2, 2, 0)),
    (1, 3, 2, (0, 2, 0, 0)),
    (2, 0x84, 2, (0, 2, 0, 0)),
    (2, 7, 2, (0, 2, 2, 0)),
)


def row(text, name, count):
    if re.fullmatch(name + rf":(?: [0-9]{{1,3}}){{{count}}}", text) is None:
        raise ValueError(f"malformed {name} observation")
    return tuple(map(int, text.split()[1:]))


def decode_report(report):
    lines = report.splitlines()
    if len(report) > 2048 or len(lines) != 1 + len(SLOT_CASES) + len(PORT_CASES):
        raise ValueError("incomplete or oversized scene input report")
    mode, mask, active, count, setting0, setting1, rank, bank = row(lines[0], "automatic", 8)
    if (mode, mask, active, count, setting0, setting1, rank, bank) != (2, 0, 15, 4, 7, 7, 0, 1):
        raise ValueError("automatic scene inputs disagree")
    automatic = {
        "mode_state": mode, "mode_input_mask": mask, "scene_active_mask": active, "scene_active_count": count,
        "team_settings": [setting0, setting1], "teams": [0, 1, 0, 1],
        "ranks": [rank] * count, "ordinary_banks": [bank] * count,
    }
    slots = []
    for text, expected in zip(lines[1:1+len(SLOT_CASES)], SLOT_CASES, strict=True):
        slot, port, enabled, input_mask, active_mask, active_count, rank, team = row(text, "selected-slot", 8)
        if (slot, port, enabled) != expected or input_mask != 5 or (
                active_mask, active_count, rank, team) != (
                    32 | ((1 << slot) if enabled else 0), 1 + enabled, 3 if enabled else 7,
                    (port & 1) if enabled else 0):
            raise ValueError("selected-slot setup escaped the reviewed inputs")
        slots.append({
            "slot": slot, "mapped_port": port, "port_status_bit_0x40_clear": bool(enabled),
            "mode_input_mask": input_mask, "scene_active_mask": active_mask, "scene_active_count": active_count,
            "scene_rank_after": rank, "assigned_team": team if enabled else None,
            "next_rng_caller": "80173A04" if enabled else "80173DB0",
        })
    enrollments = []
    for text, expected in zip(lines[1+len(SLOT_CASES):], PORT_CASES, strict=True):
        current, prior, wanted, pick, result, count, banks = row(text, "ports", 7)
        if (current, prior, wanted) != expected[:3]:
            raise ValueError("wrong declared port setup")
        words = expected[3]
        chosen = next((i for i, word in enumerate(words) if i != current and word & wanted), None)
        expected_mask = prior if chosen is None else prior | (1 << chosen)
        if (pick, result, count, banks) != (255 if chosen is None else chosen, expected_mask,
                                           (expected_mask & 15).bit_count(), ~expected_mask & 15):
            raise ValueError("port scan outputs disagree")
        enrollments.append({
            "prior_current_port": current, "prior_input_mask": prior, "requested_port_bits": wanted,
            "declared_port_words": list(words), "picked_port": chosen,
            "mode_input_mask": result, "mode_input_port_count": count,
            "ordinary_banks": [(banks >> i) & 1 for i in range(4)],
        })
    return automatic, slots, enrollments


def export(dol, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("scene damage input output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    for entry, (size, digest) in SLICES.items():
        if sha256(image.read(entry, size, "text")).hexdigest() != digest:
            raise ValueError(f"scene input producer signature mismatch at {address(entry)}")
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True))]
    result = run(argv, capture_output=True, timeout=35)
    if result.returncode or result.stderr:
        raise ValueError(f"scene input fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    automatic, slots, enrollments = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-scene-damage-inputs", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "automatic": {"entry": "80018494", "rng_initializer": "800055E0", **automatic},
        "selected_slot": {"slice": "80173680", "rank_write": "80173710", "stop_before": "800055FC", "cases": slots},
        "port_enrollment": {"entry": "8019052C", "mask_write": "80190580", "count_write": "801905D0",
                            "cases": enrollments},
        "notes": [
            "The automatic routine runs original RNG from a fresh isolated seed. It is not a replay of a live match RNG stream.",
            "Mode input mask and Scene active mask are separate, and a valid CPU slot can have no input bit.",
            "The slot builder uses a declared port map and port status. Rank 3 is published only on the admitted branch.",
            "The enrollment helper consumes declared port words, not hardware input. It does not change ranks or select a battle mode.",
            "Bank readback uses the original ordinary constructor slice with declared slot bits; no complete actor or match is constructed.",
            "Native presets remain explicit. The fixture is not a mode, controller or rank scheduler.",
        ],
    }
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode("ascii")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "automatic": 1,
            "selected_slots": len(slots), "port_enrollments": len(enrollments)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.fixture, args.module, args.output), sort_keys=True))
