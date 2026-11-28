from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_linked_correction as link


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailLinkedCorrectionTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.receipt = json.loads((MAPS / "gg4e08-linked-correction.json").read_bytes())
        cls.rows = {r["case"]: r for r in cls.receipt["cases"]}

    def test_source_receipt_pinned(self):
        prior = (MAPS / "gg4e08-stage-collision-walk.json").read_bytes()
        self.assertEqual(self.receipt["stage_collision_walk_sha256"], sha256(prior).hexdigest())
        self.assertEqual(self.receipt["propagation_call_site"], "8002BDAC")
        self.assertEqual(len(self.rows), 14)

    def test_both_masks_on_both_actors_and_a_generated_vector_are_required(self):
        for name in ("unlinked", "source-71e-missing", "target-71e-missing", "source-71f-missing", "target-71f-missing"):
            row = self.rows[name]
            self.assertEqual(row["post_walk_corrections_x"], [20, 0])
            self.assertIsNone(row["original_transfer_call_site"])
        self.assertEqual(self.rows["empty-grid"]["post_walk_corrections_x"], [0, 0])
        row = self.rows["reciprocal"]
        self.assertEqual(row["stage_only_corrections_x"], [20, 0])
        self.assertEqual(row["post_walk_corrections_x"], [20, 20])
        self.assertEqual(row["post_walk_roots_x"], [150, 270])

    def test_exact_threshold_reversed_seats_and_independent_corrections(self):
        for suffix, bits, wanted in (("predecessor", "47434FFF", 20), ("equal", "47435000", 0),
                                     ("successor", "47435001", 0)):
            row = self.rows[f"threshold-{suffix}"]
            self.assertEqual(row["declared_target_204_bits"], bits)
            self.assertEqual(row["post_walk_corrections_x"], [20, wanted])
        row = self.rows["reversed-seats"]
        self.assertEqual((row["source_seat"], row["target_seat"], row["original_transfer_call_site"]),
                         (2, 1, "8002CC34"))
        self.assertEqual(row["post_walk_corrections_x"], [20, 20])
        row = self.rows["both-corrected"]
        self.assertIsNone(row["original_transfer_call_site"])
        self.assertEqual(row["stage_only_corrections_x"], [20, 10])
        self.assertEqual(row["post_walk_corrections_x"], [20, 10])

    def test_linked_pair_moves_centers_but_only_scratch_endpoints_and_target_hp(self):
        off, on = [self.rows[f"pair-{name}"] for name in ("unlinked", "linked")]
        self.assertEqual([r["target_allocated_capsule_endpoints_x"] for r in (off, on)], [[235, 265]]*2)
        self.assertEqual([r["post_walk_capsule_centers_x"] for r in (off, on)], [[150, 250], [150, 270]])
        a, b = [r["ordinary_pair"] for r in (off, on)]
        self.assertEqual([p["target_hp_removed"] for p in (a, b)], [0, 17])
        self.assertEqual([p["source_hp_after"] for p in (a, b)], [180, 180])
        self.assertIsNone(a["narrow_capsule_kernel"])
        self.assertEqual((b["target_scratch_capsule_endpoints_x"], b["target_accepted_token"]), ([255, 285], 1))

    def test_refuses_bad_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            link.decode("")
        with self.assertRaisesRegex(ValueError, "empty-grid/linked-correction"):
            link.decode("linked-correction: invalid\n"*16)
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "linked.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                link.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
