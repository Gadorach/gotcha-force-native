from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_wall_reference as reference


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailWallReferenceTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.receipt = json.loads((MAPS / "gg4e08-wall-reference.json").read_bytes())
        cls.rows = {m["borg"]: m for m in cls.receipt["models"]}

    def test_source_receipts_and_seat_coverage(self):
        for key in ("roster_wall", "body_registry"):
            prior = (MAPS / f"gg4e08-{key.replace('_', '-')}.json").read_bytes()
            self.assertEqual(self.receipt[f"{key}_sha256"], sha256(prior).hexdigest())
        summary = self.receipt["summary"]
        self.assertEqual(reference.summarize(list(self.rows.values())), summary)
        self.assertEqual((summary["models"], summary["physical_seats_each"], summary["collision_prefixes"]), (206, 6, 3708))
        self.assertEqual(summary["reference_distribution_open"], {"123": 198, "63": 1, "-10000": 7})

    def test_producer_and_consumer_mask_controls_are_independent(self):
        row = self.rows["gg4e08:0201"]
        self.assertEqual(row["role10_reference_sphere"], {
            "queue_ordinal": 0, "descriptor": 0, "matrix": 128, "local_y": 0, "core_radius": 5,
        })
        self.assertEqual(row["observations"]["open"], [123, [[1, 0]], 60, True])
        self.assertEqual(row["observations"]["role10-masked"], [-10000, [[2, 0], [1, 70]], 70, True])
        self.assertEqual(row["observations"]["role08-masked"], [123, [], 0, False])
        special = self.rows["gg4e08:0f00"]
        self.assertEqual((special["role10_reference_sphere"]["local_y"], special["observations"]["open"][0]), (-60, 63))

    def test_original_reference_can_prevent_all_wall_correction(self):
        summary = self.receipt["summary"]
        self.assertEqual(summary["role08_solver_visits_open_per_seat"], 217)
        self.assertEqual(summary["role08_solver_visits_without_role10_per_seat"], 369)
        self.assertEqual(len(summary["models_with_wall_correction_changed_by_role10"]), 46)
        self.assertEqual(summary["models_with_wall_correction_prevented_by_role10"], ["gg4e08:0d06", "gg4e08:0d07"])
        for model, correction in (("gg4e08:0d06", 190), ("gg4e08:0d07", 90)):
            r = self.rows[model]["observations"]
            self.assertEqual(r["open"], [123, [], 0, False])
            self.assertEqual(r["role10-masked"], [-10000, [[1, 0]], correction, True])

    def test_missing_producer_keeps_reset_value_independently_of_stage_kind(self):
        for model, count in (("gg4e08:0620", 2), ("gg4e08:0e04", 0)):
            row = self.rows[model]
            self.assertIsNone(row["role10_reference_sphere"])
            obs = row["observations"]
            self.assertEqual(obs["open"], obs["role10-masked"])
            self.assertEqual((obs["open"][0], len(obs["open"][1])), (-10000, count))

    def test_refuses_bad_report_and_existing_output(self):
        row = (0, 0, [], [], [], 0, None)
        with self.assertRaisesRegex(ValueError, "report extent"):
            reference.decode("", [row])
        with self.assertRaisesRegex(ValueError, "0000/open"):
            reference.decode("roster-wall-reference: invalid\n"*3, [row])
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "reference.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                reference.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
