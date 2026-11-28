from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_support_carry as carry


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailSupportCarryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-support-carry.json").read_bytes())
        cls.rows = {r["case"]: r for r in cls.report["cases"]}

    def test_pinned_support_and_actor_update_frontier(self):
        prior = (MAPS / "gg4e08-support-anchor.json").read_bytes()
        self.assertEqual(self.report["support_anchor_sha256"], sha256(prior).hexdigest())
        self.assertEqual((self.report["actor_update"], self.report["carry_call"], self.report["stop_before"]),
                         ("80059068", "800590A4", "80059E58"))
        self.assertTrue(all(r["original_support_owner_retained"] and r["hp_after"] == 180 for r in self.rows.values()))

    def test_owner_transform_not_plane_joint_controls_root(self):
        names = ("held", "joint-only-translation", "owner-translation", "owner-quarter-turn", "owner-translation-and-turn")
        rows = [self.rows[name] for name in names]
        self.assertEqual([[carry.stage.scalar(v) for v in r["actor_root_after_bits"]] for r in rows],
                         [[130, 124, 0], [130, 124, 0], [162, 136, -20], [100, 124, -10], [132, 136, -30]])
        for r in rows:
            self.assertEqual(r["allocated_capsule_center_unchanged_bits"], r["actor_root_before_bits"])
        self.assertEqual(self.rows["held-nonzero-owner-yaw"]["actor_yaw_before_after_hex16"], ["1000", "1000"])
        self.assertEqual(self.rows["receiver-yaw-wrap"]["actor_yaw_before_after_hex16"], ["7000", "B000"])

    def test_saved_support_and_owner_allow_mask_are_separate_gates(self):
        admit = self.rows["allow-mask-includes-owner"]
        self.assertTrue(admit["carry_admitted"])
        self.assertEqual((admit["owner_seat_bit"], admit["declared_scene_owner_mask"]), (2, 2))
        for name in ("allow-mask-excludes-owner", "owner-phase-two", "owner-inactive", "previous-support-only"):
            r = self.rows[name]
            self.assertFalse(r["carry_admitted"])
            self.assertEqual(r["actor_root_before_bits"], r["actor_root_after_bits"])
            self.assertEqual(r["actor_flags_after"], "00000040")
        reset = self.rows["previous-support-only"]
        self.assertEqual((reset["current_support_flag"], reset["previous_support_flag"]), (False, True))

    def test_uninitialized_angle_control_is_not_retail_startup(self):
        expected, control = [self.rows[name] for name in ("owner-quarter-turn", "fixture-zero-gqr-control")]
        self.assertEqual([r["gqr5"] for r in (expected, control)], ["00070007", "00000000"])
        self.assertEqual(expected["actor_yaw_before_after_hex16"], control["actor_yaw_before_after_hex16"])
        self.assertNotEqual(expected["actor_root_after_bits"], control["actor_root_after_bits"])
        self.assertTrue(control["fixture_control_not_retail_startup"])
        self.assertFalse(expected["fixture_control_not_retail_startup"])

    def test_refuses_bad_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            carry.decode("")
        with self.assertRaisesRegex(ValueError, "held"):
            carry.decode("support-carry: invalid\n"*13)
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "carry.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                carry.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
