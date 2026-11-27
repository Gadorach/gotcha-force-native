"""Keep early counter reads distinct from the later scheduled lane wait."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_contact_frame_prefix as prefix


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-contact-frame-prefix.json"


def report(rows):
    lines = []
    for r in rows:
        fields = (int(r["attacking_borg"].split(":")[1], 16), 2 if r["case"] == "accepted-hit" else 1,
                  r["visit"], r["timer_before"], r["timer_after"], r["incoming_secondary_bits"],
                  r["refreshed_secondary_bits"], r["early_float_counter_bits"], r["early_integer_counter"],
                  r["scheduled_lane_wait_bits"], int(r["scheduled_lane_complete"]))
        lines.append("frame-clock: " + " ".join(map(str, fields)))
    return "\n".join(lines)


class RetailContactFramePrefixTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())
        cls.rows = cls.data["visits"]

    def test_entry_and_expiry_use_old_rate_for_early_counter_only(self):
        rows = [r for r in self.rows if r["attacking_borg"] == "gg4e08:0807" and r["case"] == "accepted-hit"]
        first, expiry = rows[0], rows[3]
        self.assertEqual((first["incoming_secondary_bits"], first["refreshed_secondary_bits"]),
                         ("3EC00000", "3CF5C28F"))
        self.assertEqual((first["early_float_counter_bits"], first["scheduled_lane_wait_bits"]),
                         ("40680000", "3F7851EC"))
        self.assertEqual((expiry["incoming_secondary_bits"], expiry["refreshed_secondary_bits"]),
                         ("3CF5C28F", "3EC00000"))
        self.assertEqual((expiry["early_float_counter_bits"], expiry["scheduled_lane_wait_bits"]),
                         ("40623D71", "3F08F5C4"))

    def test_early_integer_counter_stays_held_on_expiry_then_resumes(self):
        for r in self.rows:
            if r["timer_before"]:
                self.assertEqual(r["early_integer_counter"], 8)
        for model, seed in (("gg4e08:0807", 4), ("gg4e08:0001", 8)):
            samples = {r["visit"]: r for r in self.rows if r["attacking_borg"] == model and r["case"] == "accepted-hit"}
            self.assertEqual(samples[seed]["timer_after"], 0)
            self.assertEqual(samples[seed+1]["early_integer_counter"], 7)
            self.assertTrue(samples[seed+2]["scheduled_lane_complete"])
        self.assertEqual(self.data["entries"]["post_callback_cut"], "8006F88C")

    def test_decoder_rejects_late_counter_using_new_rate_and_early_resume(self):
        good = report(self.rows)
        self.assertEqual(prefix.decode(good), self.rows)
        for bad in (good.replace("3EC00000 3CF5C28F 40680000", "3EC00000 3CF5C28F 407E147B", 1),
                    good.replace("40623D71 8 3F08F5C4", "40623D71 7 3F08F5C4", 1)):
            with self.assertRaises(ValueError):
                prefix.decode(bad)

    def test_output_is_not_replaced(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "prefix.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                prefix.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
