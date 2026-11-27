"""Keep type-41 stock, area admission and exact recipient damage separate."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type41_area_hp as area


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type41-area-hp.json"


class RetailType41AreaHPTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())
        cls.rows = {(r["borg"], r["case"]): r for r in cls.data["cases"]}

    def test_distinct_common_amounts_and_ledger_ownership(self):
        for model, key, record in (("0106", 19, "802C49D0"), ("0107", 20, "802C49E8")):
            borg = f"gg4e08:{model}"
            near, ally = self.rows[borg, "near"], self.rows[borg, "ally-near"]
            self.assertEqual((near["area_key"], near["common_amount_record"]), (key, record))
            self.assertEqual(near["ledgers"], {"owner_opponent": 17, "owner_ally": 0, "receiver": 17})
            self.assertEqual(ally["ledgers"], {"owner_opponent": 0, "owner_ally": 4, "receiver": 4})
            self.assertEqual((near["receiver_hp_after"], ally["receiver_hp_after"]), (163, 176))

    def test_exact_tangent_is_inclusive(self):
        for model in ("0106", "0107"):
            borg = f"gg4e08:{model}"
            tangent, miss = self.rows[borg, "exact-tangent"], self.rows[borg, "first-integer-miss"]
            self.assertEqual((tangent["target_axis_separation"], tangent["first_hit_visit"], tangent["hp_lost"]),
                             (409.5, 14, 17))
            self.assertEqual(miss["target_axis_separation"], 410)
            self.assertIsNone(miss["first_hit_visit"])

    def test_return_and_retirement_are_not_damage_admission(self):
        for model in ("0106", "0107"):
            borg = f"gg4e08:{model}"
            near = self.rows[borg, "near"]
            self.assertEqual((near["owner_stock_return_visit"], near["first_hit_visit"], near["parent_carrier_return_visit"]),
                             (2, 2, 3))
            self.assertEqual(near["accepted_token"], 2)
            for name in ("shape-refused", "receiver-mask-refused"):
                row = self.rows[borg, name]
                self.assertEqual((row["owner_stock_return_visit"], row["hp_lost"]), (2, 0))
            self.assertIsNone(self.rows[borg, "shape-refused"]["bound_token"])
            self.assertEqual(self.rows[borg, "receiver-mask-refused"]["bound_token"], 2)
        self.assertEqual(self.data["time_key19_type33_comparisons"], ["shape-refused", "near", "middle"])

    def test_refuses_malformed_report_and_existing_output(self):
        with self.assertRaises(ValueError):
            area.decode("type41-area-hp: 0")
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "area.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                area.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
