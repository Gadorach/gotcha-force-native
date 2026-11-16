"""Export unaltered G-Red base locomotion programs into the existing GFMP3 format.

Reads only a qualified DOL. This packages animation instructions/descriptors,
not an actor controller, input policy, physics, event consumer or renderer.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from tools.analyze.gred_animation_controller import (
    DESCRIPTOR_ROOT, PROGRAM_ROOT, inventory,
)
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile


MODEL_HASH = 0xE2B4439A
ALLOWED_OPCODES = {0, 1, 2, 3, 6, 7, 8, 11}


def encode(read, bank, defaults):
    if len(defaults) != 2 or any(value >= 128 for value in defaults):
        raise ValueError("normal default model domain")
    tables = {row["group"]: row["choices"] for row in bank["program_groups"]}
    descriptor_tables = {row["group"]: row["choices"] for row in bank["descriptor_groups"]}
    by_address = {row["address"]: row for row in bank["programs"]}
    if 0 not in tables or 1 not in tables or tables[0] != tables[1] or len(tables[1]) < 32:
        raise ValueError("reviewed G-Red base program aliases required")
    programs, descriptors = [], {}
    for choice in range(32):
        start = tables[1][choice]
        flow = by_address[start]
        commands = flow["commands"]
        if flow["status"] != "terminal_wait" or not commands:
            raise ValueError("incomplete locomotion program")
        for command in commands:
            if command["opcode"] not in ALLOWED_OPCODES:
                raise ValueError("unported event opcode in locomotion export")
            if command["opcode"] != 1:
                continue
            group, index = command["group"], command["choice"]
            #All recovered base locomotion commands use default-group selection.
            #Retain both descriptor aliases: generic and default-aware VM entries
            #resolve group0 differently. Do not rewrite original instruction bytes.
            if group != 0 or descriptor_tables.get(0) != descriptor_tables.get(1):
                raise ValueError("unreviewed locomotion descriptor group")
            for alias in (0, 1):
                if index >= len(descriptor_tables[alias]):
                    raise ValueError("missing source descriptor")
                descriptors[alias, index] = read(descriptor_tables[alias][index], 20)
        end = max(row["address"] + row["size"] for row in commands)
        if any(row["address"] < start for row in commands) or not 0 < end - start <= 1024:
            raise ValueError("locomotion program span budget")
        raw = read(start, end - start)
        if len(raw) != end - start:
            raise ValueError("short source program")
        programs.append((1, choice, raw))
    if not descriptors or len(descriptors) > 128 or any(len(raw) != 20 for raw in descriptors.values()):
        raise ValueError("descriptor byte/count domain")
    program_table, descriptor_table = 32, 32 + len(programs) * 12
    payload = descriptor_table + len(descriptors) * 24
    size = payload + sum(len(raw) for _, _, raw in programs)
    if size > 65536:
        raise ValueError("GFMP byte budget")
    output = bytearray(size)
    struct.pack_into(">IHBBIHBBIIII", output, 0, 0x47464D50, 3, 3, len(programs),
                     MODEL_HASH, len(descriptors), *defaults, size,
                     program_table, descriptor_table, payload)
    cursor = payload
    for index, (group, choice, raw) in enumerate(programs):
        struct.pack_into(">BBHII", output, program_table + index * 12,
                         group, choice, 0, cursor, len(raw))
        output[cursor:cursor + len(raw)] = raw
        cursor += len(raw)
    for index, ((group, choice), raw) in enumerate(sorted(descriptors.items())):
        at = descriptor_table + index * 24
        struct.pack_into(">BBH", output, at, group, choice, 0)
        output[at + 4:at + 24] = raw
    return bytes(output)


def recover(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified normal G-Red source revision")
    budget = 0

    def read(address, size):
        nonlocal budget
        budget += size
        if budget > 1 << 20:
            raise ValueError("locomotion source-read budget")
        return image.read(address, size)

    bank = inventory(read, PROGRAM_ROOT, DESCRIPTOR_ROOT)
    return encode(read, bank, read(0x80365AC8 + 0xBE, 2))


def recover_height(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision for pose-height operands")
    #4C5FC's negative command multiplier and4D08C's zero/half/gain/step.
    addresses = (0x804370F4, 0x80437104, 0x80437130, 0x80437134, 0x8043714C)
    operands = b"".join(image.read(address, 4) for address in addresses)
    if operands != struct.pack(">5I", 0, 0x3F000000, 0xBD4CCCCD, 0x3F333333, 0x40A00000):
        raise ValueError("pose-height source operands mismatch")
    return struct.pack(">2I", 0x47464843, 1) + operands


def export(dol, output, kind="programs"):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("locomotion output already exists")
    if kind not in ("programs", "pose-height"):
        raise ValueError("locomotion output kind")
    with ReadOnlyFile(dol) as source:
        image = DolImage(source.read(0, source.size), SOURCE_SHA256)
        raw = recover(image) if kind == "programs" else recover_height(image)
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return dict(bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest(),
                kind=kind, model=0x615, model_hash=MODEL_HASH, source_sha256=SOURCE_SHA256)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dol", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--kind", choices=("programs", "pose-height"), default="programs")
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.output, args.kind)))


if __name__ == "__main__":
    main()
