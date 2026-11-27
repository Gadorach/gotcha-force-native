"""Distinguish a paid bomb, command consumption, child bind and stock return."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type41_bucket as bombs


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type41-bucket.json"


class RetailType41BucketTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rows = {r["case"]: r for r in json.loads(ARTIFACT.read_bytes())["cases"]}

    def test_remote_bucket_obeys_insertion_order_and_consumes_one_command(self):
        for name, cell in (("remote-bucket-first-cell", 0), ("remote-bucket-reversed-cells", 1)):
            row = self.rows[name]
            self.assertEqual(row["first_carrier_cell"], cell)
            self.assertEqual(row["requested_area_key"], 20)
            self.assertTrue(row["one_sibling_still_active"])
            self.assertEqual(row["owner_command_latch_after_terminal_visit"], 0)
            self.assertEqual((row["stock_after_placement"], row["stock_after_terminal_visit"],
                              row["stock_after_final_cleanup"]), (6, 6, 7))

    def test_refused_child_does_not_reopen_remote_command_or_skip_return(self):
        for name in ("remote-bucket-no-area-cell", "remote-bucket-no-area-descriptors"):
            row = self.rows[name]
            self.assertFalse(row["area_allocated"])
            self.assertEqual(row["owner_command_latch_after_terminal_visit"], 0)
            self.assertEqual(row["lifecycle_stock_writer_calls"], 1)
        row = self.rows["remote-command-at-stock-cap"]
        self.assertIsNone(row["requested_area_key"])
        self.assertEqual(row["stock_after_final_cleanup"], 8)

    def test_time_does_not_consume_latch_and_children_use_separate_shapes(self):
        time = self.rows["time-expiry-child-shape-admitted"]
        remote = self.rows["remote-bucket-child-shape-admitted"]
        self.assertEqual((time["requested_area_key"], remote["requested_area_key"]), (19, 20))
        self.assertTrue(time["area_bound_shape"] and remote["area_bound_shape"])
        self.assertEqual(time["owner_command_latch_after_terminal_visit"], 1)
        self.assertFalse(self.rows["time-expiry-area"]["area_bound_shape"])

    def test_factory_return_differs_from_opening_refusal_and_owner_invalidation(self):
        factory = self.rows["remote-carrier-refused"]
        self.assertEqual((factory["placement_refusal_writer"], factory["stock_after_placement"]), ("800E0E7C", 8))
        shape = self.rows["remote-opening-shape-refused"]
        self.assertIsNone(shape["placement_refusal_writer"])
        self.assertEqual((shape["stock_after_terminal_visit"], shape["stock_after_final_cleanup"]), (7, 8))
        owner = self.rows["remote-owner-invalid-phase"]
        self.assertEqual((owner["stock_after_terminal_visit"], owner["stock_after_final_cleanup"]), (8, 8))

    def test_refuses_malformed_report_and_existing_output(self):
        with self.assertRaises(ValueError):
            bombs.decode("type41-bucket: 0")
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "type41.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                bombs.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
