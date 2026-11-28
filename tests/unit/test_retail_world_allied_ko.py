"""Separate received HP, damage credit, opponent KO credit and Force results."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_world_allied_ko as allied


RECEIPT = Path(__file__).resolve().parents[2]/"docs/api-map/gg4e08-world-allied-ko.json"


class WorldAlliedKoTest(unittest.TestCase):
    def test_force_exhaustion_is_not_opponent_ko_credit(self):
        _, _, _, enemy_out, ally_out = json.loads(RECEIPT.read_bytes())["joined_world_cases"]
        enemy = enemy_out["completed_visits"][0]
        ally = ally_out["completed_visits"][0]
        self.assertEqual((enemy["winning_team"], enemy["opponent_ko_credit_cost"], enemy["force_after"]), (0, 90, [90, 0]))
        self.assertEqual((ally["winning_team"], ally["opponent_ko_credit_cost"], ally["force_after"]), (1, 0, [0, 90]))
        self.assertEqual((ally["ally_credit"], ally["own_team_ko_observer_cost"], ally["victim_ko_cost"]), (3, 90, 90))
        self.assertEqual((ally["hp_by_seat"][0], ally["credited_ko_notification_count"]), (60, 0))
        self.assertEqual(ally["exhaustion_pair"], [0, 2])
        self.assertEqual(enemy["hp_writes"][1]["live_force_before"], [90, 0])
        self.assertEqual(enemy["hp_writes"][1]["force_mirrors"], [90, 90])
        self.assertEqual((ally["hp_writes"][1]["requested"], ally["hp_writes"][1]["removed"]), (3, 1))

    def test_refreshed_hp_not_only_the_current_hp_field_selects_amount(self):
        amount = json.loads(RECEIPT.read_bytes())["amount_probe"]
        self.assertEqual(amount["ally_multiplier_before_minimum_and_truncation"], .25)
        self.assertEqual(amount["bypass_amount_rule_bit"], "1000")
        self.assertEqual(amount["refreshed_hp_bands"][0]["ally"], 3)
        self.assertEqual(amount["refreshed_hp_bands"][0]["ally_with_0x1000_override"], 14)
        self.assertEqual((amount["late_hp_edit"]["current_hp"], amount["late_hp_edit"]["ally"]), (1, 4))

    def test_incomplete_report_and_existing_output_refuse(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            allied.decode("ally-ko-open: 174 26 3 90\n")
        with TemporaryDirectory() as tmp:
            output = Path(tmp)/"existing.json"
            output.write_text("keep")
            with self.assertRaises(FileExistsError):
                allied.export(None, None, None, None, None, output)
            self.assertEqual(output.read_text(), "keep")
