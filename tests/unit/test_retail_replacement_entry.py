"""Keep entry transitions, input requests, and receiving publication separate."""
from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_replacement_entry as entry


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailReplacementEntryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads((MAPS / "gg4e08-replacement-entry.json").read_bytes())
        cls.rows = {(r["area_key"], r["initial_scene_state"]): r for r in cls.data["cases"]}

    def test_early_state_four_separates_mask_from_guard_and_pad(self):
        self.assertEqual(self.data["replacement_sha256"],
                         sha256((MAPS / "gg4e08-body-replacement.json").read_bytes()).hexdigest())
        for key in (19, 20):
            r = self.rows[key, 4]
            self.assertEqual((r["role_mask_opens"], r["first_processed_pad_read"],
                              r["first_ordinary_receiver_publication"]), (81, 82, 139))
            self.assertEqual(r["only_receiving_guard_zeroed"], {"visit": 81, "before": 0, "after": 9})
            samples = {s["visit"]: s for s in r["timeline"]}
            self.assertEqual((samples[80]["role_mask"], samples[80]["receiving_guard"]), ("00C3", 59))
            self.assertEqual((samples[81]["role_mask"], samples[81]["receiving_guard"]), ("0000", 58))
            self.assertEqual((samples[138]["receiving_guard"], samples[139]["ordinary_receiving_shapes"]), (1, 9))

    def test_first_fresh_b_pays_only_when_the_packet_is_consumed(self):
        for key in (19, 20):
            pulses = self.rows[key, 4]["b_press_forks"]
            paid = [p for p in pulses if p["type41_request"]]
            self.assertEqual(len(paid), 1)
            self.assertEqual((paid[0]["visit"], paid[0]["channel_0_stock_after"]), (82, 9))
            self.assertEqual(paid[0]["type41_request"], {"key": 2, "pose": 10, "factory_entry_cut": True})
            blocked, = [p for p in pulses if p["scene_packet_suppressed"]]
            self.assertTrue(blocked["processed_pad_reader_called"])
            self.assertFalse(blocked["b_pressed"])
            self.assertEqual(blocked["channel_0_stock_after"], 10)

    def test_late_scene_change_publishes_before_input_without_a_guard_seed(self):
        for key in (19, 20):
            row = self.rows[key, 5]
            self.assertEqual(row["scene_state_change"], {"before_visit": 161, "to": 4})
            self.assertEqual((row["role_mask_opens"], row["first_ordinary_receiver_publication"],
                              row["first_fresh_b_paid_request"]), (161, 161, 162))
            samples = {s["visit"]: s for s in row["timeline"]}
            self.assertEqual((samples[160]["actor_subphase"], samples[160]["ordinary_receiving_shapes"]), (1, 0))
            self.assertEqual((samples[161]["receiving_guard"], samples[161]["ordinary_receiving_shapes"]), (0, 9))
            self.assertFalse(samples[161]["processed_pad_reader_called"])

    def test_refuses_bad_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "changed at row"):
            entry.decode("replacement-entry: invalid\n"*70)
        with TemporaryDirectory() as name:
            out = Path(name) / "entry.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                entry.export(None, None, None, None, out)
            self.assertEqual(out.read_text(), "keep")
