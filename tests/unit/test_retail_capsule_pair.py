"""Protect the scope of original capsule broad, narrow, and HP evidence."""
from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_capsule_pair as capsule


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailCapsulePairTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-capsule-pair.json").read_bytes())
        cls.rows = {(r["case"], r["bound_update"]): r for r in cls.report["cases"]}

    def test_receipt_scope_and_source_provenance(self):
        for name in ("body-capsules", "melee-pose-clock"):
            prior = MAPS / f"gg4e08-{name}.json"
            self.assertEqual(self.report[name.replace("-", "_")+"_sha256"], sha256(prior.read_bytes()).hexdigest())
        self.assertEqual((self.report["attack_radius"], self.report["positive_broad_floor_bits"],
                          self.report["sphere_capsule_kernel"]), (70, "3DCCCCCD", "80030A3C"))
        self.assertEqual((len(self.rows), sum(r["sphere_capsule_kernel_reached"] for r in self.rows.values()),
                          sum(r["hp_writer_reached"] for r in self.rows.values())), (24, 17, 8))

    def test_broad_phase_uses_the_raw_squared_field_without_accepting_a_hit(self):
        updated = "unchanged-second-rebuild"
        for tag, distance, accepted in (("updated-broad-inside", 553, True),
                                       ("updated-broad-tangent", 554, True), ("updated-broad-outside", 555, False)):
            start, end = self.rows[tag, "initializer"], self.rows[tag, updated]
            self.assertEqual((start["receiver_broad_field"], end["receiver_broad_field"]), (22, 484))
            self.assertEqual((start["receiver_core_radius"], end["receiver_core_radius"]), (7, 7))
            self.assertEqual(end["center_separation"], distance)
            self.assertFalse(start["broad_phase_accepted"])
            self.assertEqual(end["broad_phase_accepted"], accepted)
            self.assertFalse(end["hp_writer_reached"])
            self.assertIs(end["sphere_capsule_accepted"], False if accepted else None)

    def test_actual_side_and_cap_tangencies_fail_the_narrow_test(self):
        for mode in ("initializer", "unchanged-second-rebuild"):
            for suffix, tangent in (("side", 77), ("cap", 92)):
                inside = self.rows[f"{suffix}-inside", mode]
                edge = self.rows[f"{suffix}-tangent", mode]
                self.assertEqual((inside["center_separation"], edge["center_separation"]), (tangent-1, tangent))
                self.assertTrue(edge["broad_phase_accepted"])
                self.assertFalse(edge["sphere_capsule_accepted"])
                self.assertEqual((edge["hp_before"], edge["hp_after"]), (180, 180))
                self.assertEqual((inside["hp_requested"], inside["hp_removed"], inside["same_token_repeat_hp_writes"]),
                                 (17, 17, 0))

    def test_original_degenerate_entry_does_not_reach_narrow_even_at_coincident_centers(self):
        for mode in ("initializer", "unchanged-second-rebuild"):
            zero, ordinary = [self.rows[k, mode] for k in ("zero-extent", "same-model-nondegenerate")]
            self.assertEqual((zero["receiver"], ordinary["receiver"]), ("gg4e08:0506", "gg4e08:0506"))
            self.assertEqual((zero["descriptor"], ordinary["descriptor"]), (6, 2))
            self.assertEqual((zero["center_separation"], zero["receiver_broad_field"], zero["receiver_core_radius"]),
                             (0, 0, 0))
            self.assertIsNone(zero["sphere_capsule_accepted"])
            self.assertFalse(zero["hp_writer_reached"])
            self.assertEqual((ordinary["hp_removed"], ordinary["hp_after"]), (16, 1984))

    def test_refuses_malformed_report_and_overwrite(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            capsule.decode("")
        with self.assertRaisesRegex(ValueError, "changed: center/0"):
            capsule.decode("capsule-pair: invalid\n"*24)
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "pair.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                capsule.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
