"""Retain the type-60 size queue, effect independence and expiry distinctions."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type60_scale as scale


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type60-scale.json"


class RetailType60ScaleTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())
        cls.rows = {(r["borg"], r["case"]): r for r in cls.data["cases"]}

    def test_size_does_not_depend_on_effect_allocation_or_team_in_tested_rows(self):
        for model, delta, target in ((0x902, 1, "3FB33333"), (0x909, 2, "3FE66666"),
                                     (0x907, -1, "3F4F5C29"), (0x90C, -2, "3F333333")):
            for name, fx in (("opponent", 33), ("opponent-no-effect-cell", None), ("ally", 33)):
                row = self.rows[f"gg4e08:{model:04x}", name]
                self.assertEqual(row["effect_type_allocated"], fx)
                self.assertEqual((row["queued_signed_delta"], row["timer_after_scene"], row["receiver_hp_after"]),
                                 (delta, 900, 180))
                self.assertEqual(row["float_bits"]["target_after_scene"], target)

    def test_pair_exclusion_is_not_a_failed_type33_allocation(self):
        blocked = self.data["normal_size_mask_exclusions"]
        self.assertEqual(len(blocked), 17)
        self.assertEqual(sum(self.data["normal_spec_mask_counts"].values()), 206)
        self.assertTrue(all(int(r["mask"], 16) & 12 == 12 for r in blocked))
        for route in scale.cast.ROUTES:
            row = self.rows[f"gg4e08:{route.model:04x}", "receiver-mask-refused"]
            self.assertEqual((row["receiver"], row["effect_type_allocated"], row["queued_signed_delta"]),
                             ("gg4e08:0602", 15, 0))
            self.assertFalse(row["scene_size_consumer_called"])

    def test_clamp_and_delayed_return_are_not_an_instant_size_change(self):
        capped = self.rows["gg4e08:0909", "at-upper-size-bound"]
        self.assertEqual((capped["raw_size_index_after"], capped["clamped_size_index_after"]), (33, 31))
        self.assertEqual(capped["timer_after_scene"], 900)
        row = self.rows["gg4e08:0902", "opponent"]["float_bits"]
        self.assertEqual((row["applied_before_and_after_scene"], row["applied_on_first_tick"]),
                         ("3F800000", "3F866666"))
        self.assertEqual((row["applied_before_expiry"], row["applied_on_expiry_tick"], row["applied_after_return"]),
                         ("3FB33333", "3FACCCCD", "3F800000"))

    def test_refuses_malformed_report_and_existing_output(self):
        with self.assertRaises(ValueError):
            scale.decode("type60-scale: 1", {}, 0.05)
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "scale.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                scale.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
