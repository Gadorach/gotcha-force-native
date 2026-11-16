"""Read-only inventory of normal G-Red's original animation-controller data.

This is static recovery, not a runtime controller or a source-code exporter.
Raw reports belong in ignored local evidence. Unknown bytecodes remain explicit.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from tools.analyze.lane_descriptors import descriptor_lanes
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile


PROGRAM_ROOT = 0x80366220
DESCRIPTOR_ROOT = 0x80367460
COMMON_PROGRAM_ROOT = 0x802BCC64
COMMON_DESCRIPTOR_ROOT = 0x802BB698
CLASS_CALLBACKS = 0x80365538
MOVEMENT_SELECTORS = 0x80365CF8
#Pointer increments checked against source dispatch802D0DA0 and its handlers.
#Knowing an event's byte size does NOT authorize executing its side effects.
COMMAND_SIZES = {0: 4, 1: 8, 2: 8, 3: 8, 4: 4, 5: 4, 6: 4, 7: 4,
                 8: 4, 9: 4, 10: 4, 11: 4, 12: 4, 13: 8, 14: 4, 15: 4,
                 16: 4, 17: 4}


def signed_table(read, address, maximum=128):
    """Source signed-s16 relative table, terminated by -1; preserve aliases."""
    if type(address) is not int or address & 1 or not 0 <= address <= 0xFFFFFFFF:
        raise ValueError("relative table address")
    if type(maximum) is not int or not 1 <= maximum <= 128:
        raise ValueError("relative table budget")
    result = []
    for index in range(maximum + 1):
        raw = read(address + index * 2, 2)
        if len(raw) != 2:
            raise ValueError("short relative table")
        offset = struct.unpack(">h", raw)[0]
        if offset == -1:
            return result
        target = address + offset
        if index == maximum or not 0 <= target <= 0xFFFFFFFF or target & 1:
            raise ValueError("relative table target/count")
        result.append(target)
    raise ValueError("relative table terminator")


def hierarchy(read, root):
    return [dict(group=group, address=table, choices=signed_table(read, table))
            for group, table in enumerate(signed_table(read, root, 16))]


def decode_program(read, start, max_commands=128, max_span=4096):
    """Recover reachable static flow, never execute controls or invent op sizes."""
    if not 1 <= max_commands <= 128 or not 4 <= max_span <= 4096:
        raise ValueError("program work budget")
    rows, seen, pc, waits = [], {}, start, 0
    for _ in range(max_commands):
        if pc in seen:
            cycle = rows[seen[pc]:]
            return dict(address=start, commands=rows,
                        status="yielding_loop" if any(r["opcode"] == 0 for r in cycle)
                        else "non_yielding_loop", next_address=pc)
        if not start - max_span <= pc <= start + max_span:
            return dict(address=start, commands=rows, status="span_budget", next_address=pc)
        if any(row["address"] < pc < row["address"] + row["size"] for row in rows):
            raise ValueError("branch into command operand")
        header = read(pc, 4)
        if len(header) != 4:
            raise ValueError("short command header")
        opcode, size = header[0], COMMAND_SIZES.get(header[0])
        if size is None:
            return dict(address=start, commands=rows, status="unsupported_opcode",
                        next_address=pc, opcode=opcode, header=header.hex())
        raw = read(pc, size)
        if len(raw) != size:
            raise ValueError("short command")
        if any(pc < row["address"] + row["size"] and row["address"] < pc + size
               for row in rows):
            raise ValueError("overlapping command")
        row = dict(address=pc, opcode=opcode, size=size, raw=raw.hex(), preceding_waits=waits)
        seen[pc] = len(rows)
        rows.append(row)
        next_pc = pc + size
        if opcode == 0:
            row.update(kind="wait", flags=raw[1], threshold=struct.unpack(">h", raw[2:])[0])
            if raw[1] & 1:
                return dict(address=start, commands=rows, status="terminal_wait")
            waits += 1
        elif opcode == 1:
            row.update(kind="select_descriptor", group=raw[1], choice=raw[2],
                       mode=raw[3], duration=raw[4], reserved=raw[5],
                       start_frame=struct.unpack(">h", raw[6:])[0])
        elif opcode == 6:
            next_pc = pc + 2 - struct.unpack(">h", raw[2:])[0]
            row.update(kind="relative_branch", target=next_pc)
        elif opcode == 13:
            row.update(kind="directional_select", group=raw[1],
                       choices=list(raw[2:5]), duration=raw[5],
                       start_frame=struct.unpack(">h", raw[6:])[0])
        elif opcode == 17:
            row.update(kind="class_event", lane=raw[1], selector=raw[2], operand=raw[3])
        else:
            row["kind"] = {2: "upper_controls", 3: "lower_controls",
                           4: "lane2_controls", 5: "lane3_controls", 7: "models",
                           8: "pose_height", 9: "actor_event_9", 10: "actor_event_10",
                           11: "auxiliary", 12: "input_control",
                           14: "actor_event_14", 15: "actor_event_15",
                           16: "actor_control_16"}[opcode]
        pc = next_pc
    return dict(address=start, commands=rows, status="command_budget", next_address=pc)


def inventory(read, program_root, descriptor_root):
    programs, descriptors = hierarchy(read, program_root), hierarchy(read, descriptor_root)
    unique_programs, unique_descriptors = {}, {}
    for table in programs:
        for choice, address in enumerate(table["choices"]):
            if address not in unique_programs:
                unique_programs[address] = decode_program(read, address)
                unique_programs[address]["aliases"] = []
            unique_programs[address]["aliases"].append([table["group"], choice])
    for table in descriptors:
        for choice, address in enumerate(table["choices"]):
            if address not in unique_descriptors:
                entry = descriptor_lanes(read, address, 3, supported_lanes=2)
                entry.update(address=address, aliases=[])
                for lane in entry["lanes"]:
                    group = lane["group"]
                    lane["clip"] = dict(common=bool(group & 0x80), group=group & 0x7F,
                                        choice=lane["index"])
                    #Raw references, not canonical classifier IDs (which merge aliases).
                    lane["clip"]["raw_id"] = ((10000 if group & 0x80 else 0)
                                              + (group & 0x7F) * 256 + lane["index"])
                unique_descriptors[address] = entry
            unique_descriptors[address]["aliases"].append([table["group"], choice])
    return dict(program_groups=programs, descriptor_groups=descriptors,
                programs=list(unique_programs.values()),
                descriptors=list(unique_descriptors.values()))


def recover(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified G-Red source revision")
    consumed = 0

    def read(address, size):
        nonlocal consumed
        consumed += size
        if consumed > 2 << 20:
            raise ValueError("controller source-read budget")
        return image.read(address, size)

    def pointers(address, count):
        values = struct.unpack(">" + "I" * count, read(address, count * 4))
        for target in values:
            if target:
                image.read(target, 4, "text")
        return list(values)

    local = inventory(read, PROGRAM_ROOT, DESCRIPTOR_ROOT)
    #These are installed on G-Red too, but remain a separate shared namespace.
    common = inventory(read, COMMON_PROGRAM_ROOT, COMMON_DESCRIPTOR_ROOT)
    raw_selectors = read(MOVEMENT_SELECTORS, 64)
    movement = []
    for index in range(32):
        upper, lower = struct.unpack_from(">bb", raw_selectors, index * 2)
        if upper < -1 or lower < -1 or upper >= 64 or lower >= 64:
            raise ValueError("movement dispatcher selector")
        movement.append(dict(selector=index, upper_state=upper, lower_state=lower,
                             upper_handler=pointers(0x802D36A8 + upper * 4, 1)[0] if upper >= 0 else None,
                             lower_handler=pointers(0x802D37A8 + lower * 4, 1)[0] if lower >= 0 else None))
    callbacks = list(struct.unpack(">16I", read(CLASS_CALLBACKS, 64)))
    for target in callbacks[:13]:
        if target:
            image.read(target, 4, "text")
    return dict(schema=1, model=0x615, source_sha256=image.sha256,
                scope="static normal-G-Red controller inventory; no runtime equivalence",
                tables=dict(program=PROGRAM_ROOT, descriptor=DESCRIPTOR_ROOT,
                            common_program=COMMON_PROGRAM_ROOT,
                            common_descriptor=COMMON_DESCRIPTOR_ROOT),
                class_fields=[dict(offset=0x4B4 + i * 4, key=key)
                              for i, key in enumerate(callbacks)],
                gred_dispatch=dict(program_events=pointers(0x80365578, 4),
                                   main=pointers(0x80365588, 5),
                                   main0_substates=pointers(0x8036559C, 5),
                                   main1_normal_substates=pointers(0x803655D4, 8)),
                movement_selectors=movement, local=local, common=common,
                source_read_bytes=consumed)


def markdown(report):
    lines = [
        "# Normal G-Red: recovered controller layout",
        "",
        "Static inventory, not a recovered executable controller. Unknown bytecodes stop decoding.",
        "Programs/descriptors/clips have different number spaces. Raw clip references may alias",
        "the same classifier preview. Group0 requests use the runtime default group;",
        "the base G-Red group0/group1 tables alias, but other runtime defaults are not inferred.",
        "Rows following a wait are conditional continuations, not simultaneous selections;",
        "a looping motion may never satisfy that wait.",
        "",
        f"Source SHA-256: `{report['source_sha256']}`",
        "",
        "## Layers",
        "",
        "Input/state selection → shared upper/lower handlers + G-Red overrides →",
        "program request → bytecode/waits → per-lane descriptors → motion bank → pose/blend/root systems.",
        "",
        "## Shared state selectors installed on G-Red",
        "",
        "| Selector | Upper state / handler | Lower state / handler |",
        "|---|---|---|",
    ]
    for row in report["movement_selectors"]:
        def handler(lane):
            value = row[lane + "_handler"]
            return f"{row[lane + '_state']} / `{value:08X}`" if value else "unsupported"
        lines.append(f"| {row['selector']} | {handler('upper')} | {handler('lower')} |")
    lines += ["", "## G-Red main-state override dispatch", ""]
    for index, address in enumerate(report["gred_dispatch"]["main"]):
        lines.append(f"- State {index}: `{address:08X}`")
    lines += ["", "## G-Red bytecode17 class-event dispatch", ""]
    for index, address in enumerate(report["gred_dispatch"]["program_events"]):
        lines.append(f"- Event {index}: `{address:08X}` (side effects not executed)")
    for namespace in ("local", "common"):
        bank = report[namespace]
        lines += ["", f"## {namespace.title()} animation programs", "",
                  "| Source entry aliases (group:choice) | Address | Selections in first prefix | Flow status |",
                  "|---|---|---|---|"]
        for program in bank["programs"]:
            selections = []
            for command in program["commands"]:
                if command["preceding_waits"]:
                    break
                if command["opcode"] == 1:
                    selections.append(f"D{command['group']}:{command['choice']}"
                                      f" (mode {command['mode']}, duration {command['duration']},"
                                      f" start {command['start_frame']})")
                elif command["opcode"] == 13:
                    selections.append(f"directional D{command['group']}:{command['choices']}")
            aliases = ", ".join(f"{g}:{c}" for g, c in program["aliases"])
            lines.append(f"| {aliases} | `{program['address']:08X}` | {'; '.join(selections) or '—'} | {program['status']} |")
        lines += ["", f"## {namespace.title()} descriptors", "",
                  "| Source aliases | Address | Lane0 clip | Lane1 clip | Root selector |",
                  "|---|---|---|---|---|"]
        for descriptor in bank["descriptors"]:
            aliases = ", ".join(f"{g}:{c}" for g, c in descriptor["aliases"])
            clips = []
            for lane in descriptor["lanes"]:
                clip = lane["clip"]
                clips.append(f"{'shared' if clip['common'] else 'Borg'}"
                             f" {clip['group']}:{clip['choice']} (raw ID {clip['raw_id']})")
            lines.append(f"| {aliases} | `{descriptor['address']:08X}` | {clips[0]} | {clips[1]} | {descriptor['root_selector']} |")
        lines += ["", f"## {namespace.title()} program flow details", ""]
        for program in bank["programs"]:
            lines += [f"### `{program['address']:08X}`", ""]
            for command in program["commands"]:
                prefix = f"- `{command['address']:08X}` [after {command['preceding_waits']} waits] "
                kind = command["kind"]
                if kind == "select_descriptor":
                    detail = f"D{command['group']}:{command['choice']}, mode={command['mode']}, duration={command['duration']}, start={command['start_frame']}"
                elif kind == "wait":
                    detail = f"flags={command['flags']}, threshold={command['threshold']}"
                elif kind == "relative_branch":
                    detail = f"target={command['target']:08X}"
                else:
                    detail = command["raw"]
                lines.append(prefix + kind + ": " + detail)
            lines += [f"- Decode stop: **{program['status']}**"
                      + (f" at `{program['next_address']:08X}`" if "next_address" in program else ""), ""]
    return "\n".join(lines) + "\n"


def export(dol, output, format="json"):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("controller report already exists")
    with ReadOnlyFile(dol) as source:
        report = recover(DolImage(source.read(0, source.size), SOURCE_SHA256))
    if format not in ("json", "markdown"):
        raise ValueError("controller report format")
    encoded = (json.dumps(report, indent=2) if format == "json" else markdown(report)).encode()
    if len(encoded) > 4 << 20:
        raise ValueError("controller report byte budget")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(encoded)
    return dict(bytes=len(encoded), sha256=hashlib.sha256(encoded).hexdigest(),
                programs=len(report["local"]["programs"]),
                descriptors=len(report["local"]["descriptors"]))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dol", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--format", choices=("json", "markdown"), default="json")
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.output, args.format)))


if __name__ == "__main__":
    main()
