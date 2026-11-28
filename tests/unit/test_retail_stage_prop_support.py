from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_stage_prop_support as prop


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailStagePropSupportTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-stage-prop-support.json").read_bytes())
        cls.rows = {r["case"]: r for r in cls.report["cases"]}

    def test_exact_side_archive_constructor_and_shape(self):
        prior = (MAPS / "gg4e08-support-carry.json").read_bytes()
        self.assertEqual(self.report["support_carry_sha256"], sha256(prior).hexdigest())
        self.assertEqual([self.report[k] for k in ("side_member", "side_file", "placement_ordinal", "owner_object_class",
                                                  "owner_bucket", "owner_kind", "collision_selector")],
                         [2429, "set0202.arc", 0, 3, 40, 22, 171])
        self.assertEqual(self.report["loaded_side_bank_record_counts"], [9, 8])
        self.assertEqual((self.report["collision_descriptors"], self.report["role20_plane_descriptor"]),
                         ([236, 237], 237))

    def test_support_belongs_to_the_actual_prop_plane(self):
        near, far = [self.rows[name] for name in ("original-support", "outside-plane")]
        self.assertTrue(near["support_owner_is_original_prop"])
        self.assertEqual(near["plane_narrow_calls"], 1)
        self.assertEqual(prop.stage.scalar(near["saved_local_anchor_bits"][1]), 770)
        self.assertEqual(prop.stage.scalar(near["actor_root_after_original_collision_bits"][1]), 780)
        self.assertFalse(far["support_acquired"])
        self.assertEqual(far["plane_narrow_calls"], 0)
        self.assertIsNone(far["separate_supported_surface_height"])

    def test_original_world_callback_and_carry_move_root_but_not_allocated_shape(self):
        moved = self.rows["moved-owner"]
        previous = [prop.stage.scalar(v) for v in moved["actor_root_after_original_collision_bits"]]
        now = [prop.stage.scalar(v) for v in moved["actor_root_after_carry_prefix_bits"]]
        self.assertEqual([b-a for a, b in zip(previous, now, strict=True)], [32, 12, -20])
        self.assertEqual(moved["capsule_center_held_during_carry_bits"], moved["actor_root_after_original_collision_bits"])
        self.assertEqual((moved["actor_hp_after"], moved["prop_hp_after"]), (180, 100))

    def test_scene_mask_blocks_prop_carry_not_supported_height_query(self):
        moved, masked = [self.rows[name] for name in ("moved-owner", "moved-owner-scene-masked")]
        self.assertEqual(moved["prop_root_after_world_update_bits"], masked["prop_root_after_world_update_bits"])
        self.assertTrue(masked["support_acquired"])
        self.assertFalse(masked["carry_admitted"])
        self.assertEqual(masked["actor_root_after_carry_prefix_bits"], masked["actor_root_after_original_collision_bits"])
        self.assertEqual([r["separate_supported_surface_height"] for r in (moved, masked)], [792, 792])
        self.assertEqual([r["separate_supported_landing_root_y"] for r in (moved, masked)], [792, 792])

    def test_refuses_bad_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            prop.decode("")
        with self.assertRaisesRegex(ValueError, "original-support"):
            prop.decode("stage-prop-support: invalid\n"*4)
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "stage-prop.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                prop.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
