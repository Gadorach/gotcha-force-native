"""Verify dynamic contact visual admission, heap handoff, and refusal cleanup."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import struct
from subprocess import run

from tools.analyze import retail_type32_roster as beam, retail_wall_effect_census as wall
from tools.analyze.retail_code_atlas import read_bounded
from tools.analyze.retail_projectile_census import SOURCE_SHA256, address, encode, read_metadata
from tools.extract.source_image import DolImage


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"
TAIL_SHA256 = "c3da79a83106c076b9ce91d2a36c344455243a90b2a5c64b0dd4370454716250"
ROW = re.compile(r"impact-init:(?: [0-9]{1,5}){13}")
CASES = (
    ("empty-burst-budget",       (0, 1,  0,   0,     0,     0,  896,  896,  8, 8, 1, 16, 3)),
    ("last-burst-slot",          (1, 1, 31,   0, 14251, 42834, 1064, 1088, 10, 9, 1, 19, 3)),
    ("trim-to-16-entries",       (2, 1, 31, 496, 14251, 42834,  896,  896,  8, 8, 1, 16, 3)),
    ("trim-to-one-entry",        (3, 1, 31, 511, 14251, 42834,   56,   64,  1, 0, 1,  1, 3)),
    ("full-burst-budget",        (4, 1, 32,   0, 14251, 14251,    0,    0,  0, 0, 1,  1, 3)),
    ("full-entry-budget",        (5, 1,  0, 512, 14251, 14251,    0,    0,  0, 0, 1,  1, 3)),
    ("selector-8-empty-budget",  (6, 8,  0,   0, 14251, 14251,  256,  256,  0, 0, 8,  8, 3)),
    ("selector-8-full-budgets",   (7, 8, 32, 512, 14251, 14251,  256,  256,  0, 0, 8,  8, 3)),
)
TEXT = (
    *tuple(s for s in wall.TEXT if s[0] not in (wall.STAGE, 0x80019860, 0x80019950)),
    (0x80019A14, 552, "e46efc26ad0ab0d88bd59dc48b198fee825422c32b6f90576e116c68f56d0e30"),
    (0x8001A71C, 148, "7456fc9a449ef6b3ad83639d7adb4db4fd71f7a4ef3e8143c0d488231a842094"),
    (0x80006DC8, 116, "a9d42d4b031609acdc4056ee2c21d561240c52207e9ba085f640a656a1129282"),
    (0x800055FC, 52, "74fb9a01b91b2d60cd400a8a96d2781782ea688ddd992f4e5eb472d4b9426438"),
    (0x80197A0C, 204, "96222c5bda44dae9f8bb740618f6f8c5d08d455044410156499752c80db51a82"),
    (0x80089A94, 88, "dcb8bf7edce7313e0e258ee526af82153e5e35eac0987938a42cccc6a270dde4"),
    (0x80088E50, 328, "c747b919f60eaad0d8b255db2b1fdc9ecb0bd22cea33c4930d1085dcdf34d3f4"),
)


def decode(report):
    lines = report.splitlines()
    if len(report) > 1024 or len(lines) != len(CASES):
        raise ValueError("contact visual initialization report extent")
    results = []
    for line, (name, expected) in zip(lines, CASES, strict=True):
        if ROW.fullmatch(line) is None or tuple(map(int, line.split()[1:])) != expected:
            raise ValueError(f"contact visual admission, heap frontier, or retirement changed: {name}")
        _, selector, bursts, entries, seed, after, request, heap, first, second, descriptors, count, retired = expected
        results.append({
            "case": name, "selector": selector, "scene_bursts_before": bursts, "scene_entries_before": entries,
            "rng_before": seed, "rng_after_refusal": after, "descriptors_claimed_and_returned": descriptors,
            "visual_request_bytes": request or None, "heap_frontier_bytes_with_type3_cell": heap or None,
            "selector_1_entry_split": [first, second] if selector == 1 and request else None,
            "effect_count_byte_after_init_attempt": count, "phase_2_visit": 1, "phase_3_visit": 2,
            "class_2_and_descriptor_return_visit": retired,
        })
    return results


def verify_source(image):
    beam.verify_text(image, TEXT)
    for a, n, digest in (
        (0x802C7ED0, 36, "f39c93177c925c4874b4d0e94cf156bec0c7e7ea377fbe9ccb56dd3c086c0003"),
        (0x802C8164, 80, "66a36a1bd38071388e38c2f3d9788b013b515e9d29a7c65424284ab542bb133e"),
    ):
        if sha256(image.read(a, n, "data")).hexdigest() != digest:
            raise ValueError(f"contact visual source table changed at {address(a)}")
    for pc, target in (
        (0x80019588, 0x8001959C), (0x800195D4, 0x8008893C), (0x80019A70, 0x800055FC),
        (0x80019AF4, 0x80006DC8), (0x8001A73C, 0x80006DC8), (0x80006DE4, 0x80197A0C),
        (0x80006E04, 0x8025AFA4), (0x800197D0, 0x80088E50),
    ):
        beam.link(image, pc, target)
    descriptors = image.read(0x802C7ED0, 36, "data")
    phase = struct.unpack(">4I", image.read(0x802C8164, 16, "data"))
    entries = struct.unpack(">4I", image.read(0x802C8174, 16, "data"))
    return {
        "bucket_visit": "80089A94", "effect_update": "800196D0", "phase_table": "802C8164",
        "phase_entries": list(map(address, phase)), "family_init_table": "802C8174",
        "selector_inits": [
            {"selector": i, "family": descriptors[4*i+1], "entry": address(entries[descriptors[4*i+1]])}
            for i in (1, 8)
        ],
        "visual_object_allocator": "80006DC8", "heap_call": "80006E04", "heap_entry_cut": "8025AFA4",
    }


def export(dol, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("contact visual initialization output already exists")
    inputs = [Path(p).resolve(strict=True) for p in (dol, fixture, module)]
    image = DolImage(read_bounded(inputs[0], 8 << 20), SOURCE_SHA256)
    entries = verify_source(image)
    read_metadata(MAPS / "gg4e08-contact-effect-tail.json", TAIL_SHA256)
    dol, fixture, module = inputs
    proc = run([str(fixture), str(dol), str(module)], capture_output=True, timeout=45)
    if proc.returncode or proc.stderr:
        raise ValueError(f"contact visual init fixture failed: {proc.stderr.decode('utf-8', 'replace').strip()}")
    rows = decode(proc.stdout.decode("ascii"))
    data = {
        "kind": "gf-retail-contact-effect-init", "revision": 1, "source_dol_sha256": SOURCE_SHA256,
        "accepted_contact_tail_sha256": TAIL_SHA256, "entries": entries, "cases": rows,
        "notes": [
            "The new replay calls the original helper directly; accepted-pair selector provenance is in the pinned tail receipt.",
            "Class 2 and its battle descriptors are admitted first. The later type-3 visual object comes from a separate row.",
            "A populated type-3 row is a separate observation fork ending at the original heap entry before allocation.",
            "Full visits instead use an empty visual-object row. Phase 2 on visit 1, phase 3 on visit 2, and return on visit 3 are refusal-path observations.",
            "Refusals publish no collider, consume no shape, and do not charge the scene burst or entry counts. Selector 1 can still advance RNG before storage refusal.",
            "No heap result is supplied. Successful emitter initialization, rendering, live scheduling, and native gameplay are not qualified.",
        ],
    }
    raw = encode(data, expanded=("cases",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "cases": len(rows),
            "heap_frontiers": sum(r["heap_frontier_bytes_with_type3_cell"] is not None for r in rows)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.fixture, args.module, args.output), sort_keys=True))
