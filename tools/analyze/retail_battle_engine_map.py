"""Map the original battle engine's shared call graph from curated roots.

This is a static, read-only inventory of the retail PPC image. It follows `bl`
targets from the roots that the damage/action recovery already identified, and
records the data addresses each function materialises. It is a map, not a
decompiler: function ends are approximated at the first `blr`, and only direct
calls (`bl`) are followed.

The result contains addresses and graph shape only - no model, texture, audio
or instruction bytes.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

ROOTS = {
    0x8017FCC8: "scene-job-dispatch",
    0x800282A0: "actor-update-outer",
    0x8002891C: "actor-update",
    0x800291C4: "actor-update-inner",
    0x800294F8: "actor-update-leaf",
    0x8002C4D0: "collision-service",
    0x8002E2A8: "pair-entry",
    0x8002F110: "reaction-hp-tail",
    0x8003CD5C: "damage-amount",
    0x8003D344: "hp-subtract",
    0x8006FACC: "action-main-handler",
    0x8004BEB8: "action-request",
}
TABLES = {
    0x802D0DA0: ("opcode-dispatch", 64, "text"),
    0x80365538: ("class-callbacks", 32, "text"),
    0x804335E0: ("damage-factor-rows", 16, "data"),
}
MAX_FUNCTION_BYTES = 0x2000
MAX_FUNCTIONS = 400
MAX_DEPTH = 4


def is_bl(word):
    return (word >> 26) == 18 and (word & 1) != 0


def is_blr(word):
    return (word >> 26) == 19 and ((word >> 1) & 0x3FF) == 16


def bl_target(word, pc):
    displacement = word & 0x03FFFFFC
    if displacement & 0x02000000:
        displacement -= 0x04000000
    return (pc + displacement) & 0xFFFFFFFF


def lis_addi(lis, addi):
    """Decode a `lis rD,hi` / `addi rD,rD,lo` pair into its address."""
    if (lis >> 26) != 15 or (addi >> 26) != 14:
        return None
    rd = (lis >> 21) & 31
    if ((addi >> 21) & 31) != rd or ((addi >> 16) & 31) != rd:
        return None
    high = lis & 0xFFFF
    low = addi & 0xFFFF
    if low & 0x8000:
        low -= 0x10000
    return ((high << 16) + low) & 0xFFFFFFFF


def load_words(image):
    words = {}
    for section in image.layout["sections"]:
        if section["kind"] != "text":
            continue
        base = section["address"]
        size = section["size"]
        offset = section["offset"]
        blob = image.data[offset:offset + size]
        for index in range(0, size - 3, 4):
            words[base + index] = struct.unpack_from(">I", blob, index)[0]
    return words


def data_ranges(image):
    return [(section["address"], section["address"] + section["size"])
            for section in image.layout["sections"] if section["kind"] == "data"]


def in_ranges(address, ranges):
    return any(lo <= address < hi for lo, hi in ranges)


def scan_function(words, start, data):
    calls = []
    refs = []
    for pc in range(start, start + MAX_FUNCTION_BYTES, 4):
        word = words.get(pc)
        if word is None:
            break
        if is_bl(word):
            calls.append(bl_target(word, pc))
        if (word >> 26) == 15:
            for step in range(1, 9):
                follow = words.get(pc + 4 * step)
                if follow is None:
                    break
                address = lis_addi(word, follow)
                if address is not None and in_ranges(address, data):
                    refs.append(address)
                    break
        if is_blr(word) and pc != start:
            break
    return calls, refs


def recover(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    words = load_words(image)
    data = data_ranges(image)
    functions = {}
    pending = [(address, 0) for address in ROOTS]
    while pending and len(functions) < MAX_FUNCTIONS:
        address, depth = pending.pop()
        if address in functions or address not in words:
            continue
        calls, refs = scan_function(words, address, data)
        functions[address] = {
            "role": ROOTS.get(address, ""),
            "calls": sorted(set(calls)),
            "data_refs": sorted(set(refs)),
            "depth": depth,
        }
        if depth < MAX_DEPTH:
            pending.extend((target, depth + 1) for target in calls if target in words)

    tables = {}
    for address, (name, count, kind) in TABLES.items():
        entries = []
        for index in range(count):
            try:
                value = struct.unpack(">I", image.read(address + 4 * index, 4, kind))[0]
            except Exception:
                break
            entries.append(value)
        tables[name] = {"address": f"{address:08X}", "entries": entries}

    edges = []
    for address, info in sorted(functions.items()):
        for target in info["calls"]:
            edges.append({"from": f"{address:08X}", "to": f"{target:08X}"})

    roles = {f"{address:08X}": info["role"] for address, info in sorted(functions.items())
             if info["role"]}
    return {
        "kind": "gf-retail-battle-engine-map",
        "revision": 1,
        "source_dol_sha256": SOURCE_SHA256,
        "roots": {f"{address:08X}": role for address, role in sorted(ROOTS.items())},
        "roles": roles,
        "tables": tables,
        "functions": [
            {
                "address": f"{address:08X}",
                "role": info["role"],
                "depth": info["depth"],
                "calls": [f"{target:08X}" for target in info["calls"]],
                "data_refs": [f"{address:08X}" for address in info["data_refs"]],
            }
            for address, info in sorted(functions.items())
        ],
        "edges": edges,
        "counts": {
            "functions": len(functions),
            "edges": len(edges),
            "roles": len(roles),
        },
        "notes": [
            "Function ends are approximated at the first blr; only direct bl calls are followed.",
            "No model, texture, audio or instruction bytes are included.",
            "Roots and tables are the addresses named by the attack/movement recovery.",
        ],
    }


def export(dol, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("battle engine map output already exists")
    with ReadOnlyFile(dol) as source:
        if not 0 < source.size <= 8 << 20:
            raise ValueError("source DOL extent")
        payload = recover(DolImage(source.read(0, source.size), SOURCE_SHA256))
    raw = (json.dumps(payload, indent=2, sort_keys=True) + "\n").encode()
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"bytes": len(raw), "sha256": hashlib.sha256(raw).hexdigest(),
            "functions": payload["counts"]["functions"],
            "edges": payload["counts"]["edges"]}


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("dol", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.output)))


if __name__ == "__main__":
    main()
