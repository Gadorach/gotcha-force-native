"""Distinguish sampled geometry from a synchronized moving-pair test."""
from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_moving_receiver as motion


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailMovingReceiverTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-moving-receiver.json").read_bytes())
        cls.rows = {(r["receiver_shape"], r["case"]): r for r in cls.report["cases"]}

    def test_receipt_scope(self):
        prior = MAPS / "gg4e08-sphere-sweep.json"
        self.assertEqual(self.report["sphere_sweep_sha256"], sha256(prior.read_bytes()).hexdigest())
        self.assertEqual(self.report["declared_pre_narrow_owner_offset"], [0, 0, 0])
        samples = [s for row in self.rows.values() for s in row["samples"]]
        self.assertEqual((len(self.rows), len(samples), sum(s["broad_accepted"] for s in samples),
                          sum(s["accepted_token"] is not None for s in samples)), (10, 30, 15, 4))
        self.assertTrue(all(s["kernel_received_current_geometry"] for s in samples if s["broad_accepted"]))

    def test_moving_receiver_does_not_gain_an_attack_sweep(self):
        for shape, bound in (("sphere", 262), ("capsule", 68644)):
            row = self.rows[shape, "receiver-crosses-stationary-attack"]
            middle = row["samples"][1]
            self.assertEqual(row["linear_simultaneous_reference_min_center_separation"], 0)
            self.assertEqual((middle["receiver_center_x"], middle["receiver_previous_center_x"],
                              middle["receiver_broad_field"]), (320, 80, bound))
            self.assertFalse(middle["receiver_motion_sweep_latch"])
            self.assertTrue(middle["broad_accepted"])
            self.assertFalse(middle["narrow_accepted"])
            if shape == "capsule":
                self.assertEqual(middle["receiver_endpoints_x"], [305, 335])
                self.assertEqual(middle["receiver_previous_endpoints_x"], [65, 95])

    def test_simultaneous_crossing_can_miss_and_parallel_motion_can_hit(self):
        for shape in ("sphere", "capsule"):
            missed = self.rows[shape, "opposite-crossing-ends-outside-sweep"]
            hit = self.rows[shape, "parallel-final-pose-in-sweep"]
            self.assertEqual((missed["linear_simultaneous_reference_min_center_separation"],
                              hit["linear_simultaneous_reference_min_center_separation"]), (0, 120))
            self.assertTrue(all(r["samples"][1]["broad_accepted"] for r in (missed, hit)))
            self.assertEqual((missed["samples"][1]["hp_removed"], hit["samples"][1]["hp_removed"]), (0, 17))
            self.assertFalse(any(r["samples"][n]["hp_removed"] for r in (missed, hit) for n in (0, 2)))
            self.assertEqual(hit["samples"][1]["receiver_center_x"], hit["samples"][1]["attack_center_x"])

    def test_refuses_bad_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            motion.decode("")
        with self.assertRaisesRegex(ValueError, "changed: 0/receiver-crosses"):
            motion.decode("moving-receiver: invalid\n"*30)
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "pair.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                motion.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
