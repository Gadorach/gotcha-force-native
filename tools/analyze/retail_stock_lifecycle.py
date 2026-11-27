"""Replay the original normal-roster debit, scheduled refill and stock boundaries."""
import argparse
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import re
import struct

from tools.analyze import retail_stock_startup as startup


STARTUP_SHA256 = "07f97b6382f2f7cd8482dacf2892fc4d81ca12afce9ced9f93f70aada1687c3f"
TEXT = (
    (0x8005568C, 1856, "0f964164be0f7581e45a26f8acd1113b414d13ecdbefeb94be72acde53c156ac"),
    (0x8005A378, 520, "b6a3dfe9d8bf1b34fb4d470444385f1bbdafe4ba0596786d67699ca9e92ecd2f"),
    (0x8006DE10, 52, "fdbdc64d6a82e1f539cf9aaedfdf3fb054254649f2cad9c648005b992c0dcaff"),
)
SNAPSHOT = r" -?[0-9]{1,5} -?[0-9]{1,5} [0-9A-F]{8}"
ROW = re.compile(r"stock-cycle: [0-9A-F]{4} [0-9]{1,2} [02] [01] [01]" + SNAPSHOT*3)
TIME = re.compile(r"stock-time: [0-9A-F]{4} [02] [0-9]{1,3}" + SNAPSHOT)
TIMELINES = {
    (0x0103, 2): ((0, 4, 4, 17), (1, 4, 4, 17), (2, 3, 4, 0), (3, 3, 4, 0), (4, 4, 4, 0)),
    (0x010A, 0): ((0, 1, 1, 17), (1, 1, 1, 17), (2, 0, 1, 0), (3, 0, 1, 0), (4, 1, 1, 0)),
    (0x0705, 0): ((0, 0, 0, 300), (1, -1, -1, 299), (299, -1, -1, 1), (300, 0, 0, 0), (301, 0, 0, 0)),
    (0x0800, 0): ((0, 3, 4, 599), (19, 3, 4, 314), (20, 4, 4, 299), (40, 5, 5, -1), (41, 5, 5, -1)),
    (0x0900, 2): ((0, 0, 1, 180), (179, 0, 1, 1), (180, 1, 1, 0), (181, 1, 1, 0)),
}
NEGATIVE = {("gg4e08:0615", 2), ("gg4e08:0629", 2), ("gg4e08:062a", 2),
            ("gg4e08:0705", 0), ("gg4e08:0807", 0), ("gg4e08:0e04", 0)}


def float_bits(value):
    return struct.pack(">f", value).hex().upper()


def snapshot_fields(state):
    return (*map(str, state[:2]), float_bits(state[2]))


def one_debit(row):
    count, limit, mode, preset = row
    clock = preset if count == 0 and mode in (0, 1) else 0
    if count == 0:
        return count, limit, clock
    if mode == 1 or (mode == 0 and count == 1):
        clock = preset
    return count-1, count, clock


def refill(state, limit, mode, preset, step):
    count, previous, clock = state
    if clock <= 0 or mode not in (0, 1):
        return state
    clock -= step
    if clock <= 0:
        return limit, limit, clock if mode == 1 else 0
    if mode == 0:
        return count, previous, clock
    candidate = limit-clock//preset-1
    return candidate, previous if candidate == count else candidate, clock


def decode(report, sources):
    lines = report.splitlines()
    time_count = sum(map(len, TIMELINES.values()))
    if len(report) > 550_000 or len(lines) != 2*10*len(sources)+time_count:
        raise ValueError("stock lifecycle report extent")
    cycles = iter(line for line in lines if not line.startswith("stock-time:"))
    time_rows = iter(line for line in lines if line.startswith("stock-time:"))
    models, timeline_rows = [], []
    stats, negative = Counter(), {}
    for s in sources:
        rows = []
        for level, operands in enumerate(s.level_rows, 1):
            for channel, start in ((0, 1), (2, 5)):
                line = next(cycles)
                row = operands[start:start+4]
                count, limit, mode, preset = row
                if min(count, limit, preset) < 0 or mode not in (0, 1, 2, 3):
                    raise ValueError("stock lifecycle source domain")
                paid = one_debit(row)
                normal = refill(paid, limit, mode, preset, 1)
                fast = refill(paid, limit, mode, preset, 15)
                accepted = str(int(count > 0))
                expected = (f"{s.model:04X}", str(level), str(channel), accepted, accepted,
                            *snapshot_fields(paid), *snapshot_fields(normal), *snapshot_fields(fast))
                if ROW.fullmatch(line) is None or tuple(line.split()[1:]) != expected:
                    raise ValueError(f"stock lifecycle changed: {s.id}/{level}/channel {channel}")
                stats["channel_rows"] += 1
                stats["accepted_checks_and_debits"] += int(accepted)
                stats["refill_clock_changes"] += normal[2] != paid[2]
                stats["alternate_step_changes"] += fast != normal
                if normal[0] < 0:
                    negative.setdefault((s.id, channel), []).append(level)
                rows.append(line)
        models.append({"borg": s.id, "checked_channel_rows": len(rows),
                       "lifecycle_observation_sha256": sha256(("\n".join(rows)+"\n").encode("ascii")).hexdigest()})
    for (model, channel), states in TIMELINES.items():
        observations = []
        for i, current, previous, clock in states:
            line = next(time_rows)
            if TIME.fullmatch(line) is None or tuple(line.split()[1:]) != (
                    f"{model:04X}", str(channel), str(i), *snapshot_fields((current, previous, clock))):
                raise ValueError(f"stock lifecycle timeline changed: {model:04X}/{i}")
            observations.append({"checkpoint": i, "current": current, "previous": previous, "clock": clock})
        timeline_rows.append({"borg": f"gg4e08:{model:04x}", "channel": channel, "cases": observations})
    if (stats != {"channel_rows": 4120, "accepted_checks_and_debits": 2920, "refill_clock_changes": 1140,
                  "alternate_step_changes": 1140} or set(negative) != NEGATIVE or
        any(levels != list(range(1, 11)) for levels in negative.values())):
        raise ValueError("stock lifecycle counted source coverage changed")
    return models, dict(stats), [
        {"borg": model, "channel": channel, "levels": levels} for (model, channel), levels in sorted(negative.items())
    ], timeline_rows


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("stock lifecycle output already exists")
    dol, module = (Path(p).resolve(strict=True) for p in (dol, module))
    image = startup.DolImage(startup.read_bounded(dol, 8 << 20), startup.SOURCE_SHA256)
    startup.verify_text(image, (*startup.TEXT, *startup.stock.TEXT, *TEXT))
    startup.link(image, 0x80055D08, 0x8006DCC0)
    for offset, value in ((-29528, 10.0), (-30296, 1.0), (-30280, 1.5)):
        if image.read(0x8043EA20+offset, 4, "data") != struct.pack(">f", value):
            raise ValueError("stock lifecycle exact step operand changed")
    startup.read_metadata(startup.MAPS / "gg4e08-stock-startup.json", STARTUP_SHA256)
    installed = startup.read_metadata(startup.MAPS / "gg4e08-standalone-installers.json", startup.INSTALLERS_SHA256)
    with startup.open_regular(afs) as stream:
        stamp = startup.archive_stamp(stream)
        startup.verify_archive(stream)
        plan, sources = startup.source_inputs(image, startup.RetailArchive(stream), installed)
        if startup.archive_stamp(stream) != stamp:
            raise ValueError("stock lifecycle source archive changed")
    models, stats, negative, timelines = decode(startup.fixture_report(
        fixture, dol, module, ("--lifecycle",), plan), sources)
    raw = startup.encode({
        "kind": "gf-retail-stock-lifecycle", "revision": 1, "source_dol_sha256": startup.SOURCE_SHA256,
        "source_afs_sha256": startup.AFS_SHA256, "stock_startup_sha256": STARTUP_SHA256,
        "entries": {"check_and_debit": "8006DBE0", "base_rate": "8005A378", "wrapper": "8005568C",
                    "wrapper_refill_call": "80055D08", "refill": "8006DCC0", "direct_return": "8006DE10"},
        "summary": stats, "negative_temporary_stock": negative, "timelines": timelines, "models": models,
        "notes": [
            "Each channel independently forks from the exact original normal startup. One-unit checks and commits, then single refill visits, do not establish any move release.",
            "The original rate helper produces carry-in base 1 or 1.5 for a declared zero/nonzero +0x6FC. The actual refill wrapper consumes step 1 or 15.",
            "Each inhibit-bit control holds its channel and still allows the other one. Uninitialized channel 1 stays masked.",
            "For 0103/010A, checkpoint 0 declares clock 17; 1 is refill, 2 is debit, 3 is another refill, and 4 is a direct return call without a gameplay trigger.",
            "The other timeline checkpoints are isolated wrapper visits. On 0800 the two observed debits have one ordinary refill visit between them.",
            "The zero-limit mode-1 control checks that negative stock cannot pass a one-unit check or debit. No negative stock is admitted into the native domain.",
            "No complete actor scheduling, gameplay return trigger, user input, wall-clock duration or native change is qualified.",
        ],
    }, expanded=("models", "negative_temporary_stock", "timelines"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "models": len(models), "channel_rows": stats["channel_rows"]}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
