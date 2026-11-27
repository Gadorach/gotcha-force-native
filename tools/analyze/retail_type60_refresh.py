"""Verify original type-60 size stacking, cancellation and retained-token refusal."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct

from tools.analyze import retail_type60_scale as scale


cast, vm = scale.cast, scale.vm
SCALE_SHA256 = "c0c83f075a4da714b320714c434b70db475ade3b08883e0d081da2dd76b6d4f0"
TEXT = (
    (0x8002DB58, 944, "4aa097fc70080cd5b6c849d879cd20b39efebb359b80fb70b6a02c486ba38238"),
    (0x8002E2A8, 5672, "3f79bf55474f2fdc4d39b4436d481b713312b1477289db13deb902ffa26c0f1e"),
    (0x8008AFF0, 32, "8ea8cac06c446278fad05cfae1dbe3ee6b3a321f4bfd1168269ce88283bef951"),
)
CASES = (
    ("grow-twice", 0, 0, False, False),
    ("shrink-twice", 2, 2, False, False),
    ("grow-then-cancel", 0, 2, False, False),
    ("shrink-then-cancel", 2, 0, False, False),
    ("grow-then-cross-neutral", 0, 3, False, False),
    ("batch-grow-and-cancel", 0, 2, True, False),
    ("batch-grow-twice", 0, 0, True, False),
    ("retained-token", 0, 0, False, True),
    ("batch-shrink-and-cancel", 2, 0, True, False),
    ("batch-cross-neutral", 0, 3, True, False),
)
ROW = re.compile(r"type60-refresh:(?: [0-9]{1,4}){5} -?[0-9] [0-9]{2}(?: [0-9]{1,3}){6}(?: [0-9A-F]{8}){4}")


def polarity(delta):
    return 0x20 if delta > 0 else 0x40


def float_word(value):
    return struct.pack(">f", value).hex().upper()


def advance(value, target, ramp):
    if value < target:
        value = min(value+ramp, target)
    elif value > target:
        value = max(value-ramp, target)
    return struct.unpack(">f", struct.pack(">f", value))[0]


def decode(report, table, ramp):
    lines = report.splitlines()
    if len(report) > 2048 or len(lines) != len(CASES):
        raise ValueError("type-60 refresh report extent")
    rows = []
    for i, (name, first, second, batch, retained) in enumerate(CASES):
        a, b = cast.ROUTES[first], cast.ROUTES[second]
        deltas = (1, 2, -1, -2)
        before = 0 if batch else 897
        total = deltas[first]+(0 if retained else deltas[second])
        queued = 0 if retained else total if batch else deltas[second]
        timer = before if retained or not total else 900
        flags = 0 if retained else polarity(deltas[second]) | (polarity(deltas[first]) if batch else 0)
        start = table[16][0]
        applied = start
        if not batch:
            for _ in range(3):
                applied = advance(applied, table[16+deltas[first]][0], ramp)
        target = table[16+total][0]
        next_scale = advance(applied, target, ramp)
        ints = (i, a.model, b.model, batch, retained, total, 16+total, before, timer, max(0, timer-1),
                flags, 1, 0 if retained else 2)
        floats = tuple(map(float_word, (applied, target, next_scale, start)))
        if ROW.fullmatch(lines[i]) is None:
            raise ValueError("invalid type-60 refresh row")
        fields = lines[i].split()[1:]
        if tuple(map(int, fields[:13])) != ints or tuple(fields[13:]) != floats:
            raise ValueError(f"type-60 refresh queue, timer or ramp changed: {name}")
        rows.append({
            "case": name, "first_caster": f"gg4e08:{a.model:04x}",
            "second_caster": None if retained else f"gg4e08:{b.model:04x}",
            "same_scene_pass": batch, "pre_second_isolated_ticks": 0 if batch else 3,
            "receiver_history_tokens": [1] if retained else [1, 2],
            "consumer_calls_in_final_scene_pass": int(not retained), "final_pass_signed_delta": queued,
            "final_pass_size_flag_bits": flags, "temporary_offset_after_final_pass": total,
            "size_index_after_final_pass": 16+total, "timer_before_final_pass": before,
            "timer_after_final_pass": timer, "timer_after_next_size_tick": max(0, timer-1),
            "float_bits": dict(zip(("applied_before_and_after_final_pass", "target_after_final_pass",
                                    "applied_on_next_size_tick", "applied_after_expiry_and_return"), floats, strict=True)),
        })
    return rows


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("type-60 refresh output already exists")
    dol, module = (Path(p).resolve(strict=True) for p in (dol, module))
    image = vm.DolImage(vm.read_bounded(dol, 8 << 20), vm.SOURCE_SHA256)
    vm.beam.verify_text(image, (*vm.TEXT, *cast.TEXT, *scale.TEXT, *TEXT))
    for site, target in ((0x8002C4CC, 0x8002DB58), (0x8002DDA8, 0x8002E2A8), (0x8002C5D0, 0x800681E4),
                         (0x8005909C, 0x8005BAB8), (0x8005BAF4, 0x8006817C)):
        vm.beam.link(image, site, target)
    vm.read_metadata(vm.MAPS / "gg4e08-type60-scale.json", SCALE_SHA256)
    atlas = vm.read_metadata(vm.MAPS / "gg4e08-code-atlas.json", vm.ATLAS_SHA256)
    with vm.open_regular(afs) as stream:
        stamp = vm.archive_stamp(stream)
        vm.verify_archive(stream)
        archive = vm.RetailArchive(stream)
        hurt, spec = vm.opening.read_normal(archive, vm.opening.RECEIVER, vm.opening.RECEIVER_BLOCK_SHA256,
                                           vm.ACTOR_SHA256[vm.opening.RECEIVER])
        _, casters = cast.source_routes(image, archive, atlas)
        if vm.archive_stamp(stream) != stamp:
            raise ValueError("type-60 refresh archive input changed")
    table, ramp = scale.steps(image)
    rows = decode(vm.clock.fixture_report(fixture, dol, module, ("--refresh",), hurt+spec+casters), table, ramp)
    raw = vm.encode({
        "kind": "gf-retail-type60-refresh", "revision": 1, "source_dol_sha256": vm.SOURCE_SHA256,
        "source_afs_sha256": vm.AFS_SHA256, "type60_scale_sha256": SCALE_SHA256,
        "receiver": "gg4e08:000a", "cases": rows,
        "notes": [
            "Fresh casts run separate original release and initializer visits, with independent caster, carrier and shape cells.",
            "Sequential cases execute three isolated size ticks between scene passes. The original unbind removes the old shape from publication before a fresh sequential contact.",
            "Two sources in a batch publish before one original scene pass. Queued delta and history are checked after the single size consumer call.",
            "The retained-token case republishes no new shape and leaves one existing hit-history token.",
            "No type-33 cell is offered. HP and the checked receiver damage ledger do not change.",
            "These are declared cue, pose and scheduler inlets, not live recasts, animation refresh, effect lifetime or native gameplay.",
        ],
    }, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(rows)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
