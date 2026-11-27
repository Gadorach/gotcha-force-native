"""Keep applied geometry separate from target-row movement and clock values."""
from collections import Counter
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_size_consumers as size


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-size-consumers.json"


class RetailSizeConsumerTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())
        cls.rows = {(r["caster"], r["receiver"], r["stage"]): r for r in cls.data["cases"]}

    def test_normal_roster_has_two_curves_and_distinct_clamps(self):
        counts = Counter()
        for row in self.data["normal_clamp_profiles"]:
            counts[row["family"]] += row["normal_records"]
        self.assertEqual(counts, {0: 186, 1: 20})
        self.assertEqual(len(self.data["normal_clamp_profiles"]), 15)
        self.assertEqual(self.data["normal_size_exclusions_by_family"], [10, 7])

    def test_same_cast_changes_both_curves_differently(self):
        for receiver, curve, before, after, target in (("000a", 0, 72, 89, "3FE66666"),
                                                       ("0502", 1, 90, 106, "3FB33333")):
            initial = self.rows["gg4e08:0909", f"gg4e08:{receiver}", "before-contact"]
            settled = self.rows["gg4e08:0909", f"gg4e08:{receiver}", "settled-before-expiry"]
            self.assertEqual(initial["coarse_axis_pair_pass_distance"], before)
            self.assertEqual((settled["size_family"], settled["coarse_axis_pair_pass_distance"],
                              settled["coarse_axis_pair_fail_distance"]), (curve, after, after+1))
            self.assertEqual(settled["float_bits"]["applied_scale"], target)

    def test_target_displacement_and_clock_step_precede_applied_radius(self):
        prefix = "gg4e08:0909", "gg4e08:000a"
        before = self.rows[prefix+("before-contact",)]["float_bits"]
        after = self.rows[prefix+("after-scene-size-write",)]["float_bits"]
        expiry = self.rows[prefix+("expiry-tick",)]["float_bits"]
        self.assertEqual(before["published_receiver_radius"], after["published_receiver_radius"])
        self.assertEqual((after["first_primary_rate"], after["first_secondary_rate"]), ("3CF5C28F", "3CF5C28F"))
        self.assertEqual((after["primary_after_hold"], after["secondary_after_hold"], after["x_displacement_after_hold"]),
                         ("3F800000", "3F75C28F", "418BAE14"))
        self.assertNotEqual(expiry["published_receiver_radius"], before["published_receiver_radius"])
        self.assertEqual(expiry["secondary_after_hold"], before["secondary_after_hold"])
        self.assertEqual(expiry["x_displacement_after_hold"], before["x_displacement_after_hold"])

    def test_refuses_malformed_receipt_and_existing_output(self):
        with self.assertRaises(ValueError):
            size.decode("size-consumer: 0", (), 0.05)
        with TemporaryDirectory() as tmp:
            out = Path(tmp) / "size.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                size.export(None, None, None, None, out)
            self.assertEqual(out.read_text(), "keep")
