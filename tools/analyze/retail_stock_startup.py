"""Join original normal installers to their bounded size and level-stock writes."""
import argparse
from collections import Counter
from dataclasses import dataclass
from hashlib import sha256
import json
from pathlib import Path
import re
import struct

from tools.analyze import retail_stock_operands as stock
from tools.analyze.retail_action_input import archive_stamp, verify_archive
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import AFS_SHA256, INSTALLERS_SHA256, SOURCE_SHA256, encode, read_metadata
from tools.analyze.retail_type32_roster import link, verify_text
from tools.analyze.retail_contact_pose_clock import fixture_report
from tools.extract.retail_borg_placeholders import open_regular
from tools.extract.retail_borg_roster import ACTOR_BYTES, RetailArchive, levels
from tools.extract.source_image import DolImage


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"
TEXT = (
    (0x8005614C, 52, "3ad751d94fd5b75a4ced6d2a1719b43ff420497090fec94a773a4799f48b5e4d"),
    (0x80056180, 312, "105bfba36f7609d8ecc447ebbbfbfa55bb82e52eeaee98cd9c0c8e09b0420494"),
    (0x800562B8, 1444, "f2cd0805aa1b4be0c29b96adf31b92c0f289b243496d863c0262909316ad7e95"),
    (0x8006826C, 140, "b5984640399d2952aedfaa554a6459035788415792189757db695fe6c0b4a7bf"),
)
ROW = re.compile(r"stock-start: [0-9A-F]{4} [0-9]{1,2} [0-7] [01]"
                 r"(?: [0-9A-F]{4}){2}(?: [0-9A-F]{8}){4}(?:(?: [0-9]{1,5}){6} [0-9A-F]{8}){2}")
EXAMPLES = (("gg4e08:0206", 1), ("gg4e08:0628", 1), ("gg4e08:0705", 1),
            ("gg4e08:0900", 1), ("gg4e08:0902", 1), ("gg4e08:0c06", 10))


@dataclass(frozen=True)
class Source:
    id: str
    model: int
    installer: str
    spec_index: int
    spec: bytes
    level_rows: tuple


def source_inputs(image, archive, installed):
    sources = []
    plan = bytearray()
    if len(installed["borgs"]) != 206:
        raise ValueError("normal stock startup roster extent")
    for r in installed["borgs"]:
        model, idx = int(r["id"][-4:], 16), r["spec_record"]
        name = f"pl{model:04x}.pzz".encode("ascii")
        package, _ = archive.member(archive.names.index(name), model)
        records = package.read(0, 8*ACTOR_BYTES)[0]
        count, remainder = divmod(len(records), ACTOR_BYTES)
        if remainder or not 0 <= idx < count <= 8:
            raise ValueError("normal stock startup actor data extent")
        spec = records[idx*ACTOR_BYTES:(idx+1)*ACTOR_BYTES]
        if not 0 < spec[0xA2] <= 16 <= spec[0xA1] <= 31 or spec[0xA3] not in (0, 1):
            raise ValueError("normal startup size curve/selector domain")
        plan += struct.pack(">HIBB", model, int(r["installer"], 16), count, 0)+records
        sources.append(Source(r["id"], model, r["installer"], idx, spec, tuple(levels(image, model))))
    return bytes(plan), sources


def decode(report, sources):
    lines = report.splitlines()
    if len(report) > 325_000 or len(lines) != len(sources)*10:
        raise ValueError("normal stock startup report extent")
    models, examples = [], []
    counts, initial_clocks, extents, frontiers = Counter(), Counter(), Counter(), Counter()
    for index, source in enumerate(sources):
        spec = source.spec
        masks = tuple(spec[at:at+2].hex().upper() for at in (0xA8, 0xAA))
        size_bytes = bytes((16, 16, spec[0xA2], spec[0xA1])).hex().upper()
        checked = []
        for level, raw in enumerate(source.level_rows, 1):
            line = lines[10*index+level-1]
            if ROW.fullmatch(line) is None:
                raise ValueError("invalid stock startup report row")
            model, observed_level, observed_spec, cut, *fields = line.split()[1:]
            if ((int(model, 16), int(observed_level), int(observed_spec)) != (source.model, level, source.spec_index) or
                fields[:6] != [*masks, size_bytes, "3F800000", "3F800000", "3F800000"]):
                raise ValueError(f"normal stock startup identity, size or masks changed: {source.id}/{level}")
            frontiers[cut] += 1
            chunks = []
            for channel, at, start in ((0, 6, 1), (2, 13, 5)):
                count, limit, mode, preset = raw[start:start+4]
                if min(count, limit, preset) < 0 or mode not in (0, 1, 2, 3):
                    raise ValueError("normal stock startup source domain")
                clock = preset if count == 0 and mode in (0, 1) else 0
                clock_bits = struct.pack(">f", clock).hex().upper()
                observed = fields[at:at+7]
                expected = (count, limit, mode, preset, count, limit)
                if tuple(map(int, observed[:6])) != expected or observed[6] != clock_bits:
                    raise ValueError(f"normal stock startup channel {channel} changed: {source.id}/{level}")
                counts[channel, mode] += 1
                extents["empty_stocks"] += count == 0
                extents["initial_over_limit"] += count > limit
                initial_clocks[channel] += clock != 0
                chunks.append({"channel": channel, "initial": count, "limit": limit, "mode": mode,
                               "preset": preset, "current": count, "previous": limit, "initial_clock": clock})
            if (source.id, level) in EXAMPLES:
                examples.append({"borg": source.id, "level": level, "channels": chunks})
            checked.append(line)
        models.append({
            "borg": source.id, "installer": source.installer, "spec_record": source.spec_index,
            "actor_spec_sha256": sha256(spec).hexdigest(), "checked_levels": len(checked),
            "initial_size_index": 16, "size_family": spec[0xA3], "size_bounds": [spec[0xA2], spec[0xA1]],
            "stock_and_size_observation_sha256": sha256(("\n".join(checked)+"\n").encode("ascii")).hexdigest(),
        })
    if (frontiers != {"0": 1440, "1": 620} or dict(extents) != {"empty_stocks": 1200, "initial_over_limit": 1} or
        initial_clocks != {0: 60, 2: 190} or len(examples) != len(EXAMPLES)):
        raise ValueError("normal stock startup coverage changed")
    summary = {
        "models": len(models), "model_level_visits": 2060, "checked_channel_rows": 4120, **extents,
        "neutral_size_1_0_visits": 2060, "untouched_channel_1_visits": 2060,
        "initialized_nonzero_clock_rows": {str(ch): n for ch, n in sorted(initial_clocks.items())},
        "rows_by_channel_and_mode": {str(ch): {str(mode): counts[ch, mode] for mode in range(4)} for ch in (0, 2)},
        "resource_frontiers": {"80006238": frontiers["0"], "8013BD80": frontiers["1"]},
    }
    return models, examples, summary


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("stock startup output already exists")
    dol, module = (Path(p).resolve(strict=True) for p in (dol, module))
    image = DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256)
    verify_text(image, TEXT)
    schema = stock.stock_schema(image)
    for site, target in ((0x80056160, 0x80056180), (0x80056168, 0x800562B8), (0x80056218, 0x8006826C),
                         (0x80056600, 0x8013BD80), (0x80056820, 0x80006238)):
        link(image, site, target)
    installed = read_metadata(MAPS / "gg4e08-standalone-installers.json", INSTALLERS_SHA256)
    with open_regular(afs) as stream:
        stamp = archive_stamp(stream)
        verify_archive(stream)
        plan, sources = source_inputs(image, RetailArchive(stream), installed)
        if archive_stamp(stream) != stamp:
            raise ValueError("normal stock startup archive changed")
    models, examples, summary = decode(fixture_report(fixture, dol, module, payload=plan), sources)
    raw = encode({
        "kind": "gf-retail-stock-startup", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "source_afs_sha256": AFS_SHA256, "standalone_installers_sha256": INSTALLERS_SHA256,
        "level_row_channels": schema["level_row_channels"], "normal_level_row_base": schema["normal_row_base"],
        "untouched_channel": 1, "summary": summary, "examples": examples, "models": models,
        "notes": [
            "Original normal installer selects an exact archived role-0 record. Stock and size initialization consume it with declared normal level inputs.",
            "No equipment, material, actor admission, device input or full battle constructor is executed beyond the declared frontier.",
            "Channel 1 has distinct sentinels for both stock rows and stays untouched by this prefix. This does not exclude writes from other code.",
            "The observation digest per model covers ten checked rows. Initial clocks are startup values, not a test of later debit, refill or reset scheduling.",
            "The corrected source index does not enable moves or change native stock behavior.",
        ],
    }, expanded=("models", "examples"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "models": len(models), "level_visits": summary["model_level_visits"]}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
