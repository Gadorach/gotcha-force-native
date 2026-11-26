"""Map the conditional branches of the pair loop `8002DB58`.

The loop has 32 conditional branches: outer prepare, inner validity,
the direct/nearest selection and the publish band. This tool decodes each
`bc` with the nearest compare/mask, plus the two CTR matrix-copy loops.
Addresses and operands only.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from tools.analyze.collision_branch_map import CONDS, _condition
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

FUNCTION = 0x8002DB58
SIZE = 944
INNER = 0x8002DBF4
SELECT = 0x8002DD90
PUBLISH = 0x8002DE08


def _phase(site):
    if site < INNER:
        return "prepare"
    if site < SELECT:
        return "inner"
    if site < PUBLISH:
        return "select"
    return "publish"


def recover(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    code = image.read(FUNCTION, SIZE, "text")
    branches = []
    for off in range(0, len(code), 4):
        word = struct.unpack_from(">I", code, off)[0]
        if word >> 26 != 16:
            continue
        site = FUNCTION + off
        bo = (word >> 21) & 31
        bi = (word >> 16) & 31
        displacement = word & 0xFFFC
        if displacement & 0x8000:
            displacement -= 0x10000
        target = site + displacement if not (word & 2) else displacement
        if bo in (16, 18):
            condition, source = {"kind": "bdnz", "text": "decrement CTR"}, None
        else:
            condition, source = _condition(code, site, FUNCTION)
        branches.append({
            "site": f"{site:08X}",
            "target": f"{target & 0xFFFFFFFF:08X}",
            "phase": _phase(site),
            "bo": bo,
            "bi": bi,
            "cr": bi >> 2,
            "cond": CONDS[bi & 3],
            "sense": "true" if bo & 8 else "false",
            "condition": condition,
            **({"source": f"{source:08X}"} if source is not None else {}),
        })
    if len(branches) != 32:
        raise ValueError(f"branch count changed: {len(branches)}")
    phases = {}
    for branch in branches:
        phases[branch["phase"]] = phases.get(branch["phase"], 0) + 1
    return {
        "kind": "gf-pair-loop-branch-map",
        "revision": 1,
        "source_dol_sha256": SOURCE_SHA256,
        "function": {"address": f"{FUNCTION:08X}", "size": SIZE,
                     "branches": len(branches)},
        "phases": phases,
        "branches": branches,
        "notes": [
            "Each branch records the nearest flag-setting compare/mask in the "
            "eight instructions before it.",
            "The two CTR branches are the 45-dword matrix copies.",
            "Conditions are operands; the pair validity rules are documented "
            "separately.",
        ],
    }


def export(dol, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("pair loop branch output already exists")
    with ReadOnlyFile(dol) as source:
        if not 0 < source.size <= 8 << 20:
            raise ValueError("source DOL extent")
        payload = recover(DolImage(source.read(0, source.size), SOURCE_SHA256))
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode()
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"bytes": len(raw), "sha256": hashlib.sha256(raw).hexdigest(),
            "branches": len(payload["branches"]), "phases": payload["phases"]}


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("dol", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.output), sort_keys=True))


if __name__ == "__main__":
    main()
