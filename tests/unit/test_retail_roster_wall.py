from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_roster_wall as wall


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailRosterWallTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.receipt = json.loads((MAPS / "gg4e08-roster-wall.json").read_bytes())
        cls.rows = {r["borg"]: r for r in cls.receipt["models"]}

    def test_census_scope_and_source_receipts(self):
        for name in ("body_registry", "st42_wall"):
            prior = (MAPS / f"gg4e08-{name.replace('_', '-')}.json").read_bytes()
            self.assertEqual(self.receipt[f"{name}_sha256"], sha256(prior).hexdigest())
        summary = self.receipt["summary"]
        self.assertEqual(summary, wall.totals(list(self.rows.values())))
        self.assertEqual([summary[k] for k in ("models", "probes", "role08_published", "kind0_solver_visits_in_overlap",
                                                "kind1_published_not_dispatched", "models_with_overlap_correction")],
                         [206, 1442, 392, 369, 23, 205])
        self.assertEqual(summary["overlap_correction_range_x"], [20, 950])
        kind1_only = self.rows["gg4e08:0e04"]
        self.assertEqual(kind1_only["published_kind1_stage_ordinals"], [16, 15, 3, 2, 1])
        self.assertEqual(kind1_only["observations"]["overlap"], [[], 0, False, 0])

    def test_two_visits_share_one_running_correction(self):
        summary = self.receipt["summary"]
        self.assertEqual(summary["models_with_two_increases"],
                         ["gg4e08:0504", "gg4e08:0505", "gg4e08:0511", "gg4e08:0620", "gg4e08:0628"])
        row = self.rows["gg4e08:0504"]
        self.assertEqual(row["observations"]["overlap"], [[[5, 0], [2, 570]], 610, True, 10])
        self.assertEqual(row["observations"]["empty-secondary"], [[[5, 0], [2, 0]], 0, False, 0])
        self.assertEqual(row["observations"]["role08-masked"], [[], 0, False, 0])

    def test_height_reference_filters_published_spheres_before_solver(self):
        summary = self.receipt["summary"]
        self.assertEqual(summary["sphere_visits_excluded_at_low_reference_zero"], 37)
        self.assertEqual(summary["models_with_reduced_correction_at_low_reference_zero"],
                         ["gg4e08:0d06", "gg4e08:0e01", "gg4e08:0e02"])
        for row in self.rows.values():
            r = row["observations"]
            self.assertEqual(r["low-reference-reset"], r["overlap"])
        r = self.rows["gg4e08:0e01"]["observations"]
        self.assertEqual(r["low-reference-zero"], [[], 0, False, 0])
        self.assertEqual(r["low-reference-reset"], [[[10, 0], [0, 270]], 270, True, 11])

    def test_exact_tangency_and_one_step_out_for_sphere_bearing_models(self):
        for row in self.rows.values():
            tangent, outside = [row["observations"][tag] for tag in ("tangent", "one-step-out")]
            self.assertEqual(tangent[1:], [0, bool(row["sphere_fields_ordinal_local_x_local_y_radius"]), 0])
            self.assertEqual(outside[1:], [0, False, 0])
            self.assertEqual(tangent[0], outside[0])

    def test_refuses_bad_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            wall.decode("", [(0, 0, [], [], [], 0)])
        with self.assertRaisesRegex(ValueError, "0000/overlap"):
            wall.decode("roster-wall: invalid\n"*7, [(0, 0, [], [], [], 0)])
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "roster-wall.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                wall.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
