"""Preserve the request, original track sampling and local publication boundaries."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_joint_clock_sample as joint


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-joint-clock-sample.json"


def report(rows):
    return "\n".join(
        "joint-clock: " + " ".join(map(str, (i//4, r["visit"], r["body_timer_after"], r["aobj_rate_bits"],
                                             r["sample_frame_bits"], *r["euler_bits"], *r["local_quaternion_bits"])))
        for i, r in enumerate(rows)
    )


class RetailJointClockSampleTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())
        cls.rows = cls.data["samples"]

    def test_only_the_isolated_joint_is_bound_to_the_exact_three_channels(self):
        joint_row = self.data["source_joint"]
        self.assertEqual((joint_row["borg"], joint_row["logical_joint"], joint_row["joint_desc_offset"]),
                         ("gg4e08:0600", 2, 0x13A88))
        self.assertTrue(joint_row["isolated_instance"])
        self.assertEqual(joint_row["source_links"], [3, 13, 1])
        track = self.data["track"]
        self.assertEqual(track["clip_slot"], [0, 9])
        self.assertEqual([(c["channel"], c["stream_bytes"]) for c in track["channels"]], [(1, 29), (2, 45), (3, 45)])

    def test_explicit_first_sample_is_same_then_diverges(self):
        self.assertEqual(self.rows[8]["sample_frame_bits"], "40200000")
        self.assertEqual(self.rows[8]["euler_bits"], self.rows[12]["euler_bits"])
        self.assertEqual(self.rows[8]["local_quaternion_bits"], self.rows[12]["local_quaternion_bits"])
        self.assertNotEqual(self.rows[9]["euler_bits"], self.rows[13]["euler_bits"])
        self.assertNotEqual(self.rows[9]["local_quaternion_bits"], self.rows[13]["local_quaternion_bits"])

    def test_relative_first_frame_uses_rate_and_expiry_resumes_from_held_progress(self):
        self.assertEqual([r["sample_frame_bits"] for r in self.rows[:4]],
                         ["40CC0000", "40D80000", "40E40000", "40F00000"])
        slowed = self.rows[4:8]
        self.assertEqual([r["sample_frame_bits"] for r in slowed],
                         ["40C0F5C3", "40C1EB86", "40C2E149", "40CEE149"])
        self.assertEqual([r["aobj_rate_bits"] for r in slowed], ["3CF5C28F"]*3 + ["3EC00000"])
        self.assertNotEqual(self.rows[0]["euler_bits"], self.rows[4]["euler_bits"])

    def test_report_refuses_premature_frame_advance_or_nonfinite_rotation(self):
        good = report(self.rows)
        self.assertEqual(joint.decode(good), self.rows)
        for bad in (good.replace("3EC00000 40CC0000", "3EC00000 40D80000", 1),
                    good.replace("3D865012", "7FC00000", 1)):
            with self.assertRaises(ValueError):
                joint.decode(bad)

    def test_existing_receipt_is_not_replaced(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "sample.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                joint.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
