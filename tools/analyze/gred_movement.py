"""Bounded static recovery of normal G-Red movement inputs and numeric operands.

No actor execution, source-code export, animation annotation edits or runtime
movement tuning. Reports containing original data belong in ignored local paths.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct

from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile


SETUP_SHA256 = "45770f8c027f88cf1c52322b6668bf6fad6dedbd0b719ee98395b641dfcafa8f"
ACTOR_DATA_SHA256 = "44def5423ee908d01fa73c62598d8c2837fa94291773bfcd923af83a1e5cb161"
CONFIG = 0x80365AC8
SELECTION_DISPATCH = 0x802D3300
COMMANDS = (
    (0x802D38F8, 0x614, 0x20), (0x802D3904, 0x618, 0x40),
    (0x802D3910, 0x61C, 0x80), (0x802D391C, 0x624, 0x200),
    (0x802D3928, 0x600, 1), (0x802D3934, 0x604, 2),
    (0x802D3940, 0x608, 4), (0x802D394C, 0x60C, 8),
    (0x802D3958, 0x610, 0x10), (0x802D3964, 0x628, 0x2000),
)
FOCUSED_SELECTORS = (*range(9), *range(12, 18))


def movement_data(raw):
    """Typed operands only; this does not qualify a model or infer SI units."""
    if len(raw) != 432:
        raise ValueError("actor-data extent")
    rows = []

    def take(offset, kind, role, consumer):
        size = struct.calcsize(">" + kind)
        value = struct.unpack_from(">" + kind, raw, offset)[0]
        if kind == "f" and (not math.isfinite(value) or abs(value) > 1e5):
            raise ValueError("movement scalar domain")
        rows.append(dict(offset=offset, type=kind, bits=raw[offset:offset + size].hex(),
                         value=value, role=role, consumer=consumer))

    for offset, role, consumer in (
        (4, "resource_limit736", 0x80056180),
        (6, "resource_recovery_delay", 0x800664A0),
        (8, "resource_recovery_increment", 0x8005568C),
        (10, "resource_consumption_entry", 0x800664A0),
        (12, "resource_consumption_active", 0x800664A0),
        (14, "resource_consumption_air_dash_entry", 0x800664A0),
        (16, "resource_consumption_air_dash_active", 0x800664A0),
        (0x42, "air_steering_angle", 0x80061F50),
    ):
        take(offset, "h", role, consumer)
    for index in range(6):
        take(0x18 + index * 4, "f", f"walk_turn_speed_slot{index}",
             0x8005F578 if index == 5 else 0x8005F3A0)
    for offset, role in ((0x3D, "air_timer3d"),
                         (0x3E, "air_timer690"), (0x41, "air_effect_policy"),
                         (0x9F, "air_repeat_count_limit_when_count_policy")):
        take(offset, "B", role, 0x80069210 if offset == 0x9F else 0x80061F50)
    for offset, role, consumer in (
        (0x44, "air_acceleration_scalar", 0x80061F50),
        (0x48, "air_launch_scalar", 0x80061F50),
        (0x4C, "alternate_air_launch_scalar", 0x8006235C),
        (0x50, "air_planar_speed", 0x80061F50),
        (0x54, "air_steering_scalar", 0x8005FECC),
        (0x58, "air_dash_planar_speed", 0x80061560),
        (0x5C, "air_dash_planar_change", 0x80061560),
        (0x60, "air_dash_vertical_speed", 0x80061560),
        (0x64, "air_dash_timer", 0x80061560),
        (0x68, "air_vertical_base_scalar", 0x80061F50),
        (0x6C, "fall_vertical_base_scalar", 0x8005FECC),
        (0x70, "air_dash_vertical_base_scalar", 0x80061338),
    ):
        take(offset, "f", role, consumer)
    for index, category in enumerate(("forward", "side", "backward")):
        for component, role in enumerate(("speed", "change", "minimum_speed", "duration")):
            take(0xF0 + index * 16 + component * 4, "f",
                 f"dash_{category}_{role}", 0x80060C94)
    return rows


def direct_calls(image, address, size):
    """Direct PPC bl targets in source order, NOT a recovered control-flow graph."""
    if address & 3 or size & 3 or not 4 <= size <= 4096:
        raise ValueError("bounded aligned instruction span")
    raw = image.read(address, size, "text")
    if len(raw) != size:
        raise ValueError("short instruction span")
    result = []
    for offset in range(0, size, 4):
        word = struct.unpack_from(">I", raw, offset)[0]
        if word >> 26 != 18 or not word & 1:
            continue
        displacement = word & 0x03FFFFFC
        if displacement & 0x02000000:
            displacement -= 0x04000000
        target = (displacement if word & 2 else address + offset + displacement) & 0xFFFFFFFF
        image.read(target, 4, "text")
        result.append(dict(callsite=address + offset, target=target))
    return dict(address=address, bytes=size, sha256=hashlib.sha256(raw).hexdigest(),
                direct_calls=result)


def qualify_setup(image, raw):
    if len(raw) != 1064 or hashlib.sha256(raw).hexdigest() != SETUP_SHA256:
        raise ValueError("unqualified normal G-Red setup profile")
    if struct.unpack_from(">IHHI", raw) != (0x47464353, 1, 0x615, 0xE2B4439A):
        raise ValueError("normal G-Red setup identity")
    data, config = raw[48:480], raw[480:672]
    if hashlib.sha256(data).hexdigest() != ACTOR_DATA_SHA256 or config != image.read(CONFIG, 192):
        raise ValueError("same-source actor data/class config required")
    return data, config


def recover(image, setup):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source DOL revision")
    data, config = qualify_setup(image, setup)
    handlers = list(struct.unpack(">32I", image.read(SELECTION_DISPATCH, 128)))
    for address in handlers:
        image.read(address, 4, "text")
    boundaries = sorted(set(handlers))
    selections = []
    for selector in FOCUSED_SELECTORS:
        start = handlers[selector]
        end = next(value for value in boundaries if value > start)
        selections.append(dict(selector=selector, **direct_calls(image, start, end - start)))
    commands = []
    for address, buffer, output in COMMANDS:
        words = struct.unpack(">6H", image.read(address, 12))
        commands.append(dict(address=address, buffer_offset=buffer, output_mask=output,
                             kind=words[0], flags=words[1], window=words[2],
                             pulse=words[3], button_masks=list(words[4:])))
    return dict(
        schema=1, model=0x615, source_sha256=image.sha256,
        setup_sha256=SETUP_SHA256, actor_data_sha256=ACTOR_DATA_SHA256,
        scope="read-only source operands and static calls; no controller execution",
        movement_type=config[1], config_flags=int.from_bytes(config[8:12], "big"),
        resource_policy_byte=config[0x11],
        installed_resource_policy=(2 if config[1] == 2 else 1) if config[0x11] else 0,
        parameters=movement_data(data), selection_dispatch=handlers,
        selection_calls=selections, input_commands=commands,
        direction_nibbles=list(struct.unpack(">16b", image.read(0x802DA770, 16))),
        ground_direction_classes=list(image.read(0x80433838, 8)),
        turn_operands=list(struct.unpack(">5H", image.read(CONFIG + 0xC0, 10))),
        input_spans=[direct_calls(image, address, size) for address, size in
                     ((0x800685C0, 304), (0x80068980, 524), (0x80068DF8, 420),
                      (0x80069038, 104), (0x80069140, 152), (0x80069210, 172),
                      (0x80069354, 168), (0x800693FC, 196), (0x800694C0, 40))],
    )


def markdown(report):
    lines = [
        "# Normal G-Red: movement admission and operands", "",
        "Static recovery, not native execution or recovered physical units.",
        "Direct-call order alone is not a control-flow graph; predicates and",
        "early returns must be respected before executing any transition.", "",
        f"- Movement type: {report['movement_type']}",
        f"- Resource policy byte: {report['resource_policy_byte']}",
        f"- Installed resource policy: {report['installed_resource_policy']}",
        f"- DOL SHA-256: `{report['source_sha256']}`",
        f"- Qualified setup SHA-256: `{report['setup_sha256']}`", "",
        "## Movement-selection handlers: source-order calls", "",
        "| Current selector | Handler | Direct calls |", "|---|---|---|",
    ]
    for row in report["selection_calls"]:
        calls = " → ".join(f"`{c['target']:08X}`" for c in row["direct_calls"])
        lines.append(f"| {row['selector']} | `{row['address']:08X}` | {calls} |")
    lines += ["", "## Buffered input commands", "",
              "| Output bit(s) | Source | Flags | Window | Pulse | Source button masks |",
              "|---|---|---|---|---|---|"]
    for row in report["input_commands"]:
        lines.append(f"| `{row['output_mask']:X}` | `{row['address']:08X}` |"
                     f" {row['flags']} | {row['window']} | {row['pulse']} |"
                     f" {row['button_masks']} |")
    lines += ["", "## Typed actor-data operands", "",
              "Offsets are inside the original432-byte actor-data resource.",
              "Roles identify source uses, not permission to copy values into MVP tuning.", "",
              "| Offset | Type | Role | Value | Consumer |", "|---|---|---|---|---|"]
    for row in report["parameters"]:
        lines.append(f"| `{row['offset']:03X}` | {row['type']} | {row['role']} |"
                     f" {row['value']} | `{row['consumer']:08X}` |")
    lines += ["", f"Direction-nibble lookup: `{report['direction_nibbles']}`",
              f"Ground direction classes: `{report['ground_direction_classes']}`",
              f"Turn operands (binary-angle domain): `{report['turn_operands']}`", ""]
    return "\n".join(lines)


def export(dol, setup, output, format="json"):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("movement report already exists")
    if format not in ("json", "markdown"):
        raise ValueError("report format")
    with ReadOnlyFile(dol) as source, ReadOnlyFile(setup) as profile:
        if profile.size != 1064:
            raise ValueError("setup file extent")
        report = recover(DolImage(source.read(0, source.size), SOURCE_SHA256),
                         profile.read(0, profile.size))
    encoded = (json.dumps(report, indent=2) if format == "json" else markdown(report)).encode()
    if len(encoded) > 1 << 20:
        raise ValueError("report byte budget")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(encoded)
    return dict(bytes=len(encoded), sha256=hashlib.sha256(encoded).hexdigest(),
                parameters=len(report["parameters"]), selection_handlers=len(report["selection_calls"]))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dol", type=Path)
    parser.add_argument("setup", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--format", choices=("json", "markdown"), default="json")
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.setup, args.output, args.format)))


if __name__ == "__main__":
    main()
