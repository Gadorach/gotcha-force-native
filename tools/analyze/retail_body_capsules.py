"""Verify original normal receiver capsule endpoints and the stored broad bound."""
import argparse
from collections import Counter
from hashlib import sha256
import json
import math
from pathlib import Path
import struct

from tools.analyze import retail_body_geometry as geometry


registry = geometry.registry
GEOMETRY_SHA256 = "04a655f265bbe976713e3afbc267f985267dbcfbdffbc9fdd6e494b8f701c8e4"
VIEWS = geometry.VIEWS[:4]
TEXT = (
    (0x8003B170, 404, "d860895a26a36c0288e9ba750edace11d62e5c7ca3a79cb307d1772eaefecfbd"),
    (0x8003B304, 2692, "06a06236e9e610b581fa099742287697a9b2dbae47168ef0a3f5ac1e13f44edc"),
)


def read_capsule(fields, local, view, visit):
    point, extent, radius = local[:3], local[3:6], local[6]
    axes, scale = VIEWS[view][1:3]
    old_axes = VIEWS[view-1 if view and not visit else view][1]
    low = [geometry.rounded(a-b) for a, b in zip(point, extent, strict=True)]
    high = [geometry.rounded(a+b) for a, b in zip(point, extent, strict=True)]
    center = geometry.center(point, axes)
    previous = geometry.center(point, old_axes)
    endpoints = (geometry.center(low, axes), geometry.center(high, axes))
    old_endpoints = (geometry.center(low, old_axes), geometry.center(high, old_axes))
    expect = [geometry.bits(n) for triple in (center, previous, *endpoints, *old_endpoints) for n in triple]
    if fields[:18] != expect:
        raise ValueError("normal body capsule center or endpoint history changed")
    if endpoints[0] == endpoints[1]:
        if fields[18:] != ["00000000", "00000000"]:
            raise ValueError("normal body degenerate capsule did not retain zero bounds")
        return None
    core = geometry.rounded(radius*scale)
    if fields[18] != geometry.bits(core):
        raise ValueError("normal body capsule core radius changed")
    raw_length = geometry.rounded(math.hypot(*extent))
    scaled_length = geometry.rounded(raw_length*scale)
    simple = geometry.rounded(scaled_length+core)
    travel = geometry.rounded(math.dist(center, previous))
    if view or visit:
        envelope = geometry.rounded(travel+simple)
        expected = geometry.rounded(envelope*envelope)
    else:
        expected = simple
    broad_bits = fields[19]
    if len(broad_bits) != 8 or any(c not in "0123456789ABCDEF" for c in broad_bits):
        raise ValueError("normal body capsule bound report encoding")
    error = abs(int(broad_bits, 16)-int(geometry.bits(expected), 16))
    # The original paired-single vector length and update order can round
    # differently from the independent distance reference before squaring.
    if error > 3:
        raise ValueError("normal body capsule broad field disagrees with initializer or update")
    return error


def decode(report, payload, sources):
    lines = report.splitlines()
    if len(report) > 1_400_000 or len(lines) != 6632 or len(sources) != 206:
        raise ValueError("normal body capsule report extent")
    block_bytes = registry.reaction_emit.HIT_BYTES
    if len(payload) != (2+block_bytes)*len(sources):
        raise ValueError("normal body capsule source extent")
    pos = 0
    models, ulps, degenerate = [], Counter(), []
    for j, (model, digest, queue) in enumerate(sources):
        start = j*(2+block_bytes)
        bank = payload[start+2:start+2+block_bytes]
        if int.from_bytes(payload[start:start+2], "big") != model or sha256(bank).hexdigest() != digest:
            raise ValueError("normal body capsule source bank changed")
        capsules = {}
        for i, shape in enumerate(queue):
            if shape.kind != 2 or not shape.roles & 2:
                continue
            local = struct.unpack_from(">7f", bank, 32+80*shape.descriptor+12)
            if shape.roles & 0x81 or not all(map(math.isfinite, local)) or local[6] <= 0:
                raise ValueError("normal body capsule operand outside qualified scope")
            capsules[i] = local
        numeric = sha256()
        for view, (name, *_) in enumerate(VIEWS):
            mask = geometry.receivers(queue, view)
            if lines[pos] != f"geometry-registry: {model:04X} {view} {mask:08X} {mask:08X}":
                raise ValueError(f"normal body capsule admission changed at {model:04X}/{name}")
            pos += 1
            for visit in (0, 1):
                for n, local in capsules.items():
                    fields = lines[pos].split()
                    if len(fields) != 26 or fields[:6] != ["geometry-capsule:", f"{model:04X}", str(view), str(visit), str(n), "2"]:
                        raise ValueError(f"normal body capsule identity changed at {model:04X}/{name}/{n}")
                    error = read_capsule(fields[6:], local, view, visit)
                    if error is not None:
                        ulps[error] += 1
                    elif not view and not visit:
                        degenerate.append({"borg": f"gg4e08:{model:04x}", "queue_ordinal": n,
                                           "descriptor": queue[n].descriptor, "both_receiver_lists": True})
                    numeric.update((lines[pos]+"\n").encode("ascii"))
                    pos += 1
        models.append({
            "borg": f"gg4e08:{model:04x}", "capsule_queue_ordinals": list(capsules),
            "capsule_observations_sha256": numeric.hexdigest(),
        })
    if pos != len(lines):
        raise ValueError("extra normal body capsule rows")
    return models, dict(ulps), degenerate


def verify_source(image):
    geometry.verify_source(image)
    registry.verify_text(image, TEXT)
    for site, target in ((0x8003B198, 0x8020B7A8), (0x8003B2D0, 0x8020BB14), (0x8003BC50, 0x8020B7A8),
                         (0x8003BC70, 0x8020B7A8), (0x8003BD14, 0x8020BB14), (0x8003BD34, 0x8020BB14)):
        registry.link(image, site, target)
    if (image.read(0x802DA748, 4, "data") != bytes.fromhex("8003B304")
            or image.read(0x802DA760, 4, "data") != bytes.fromhex("8003B170")
            or image.read(0x8003BD50, 16, "text") != bytes.fromhex("D01C007CC01C007CEC000032D01C007C")):
        raise ValueError("normal body capsule dispatch or squared field write changed")


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("normal body capsule output already exists")
    dol = Path(dol).resolve(strict=True)
    image = registry.DolImage(registry.read_bounded(dol, 8 << 20), registry.SOURCE_SHA256)
    verify_source(image)
    prior = registry.read_metadata(registry.MAPS / "gg4e08-body-geometry.json", GEOMETRY_SHA256)
    installed = registry.read_metadata(registry.MAPS / "gg4e08-standalone-installers.json", registry.INSTALLERS_SHA256)
    payload, sources = registry.source_inputs(afs, installed)
    report = registry.fixture_report(fixture, dol, module, ("--capsules",), payload)
    models, ulps, zero_bounds = decode(report, payload, sources)
    raw = registry.encode({
        "kind": "gf-retail-body-capsules", "revision": 1, "source_dol_sha256": registry.SOURCE_SHA256,
        "source_afs_sha256": registry.AFS_SHA256, "body_geometry_sha256": GEOMETRY_SHA256,
        "seat": 0, "translation": prior["translation"], "declared_views": {name: prior["declared_views"][name] for name, *_ in VIEWS},
        "shape_kind": 2, "initializer": "8003B170", "update": "8003B304", "squared_field_store": "8003BD5C",
        "broad_reference_ulp_error_counts_nondegenerate": ulps, "degenerate_capsules": zero_bounds, "models": models,
        "notes": [
            "All receivers are bound in the normal body queue and both original lists remain open. Only kind-2 geometry is compared numerically.",
            "The source transforms the local center and local center plus/minus extent. Actor +0xC0 separately scales the core radius. Previous center and previous endpoints belong to the earlier callback, not a second set of source descriptors.",
            "Nondegenerate initializer +0x7C is a linear local extent length times actor scale plus core radius. Ordinary updates add center travel and store the square of the sum, including when the center did not move.",
            "A zero-extent source capsule still publishes, with identical endpoints and zero stored radii in these inlets. Reference comparisons for nondegenerate bounds allow up to three binary32 steps; per-Borg digests pin original observed bits.",
            "Declared matrices only. Special swept attack flags, the consumer meaning of broad-field units, actual pair acceptance, animation, hardware and native gameplay are not qualified.",
        ],
    }, expanded=("models",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "models": len(models),
            "registry_rebuilds": len(models)*len(VIEWS)*2, "capsule_observations": sum(ulps.values())+len(zero_bounds)*len(VIEWS)*2}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
