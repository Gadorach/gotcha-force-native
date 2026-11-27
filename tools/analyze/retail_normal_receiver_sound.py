"""Replay original contact sound with the installed normal receiver configuration."""
import argparse
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import re
import struct

from tools.analyze import contact_sound as sound
from tools.analyze import retail_contact_sound_replay as replay
from tools.analyze import retail_stock_startup as startup
from tools.analyze.retail_stock_lifecycle import STARTUP_SHA256


REPLAY_SHA256 = "a0505d499ca4173d7ec3a8e179334d98d7d78a4f024bb2e7f207f35591d22308"
ROW = re.compile(r"receiver-sound: [0-9A-F]{4} [0-7] [0-9A-F]{8} [0-9A-F]{8}(?: [0-9]{1,2}){8}")


def decode(report, image, rules, sources):
    lines = report.splitlines()
    if len(report) > 16384 or len(lines) != len(sources):
        raise ValueError("normal receiver sound report extent")
    rows = []
    for line, source in zip(lines, sources, strict=True):
        if ROW.fullmatch(line) is None:
            raise ValueError("malformed normal receiver sound report")
        model, spec, config, raw_size, *choices = line.split()[1:]
        samples = sound.configuration_samples(rules, image, [config])
        if samples is None:
            raise ValueError(f"{source.id}: receiver sound config dimension outside scope")
        sample, = samples
        band = sample["band"]
        lanes = [sound.normal_pairs(rules, code)[band] for code in sound.CODES]
        if ((model, int(spec), raw_size) != (f"{source.model:04X}", source.spec_index,
                                            struct.pack(">f", sample["size"]).hex().upper()) or
                tuple(map(int, choices)) != tuple(value for pair in lanes for value in pair)):
            raise ValueError(f"{source.id}: selected record, sound size or tone changed")
        rows.append({"borg": source.id, "installer": source.installer, "spec_record": source.spec_index,
                     "record_sha256": sha256(source.spec).hexdigest(), **sample,
                     "size_bits": raw_size, "lanes_and_even_tones": lanes})
    counts = Counter(row["band"] for row in rows)
    if counts != {0: 136, 1: 26, 2: 19, 3: 25}:
        raise ValueError("normal receiver sound band coverage changed")
    return rows, dict(sorted(counts.items()))


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("normal receiver sound output already exists")
    dol = Path(dol).resolve(strict=True)
    image = startup.DolImage(startup.read_bounded(dol, 8 << 20), startup.SOURCE_SHA256)
    rules = sound.recover(image)
    startup.verify_text(image, (*startup.TEXT, *replay.TEXT))
    if rules != startup.read_metadata(startup.MAPS / "contact-sound.json", replay.SOUND_SHA256):
        raise ValueError("normal receiver sound rules changed")
    startup.read_metadata(startup.MAPS / "gg4e08-stock-startup.json", STARTUP_SHA256)
    startup.read_metadata(startup.MAPS / "gg4e08-contact-sound-replay.json", REPLAY_SHA256)
    installed = startup.read_metadata(startup.MAPS / "gg4e08-standalone-installers.json", startup.INSTALLERS_SHA256)
    with startup.open_regular(afs) as stream:
        stamp = startup.archive_stamp(stream)
        startup.verify_archive(stream)
        plan, sources = startup.source_inputs(image, startup.RetailArchive(stream), installed)
        if startup.archive_stamp(stream) != stamp:
            raise ValueError("normal receiver sound archive input changed")
    records, counts = decode(startup.fixture_report(fixture, dol, module, ("--receiver-sound",), plan), image,
                             rules, sources)
    data = {
        "kind": "gf-retail-normal-receiver-sound", "revision": 1, "source_dol_sha256": startup.SOURCE_SHA256,
        "source_afs_sha256": startup.AFS_SHA256, "stock_startup_sha256": STARTUP_SHA256,
        "contact_sound_replay_sha256": REPLAY_SHA256, "codes": list(sound.CODES),
        "band_counts": counts, "records": records,
        "notes": [
            "Install the exact normal actor record; confirm the configuration pointer survives the level-1 initialization prefix.",
            "Call the original size producer directly after the prefix. The full constructor and its intervening resource work are not replayed.",
            "Each original sound helper call starts with partition zero, a clear receiver gate and zero parity counters.",
            "Observe the counter and selected even ID with playback disabled; no live contact, mixing, dynamic size switch or combined component is qualified.",
        ],
    }
    raw = startup.encode(data, expanded=("records",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "receivers": len(records), "probes": len(records)*len(sound.CODES),
            "band_counts": counts}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
