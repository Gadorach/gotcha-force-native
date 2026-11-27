"""Recover normal, direct-contact sound pairs and their receiver size bands."""
import argparse
from hashlib import sha256
import json
import math
from pathlib import Path
import struct

from tools.analyze import damage_contact_power, retail_projectile_sfx
from tools.analyze.reaction_attack_table import SIZE
from tools.extract.battle_initial_target import SOURCE_SHA256
from tools.extract.source_image import DolImage, ReadOnlyFile

TABLE = 0x8031B874
CODES = (0, 0x80, 0x84, 0x88)
STATUS_MASKS = (0x030E, 0x102C)
THRESHOLDS = (0x80436FB8, 0x80436F94, 0x80436FBC)
SIZE_DIVISOR = 0x80437620
TEXT = (
    (0x8002F514, 112, "4e9836a76e4179e44f6b7341980a91bd956e4e05c0da116feecdfe22656ffaf1"),
    (0x8002F6B8, 408, "769d9107a3e0f31b17d74edf1567e01ec5ddf1d6164f5dbf9c5dd76a4b231358"),
    (0x8002FC18, 164, "3292958ed7d95859bec49a2d87c4d022d0f977ffe7fd2df767df4c7781631171"),
    (0x800F088C, 148, "a58f27f1d381850dc75d0cf6ccba6f575dd54aa0ce5b75e7a385545e1477276e"),
    (0x800F04B4, 120, "839c4c6e4469de152b62eef05230add8b4fcb5e3eab918ac7c61dca5632cca3e"),
    (0x80068004, 44, "f6b33009231fcc4c680dd16c4bbe7b6c08575ee1404186b0c9c499d134b03b76"),
)
CALLS = {0x8005442C: 0x48013BD9, 0x80057C98: 0x4801036D, 0x80058304: 0x4800FD01}


def recover(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("unqualified source revision")
    if any(sha256(image.read(site, size, "text")).hexdigest() != digest for site, size, digest in TEXT):
        raise ValueError("contact sound dispatch or size calculation changed")
    if any(image.read(site, 4, "text") != struct.pack(">I", word) for site, word in CALLS.items()):
        raise ValueError("receiver size initialization call changed")
    read_float = lambda at: struct.unpack(">f", image.read(at, 4, "data"))[0]
    thresholds = [read_float(at) for at in THRESHOLDS]
    divisor = read_float(SIZE_DIVISOR)
    bases = struct.unpack(">13h", image.read(TABLE, 26, "data"))
    if thresholds != [1.5, 2.5, 3.5] or not 167.75 < divisor < 167.77 or bases != (*range(48, 72, 2), 116):
        raise ValueError("contact sound table contents changed")
    return {"kind": "gf-contact-sound", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
            "source": {"cue_offset": 10, "visual_selector_offset": 9, "normal_call": "8002F894",
                       "normal_helper": "8002FC18", "alternate_helper_fixed_id": 38,
                       "size_field_offset": 2044, "size_pointer_offset": 1200,
                       "size_config_offsets": [76, 80], "size_producer": "80068004",
                       "size_call_sites": [f"{at:08X}" for at in CALLS],
                       "size_expression": "max(config+76, config+80) divided by divisor",
                       "divisor_address": f"{SIZE_DIVISOR:08X}", "divisor": divisor,
                       "thresholds": thresholds, "range_mask": 0x80, "special_group_mask": 0x40,
                       "silence_code": 255, "counter_rows": "803D3AE8", "normal_counter_width": 13,
                       "receiver_gate_offset": 686, "cooldown": 4,
                       "sound_table": f"{TABLE:08X}", "dispatch": "800F088C", "normal_base_ids": list(bases)},
            "policy": {"scope": "normal direct contacts; source tone pairs, native impact gate and receiver-local parity",
                       "ordinary_codes": list(CODES), "status_refusal_masks": list(STATUS_MASKS),
                       "unbound": "special audio slots, status/model branches, mixed-size configurations, combined receivers"}}


def normal_pairs(report, code):
    if code not in CODES:
        return []
    base = code & 0x7C
    return [(base + (band if code & 0x80 else 0),
             report["source"]["normal_base_ids"][base + (band if code & 0x80 else 0)])
            for band in range(4)]


def receiver_band(report, image, actions, binding, actor_size):
    profiles = actions.get("profiles", [])
    if actor_size != 432 or not binding or binding.get("status") != "one-static-slot-0-record" or (
            binding.get("selected_record") != 0):
        return {"band": None, "status": "not a proved single normal record"}
    if not 1 <= len(profiles) <= 32 or any(actions.get(k) for k in (
            "refused_bindings", "open_path_boundaries", "unresolved_stores")):
        return {"band": None, "status": "unqualified installer configurations"}
    samples = []
    for config in sorted({row["config"] for row in profiles}):
        dimensions = struct.unpack(">2f", image.read(int(config, 16) + 76, 8, "data"))
        if any(not math.isfinite(value) or not 0 <= value <= 10000 for value in dimensions):
            return {"band": None, "status": "invalid receiver size dimension"}
        size, = struct.unpack(">f", struct.pack(">f", max(dimensions) / report["source"]["divisor"]))
        samples.append({"config": config, "dimensions": list(dimensions), "size": size,
                        "band": sum(size > floor for floor in report["source"]["thresholds"])})
    choices = {row["band"] for row in samples}
    return {"band": next(iter(choices)) if len(choices) == 1 else None, "samples": samples,
            "status": "one static size band" if len(choices) == 1 else "installer bands disagree"}


def contact_records(image, binding):
    power = damage_contact_power.move_power(image, binding)
    table = int(power["table"], 16)
    rows = []
    for value in power["entries"]:
        index = value["index"]
        row = image.read(table + index * SIZE, SIZE, "data")
        rows.append({"index": index, "base": value["hp_base"], "visual": row[9], "code": row[10],
                     "flags": list(struct.unpack_from(">2H", row, 16))})
    return rows


def ordinary_refusal(rows):
    if any(not 1 <= row["base"] <= 10000 for row in rows):
        return "zero or out-of-range HP base"
    if any(any(mask & flag for mask, flag in zip(STATUS_MASKS, row["flags"], strict=True)) for row in rows):
        return "special status operands"
    return None


def move_sound(image, binding):
    rows = contact_records(image, binding)
    codes = {row["code"] for row in rows}
    status = ordinary_refusal(rows)
    if not status:
        if any(row["visual"] == 255 for row in rows) or not codes <= set(CODES):
            status = "unsupported or silent contact sound"
        elif len(codes) != 1:
            status = "source sound codes disagree"
        else:
            status = "qualified ordinary sound operands"
    return {"table": binding["table"], "entries": rows, "status": status,
            "code": next(iter(codes)) if status == "qualified ordinary sound operands" else None}


def tone_files(report, banks):
    ids = {base + variant for code in CODES for _, base in normal_pairs(report, code) for variant in (0, 1)}
    tones = {sound: retail_projectile_sfx.tone_candidate(sound, banks) for sound in sorted(ids)}
    if any(row["status"] != "static-tone-record" or row["bank_slot"] != 0 for row in tones.values()):
        raise ValueError("ordinary contact cue bank identity")
    return tones


def profile_rows(report, image, bindings, receiver, tones):
    lines = []
    if receiver["band"] is not None:
        lines.append(f"contact-sound-band={receiver['band']}")
    moves = {name: move_sound(image, route) for name, route in sorted(bindings.items())}
    files = {}
    for move, result in moves.items():
        if result["code"] is None:
            continue
        result["bands"] = []
        for band, (lane, base) in enumerate(normal_pairs(report, result["code"])):
            paths = [f"audio/contact-{sound}.pcm" for sound in (base, base + 1)]
            for i, path in enumerate(paths):
                tone = tones[base + i]
                files[path] = tone["bank"], tone["tone"]
            lines.append(f"contact-sound={move}|{band}|{lane}|{'|'.join(paths)}")
            result["bands"].append({"band": band, "lane": lane, "sound_ids": [base, base + 1]})
    return {"receiver": receiver, "moves": moves, "lines": lines}, files


def export(dol, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("contact sound output already exists")
    with ReadOnlyFile(dol) as source:
        if not 0 < source.size <= 8 << 20:
            raise ValueError("source DOL extent")
        report = recover(DolImage(source.read(0, source.size), SOURCE_SHA256))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("x", encoding="ascii") as stream:
        stream.write(json.dumps(report, indent=2, sort_keys=True) + "\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dol", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    export(args.dol, args.output)
