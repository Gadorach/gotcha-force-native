import hashlib
import math
import struct
import unittest

from tools.analyze.stage_box_contact import audit


def box_trace(*, flags=0x806, normal_count=6, bad_plane=False, rotate=False, bad_registry=False):
    corners = [(x, y, z) for x in (0, 2) for y in (0, 2) for z in (0, 2)]
    normals = [(0, 1, 0), (0, 0, 1), (-1, 0, 0), (0, -1, 0), (0, 0, -1), (1, 0, 0)]
    anchors = [(2, 2, 0), (2, 2, 2), (0, 2, 0), (2, 0, 0), (0, 0, 0), (2, 0, 2)]
    if bad_plane:
        anchors[2] = (1, 2, 0)

    def point(p):
        x, y, z = p
        if not rotate:
            return p
        q = math.sqrt(.5)
        return ((x + z) * q, y, (z - x) * q)

    floats = [0.] * 13 + [x for p in corners + normals + normals + anchors for x in point(p)]
    bits = struct.unpack(">91I", struct.pack(">91f", *floats))
    lines = [
        f"BODY 0 3 3 19 1 0 {flags} 2 0 384 1 {normal_count} 1065353216 0 0 0 0",
        "FIELDS 0 " + " ".join(map(str, bits)),
        "BODY 1 3 4 20 0 0 16 2 0 16 1 0 1065353216 0 0 0 0",
        "FIELDS 1 0",
    ]
    for i in range(10):
        slots = {4: " 1", 7: " 0", 8: " 1" if bad_registry else " 0"}
        lines.append(f"REGISTRY {i}" + slots.get(i, ""))
    raw = ("\n".join(lines) + "\n").encode()
    receipt = dict(status="PASS", source_compared_collider_trace_exact=True, source_compared_collider_count=2,
                   trace_sha256=hashlib.sha256(raw).hexdigest())
    # Enter from the west, then correct out; the start-to-projected shot stays outside.
    path = "SEGMENT 5 " + " ".join(str(x) for p in ((-2, 1, 1), (1, 1, 1), (-1, 1, 1)) for x in point(p))
    return raw, receipt, path.encode()


class StageBoxContactTests(unittest.TestCase):
    def test_approach_and_correction_are_not_projected_travel(self):
        for rotate in (False, True):
            with self.subTest(rotate=rotate):
                report = audit(*box_trace(rotate=rotate))
                self.assertEqual(report["tested_stage_boxes"], 1)
                self.assertEqual(report["other_stage_shapes_not_tested"], 1)
                step, = report["steps"]
                self.assertEqual(step["tick"], 5)
                hit, = step["box_hits"]
                self.assertEqual(hit["descriptor"], 19)
                self.assertEqual(hit["registries"], ("3B7C", "417C"))
                self.assertTrue(hit["flag800_excludes_ordinary_solid_pair"])
                self.assertAlmostEqual(hit["approach"]["enter"], 2 / 3)
                self.assertEqual(hit["approach"]["entry_plane"], 2)
                self.assertAlmostEqual(hit["projection_correction"]["leave"], .5)
                self.assertEqual(hit["projection_correction"]["exit_plane"], 2)
                self.assertIsNone(hit["projected_travel"])

    def test_unflagged_shape_is_not_claimed_to_produce_a_response(self):
        step, = audit(*box_trace(flags=6))["steps"]
        hit, = step["box_hits"]
        self.assertFalse(hit["flag800_excludes_ordinary_solid_pair"])
        self.assertIsNotNone(hit["approach"])

    def test_late_intersection_at_segment_endpoint_is_observed(self):
        raw, receipt, _ = box_trace()
        step, = audit(raw, receipt, b"SEGMENT 2 -1 1 1 0 1 1 -1 1 1\n")["steps"]
        hit, = step["box_hits"]
        self.assertEqual(hit["approach"]["enter"], 1)
        self.assertEqual(hit["approach"]["leave"], 1)
        self.assertIsNone(hit["projected_travel"])

    def test_verified_trace_and_complete_boxes_are_required(self):
        raw, receipt, segments = box_trace()
        with self.assertRaisesRegex(ValueError, "does not match"):
            audit(raw + b"\n", receipt, segments)
        with self.assertRaisesRegex(ValueError, "source-compared"):
            audit(raw, dict(receipt, source_compared_collider_trace_exact=False), segments)
        for kw, reason in (({"normal_count": 5}, "incomplete"), ({"bad_registry": True}, "registry"),
                           ({"bad_plane": True}, "captured corners")):
            with self.subTest(kw=kw), self.assertRaisesRegex(ValueError, reason):
                audit(*box_trace(**kw))

    def test_malformed_duplicate_or_missing_segments_are_refused(self):
        raw, receipt, segments = box_trace()
        for sample, reason in (
            (b"SEGMENT 1 0 0\n", "expected SEGMENT"),
            (b"SEGMENT 1 nan 0 0 0 0 0 0 0 0\n", "numeric domain"),
            (segments + b"\n" + segments, "duplicate"),
            (b"not a segment\n", "no staged segments"),
        ):
            with self.subTest(sample=sample), self.assertRaisesRegex(ValueError, reason):
                audit(raw, receipt, sample)


if __name__ == "__main__":
    unittest.main()
