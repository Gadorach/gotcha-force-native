"""Keep original size batching, cancellation and token refusal distinct."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type60_refresh as refresh


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type60-refresh.json"


class RetailType60RefreshTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rows = {r["case"]: r for r in json.loads(ARTIFACT.read_bytes())["cases"]}

    def test_fresh_contacts_refresh_nonzero_offsets(self):
        for name, queued, offset in (("grow-twice", 1, 2), ("shrink-twice", -1, -2), ("grow-then-cross-neutral", -2, -1)):
            r = self.rows[name]
            self.assertEqual((r["final_pass_signed_delta"], r["temporary_offset_after_final_pass"]), (queued, offset))
            self.assertEqual((r["timer_before_final_pass"], r["timer_after_final_pass"]), (897, 900))
            self.assertEqual(r["receiver_history_tokens"], [1, 2])

    def test_exact_cancellation_keeps_elapsed_countdown_and_returns_toward_neutral(self):
        for name in ("grow-then-cancel", "shrink-then-cancel"):
            r = self.rows[name]
            self.assertEqual(r["temporary_offset_after_final_pass"], 0)
            self.assertEqual((r["timer_before_final_pass"], r["timer_after_final_pass"], r["timer_after_next_size_tick"]),
                             (897, 897, 896))
            bits = r["float_bits"]
            self.assertNotEqual(bits["applied_before_and_after_final_pass"], bits["applied_on_next_size_tick"])
            self.assertEqual(bits["target_after_final_pass"], "3F800000")

    def test_same_pass_uses_one_consumer_and_aggregates_signed_inputs(self):
        for name, delta, timer in (("batch-grow-and-cancel", 0, 0), ("batch-shrink-and-cancel", 0, 0),
                                   ("batch-grow-twice", 2, 900), ("batch-cross-neutral", -1, 900)):
            r = self.rows[name]
            self.assertTrue(r["same_scene_pass"])
            self.assertEqual(r["consumer_calls_in_final_scene_pass"], 1)
            self.assertEqual((r["final_pass_signed_delta"], r["timer_after_final_pass"]), (delta, timer))

    def test_retained_token_is_not_a_refresh(self):
        r = self.rows["retained-token"]
        self.assertIsNone(r["second_caster"])
        self.assertEqual((r["receiver_history_tokens"], r["consumer_calls_in_final_scene_pass"]), ([1], 0))
        self.assertEqual((r["temporary_offset_after_final_pass"], r["timer_after_final_pass"]), (1, 897))
        self.assertTrue(all(row["float_bits"]["applied_after_expiry_and_return"] == "3F800000" for row in self.rows.values()))

    def test_refuses_incomplete_report_and_existing_output(self):
        with self.assertRaises(ValueError):
            refresh.decode("type60-refresh: 0", {}, 0.05)
        with TemporaryDirectory() as tmp:
            out = Path(tmp) / "refresh.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                refresh.export(None, None, None, None, out)
            self.assertEqual(out.read_text(), "keep")
