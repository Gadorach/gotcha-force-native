from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_stage_side_recovery as recovery


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailStageSideRecoveryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-stage-side-recovery.json").read_bytes())
        cls.archives = {row["filename"]: row for row in cls.report["archives"]}
        cls.owners = [o for row in cls.archives.values() for o in row["owners"]]

    def test_all_construction_members_and_original_prop_receipt_are_pinned(self):
        for stem, key in (
            ("stage-side-census", "stage_side_census_sha256"), ("stage-prop-recovery", "stage_prop_recovery_sha256"),
        ):
            raw = (MAPS / f"gg4e08-{stem}.json").read_bytes()
            self.assertEqual(self.report[key], sha256(raw).hexdigest())
        self.assertEqual(tuple(self.archives), recovery.side.NAMES)
        self.assertEqual((len(self.owners), self.report["post_break_bucket_visits_per_archive"]), (180, 664))
        self.assertEqual(sum(o["shape_cells_returned_on_break"] for o in self.owners), 265)
        self.assertEqual(self.report["declared_break_hp"], 0)

    def test_timed_types_rebind_together_but_settle_at_distinct_float_edges(self):
        timed = [o for o in self.owners if o["collision_rebind_visit"] is not None]
        self.assertEqual(len(timed), 172)
        self.assertEqual(self.report["summary"]["rebound_shape_cells"], 256)
        self.assertEqual(self.report["summary"]["rise_step_counts"], {"60": 64, "61": 108})
        self.assertEqual({o["collision_rebind_visit"] for o in timed}, {600})
        for row in timed:
            self.assertEqual(row["settled_visit"], 600+row["rise_steps"])
            self.assertEqual(row["companion_enable_visit"], row["settled_visit"]+1)
            self.assertIsNone(row["owner_return_visit"])
        self.assertEqual({o["type"] for o in timed if o["rise_steps"] == 60}, {0, 3, 6})

    def test_terminal_path_returns_owner_and_model_descriptors_without_rebind(self):
        special = self.archives["set0904.arc"]
        self.assertEqual((special["rebound_owners"], special["retired_owners"]), (0, 8))
        self.assertEqual(self.report["summary"]["returned_model_descriptors"], 16)
        for row in special["owners"]:
            self.assertEqual((row["break_phase"], row["owner_return_visit"], row["model_descriptors_returned"]), (2, 2, 2))
            self.assertIsNone(row["collision_rebind_visit"])
            self.assertIsNone(row["companion_enable_visit"])

    def test_same_type_sibling_does_not_share_the_break_bit(self):
        isolate = self.report["isolation_case"]
        self.assertEqual((isolate["member"], isolate["broken_owner_placement"], isolate["unbroken_same_type_placement"]),
                         (2429, 0, 1))
        before, after = isolate["checkpoints"]
        self.assertEqual([before["type4_mask"], after["type4_mask"]], [1, 1])
        self.assertTrue(before["broken_companion_enabled"])
        self.assertFalse(after["broken_companion_enabled"])
        self.assertTrue(after["unbroken_companion_enabled"])
        self.assertEqual(after["unbroken_owner_hp"], 100)

    def test_refuses_bad_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            recovery.decode("", [], [])
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "side-recovery.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                recovery.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
