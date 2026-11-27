"""Keep original rate consumers distinct from frame scheduling and pose sampling."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_contact_clock_consumers as consumers


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-contact-clock-consumers.json"


def report(rows):
    lines = []
    for row in rows:
        fields = (int(row["attacking_borg"].split(":")[1], 16), 2 if row["case"] == "accepted-hit" else 1,
                  row["visit"], row["timer_after"], row["primary_bits"], row["secondary_bits"],
                  row["lane_wait_bits"], int(row["lane_complete"]), row["z_after_planar_advance"], row["heading_step"],
                  row["heading_gap_after"], row["relative_clip_frame_bits"], row["explicit_clip_frame_bits"])
        lines.append("clock-consumers: " + " ".join(map(str, fields)))
    return "\n".join(lines)


class RetailContactClockConsumersTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())
        cls.rows = cls.data["visits"]

    def test_planar_and_heading_helpers_keep_advancing_at_reduced_rates(self):
        clear, armed = self.rows[0], self.rows[3]
        self.assertEqual((clear["planar_advance"], clear["heading_step"]), (100, 15))
        self.assertEqual((armed["planar_advance"], armed["heading_step"]), (3, 1))
        for r in self.rows:
            self.assertEqual(r["planar_advance"], 3 if r["timer_after"] else 100)

    def test_exact_terminal_wait_finishes_after_three_six_or_ten_visits(self):
        done = {(r["attacking_borg"], r["case"]): r["visit"] for r in self.rows if r["lane_complete"]}
        self.assertEqual(done, {("gg4e08:0807", "whiff-control"): 3, ("gg4e08:0807", "accepted-hit"): 6,
                                ("gg4e08:0001", "whiff-control"): 3, ("gg4e08:0001", "accepted-hit"): 10})
        self.assertEqual(len([r for r in self.rows if r["timer_after"]]), 10)
        self.assertEqual(self.data["direct_call_census"]["8004CD24"], {"direct_sites": 1128, "parent_functions": 1053})

    def test_relative_request_uses_secondary_rate_but_explicit_frame_does_not(self):
        for r in self.rows:
            self.assertEqual(r["relative_clip_frame_bits"], "40C0F5C3" if r["timer_after"] else "40CC0000")
            self.assertEqual(r["explicit_clip_frame_bits"], "40200000")

    def test_report_refuses_frozen_motion_or_perturbed_explicit_request(self):
        good = report(self.rows)
        self.assertEqual(consumers.decode(good), self.rows)
        for bad in (good.replace("328 1 1023", "325 1 1023", 1), good.replace("40200000", "40200001", 1)):
            with self.assertRaises(ValueError):
                consumers.decode(bad)

    def test_existing_output_is_not_replaced(self):
        with TemporaryDirectory() as tmp:
            out = Path(tmp) / "consumers.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                consumers.export(None, None, None, None, out)
            self.assertEqual(out.read_text(), "keep")
