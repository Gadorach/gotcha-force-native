"""Keep the type-89 parent hit distinct from the key-16 area transaction."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type89_followup as followup


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type89-followup.json"


class RetailType89FollowupTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())

    def test_spare_shape_is_required_at_first_bind(self):
        self.assertEqual(len(self.data["cases"]), 12)
        self.assertEqual(self.data["area_source"]["group"], 110)
        for row in self.data["cases"]:
            area = row["area"]
            self.assertEqual(area["bind_attempt_bucket_visit"], 33)
            self.assertEqual(area["initial_registry_count"], 0)
            self.assertEqual(row["parent_return_bucket_visit"], 44)
            if row["case"] == "no-spare-shape":
                self.assertFalse(area["bound"])
                self.assertIsNone(area["token"])
                self.assertIsNone(area["hit"])
                self.assertEqual(area["hp_loss"], 0)
                self.assertEqual(area["carrier_and_descriptors_return_bucket_visit"], 35)
                self.assertEqual(area["receiver_hp_after"], row["direct"]["receiver_hp_after"])
            else:
                self.assertTrue(area["bound"])
                self.assertEqual(area["token"], 2)
                self.assertEqual(area["hit"]["common_amount_record"], "802C4988")
                self.assertEqual(area["hp_loss"], 17)
                self.assertEqual(area["receiver_hp_after"], row["direct"]["receiver_hp_after"]-17)
                self.assertEqual(area["peak_radius"], 387.5)
                self.assertEqual(area["shape_unlink_bucket_visit"], 50)
                self.assertEqual(area["carrier_and_descriptors_return_bucket_visit"], 83)

    def test_displaced_body_is_hit_only_after_further_growth(self):
        hits = {r["case"]: r["area"]["hit"] for r in self.data["cases"] if r["area"]["bound"]}
        self.assertEqual((hits["original-receiver-pose"]["bucket_visit"], hits["original-receiver-pose"]["radius"]),
                         (34, 34.1))
        self.assertEqual((hits["receiver-offset-100"]["bucket_visit"], hits["receiver-offset-100"]["radius"]),
                         (36, 96.1))
        self.assertTrue(all(hit["same_token_refused_on_repeat"] for hit in hits.values()))

    def test_report_refuses_reusing_parent_token_or_admitting_after_refusal(self):
        def line(row):
            area = row["area"]
            hit = area["hit"] or {}
            counts = (int(row["borg"][-4:], 16), followup.CASES.index(row["case"]),
                      row["direct"]["hp_loss"], area["hp_loss"], area["receiver_hp_after"], area["token"] or 0,
                      hit.get("bucket_visit", 0), area["shape_unlink_bucket_visit"] or 0,
                      area["carrier_and_descriptors_return_bucket_visit"], row["parent_return_bucket_visit"])
            radii = (area["initial_bound_radius"] or 0, hit.get("radius", 0), area["peak_radius"] or 0)
            return "type89-followup: " + " ".join(map(str, counts)) + " " + " ".join(f"{x:.4f}" for x in radii)

        report = "\n".join(map(line, self.data["cases"]))
        self.assertEqual(followup.decode_report(report), self.data["cases"])
        for old, new in (("146 2 34 50 83 44", "146 1 34 50 83 44"),
                         ("0 163 0 0 0 35 44", "17 146 2 34 50 83 44"),
                         ("146 2 36 50 83 44", "146 2 34 50 83 44")):
            self.assertIn(old, report)
            with self.assertRaises(ValueError):
                followup.decode_report(report.replace(old, new, 1))

    def test_existing_receipt_is_not_overwritten(self):
        with TemporaryDirectory() as tmp:
            out = Path(tmp) / "receipt.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                followup.export(None, None, None, None, out)
            self.assertEqual(out.read_text(), "keep")
