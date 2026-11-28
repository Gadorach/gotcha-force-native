"""Verify original normal receiver geometry under declared matrix and scale changes."""
import argparse
from collections import Counter
from hashlib import sha256
import json
import math
from pathlib import Path
import struct

from tools.analyze import retail_body_registry as registry


REGISTRY_SHA256 = "63682c1f85334bcdb5565a69191676fba916c7071f95230d340f77e220efac92"
TEXT = (
    (0x8003A4B8, 300, "b3238a9a5df814d1f999a4007e720d90c7c090d74a263d9542e138a21044aea2"),
    (0x8003A5E4, 792, "c061830cbfaee70e83dbbb502a66c10e4d12c4ba053f28695885896a2df7c0c3"),
    (0x8020B7A8, 84, "0e86b49668bd9172bfd1256aa2842a4326032efab48cb6b8df58523a63992e78"),
    (0x8020BB14, 68, "492e3d053ee6336c226ad94d8947492f358b1941ca2292029efea8a6d4d382bb"),
    (0x8020BC88, 40, "a5dc9dd558d92f78fdebc97036e9c4e9b250a0f57eed1b449a8515f13c029e74"),
)
TRANSLATION = (256, 128, -64)
VIEWS = (
    ("rigid", (1, 1, 1), 1, "all"),
    ("actor-scale-2", (1, 1, 1), 2, "all"),
    ("nonuniform", (2, .5, 4), 1, "all"),
    ("reflect-x", (-2, .5, 4), 1, "all"),
    ("zero-x", (0, 1, 1), 1, "all"),
    ("below-y", (1, .005, 1), 1, "all"),
    ("threshold-z", (1, 1, .01), 1, "all"),
    ("above-z", (1, 1, .02), 1, "all"),
    ("zero-root-z", (1, 1, 0), 1, "root"),
    ("zero-joint-z", (1, 1, 0), 1, "joints"),
    ("threshold-successor-z", (1, 1, struct.unpack(">f", bytes.fromhex("3C23D70B"))[0]), 1, "all"),
)


def rounded(value):
    return struct.unpack(">f", struct.pack(">f", value))[0]


def bits(value):
    return struct.pack(">f", value).hex().upper()


def center(local, diagonal):
    return tuple(rounded(t + a*x) for t, a, x in zip(TRANSLATION, diagonal, local, strict=True))


def receivers(queue, view):
    if view in (4, 5, 6):
        return 0
    return sum(1 << n for n, shape in enumerate(queue) if shape.roles & 2
               and (view != 8 or shape.matrix != 128) and (view != 9 or shape.matrix == 128))


def read_sphere(fields, local, view, visit):
    current = center(local[:3], VIEWS[view][1])
    previous = center(local[:3], VIEWS[view-1 if view and not visit else view][1])
    tight = rounded(local[3]*VIEWS[view][2])
    expect = [bits(n) for n in (*current, *previous, tight)]
    if fields[:7] != expect:
        raise ValueError("normal body sphere center, history, or core radius changed")
    broad_bits = fields[7]
    if len(broad_bits) != 8 or any(c not in "0123456789ABCDEF" for c in broad_bits):
        raise ValueError("normal body sphere radius report encoding")
    broad = int(broad_bits, 16)
    expected = int(bits(tight + math.dist(current, previous)), 16)
    error = abs(broad-expected)
    # The original paired-single distance and square-root path can round
    # differently from the independent double-precision reference.
    if error > int(current != previous):
        raise ValueError("normal body sphere broad radius disagrees with displacement")
    return error, broad > int(expect[-1], 16)


def decode(report, payload, sources):
    rows = report.splitlines()
    if len(report) > 850_000 or len(rows) != 8426 or len(sources) != 206:
        raise ValueError("normal body geometry report extent")
    block_bytes = registry.reaction_emit.HIT_BYTES
    if len(payload) != (block_bytes+2)*len(sources):
        raise ValueError("normal body geometry source extent")
    pos = 0
    models, counts, ulps, widened = [], Counter(), Counter(), Counter()
    for j, (model, digest, queue) in enumerate(sources):
        start = j*(block_bytes+2)
        bank = payload[start+2:start+2+block_bytes]
        if int.from_bytes(payload[start:start+2], "big") != model or sha256(bank).hexdigest() != digest:
            raise ValueError("normal body geometry source bank changed")
        spheres = {}
        for n, shape in enumerate(queue):
            if shape.kind == 0 and shape.roles & 2:
                local = struct.unpack_from(">4f", bank, 32+80*shape.descriptor+12)
                if not all(map(math.isfinite, local)) or local[3] <= 0 or shape.roles & 0x10:
                    raise ValueError("normal body sphere operand outside qualified scope")
                spheres[n] = local
        digest = sha256()
        observed = {}
        for v, (name, *_) in enumerate(VIEWS):
            mask = receivers(queue, v)
            expected = f"geometry-registry: {model:04X} {v} {mask:08X} {mask:08X}"
            if rows[pos] != expected:
                raise ValueError(f"normal body geometry admission changed at {model:04X}/{name}")
            pos += 1
            observed[name] = mask.bit_count()
            counts[name] += mask.bit_count()
            for visit in range(2 if v < 4 else 0):
                for n, local in spheres.items():
                    fields = rows[pos].split()
                    if len(fields) != 13 or fields[:5] != ["geometry-sphere:", f"{model:04X}", str(v), str(visit), str(n)]:
                        raise ValueError(f"normal body geometry sphere identity changed at {model:04X}/{name}/{n}")
                    error, broad = read_sphere(fields[5:], local, v, visit)
                    ulps[error] += 1
                    if broad:
                        widened[name, visit] += 1
                    digest.update((rows[pos]+"\n").encode("ascii"))
                    pos += 1
        models.append({
            "borg": f"gg4e08:{model:04x}", "receiver_counts_both_lists_individually": observed,
            "sphere_queue_ordinals": list(spheres), "sphere_observations_sha256": digest.hexdigest(),
        })
    if pos != len(rows):
        raise ValueError("extra normal body geometry rows")
    return models, dict(counts), dict(ulps), {f"{n}/pass-{v}": total for (n, v), total in widened.items()}


def verify_source(image):
    registry.verify_source(image)
    registry.verify_text(image, TEXT)
    for site, target in ((0x8003A4E0, 0x8020B7A8), (0x8003A634, 0x8020B7A8), (0x8003A82C, 0x8020BC88),
                         (0x8008AA74, 0x8020BB14), (0x8008AAB8, 0x8020BB14), (0x8008AAFC, 0x8020BB14)):
        registry.link(image, site, target)
    if image.read(0x80437C44, 4, "data") != bytes.fromhex("3C23D70A"):
        raise ValueError("normal body geometry basis threshold changed")


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("normal body geometry output already exists")
    dol = Path(dol).resolve(strict=True)
    image = registry.DolImage(registry.read_bounded(dol, 8 << 20), registry.SOURCE_SHA256)
    verify_source(image)
    registry.read_metadata(registry.MAPS / "gg4e08-body-registry.json", REGISTRY_SHA256)
    installed = registry.read_metadata(registry.MAPS / "gg4e08-standalone-installers.json", registry.INSTALLERS_SHA256)
    payload, sources = registry.source_inputs(afs, installed)
    report = registry.fixture_report(fixture, dol, module, ("--geometry",), payload)
    rows, counts, ulps, widened = decode(report, payload, sources)
    raw = registry.encode({
        "kind": "gf-retail-body-geometry", "revision": 1, "source_dol_sha256": registry.SOURCE_SHA256,
        "source_afs_sha256": registry.AFS_SHA256, "body_registry_sha256": REGISTRY_SHA256,
        "seat": 0, "translation": TRANSLATION, "basis_column_threshold_bits": "3C23D70A",
        "declared_views": {name: {"matrix_diagonal_bits": [bits(n) for n in axes], "actor_scale": scale,
                                   "changed_matrices": selector} for name, axes, scale, selector in VIEWS},
        "both_receiver_list_totals_individually": counts, "sphere_reference_radius_ulp_error_counts": ulps,
        "sphere_broad_radius_expansion_counts": widened, "models": rows,
        "notes": [
            "The original full body queue is bound once per model. Matrix and scale operands are then changed on the same cells; no source descriptor or role flag is overwritten.",
            "The sphere center and previous center are observed separately from core +0x88 and broad-phase +0x7C radii. Geometry views 0 to 3 get an extra unchanged rebuild, so motion widening is not confused with a radius scale.",
            "The sphere fields match the qualified matrix and actor-scale operands. The separate reference uses Euclidean displacement plus core radius; transitional results may round one binary32 step differently. Per-Borg digests pin the observed original bits.",
            "Near-zero column tests cover receiver publication for all source receiver kinds; non-sphere geometry and pair acceptance are not evaluated here.",
            "No original animated pose, actual collision sweep, stage, hardware, or native gameplay is qualified.",
        ],
    }, expanded=("models",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "models": len(rows),
            "registry_rebuilds": len(rows)*(len(VIEWS)+4), "sphere_observations": sum(ulps.values())}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
