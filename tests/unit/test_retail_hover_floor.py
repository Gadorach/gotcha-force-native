"""The pose-offset contact receipt distinguishes static contact from recovery."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_hover_floor as hover


RECEIPT = Path(__file__).resolve().parents[2]/"docs/api-map/gg4e08-pose-offset-contact.json"


class PoseOffsetContactReceiptTest(unittest.TestCase):
    def test_pose_offset_and_ground_recovery_are_distinct(self):
        data = json.loads(RECEIPT.read_bytes())
        nurse, wing = data["source_cases"]
        self.assertEqual((nurse["borg"], wing["borg"]), ("gg4e08:0900", "gg4e08:0a00"))
        snap = nurse["floor_callback"]
        self.assertLess(snap["root_y"], snap["floor_y"])
        self.assertGreater(snap["contact_offset"], 0)
        self.assertEqual(snap["root_y_plus_contact_offset"], snap["floor_y"])
        self.assertFalse(snap["saved_support_owner"])
        for case, meter, points in ((nurse, 4000, 0), (wing, 6000, 1)):
            edges = case["edges"]
            self.assertEqual(edges["contact"]["airborne_cache"], 1)
            self.assertEqual(edges["landing-fork"]["airborne_cache"], 0)
            self.assertEqual((edges["landing-fork"]["meter"], edges["landing-fork"]["points_used"]), (meter, 1))
            self.assertEqual((edges["refill-fork"]["meter"], edges["refill-fork"]["points_used"]), (meter+2000, points))
        wing_air = wing["edges"]["in-air"]
        self.assertGreater(wing_air["root_y"], wing["floor_callback"]["floor_y"])
        self.assertEqual((wing_air["class_pose_y"], wing_air["contact_offset"], wing_air["meter"]), (24, 0, 6000))
        self.assertTrue(data["native_contrast"]["resource_forks_matched"])

    def test_unbounded_or_incomplete_reports_and_overwrite_refuse(self):
        for text in ("hover-contact: incomplete", "0 " * 3000):
            with self.assertRaisesRegex(ValueError, "report extent"):
                hover.decode(text)
        with TemporaryDirectory() as temp:
            existing = Path(temp)/"keep.json"
            existing.write_text("keep")
            with self.assertRaises(FileExistsError):
                hover.export(None, None, None, None, None, existing)
            self.assertEqual(existing.read_text(), "keep")
