"""Recover the shared GG4E08 action-gate return contract and its direct callers."""
import argparse
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import struct

from tools.analyze.retail_action_links import direct_branch
from tools.analyze.retail_code_atlas import MAX_SYMBOLS_BYTES, address, boundaries, read_bounded, scan
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage

GATE = 0x80066838
GATE_SIZE = 0x94
GATE_SHA = "c78ea72055e3f72c69bc19d9db2f1268122b947bb384c1aac9d4c803428716fe"
HELPERS = {
    0x8020BA78: (0x24, "5997a5a32c7df8eb2a03901039345f35a84f00b017545800218ef21bdbfaf81a"),
    0x8020BB14: (0x44, "492e3d053ee6336c226ad94d8947492f358b1941ca2292029efea8a6d4d382bb"),
}
ZERO_CONSTANT = 0x804375D0  # r2 (8043EA20) - 0x7450
BRANCHES = {
    0x4181: ("gt", [1]),
    0x4081: ("le", [-1, 0]),
    0x4182: ("eq", [0]),
    0x4082: ("ne", [-1, 1]),
}


def caller_branch(image, pc):
    cmp, bc = struct.unpack(">II", image.read(pc + 4, 8, "text"))
    if cmp != 0x2C030000 or bc & 3 or (bc >> 16) not in BRANCHES:
        return None
    kind, results = BRANCHES[bc >> 16]
    return {"branch": address(pc + 8), "target": address(direct_branch(pc + 8, bc)),
            "fallthrough": address(pc + 12), "test_vs_zero": kind, "takes_branch_for": results}


def recover(image, symbols):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    for start, (size, expected) in {GATE: (GATE_SIZE, GATE_SHA), **HELPERS}.items():
        if sha256(image.read(start, size, "text")).hexdigest() != expected:
            raise ValueError(f"phase-gate/helper signature mismatch at {address(start)}")
    if image.read(ZERO_CONSTANT, 4, "data") != bytes(4):
        raise ValueError("phase-gate vertical projection constant")

    target = address(GATE)
    callers, tests, functions = [], Counter(), set()
    for start, size in boundaries(image, symbols).items():
        direct, _, _ = scan(start, image.read(start, size, "text"))
        for offset, dest, kind in direct:
            if kind != "call" or dest != target:
                continue
            pc = start + offset
            condition = caller_branch(image, pc) if offset + 12 <= size else None
            callers.append({"call": address(pc), "function": address(start),
                            **(condition or {"test_vs_zero": "unresolved"})})
            tests[callers[-1]["test_vs_zero"]] += 1
            functions.add(start)
    return {
        "kind": "gf-retail-action-phase-gate", "revision": 1,
        "source_dol_sha256": SOURCE_SHA256, "symbol_metadata_sha256": sha256(symbols).hexdigest(),
        "entry": target, "comparison": "80066898", "return": "800668C8",
        "contract": {
            "priority": [
                {"if": "actor+0xCC is zero", "result": -1, "site": "800668B0"},
                {"if": "actor+0x1DD has bit 0x80", "result": 1, "site": "80066868"},
                {"if": "projected distance exceeds scaled threshold", "result": 0, "site": "800668A8"},
                {"if": "ordered distance does not exceed scaled threshold", "result": 1, "site": "800668A8"},
            ],
            "threshold": "incoming f1 multiplied by actor+0xB4",
            "distance": "norm of X/Z difference between vectors at actor+0x518 and actor+0x20",
            "validation_scope": "finite inputs, argument and comparison dataflow, no full phase timing",
        },
        "counts": {"functions": len(functions), "calls": len(callers), "test_vs_zero": tests},
        "callers": callers,
        "notes": [
            "Compare and conditional branch are checked in the two words after each entry-reachable call.",
            "Taken versus fallthrough is not advance versus hold: the destination code determines that meaning.",
            "This gate is not the receiver reaction timer or the pose-pause clock.",
        ],
    }


def export(dol, symbols, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("phase-gate map output already exists")
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    payload = recover(image, read_bounded(symbols, MAX_SYMBOLS_BYTES))
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode("ascii")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), **payload["counts"]}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for arg in ("dol", "symbols", "output"):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.symbols, args.output), sort_keys=True))
