"""Compare staged foot segments with boxes in a verified, frozen stage trace.

This is a point-path witness, not a swept Borg body or a live collision pass.
It neither reads a game executable nor writes to its inputs.
"""

import argparse
from dataclasses import asdict, dataclass
import hashlib
import json
import math
from pathlib import Path
import struct

from tools.extract.source_image import ReadOnlyFile


LISTS = ("117C", "177C", "1D7C", "237C", "297C", "2F7C", "357C", "3B7C", "417C", "477C")
MAX_TRACE = 1 << 20


@dataclass(frozen=True)
class BoxSpan:
    enter: float
    leave: float
    entry_plane: int | None
    exit_plane: int | None


@dataclass(frozen=True)
class StageBox:
    lease: int
    parent: int
    record: int
    flags: int
    registries: tuple
    normals: tuple
    anchors: tuple


def _point(values):
    values = tuple(values)
    if len(values) != 3 or any(not math.isfinite(x) or abs(x) > 1e7 for x in values):
        raise ValueError("point outside numeric domain")
    return values


def _dot(a, b):
    return math.fsum(x * y for x, y in zip(a, b))


def _distance(normal, anchor, point):
    return _dot(normal, (p - o for p, o in zip(point, anchor)))


def _clip(box, start, end):
    enter, leave = 0.0, 1.0
    entry_plane = exit_plane = None
    for i, (normal, anchor) in enumerate(zip(box.normals, box.anchors)):
        a = _distance(normal, anchor, start)
        b = _distance(normal, anchor, end)
        if a > 0 and b > 0:
            return None
        if a > 0:
            t = a / (a - b)
            if t > enter:
                enter, entry_plane = t, i
        elif b > 0:
            t = a / (a - b)
            if t < leave:
                leave, exit_plane = t, i
        if enter > leave:
            return None
    return BoxSpan(enter, leave, entry_plane, exit_plane)


def _triples(values):
    return tuple(_point(values[i:i + 3]) for i in range(0, len(values), 3))


def _shape(values):
    if len(values) != 91 or any(not 0 <= x <= 0xFFFFFFFF for x in values):
        raise ValueError("box fields do not match the verified trace layout")
    f = struct.unpack(">91f", struct.pack(">91I", *values))
    corners = _triples(f[13:37])
    normals = _triples(f[37:55])
    anchors = _triples(f[73:91])
    if len(set(corners)) != 8:
        raise ValueError("incomplete box corners")
    for i, (normal, anchor) in enumerate(zip(normals, anchors)):
        other_dots = [_dot(normal, n) for j, n in enumerate(normals) if j != i]
        if (abs(_dot(normal, normal) - 1) > .001
                or sum(x < -.999 for x in other_dots) != 1
                or any(x >= -.999 and abs(x) > .001 for x in other_dots)):
            raise ValueError("box requires six opposing, orthogonal planes")
        distances = [_distance(normal, anchor, c) for c in corners]
        if max(distances) > .01 or sum(abs(x) <= .01 for x in distances) != 4:
            raise ValueError("box planes do not bound the captured corners")
    return normals, anchors


def _boxes(raw, receipt):
    if (not isinstance(receipt, dict) or len(raw) > MAX_TRACE or receipt.get("status") != "PASS"
            or receipt.get("source_compared_collider_trace_exact") is not True):
        raise ValueError("a bounded, source-compared collider trace is required")
    if receipt.get("trace_sha256") != hashlib.sha256(raw).hexdigest():
        raise ValueError("collider trace does not match its receipt")
    rows = {tag: {} for tag in ("BODY", "FIELDS", "REGISTRY")}
    for line in raw.decode("ascii").splitlines():
        words = line.split()
        if not words or words[0] not in rows:
            continue
        tag = words[0]
        key, *values = map(int, words[1:])
        if key < 0 or key in rows[tag]:
            raise ValueError("duplicate or invalid trace row")
        rows[tag][key] = values
    bodies, fields, lists = (rows[tag] for tag in ("BODY", "FIELDS", "REGISTRY"))
    if (not 1 <= len(bodies) <= 384 or len(bodies) != receipt.get("source_compared_collider_count")
            or bodies.keys() != fields.keys() or set(lists) != set(range(len(LISTS)))):
        raise ValueError("incomplete collider trace or registry")
    if any(len(v) != len(set(v)) or any(k not in bodies for k in v) for v in lists.values()):
        raise ValueError("registry references missing or repeated collider")
    boxes = []
    nonboxes = 0
    for lease, b in bodies.items():
        if len(b) != 16:
            raise ValueError("invalid collider descriptor row")
        physical, parent, record, shape, _, flags, phase, degenerate, membership, complete, normals_written, *_ = b
        if physical != 3:
            continue
        if shape != 1:
            nonboxes += 1
            continue
        indices = tuple(i for i, entries in lists.items() if lease in entries)
        if (parent < 0 or record < 0 or not 0 <= flags <= 0xFFFF or phase != 2 or degenerate
                or complete != 1 or normals_written != 6 or membership != sum(1 << i for i in indices)):
            raise ValueError("stage box is incomplete or its registry disagrees")
        normals, anchors = _shape(fields[lease])
        boxes.append(StageBox(lease, parent, record, flags, tuple(LISTS[i] for i in sorted(indices)), normals, anchors))
    if not boxes:
        raise ValueError("no complete stage boxes in trace")
    return boxes, nonboxes


def _segments(raw):
    if len(raw) > MAX_TRACE:
        raise ValueError("segment trace exceeds byte budget")
    seen = set()
    for line in raw.decode("ascii").splitlines():
        words = line.split()
        if not words or words[0] != "SEGMENT":
            continue
        if len(words) != 11:
            raise ValueError("expected SEGMENT tick followed by start, integrated and projected XYZ")
        tick = int(words[1])
        if not 0 <= tick < 1 << 32 or tick in seen or len(seen) >= 256:
            raise ValueError("invalid, duplicate or excessive segment tick")
        seen.add(tick)
        yield tick, _triples(list(map(float, words[2:])))
    if not seen:
        raise ValueError("no staged segments supplied")


def audit(raw_trace, receipt, raw_segments):
    boxes, nonboxes = _boxes(raw_trace, receipt)
    steps = []
    for tick, (start, integrated, projected) in _segments(raw_segments):
        hits = []
        for box in boxes:
            segments = {
                "approach": _clip(box, start, integrated),
                "projection_correction": _clip(box, integrated, projected),
                "projected_travel": _clip(box, start, projected),
            }
            if any(span is not None for span in segments.values()):
                hits.append({
                    "lease": box.lease, "stage_owner": box.parent, "descriptor": box.record,
                    "flags": box.flags, "registries": box.registries,
                    "flag800_excludes_ordinary_solid_pair": bool(box.flags & 0x800 and "3B7C" in box.registries),
                    **{k: asdict(v) if v is not None else None for k, v in segments.items()},
                })
        steps.append({"tick": tick, "box_hits": hits})
    return {"scope": "frozen stage boxes, foot points only; not live solid response",
            "tested_stage_boxes": len(boxes), "other_stage_shapes_not_tested": nonboxes, "steps": steps}


def _read(path):
    with ReadOnlyFile(path) as source:
        if source.size > MAX_TRACE:
            raise ValueError(f"{path.name}: input exceeds byte budget")
        return source.read(0, source.size)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("collider_trace", type=Path)
    parser.add_argument("comparison_receipt", type=Path)
    parser.add_argument("staged_segments", type=Path)
    args = parser.parse_args()
    try:
        receipt = json.loads(_read(args.comparison_receipt))
        print(json.dumps(audit(_read(args.collider_trace), receipt, _read(args.staged_segments)), indent=2))
    except (OSError, ValueError) as e:
        parser.error(str(e))


if __name__ == "__main__":
    main()
