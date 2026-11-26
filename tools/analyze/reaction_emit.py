"""Emit reaction content from a model's action context.

The source action context's first byte selects the `pl####hit.bin` queue
group; the group's first step indexes the model's per-Borg 24-byte table.
This module turns that into portable `reaction-offset`/`reaction-mode`
lines for an authored move. Source modes outside 1/3 keep the native
default direction and are labelled.
"""
import struct

from tools.analyze import reaction_attack_table, reaction_installer_tables

QUEUE = 0xA20
STEPS = 0xAA0
STEP_SIZE = 0x50
GROUPS = 32


def entry_for_context(image, archive, model, context):
    _, table = reaction_installer_tables.model_table(image, model)
    if not table:
        return None, "no reaction table"
    name = f"pl{model:04x}hit.bin".encode("ascii")
    if name not in archive.names:
        return None, "no hit block"
    payload = archive.payload(archive.names.index(name), 1 << 20)
    group = int(context[0])
    if not 0 <= group < GROUPS:
        return None, "context group"
    steps = [byte for byte in payload[QUEUE + group * 4:QUEUE + group * 4 + 4]
             if byte != 0xFF]
    if not steps:
        return None, "empty group"
    step = steps[0]
    index = struct.unpack_from(">H", payload, STEPS + step * STEP_SIZE + 4)[0]
    if index >= reaction_attack_table.COUNT:
        return None, "table index"
    return reaction_attack_table.entry(image, table, index), None


def content_lines(image, archive, model, context, move="basic-shot"):
    entry, _ = entry_for_context(image, archive, model, context)
    if entry is None:
        return []
    lines = []
    if entry["mode"] in (0, 1, 2, 3):
        lines.append(f"reaction-mode={move}|{entry['mode']}")
    else:
        lines.append(f"# source reaction mode {entry['mode']} not native; fallback mode 1")
        lines.append(f"reaction-mode={move}|1")
    lines.append(f"reaction-offset={move}|{entry['yaw']}|{entry['pitch']}")
    return lines
