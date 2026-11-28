from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_tether_wall as join


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailTetherWallTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-tether-wall.json").read_bytes())
        cls.rows = {r["case"]: r for r in cls.report["cases"]}

    def test_both_original_replays_are_pinned(self):
        for name in ("type25_link", "linked_correction"):
            prior = (MAPS / f"gg4e08-{name.replace('_', '-')}.json").read_bytes()
            self.assertEqual(self.report[f"{name}_sha256"], sha256(prior).hexdigest())
        self.assertEqual((self.report["tether_owner"], self.report["tether_target"]),
                         ("gg4e08:0201", "gg4e08:000a"))
        self.assertEqual(self.report["declared_wall_plane"], [1, 0, 0, -800])

    def test_actual_tether_masks_gate_generated_wall_transfer_at_same_pose(self):
        before, stretched, expired = [self.rows[name] for name in ("before-pull-wall", "stretched-wall", "expired-wall")]
        self.assertEqual([r["pre_walk_roots_x_owner_target"] for r in (before, stretched, expired)], [[200, 830]]*3)
        self.assertEqual([r["stage_only_corrections_x_owner_target"] for r in (before, stretched, expired)], [[0, 20]]*3)
        self.assertEqual([r["produced_masks_71f_owner_target"] for r in (before, stretched, expired)],
                         [[0, 0], [2, 1], [0, 0]])
        self.assertEqual([r["transfer_writer_call_site"] for r in (before, stretched, expired)],
                         [None, "8002CC34", None])
        self.assertEqual([r["post_walk_root_x_owner_target"] for r in (before, stretched, expired)],
                         [[200, 850], [220, 850], [200, 850]])
        empty = self.rows["stretched-empty"]
        self.assertEqual(empty["produced_masks_71f_owner_target"], [2, 1])
        self.assertEqual(empty["post_walk_correction_x_owner_target"], [0, 0])

    def test_narrow_phase_can_miss_after_transfer_without_changing_allocated_endpoints(self):
        empty, wall = [self.rows[f"tau-stretched-{name}"] for name in ("empty", "wall")]
        self.assertEqual([r["allocated_chainsaw_capsule_endpoints_x"] for r in (empty, wall)], [[185, 215]]*2)
        self.assertEqual([r["post_walk_broad_centers_x_owner_target"] for r in (empty, wall)], [[200, 830], [220, 850]])
        pairs = [r["tau_pair"] for r in (empty, wall)]
        self.assertEqual([p["original_narrow_kernel"] for p in pairs], ["80030A3C"]*2)
        self.assertEqual([p["chainsaw_scratch_capsule_endpoints_x"] for p in pairs], [[185, 215], [205, 235]])
        self.assertEqual([p["hp_removed"] for p in pairs], [17, 0])
        self.assertEqual([p["chainsaw_hp_after"] for p in pairs], [233, 250])

    def test_tether_and_tau_tokens_belong_to_different_contacts(self):
        empty, wall = [self.rows[f"tau-stretched-{name}"] for name in ("empty", "wall")]
        self.assertEqual([r["sasuke_retained_tether_token"] for r in (empty, wall)], [1, 1])
        self.assertEqual([r["sasuke_hp_after"] for r in (empty, wall)], [180, 180])
        self.assertEqual([r["tau_pair"]["tau_attack_token"] for r in (empty, wall)], [2, 2])
        self.assertEqual([r["tau_pair"]["chainsaw_accepted_token"] for r in (empty, wall)], [2, None])

    def test_refuses_bad_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            join.decode("")
        with self.assertRaisesRegex(ValueError, "stretched-empty/tether-wall"):
            join.decode("tether-wall: invalid\n"*8)
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "tether-wall.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                join.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
