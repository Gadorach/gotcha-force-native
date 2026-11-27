"""Keep normal receiver capacity, damage sampling and independent expiry distinct."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_damage_tier_clock as tier


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-damage-tier-clock.json"


class RetailDamageTierClockTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())
        cls.rows = {(r["caster"], r["case"]): r for r in cls.data["cases"]}

    def test_all_normal_records_initialize_their_own_gauge(self):
        self.assertEqual(self.data["capacity_counts"], {"500": 198, "700": 7, "30000": 1})
        self.assertEqual(len(self.data["normal_receivers"]), 206)
        self.assertTrue(all(r["capacity"] == r["initial_gauge"] for r in self.data["normal_receivers"]))

    def test_old_tier_is_sampled_first_and_meter_overflow_is_discarded(self):
        for caster in ("gg4e08:0106", "gg4e08:0107"):
            r = self.rows[caster, "discard-overflow"]
            self.assertEqual((r["before_hp_writer"]["tier"], r["signed_hp_request"]), (2, 8))
            self.assertEqual((r["after_pair"]["meter"], r["after_pair"]["tier"]), (0, 3))
            self.assertEqual(self.rows[caster, "tier-capped"]["after_pair"]["tier"], 63)

    def test_gauge_reset_and_hp_hold_do_not_cancel_charge(self):
        for caster in ("gg4e08:0106", "gg4e08:0107"):
            for cause in ("wear-depletes-gauge", "scene-holds-hp"):
                r = self.rows[caster, cause]
                self.assertEqual(r["after_pair"], {"gauge": 500, "meter": 50, "tier": 0, "flags": 166})
            r = self.rows[caster, "scene-holds-hp"]
            self.assertEqual((r["signed_hp_request"], r["hp_removed"]), (14, 0))
            self.assertEqual(self.rows[caster, "wear-inhibited"]["after_pair"]["gauge"], 100)

    def test_expiry_uses_raw_lane_factor_not_the_contact_hold(self):
        controls = self.data["clock_controls"]
        self.assertEqual((controls["skip_actor_flag"], controls["source_step_field"]), ("01000000", "1DCC"))
        self.assertEqual((controls["declared_step"], controls["refreshed_secondary_while_held"], controls["expiry_visit"]),
                         (0.5, 0.03, 120))
        self.assertTrue(all(r["independent_expiry_checked"] for r in self.rows.values()))

    def test_refuses_malformed_report_and_existing_output(self):
        with self.assertRaises(ValueError):
            tier.decode_contacts("tier-contact: 6")
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "tier.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                tier.export(None, None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
