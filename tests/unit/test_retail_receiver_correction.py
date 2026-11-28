"""Protect the separate original root, broad center and scratch endpoint writes."""
from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_receiver_correction as correction


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailReceiverCorrectionTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-receiver-correction.json").read_bytes())
        cls.rows = {(r["case"], r["body_center_pass_run"]): r for r in cls.report["cases"]}

    def test_scope_and_source_roles(self):
        prior = MAPS / "gg4e08-moving-receiver.json"
        self.assertEqual(self.report["moving_receiver_sha256"], sha256(prior.read_bytes()).hexdigest())
        self.assertEqual((self.report["original_correction_writer"], self.report["receiver_center_pass"],
                          self.report["center_bucket"], self.report["capsule_scratch_preparation"]),
                         ("80035CA4", "8002CE90", 50, "8003A260"))
        self.assertEqual((len(self.rows), sum(r["hp_removed"] > 0 for r in self.rows.values())), (12, 5))

    def test_capsule_endpoints_shift_in_scratch_without_shifting_the_published_body(self):
        inward = self.rows["capsule-inward", False]
        self.assertEqual((inward["root_y"], inward["broad_center_y"], inward["owner_correction_y"]), (226, 228, -2))
        self.assertEqual((inward["allocated_capsule_endpoint_y"], inward["scratch_capsule_endpoint_y"]), (228, 226))
        self.assertEqual((inward["broad_phase_accepted"], inward["hp_removed"]), (True, 17))
        outward = self.rows["capsule-outward", True]
        self.assertEqual((outward["allocated_capsule_endpoint_y"], outward["scratch_capsule_endpoint_y"]), (226, 228))
        self.assertTrue(outward["broad_phase_accepted"])
        self.assertFalse(outward["narrow_accepted"])
        self.assertEqual(outward["hp_removed"], 0)

    def test_two_writer_visits_accumulate_but_center_pass_is_needed_for_broad_admission(self):
        stale, updated = [self.rows["capsule-inward-past-broad-gate", flag] for flag in (False, True)]
        self.assertEqual(stale["declared_writer_y_operands"], [-10, -14])
        self.assertEqual((stale["root_y"], updated["root_y"], updated["owner_correction_y"]), (226, 226, -24))
        self.assertEqual((stale["broad_center_y"], updated["broad_center_y"]), (250, 226))
        self.assertEqual((stale["allocated_capsule_endpoint_y"], updated["allocated_capsule_endpoint_y"]), (250, 250))
        self.assertIsNone(stale["scratch_capsule_endpoint_y"])
        self.assertEqual((updated["scratch_capsule_endpoint_y"], updated["hp_removed"]), (226, 17))

    def test_sphere_narrow_preparation_does_not_add_capsule_endpoint_correction(self):
        for name, before, after in (("sphere-outward", 17, 0), ("sphere-inward", 0, 17)):
            stale, updated = [self.rows[name, flag] for flag in (False, True)]
            self.assertEqual((stale["hp_removed"], updated["hp_removed"]), (before, after))
            self.assertIsNone(stale["scratch_capsule_endpoint_y"])
            self.assertEqual(stale["scratch_center_y"], stale["broad_center_y"] if before else None)

    def test_refuses_bad_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            correction.decode("")
        with self.assertRaisesRegex(ValueError, "changed: capsule-outside-control"):
            correction.decode("correction-pair: invalid\n"*12)
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "correction.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                correction.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
