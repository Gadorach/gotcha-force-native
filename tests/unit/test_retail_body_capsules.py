"""Separate ordinary capsule endpoints, core radius and the stored broad field."""
from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_body_capsules as capsule


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailBodyCapsuleTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-body-capsules.json").read_bytes())

    def test_pinned_roster_and_dispatch(self):
        prior = (MAPS / "gg4e08-body-geometry.json").read_bytes()
        self.assertEqual(self.report["body_geometry_sha256"], sha256(prior).hexdigest())
        self.assertEqual((self.report["shape_kind"], self.report["initializer"],
                          self.report["update"], self.report["squared_field_store"]),
                         (2, "8003B170", "8003B304", "8003BD5C"))
        self.assertEqual(len(self.report["models"]), 206)
        self.assertEqual(sum(bool(row["capsule_queue_ordinals"]) for row in self.report["models"]), 173)
        self.assertEqual(sum(len(row["capsule_queue_ordinals"]) for row in self.report["models"]), 726)
        self.assertEqual(self.report["broad_reference_ulp_error_counts_nondegenerate"],
                         {"0": 5314, "1": 102, "2": 327, "3": 57})

    def test_linear_constructor_and_squared_unchanged_update_are_not_interchangeable(self):
        loc = (0, 0, 0, 0, 4, 0, 6)
        center, low, high = (256, 128, -64), (256, 124, -64), (256, 132, -64)
        geometry = (*center, *center, *low, *high, *low, *high)
        seed = [capsule.geometry.bits(v) for v in (*geometry, 6, 10)]
        settled = [capsule.geometry.bits(v) for v in (*geometry, 6, 100)]
        self.assertEqual(capsule.read_capsule(seed, loc, 0, 0), 0)
        self.assertEqual(capsule.read_capsule(settled, loc, 0, 1), 0)
        with self.assertRaisesRegex(ValueError, "broad field disagrees"):
            capsule.read_capsule(seed, loc, 0, 1)
        scaled = [capsule.geometry.bits(v) for v in (*geometry, 12, 400)]
        self.assertEqual(capsule.read_capsule(scaled, loc, 1, 0), 0)

    def test_matrix_transforms_endpoints_but_update_uses_local_extent(self):
        loc = (4, 0, 0, 0, 4, 0, 6)
        current, old = (264, 128, -64), (260, 128, -64)
        low, high = (264, 126, -64), (264, 130, -64)
        prev_low, prev_high = (260, 124, -64), (260, 132, -64)
        shape = [capsule.geometry.bits(v) for v in (*current, *old, *low, *high, *prev_low, *prev_high, 6, 196)]
        self.assertEqual(capsule.read_capsule(shape, loc, 2, 0), 0)
        with self.assertRaisesRegex(ValueError, "broad field disagrees"):
            capsule.read_capsule(shape[:-1]+[capsule.geometry.bits(144)], loc, 2, 0)

    def test_original_degenerate_descriptor_remains_published(self):
        self.assertEqual(self.report["degenerate_capsules"], [
            {"borg": "gg4e08:0506", "queue_ordinal": 14, "descriptor": 6, "both_receiver_lists": True},
        ])
        geometry = [capsule.geometry.bits(v) for v in (256, 128, -64)*6]
        self.assertIsNone(capsule.read_capsule(geometry+["00000000", "00000000"], (0, 0, 0, 0, 0, 0, 11), 0, 1))

    def test_refuses_report_truncation_and_output_overwrite(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            capsule.decode("", b"", [])
        with TemporaryDirectory() as tmp:
            output = Path(tmp) / "capsules.json"
            output.write_text("keep")
            with self.assertRaises(FileExistsError):
                capsule.export(None, None, None, None, output)
            self.assertEqual(output.read_text(), "keep")
