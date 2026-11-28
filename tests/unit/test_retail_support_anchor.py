from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_support_anchor as anchor


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailSupportAnchorTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-support-anchor.json").read_bytes())
        cls.rows = {row["case"]: row for row in cls.report["cases"]}

    def test_exact_prior_and_saved_support_state(self):
        prior = (MAPS / "gg4e08-plane-reference.json").read_bytes()
        self.assertEqual(self.report["plane_reference_sha256"], sha256(prior).hexdigest())
        self.assertEqual((self.report["plane_descriptor"], self.report["plane_matrix_slot"]), (3, 12))
        for row in self.rows.values():
            self.assertEqual(row["saved_local_anchor_bits"], ["00000000", "4214CCCC", "00000000"])
            self.assertTrue(row["original_plane_cell_retained"])
            self.assertEqual(row["hp_after"], 180)

    def test_height_follows_support_but_only_upward_landing_moves_root(self):
        order = ("held-support", "raise-12", "translate-xz", "lower-12")
        rows = [self.rows[name] for name in order]
        self.assertEqual([anchor.stage.scalar(r["ground_height_query_bits"]) for r in rows], [124, 136, 124, 112])
        self.assertEqual([r["ground_height_query_bits"] for r in rows], [r["surface_height_query_bits"] for r in rows])
        self.assertEqual([anchor.stage.scalar(r["actor_root_after_landing_bits"][1]) for r in rows], [124, 136, 124, 124])
        self.assertEqual([r["original_landing_return"] for r in rows], [1, 1, 1, 0])
        for r in rows:
            self.assertEqual(r["actor_root_after_landing_bits"][::2], r["actor_root_before_bits"][::2])
            self.assertEqual(r["allocated_capsule_center_held_during_landing_bits"][1], "42F80000")

    def test_rotation_reprojects_anchor_but_exposes_saved_not_rebuilt_normal(self):
        plain, tilted, rebuilt = [self.rows[tag] for tag in ("raise-matched-untilted", "tilt-and-raise",
                                                            "tilt-raise-and-rebuild")]
        self.assertEqual(plain["declared_plane_translation_change"], tilted["declared_plane_translation_change"])
        self.assertEqual([r["surface_height_query_bits"] for r in (plain, tilted, rebuilt)],
                         ["433B3333", "43160000", "43160000"])
        self.assertTrue(rebuilt["geometry_rebuilt_before_query"])
        self.assertEqual(rebuilt["plane_normal_at_query_bits"], ["BF800000", "00000000", "00000000"])
        self.assertEqual(rebuilt["surface_normal_query_bits"], ["00000000", "3F7FFFFF", "00000000"])
        self.assertEqual(rebuilt["allocated_capsule_center_held_during_landing_bits"][1], "43000000")

    def test_previous_support_is_not_a_live_anchor_gate(self):
        r = self.rows["raise-after-reset"]
        self.assertEqual((r["current_support_flag"], r["previous_support_flag"]), (False, True))
        self.assertTrue(r["original_plane_cell_retained"])
        self.assertEqual(anchor.stage.scalar(r["surface_height_query_bits"]), -9000)
        self.assertEqual(r["actor_root_before_bits"], r["actor_root_after_landing_bits"])

    def test_refuses_bad_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            anchor.decode("")
        with self.assertRaisesRegex(ValueError, "held-support"):
            anchor.decode("support-anchor: invalid\n"*8)
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "anchor.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                anchor.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
