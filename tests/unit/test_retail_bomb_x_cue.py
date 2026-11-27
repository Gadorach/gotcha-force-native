"""Keep the original VM cue, stock guard, and allocation separate."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_bomb_x_cue as cue


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-bomb-x-cue.json"
REPORT = "".join(f"""\
bomb-x-cue: {model} {choice} 0 {start} {first} {end} {age} {age} 2 1 1 1 0 300
bomb-x-cue: {model} {choice} 1 {start} {first} {end} {age} {age} 2 1 0 0 0 0
bomb-x-cue: {model} {choice} 2 {start} {first} {end} {age} {age} 2 1 1 0 0 300
bomb-x-cue: {model} {choice} 3 {start} {first} {first} 24 0 1 0 0 0 1 0
""" for model in (0, 3, 10, 12) for choice, (start, first, end, age) in enumerate(
    (("80377188", "803771A0", "803771BC", 22), ("803771E0", "803771FC", "80377210", 22)) if model == 12
    else (("802BE5EE", "802BE606", "802BE62E", 21), ("802BE656", "802BE672", "802BE692", 21))))


class RetailBombXCueTest(unittest.TestCase):
    def test_installed_pilots_select_both_choices_and_keep_stock_controls_separate(self):
        data = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(data["source_dol_sha256"], cue.SOURCE_SHA256)
        for pilot, parsed, key, age in zip(data["pilots"], cue.decode_report(REPORT), (0, 1, 3, 5),
                                           (21, 21, 21, 22), strict=True):
            self.assertEqual((pilot["borg"], pilot["cases"]), (parsed["borg"], parsed["cases"]))
            self.assertEqual(pilot["source"]["factory_key"], key)
            self.assertEqual(pilot["source"]["constructor_joint"], 11)
            self.assertEqual(pilot["source"]["normal_lane_mask_seed"], 3)
            self.assertEqual(pilot["source"]["program_group"], 4)
            self.assertEqual(pilot["source"]["stock_channel"], 2)
            programs = pilot["source"]["programs"]
            self.assertNotEqual(programs[0]["program"], programs[1]["program"])
            for flag in (0, 1):
                paid, empty_stock, empty_carriers, masked = pilot["cases"][4*flag:4*(flag+1)]
                self.assertEqual(paid["program"], programs[flag]["program"])
                self.assertEqual(paid["positive_cue_first_visit"], age)
                self.assertEqual(paid["constructor_origin"], [205, 220, 345])
                self.assertEqual((empty_stock["debit_attempts"], empty_stock["factory_calls"]), (1, 0))
                self.assertEqual((empty_carriers["debit_attempts"], empty_carriers["factory_calls"]), (1, 1))
                self.assertFalse(empty_carriers["carrier_created"])
                self.assertEqual((empty_carriers["stock_after"], empty_carriers["refill_clock"]), (0, 300))
                self.assertIsNone(masked["positive_cue_first_visit"])
                self.assertEqual((masked["phase_after"], masked["phase_one_visits"], masked["stock_after"]), (1, 24, 1))
                self.assertEqual(masked["cursor_after_request"], masked["cursor_at_observation_end"])

    def test_refuses_early_cues_wrong_choices_and_an_unpaid_factory(self):
        for report in (
            REPORT*2,
            REPORT.replace("21 21 2", "20 20 2", 1),
            REPORT.replace("803771FC 80377210", "803771A0 803771BC", 1),
            REPORT.replace("21 21 2 1 0 0 0 0", "21 21 2 1 1 0 0 0", 1),
            REPORT.replace("24 0 1 0 0 0 1 0", "24 24 2 1 1 1 0 300", 1),
        ):
            with self.subTest(report=report), self.assertRaises(ValueError):
                cue.decode_report(report)

    def test_existing_receipt_is_never_overwritten(self):
        with TemporaryDirectory() as temp:
            out = Path(temp) / "receipt.json"
            out.write_text("unchanged")
            with self.assertRaises(FileExistsError):
                cue.export(None, None, None, out)
            self.assertEqual(out.read_text(), "unchanged")
