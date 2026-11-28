from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_stage_collision_walk as walk


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailStageCollisionWalkTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-stage-collision-walk.json").read_bytes())
        cls.rows = {(r["case"], r["seeded_stale_correction_x"]): r["visits"] for r in cls.report["cases"]}

    def test_source_receipt_and_frontier(self):
        prior = MAPS / "gg4e08-stage-correction.json"
        self.assertEqual(self.report["stage_correction_sha256"], sha256(prior.read_bytes()).hexdigest())
        self.assertEqual(self.report["stop_before"], {"call_site": "8002C86C", "entry": "8005A580"})
        self.assertEqual(len(self.rows), 8)

    def test_enclosing_walk_discards_stale_correction(self):
        for (name, stale), visits in self.rows.items():
            if stale is None:
                continue
            fresh = self.rows[name, None]
            self.assertEqual(visits[0]["pre_walk_correction_x"], -64)
            self.assertEqual(fresh[0]["pre_walk_correction_x"], 0)
            self.assertEqual(visits[0] | {"pre_walk_correction_x": 0}, fresh[0])
            self.assertEqual(visits[1], fresh[1])

    def test_corrected_root_is_not_displaced_twice(self):
        first, second = self.rows["attack-right-wall", None]
        self.assertEqual((first["post_walk_correction_x"], second["pre_walk_correction_x"],
                          second["post_walk_correction_x"]), (20, 20, 0))
        self.assertEqual((first["post_walk_root_x"], second["declared_pose_x"],
                          second["post_walk_root_x"]), (150, 150, 150))
        self.assertTrue(second["stage_contact_flag"])
        self.assertEqual(first["allocated_capsule_endpoints_x"], [115, 145])
        self.assertEqual(first["narrow_scratch_capsule_endpoints_x"], [135, 165])
        self.assertEqual(second["allocated_capsule_endpoints_x"], [135, 165])

    def test_second_rebuild_distinguishes_narrow_miss_from_receiver_absence(self):
        for name in ("attack-left-wall", "attack-right-empty"):
            first, second = self.rows[name, None]
            self.assertEqual([v["capsule_broad_field"] for v in (first, second)], [22, 484])
            self.assertIsNone(first["narrow_overlap"])
            self.assertFalse(second["narrow_overlap"])
            self.assertEqual(second["ordinary_receiver_entries_on_rebuild"], 1)
            self.assertEqual(second["hp_after"], 180)
        for name in ("attack-left-empty", "attack-right-wall"):
            first, second = self.rows[name, None]
            self.assertEqual([v["hp_write_count"] for v in (first, second)], [1, 0])
            self.assertEqual((first["hp_removed"], second["hp_after"], second["last_accepted_token"]), (17, 163, 1))
            self.assertEqual((second["pre_walk_receiver_role_mask"], second["pre_walk_receiver_guard"]), (0x43, 60))
            self.assertEqual((second["ordinary_attack_entries_on_rebuild"],
                              second["ordinary_receiver_entries_on_rebuild"]), (1, 0))
            self.assertIsNone(second["narrow_overlap"])

    def test_refuses_bad_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            walk.decode("")
        with self.assertRaisesRegex(ValueError, "attack-left-empty/0/1"):
            walk.decode("stage-walk: invalid\n"*16)
        with TemporaryDirectory() as tmp:
            output = Path(tmp) / "walk.json"
            output.write_text("keep")
            with self.assertRaises(FileExistsError):
                walk.export(None, None, None, None, output)
            self.assertEqual(output.read_text(), "keep")
