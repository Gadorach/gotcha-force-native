"""Pin the original motion threshold, consumed swept shape, and withheld history."""
from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_sphere_sweep as sweep


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailSphereSweepTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-sphere-sweep.json").read_bytes())
        cls.rows = {r["case"]: r for r in cls.report["cases"]}

    def test_receipt_scope_and_motion_threshold(self):
        prior = MAPS / "gg4e08-capsule-pair.json"
        self.assertEqual(self.report["capsule_pair_sha256"], sha256(prior.read_bytes()).hexdigest())
        self.assertEqual(self.report["motion_threshold"], .5)
        self.assertEqual((len(self.rows), sum(len(r["samples"]) for r in self.rows.values())), (14, 42))
        for tag, shape, broad in (("below", "sphere", 70.25), ("exact", "sphere", 70.5), ("above", "capsule", 70.375)):
            _, moved, settled = self.rows[f"{tag}-motion-threshold"]["samples"]
            self.assertEqual((moved["effective_shape"], moved["broad_field"]), (shape, broad))
            self.assertEqual((settled["effective_shape"], settled["broad_field"]), ("sphere", 70))
            self.assertEqual(moved["core_radius"], 70)

    def test_sweep_hits_when_both_endpoint_spheres_miss(self):
        for name, kernel, swapped in (("sphere-crossing", "80030A3C", True),
                                      ("capsule-parallel-crossing", "80031E50", False),
                                      ("capsule-perpendicular-crossing", "80031E50", False)):
            row = self.rows[name]
            start, moved, end = row["samples"]
            self.assertFalse(start["hp_writer_reached"] or end["hp_writer_reached"])
            self.assertEqual((moved["narrow_kernel"], moved["kernel_operands_swapped"]), (kernel, swapped))
            self.assertEqual((moved["hp_removed"], moved["accepted_token"], moved["broad_field"]), (17, 1, 190))
            self.assertEqual(moved["stored_edges"], [start["center"], end["center"]])
            self.assertEqual(row["shape_kept_same_attack_token"], 1)

    def test_swept_tangencies_are_not_the_previous_stationary_tests(self):
        for shape in ("sphere", "capsule"):
            tangent = self.rows[f"{shape}-side-tangent"]["samples"][1]
            outside = self.rows[f"{shape}-side-outside"]["samples"][1]
            self.assertTrue(tangent["narrow_accepted"])
            self.assertEqual(tangent["hp_removed"], 17)
            self.assertTrue(outside["broad_accepted"])
            self.assertFalse(outside["narrow_accepted"])

    def test_two_publication_gates_consume_but_do_not_replay_motion_history(self):
        crossing = self.rows["sphere-crossing"]["samples"][1]
        for tag, gate in (("role", "role-01-mask"), ("late", "late-276")):
            row = self.rows[f"crossing-{tag}-excluded"]
            self.assertEqual(row["middle_publication_gate"], gate)
            _, moved, end = row["samples"]
            self.assertTrue(moved["motion_latch"])
            self.assertEqual(moved["stored_edges"], crossing["stored_edges"])
            self.assertFalse(moved["published"])
            self.assertIsNone(moved["broad_accepted"])
            self.assertTrue(end["published"])
            self.assertEqual((end["effective_shape"], end["stored_edges"]), ("sphere", [end["center"]]*2))
            self.assertFalse(end["hp_writer_reached"])

    def test_refuses_report_truncation_and_output_overwrite(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            sweep.decode("")
        with self.assertRaisesRegex(ValueError, "changed: below-motion-threshold/0"):
            sweep.decode("sphere-motion: invalid\n"*42)
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "sphere.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                sweep.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
