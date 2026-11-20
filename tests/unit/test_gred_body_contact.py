import hashlib
import struct
import unittest
from unittest.mock import patch

from tests.unit.test_stage_box_contact import box_trace
from tools.analyze.gred_body_contact import _body_descriptors, _segment_box_distance_squared, audit


def normal_fixture():
    bank = bytearray(7840)
    bank[:32] = bytes([0, 1] + [255] * 30)
    struct.pack_into(">BBH", bank, 32, 2, 3, 0x406)
    struct.pack_into(">hh", bank, 32 + 6, 0, -1)
    struct.pack_into(">f", bank, 32 + 36, 1)
    struct.pack_into(">BBH", bank, 112, 0, 128, 0x10)
    struct.pack_into(">hh", bank, 112 + 6, 0, -1)
    slots = struct.pack(">6I", 0, 0, 1, 0, 2, len(bank))
    bundle = struct.pack(">4I", 0x47464E42, 1, 3, len(slots) + len(bank)) + slots + bank
    integrated = b"VOLUME 17 integrated 0 0 3 1030 1 -1 1 1 3 1 1\n"
    projected = b"VOLUME 17 projected 0 0 3 1030 1 -.5 1 .5 -.5 1 1.5\n"
    return bank, bundle, integrated, projected


class GRedBodyContactTests(unittest.TestCase):
    def test_segment_to_box_checks_interior_minimum(self):
        low, high = (0, 0, 0), (2, 2, 2)
        self.assertEqual(_segment_box_distance_squared((-2, 1, 1), (4, 1, 1), low, high), 0)
        self.assertEqual(_segment_box_distance_squared((-2, 3, 1), (4, 3, 1), low, high), 1)
        self.assertAlmostEqual(_segment_box_distance_squared((-2, .5, 0), (.5, -2, 0), low, high), 1.125)
        self.assertEqual(_segment_box_distance_squared((-1, 1, 1), (-1, 1, 1), low, high), 1)

    def test_body_table_refuses_invalid_or_duplicate_ids(self):
        bank, _, _, _ = normal_fixture()
        self.assertEqual(list(_body_descriptors(bank)), [0, 1])
        bank[1] = 0
        with self.assertRaisesRegex(ValueError, "repeated"):
            _body_descriptors(bank)

    def test_projected_capsule_touches_even_while_its_centerline_misses(self):
        raw, receipt, _ = box_trace()
        _, bundle, a, b = normal_fixture()
        with patch("tools.analyze.gred_body_contact.NORMAL_GRED_SHA256", hashlib.sha256(bundle).hexdigest()):
            report = audit(raw, receipt, bundle, a + b, 3, 19)
        self.assertEqual(report["ordinary_body_count"], 2)
        self.assertEqual(report["possible_by_descriptor_flag"], {"3B7C": [0], "477C": [], "177C": []})
        self.assertEqual(report["box"]["flags"], 0x806)
        approach, projected = report["poses"]
        self.assertEqual(approach["root"], "integrated")
        self.assertEqual(approach["overlaps"][0]["centerline_distance"], 0)
        self.assertEqual(projected["root"], "projected")
        self.assertFalse(projected["lab_step_accepted"])
        self.assertEqual(projected["overlaps"][0]["centerline_distance"], .5)
        self.assertIn("not a live solid response", report["scope"])

    def test_unqualified_bundle_and_incomplete_or_undocumented_pose_refuse(self):
        raw, receipt, _ = box_trace()
        _, bundle, a, b = normal_fixture()
        with self.assertRaisesRegex(ValueError, "qualified normal"):
            audit(raw, receipt, bundle, a + b, 3, 19)
        with patch("tools.analyze.gred_body_contact.NORMAL_GRED_SHA256", hashlib.sha256(bundle).hexdigest()):
            with self.assertRaisesRegex(ValueError, "specific frozen stage box"):
                audit(raw, receipt, bundle, a + b, 4, 19)
            for raw_pose, why in (
                (a, "incomplete"),
                (a + b.replace(b"1030 1", b"1030 2"), "does not match"),
                (a + b + b, "duplicate"),
                (a + b.replace(b"projected 0", b"projected 1"), "inconsistent"),
            ):
                with self.subTest(why=why), self.assertRaisesRegex(ValueError, why):
                    audit(raw, receipt, bundle, raw_pose, 3, 19)


if __name__ == "__main__":
    unittest.main()
