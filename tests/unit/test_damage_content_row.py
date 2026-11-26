"""Pin the recovered tables to the native damage-vs content row."""
import unittest
from pathlib import Path

from tools.analyze import damage_content_row as row_tool

DOL = Path("/home/gadorach/Projects/GotchaForce/original/extracted/sys/main.dol")
SELECTORS = (Path(__file__).resolve().parents[2]
             / "docs/api-map/borg-damage-selectors.json")
ATTACKER = {"model": 0x0615, "side": 0, "settings": 0, "rank": 0}
DEFENDER = {"model": 0x000A, "side": 1, "settings": 16, "rank": 0}


class NumberFormatTest(unittest.TestCase):
    def test_one_keeps_a_decimal(self):
        self.assertEqual(row_tool._number(1.0), "1.0")
        self.assertEqual(row_tool._number(0.867), "0.867")
        self.assertEqual(row_tool._number(1.13), "1.13")
        self.assertEqual(row_tool._number(0.0), "0.0")


@unittest.skipUnless(DOL.is_file() and SELECTORS.is_file(),
                     "original DOL or selector artifact not available")
class DamageContentRowTest(unittest.TestCase):
    def test_known_gred_sasuke_row(self):
        row = row_tool.load(DOL, SELECTORS, ATTACKER, DEFENDER, 20, 1.0)
        self.assertEqual(row["amount"], 22)
        self.assertAlmostEqual(row["product"], 1.126666, places=5)
        self.assertEqual(row["content"]["line"],
                         "damage-vs=10|0.867|1.13|1.15|1.0")
        self.assertEqual(row["stages"]["credit_difficulty"], 1.02)
        self.assertEqual(row["stages"]["attacker_rank"], 0.85)
        self.assertEqual(row["stages"]["defender_class"], 1.13)
        self.assertEqual(row["stages"]["receiver_rank"], 1.15)

    def test_multiple_defender_rows(self):
        block = row_tool.load_rows(
            DOL, SELECTORS, ATTACKER,
            [DEFENDER, dict(DEFENDER, model=0x0615)], 20, 1.0)
        self.assertEqual(len(block["rows"]), 2)
        self.assertEqual([row["amount"] for row in block["rows"]], [22, 20])
        self.assertIn("damage-vs=10|", block["block"])
        self.assertIn("damage-vs=1557|", block["block"])
        self.assertNotEqual(block["rows"][0]["stages"]["defender_class"],
                            block["rows"][1]["stages"]["defender_class"])

    def test_duplicate_defender_model_refused(self):
        with self.assertRaises(ValueError):
            row_tool.load_rows(DOL, SELECTORS, ATTACKER,
                               [DEFENDER, dict(DEFENDER)], 20, 1.0)

    def test_settings_select_a_different_class_cell(self):
        low = row_tool.load(DOL, SELECTORS, ATTACKER, DEFENDER, 20, 1.0)
        changed = row_tool.load(DOL, SELECTORS, ATTACKER,
                                dict(DEFENDER, settings=0), 20, 1.0)
        self.assertNotEqual(low["stages"]["defender_class"],
                            changed["stages"]["defender_class"])


if __name__ == "__main__":
    unittest.main()
