"""Join original processed-pad reads to menu enrollment and ordinary damage banks."""
import argparse
from dataclasses import dataclass
from hashlib import sha256
import json
from pathlib import Path
import re
from subprocess import run

from tools.analyze.retail_code_atlas import address, read_bounded
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage

SLICES = {
    0x80190038: (596, "3c937c33b26c3c1b8f2ccdb6a41a6f3527cff6d0291a922c63c30f6981033365"),
    0x8016E694: (360, "d53eed10f7df1cbdbb6ea59e7bc95bbc4db33d4a0002119f61e5c58898e3ffb4"),
    0x801904F4: (56, "6957da8c3cf8bc8328cdf838839ef88dd763fd8c430879c44c0bd7471b741c9b"),
    0x8019052C: (180, "b28f783638cb53694cb3ecd5ad1d211d2999657c52ddb7eb0595e99ab739aa71"),
    0x8005438C: (140, "d475c1886c214369ba4ea322131dda6fd43e2cc68132404cd436eab961be7735"),
}
NO_PORT = 255
BOUNDARIES = ("menu-return", "before-audio-80190184", "before-audio-801901F0", "before-transition-80190258")
# Expected menu halfwords (+4, +6), from the declared initial (4, 8).
MENU_STEPS = ((4, 8), (5, 0), (4, 9), (4, 10))

IDLE = (0, 0, 1)
EDGE_100 = (0x100, 0, 1)
ALT_80 = (0x1000, 0, 1)
HELD_100 = (0x100, 0x100, 1)


@dataclass(frozen=True)
class Case:
    name: str
    packets: tuple[tuple[int, int, int], ...]
    menu_edges: tuple[int, int, int, int]
    boundary: int
    enrolled: int = NO_PORT
    other_port: int = NO_PORT
    menu_branch: int = 2
    current: int = 0
    mask: int = 1


CASES = (
    Case("first-match", (EDGE_100, EDGE_100, EDGE_100, IDLE), (0, 256, 256, 0), 2, enrolled=1),
    Case("held-ignored", (IDLE, HELD_100, EDGE_100, IDLE), (0, 0, 256, 0), 2, enrolled=2),
    Case("status-zero", (IDLE, (0x100, 0, 0), EDGE_100, IDLE), (0, 0, 256, 0), 2, enrolled=2),
    Case("status-nonzero", (IDLE, (0x100, 0, 255), EDGE_100, IDLE), (0, 256, 256, 0), 2, enrolled=1),
    Case("current-excluded", (EDGE_100, IDLE, IDLE, IDLE), (0, 0, 0, 0), 0),
    Case("held-no-edge", (IDLE, HELD_100, IDLE, IDLE), (0, 0, 0, 0), 0),
    Case("alternate-bit", (IDLE, IDLE, ALT_80, IDLE), (0, 0, 128, 0), 3, enrolled=2),
    Case("edge-priority", (IDLE, ALT_80, EDGE_100, IDLE), (0, 128, 256, 0), 2, enrolled=2),
    Case("upper-bit-priority", (IDLE, EDGE_100, (0x200, 0, 1), ALT_80), (0, 256, 512, 128), 1, other_port=2),
    Case("single-port", (EDGE_100, EDGE_100, IDLE, IDLE), (256, 64, 64, 64), 2, menu_branch=1),
    Case("single-port-alternate", (ALT_80, EDGE_100, IDLE, IDLE), (128, 64, 64, 64), 3, menu_branch=1),
    Case("nonzero-current", (EDGE_100, EDGE_100, EDGE_100, IDLE), (256, 256, 0, 0), 2,
         enrolled=0, current=2, mask=4),
    Case("already-enrolled", (EDGE_100, EDGE_100, EDGE_100, IDLE), (256, 256, 0, 0), 2,
         enrolled=0, current=2, mask=5),
)


def decode_report(report):
    if len(report) > 2048:
        raise ValueError("oversized menu port report")
    lines = report.splitlines()
    if len(lines) != len(CASES):
        raise ValueError("incomplete menu port report")
    cases = []
    for line, c in zip(lines, CASES, strict=True):
        if re.fullmatch(re.escape(c.name) + r":(?: [0-9]{1,3}){13}", line) is None:
            raise ValueError(f"malformed menu port observation: {c.name}")
        *edges, stop, enrolled, other_port, current, mask, count, banks, major, step = map(int, line.split()[1:])
        expected_current = c.current if c.enrolled == NO_PORT else c.enrolled
        expected_mask = c.mask if c.enrolled == NO_PORT else c.mask | (1 << c.enrolled)
        if (tuple(edges), stop, enrolled, other_port, current, mask, count, banks, (major, step)) != (
            c.menu_edges, c.boundary, c.enrolled, c.other_port, expected_current, expected_mask,
            expected_mask.bit_count(), ~expected_mask & 15, MENU_STEPS[c.boundary],
        ):
            raise ValueError(f"menu port observation disagrees: {c.name}")
        cases.append({
            "case": c.name, "menu_branch": c.menu_branch,
            "prior_current_port": c.current, "prior_mode_input_mask": c.mask,
            "declared_processed_pads": [
                {"word": word, "status": status, "prior_menu_held": held}
                for word, held, status in c.packets
            ],
            "menu_edge_words_after": edges, "boundary": BOUNDARIES[stop],
            "enrolled_port": None if enrolled == NO_PORT else enrolled,
            "branch_0x200_port": None if other_port == NO_PORT else other_port,
            "current_port_after": current, "mode_input_mask": mask, "mode_input_port_count": count,
            "ordinary_banks": [(banks >> i) & 1 for i in range(4)],
            "menu_state_after": [major, step],
        })
    return cases


def export(dol, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("menu port output already exists")
    dol = Path(dol).resolve(strict=True)
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    for entry, (size, digest) in SLICES.items():
        if sha256(image.read(entry, size, "text")).hexdigest() != digest:
            raise ValueError(f"menu port signature mismatch at {address(entry)}")
    argv = [str(Path(fixture).resolve(strict=True)), str(dol), str(Path(module).resolve(strict=True))]
    result = run(argv, capture_output=True, timeout=35)
    if result.returncode or result.stderr:
        raise ValueError(f"menu port fixture failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    cases = decode_report(result.stdout.decode("ascii"))
    payload = {
        "kind": "gf-retail-menu-port-input", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "processed_pad_base": "803C727C", "processed_pad_stride": 32,
        "menu_update": "80190038", "per_port_reader": "8016E694", "enrollment": "8019052C",
        "reviewed_enrollment_calls": {"801901E0": 0x100, "80190250": 0x80},
        "ordinary_bank_slice": "8005438C", "declared_mode_state": 2, "declared_actor_teams": [0, 1, 0, 1],
        "cases": cases,
        "notes": [
            "Replay starts at the processed-pad buffer with declared prior menu-held values, not raw polling.",
            "Original menu code calls the enrollment helper. Execution stops before audio or transition calls, or returns without them.",
            "Menu branch selector +23 and battle Mode state +50 are distinct fields.",
            "The bank readback uses the resulting Mode input mask with declared ordinary actor slots, not an entire Scene or match.",
            "No native defaults or runtime controller assignment changed.",
        ],
    }
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode("ascii")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(cases)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for arg in ("dol", "fixture", "module", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.fixture, args.module, args.output), sort_keys=True))
