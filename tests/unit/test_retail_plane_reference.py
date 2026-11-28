from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_plane_reference as plane


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailPlaneReferenceTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-plane-reference.json").read_bytes())
        cls.rows = {r["case"]: r for r in cls.report["cases"]}

    def test_source_reference_and_original_body_records(self):
        prior = (MAPS / "gg4e08-wall-reference.json").read_bytes()
        self.assertEqual(self.report["wall_reference_sha256"], sha256(prior).hexdigest())
        self.assertEqual([self.report[k] for k in ("role10_sphere_descriptor", "role20_plane_descriptor",
                                                   "role08_wall_sphere_descriptor", "ordinary_capsule_descriptor")],
                         [0, 3, 2, 11])
        self.assertEqual(len(self.rows), 8)

    def test_later_broad_miss_refreshes_reference_without_becoming_support(self):
        near, after, before = [self.rows[tag] for tag in ("near-only", "near-then-far", "far-then-near")]
        self.assertEqual([r["support_root_y"] for r in (near, after, before)], [124]*3)
        self.assertEqual([r["support_correction_y_bits"] for r in (near, after, before)], ["C07FFFFF"]*3)
        self.assertEqual([r["wall_reference_after_plane_pass"] for r in (near, after, before)], [123, 119, 123])
        self.assertEqual([r["wall_correction_x"] for r in (near, after, before)], [0, 20, 0])
        self.assertEqual(after["ordered_foreign_checks"][-1],
                         {"plane": "far", "cell": 4, "reference_before_broad": 119, "broad_accepted": False})
        self.assertEqual((after["narrow_support_cell"], before["narrow_support_cell"]), (3, 4))

    def test_equal_role08_bottom_passes_and_one_pose_step_below_does_not(self):
        exact, below = [self.rows[tag] for tag in ("refreshed-equality", "refreshed-one-step-below")]
        self.assertEqual([r["wall_reference_after_plane_pass"] for r in (exact, below)], [119, 119])
        self.assertEqual([r["raw_role08_bottom_bits"] for r in (exact, below)], ["42EE0000", "42EDFFFE"])
        self.assertEqual([r["role08_solver_reached"] for r in (exact, below)], [True, False])
        self.assertEqual([r["capsule_broad_center_xy"] for r in (exact, below)], [[150, 124], [130, 124]])

    def test_same_owner_plane_publication_is_not_foreign_support(self):
        same = self.rows["self-plane"]
        self.assertEqual(same["published_planes"], ["self"])
        self.assertEqual(same["ordered_foreign_checks"], [])
        self.assertIsNone(same["narrow_support_cell"])
        self.assertEqual(same["wall_reference_after_plane_pass"], self.rows["no-plane"]["wall_reference_after_plane_pass"])
        self.assertTrue(all(r["allocated_capsule_endpoints_unchanged"] and r["receiver_hp_after"] == 180
                            for r in self.rows.values()))

    def test_refuses_bad_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            plane.decode("")
        with self.assertRaisesRegex(ValueError, "no-plane"):
            plane.decode("plane-reference: invalid\n"*16)
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "plane.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                plane.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
