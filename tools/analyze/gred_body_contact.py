"""Compare normal G-Red's staged body volumes with a frozen stage box.

Bone endpoints are supplied by an offline pose trace, not live actor receipts.
Descriptor flags bound eligibility; current registry membership is not inferred.
"""

import argparse
from dataclasses import dataclass
import hashlib
import json
import math
from pathlib import Path
import struct

from tools.analyze.stage_box_contact import MAX_TRACE, _boxes, _read, _triples
from tools.extract.native_battle_mvp import blocks, read


NORMAL_GRED_SHA256 = "3c0640ea1d27978f4acc6aa0328127f2e63b9efa17d14bebc14e0bb998a786e5"


@dataclass(frozen=True)
class BodyDescriptor:
    kind: int
    bone: int
    flags: int
    radius: float | None


def _body_descriptors(bank):
    if len(bank) != 7840:
        raise ValueError("exact normal collision slot extent required")
    result = {}
    for record in bank[:32]:
        if record >= 128:
            break
        if record >= 32 or record in result:
            raise ValueError("invalid or repeated ordinary body index")
        at = 32 + 80 * record
        kind, bone, flags = struct.unpack_from(">BBH", bank, at)
        if kind >= 6 or struct.unpack_from(">hh", bank, at + 6) != (0, -1):
            raise ValueError("unreviewed ordinary body descriptor")
        radius = None
        if flags & 4:
            if kind not in (0, 2) or bone & 0x80:
                raise ValueError("only bone-bound sphere and capsule solids are supported")
            radius, = struct.unpack_from(">f", bank, at + (24 if kind == 0 else 36))
            if not math.isfinite(radius) or not 0 < radius <= 1e4:
                raise ValueError("unsupported body radius")
        result[record] = BodyDescriptor(kind, bone, flags, radius)
    if not result:
        raise ValueError("empty ordinary body table")
    return result


def _axis_bounds(box):
    low, high = [None] * 3, [None] * 3
    for n, p in zip(box.normals, box.anchors):
        axes = [i for i, x in enumerate(n) if x]
        if len(axes) != 1 or abs(n[axes[0]]) != 1:
            raise ValueError("body comparison requires an axis-aligned captured box")
        i = axes[0]
        (low if n[i] < 0 else high)[i] = p[i]
    if any(a is None or b is None or a >= b for a, b in zip(low, high)):
        raise ValueError("invalid box extent")
    return tuple(low), tuple(high)


def _segment_box_distance_squared(start, end, low, high):
    # Squared distance is quadratic between coordinate crossings of the six
    # faces. Evaluate each interval's minimum, not just the capsule endpoints.
    delta = tuple(b - a for a, b in zip(start, end))
    cuts = {0., 1.}
    for i, dx in enumerate(delta):
        if dx:
            for face in (low[i], high[i]):
                t = (face - start[i]) / dx
                if 0 < t < 1:
                    cuts.add(t)

    def distance(t):
        p = tuple(a + t * d for a, d in zip(start, delta))
        return sum(max(lo - x, x - hi, 0.) ** 2 for x, lo, hi in zip(p, low, high))

    ordered = sorted(cuts)
    best = min(map(distance, ordered))
    for first, last in zip(ordered, ordered[1:]):
        mid = (first + last) * .5
        square = linear = 0.
        for a, d, lo, hi in zip(start, delta, low, high):
            x = a + mid * d
            side = lo if x < lo else hi if x > hi else None
            if side is not None:
                square += d * d
                linear += d * (a - side)
        t = min(last, max(first, -linear / square)) if square else mid
        best = min(best, distance(t))
    return best


def _samples(raw, bodies):
    if len(raw) > MAX_TRACE:
        raise ValueError("pose trace exceeds byte budget")
    groups = {}
    for line in raw.decode("ascii").splitlines():
        w = line.split()
        if not w or w[0] != "VOLUME":
            continue
        if len(w) != 14:
            raise ValueError("invalid VOLUME row")
        tick, accepted, record, bone, flags = map(int, (w[1], *w[3:7]))
        variant, radius = w[2], float(w[7])
        descriptor = bodies.get(record)
        if (not 0 <= tick < 1 << 32 or variant not in ("integrated", "projected") or accepted not in (0, 1)
                or descriptor is None or descriptor.radius is None
                or (bone, flags, radius) != (descriptor.bone, descriptor.flags, descriptor.radius)):
            raise ValueError("pose sample does not match an ordinary solid body descriptor")
        a, b = _triples(tuple(map(float, w[8:14])))
        if descriptor.kind == 0 and a != b:
            raise ValueError("sphere sample cannot have distinct endpoints")
        group = groups.setdefault((tick, variant), {})
        if record in group or len(groups) > 512:
            raise ValueError("duplicate or excessive pose samples")
        group[record] = (accepted, a, b)
    expected = {record for record, body in bodies.items() if body.radius is not None}
    if not groups or not expected:
        raise ValueError("no ordinary solid poses supplied")
    for (tick, variant), group in groups.items():
        opposite = "projected" if variant == "integrated" else "integrated"
        other = groups.get((tick, opposite))
        if (set(group) != expected or not other
                or len({v[0] for v in group.values()} | {v[0] for v in other.values()}) != 1):
            raise ValueError("incomplete or inconsistent paired pose samples")
    return groups


def audit(raw_trace, receipt, bundle, raw_poses, stage_owner, descriptor):
    if len(bundle) > 32 << 20 or hashlib.sha256(bundle).hexdigest() != NORMAL_GRED_SHA256:
        raise ValueError("qualified normal G-Red battle bundle required")
    parts = blocks(bundle)
    if len(parts) < 3:
        raise ValueError("missing ordinary collision slot")
    bodies = _body_descriptors(parts[2])
    boxes, _ = _boxes(raw_trace, receipt)
    selected = [box for box in boxes if (box.parent, box.record) == (stage_owner, descriptor)]
    if len(selected) != 1:
        raise ValueError("specific frozen stage box owner and descriptor required")
    box, = selected
    bounds = _axis_bounds(box)
    steps = []
    for (tick, place), group in sorted(_samples(raw_poses, bodies).items()):
        overlaps = []
        for record, (_, a, b) in sorted(group.items()):
            radius = bodies[record].radius
            distance = _segment_box_distance_squared(a, b, *bounds)
            if distance <= radius * radius:
                overlaps.append({"body_record": record, "bone": bodies[record].bone, "radius": radius,
                                 "centerline_distance": math.sqrt(distance)})
        steps.append({"tick": tick, "root": place, "lab_step_accepted": bool(next(iter(group.values()))[0]),
                      "overlaps": overlaps})
    return {
        "scope": "staged renderer-rig geometry and frozen stage box; not a live solid response",
        "ordinary_body_count": len(bodies),
        "possible_by_descriptor_flag": {name: [r for r, b in bodies.items() if b.flags & bit]
                                        for name, bit in (("3B7C", 4), ("477C", 1), ("177C", 0x80))},
        "box": {"stage_owner": stage_owner, "descriptor": descriptor, "flags": box.flags, "registries": box.registries},
        "poses": steps,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("collider_trace", "comparison_receipt", "normal_gred_bundle", "posed_volumes"):
        parser.add_argument(name, type=Path)
    parser.add_argument("--stage-owner", type=int, required=True)
    parser.add_argument("--descriptor", type=int, required=True)
    args = parser.parse_args()
    try:
        result = audit(_read(args.collider_trace), json.loads(_read(args.comparison_receipt)),
                       read(args.normal_gred_bundle, 32 << 20), _read(args.posed_volumes),
                       args.stage_owner, args.descriptor)
        print(json.dumps(result, indent=2))
    except (OSError, ValueError) as e:
        parser.error(str(e))


if __name__ == "__main__":
    main()
