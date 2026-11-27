"""Replay retirement and reuse with the original contact-effect model archives."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
from subprocess import run

from tools.analyze import retail_contact_effect_source_art as source, retail_type32_roster as beam
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import AFS_SHA256, SOURCE_SHA256, encode, read_metadata
from tools.extract.source_image import DolImage


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"
ART_SHA256 = "025c745717b4c40844049721831a40d402bc82ad31d07fddc451187efb8eb86e"
ROW = re.compile(r"effect-source-lifetime:(?: [0-9]{1,3}){12}")
# selector, first model IDs by epoch, total model loads, peak queued,
# last queued visit, retirement visit, returned descriptors, freed heap block.
CASES = (
    (0, ((21,), (21,)), 1, 1, 21, 23, 1, 0),
    (1, ((2,), (3,)), 1, 1, 16, 19, 1, 96),
    (8, ((53, 54), (53, 54)), 6, 6, 10, 13, 8, 288),
)
TEXT = (
    (0x80019950, 132, "772e20902700701da19b8e30231afd7b7419fcd2efc5a16641ff2685bfdca385"),
    (0x80006D80, 72, "af3ef03d2ff4dfbd26b5e33794ffac72543f7b6b70aaa2a0c7f7af14cdf13768"),
    (0x8024FD74, 328, "537997f5603206fdec743ca01c199d79c052020f38fd2e9ae8b8afb07ba8caf4"),
)


def decode(report, slot):
    lines = report.splitlines()
    if slot not in (1, 2) or len(report) > 1024 or len(lines) != 6:
        raise ValueError("source model lifetime report extent")
    rows = []
    for selector, records, loads, peak, last, retire, descriptors, block in CASES:
        for generation, first in enumerate(records, 1):
            expected = (slot, selector, generation, first[0], first[1] if len(first) == 2 else 0,
                        loads, peak, last, retire, descriptors, block, 2)
            line = lines[len(rows)]
            if ROW.fullmatch(line) is None or tuple(map(int, line.split()[1:])) != expected:
                raise ValueError(f"source model lifetime or reuse changed: {slot}/{selector}/{generation}")
            rows.append({
                "selector": selector, "generation": generation, "first_update_models": list(first),
                "later_model_loads": [{"visit": i, "record": 54} for i in range(2, 6)] if selector == 8 else [],
                "total_model_loads": loads, "peak_lane_5_descriptors": peak, "last_lane_5_visit": last,
                "scene_budget_return_visit": retire-2 if selector == 1 else None,
                "phase_2_visit": retire-2, "phase_3_visit": retire-1, "retirement_visit": retire,
                "pre_retirement_refusal_visits": [retire-2, retire-1], "returned_descriptors": descriptors,
                "returned_type3_heap_block_bytes": block or None,
            })
    return rows


def verify_source(image):
    source.verify_source(image)
    beam.verify_text(image, TEXT)
    for pc, target in ((0x80088F58, 0x80006D80), (0x80006DA0, 0x8024FD74), (0x80006DB0, 0x80197AD8),
                       (0x8001AAF8, 0x8001A950)):
        beam.link(image, pc, target)


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("contact visual source lifetime output already exists")
    dol, fixture, module = (Path(p).resolve(strict=True) for p in (dol, fixture, module))
    verify_source(DolImage(read_bounded(dol, 8 << 20), SOURCE_SHA256))
    prior = read_metadata(MAPS / "gg4e08-contact-effect-source-art.json", ART_SHA256)
    slots = []
    for slot, raw in source.source_blocks(afs):
        digest = sha256(raw).hexdigest()
        parent = prior["slots"][slot-1]
        if (parent["pzz_slot"], parent["archive_sha256"]) != (slot, digest):
            raise ValueError("source model lifetime archive does not match the first-update receipt")
        proc = run([str(fixture), str(dol), str(module), "--source-lifetime"], input=raw, capture_output=True, timeout=45)
        if proc.returncode or proc.stderr:
            raise ValueError(f"contact visual source lifetime fixture failed: {proc.stderr.decode('utf-8', 'replace').strip()}")
        slots.append({"pzz_slot": slot, "archive_sha256": digest, "cases": decode(proc.stdout.decode("ascii"), slot)})
    data = {
        "kind": "gf-retail-contact-effect-source-lifetime", "revision": 1,
        "source_dol_sha256": SOURCE_SHA256, "source_afs_sha256": AFS_SHA256,
        "contact_effect_source_art_sha256": ART_SHA256,
        "source_entries": {"bucket_visit": "80089A94", "phase_3_release": "800197A0",
                           "class2_release": "80088E50", "joint_descriptor_release": "80006D80",
                           "joint_release": "8024FD74", "visual_payload_destructor": "80006F1C"},
        "slots": slots,
        "notes": [
            "The original archive is published once per two consecutive visual generations.",
            "Each first visit binds the real model data. Thereafter the declared parent is inactive and moved, but its backing is retained.",
            "With only one class-2 cell the original helper refuses another request in phases 2 and 3 before retirement, even after the selector-1 scene debit is returned.",
            "Retirement detaches model pointers, returns the class-2 and descriptor cells, and frees the separate type-3 payload where present.",
            "The next original constructor reuses the returned cell from a different declared parent with a new origin. The selector-1 lane flips from record 2 to 3.",
            "The original archive owner and once-relocated bytes remain unchanged across both generations. This does not establish archive teardown.",
            "Only bounded updates and draw-list preparation run. Video submission, live battle scheduling, and native gameplay are not qualified.",
        ],
    }
    raw = encode(data, expanded=("slots",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    rows = [r for s in slots for r in s["cases"]]
    return {"sha256": sha256(raw).hexdigest(), "lifetimes": len(rows),
            "bucket_visits": sum(r["retirement_visit"] for r in rows),
            "model_loads": sum(r["total_model_loads"] for r in rows)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
