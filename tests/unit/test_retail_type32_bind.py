"""Keep type-32 carrier admission distinct from type-11 segment admission."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type32_bind as binding


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type32-bind.json"
REPORT = """\
type32-branch: 3331 8 0 0 0 0 0 0 0 0 0
type32-branch: 3331 8 1 1 1 0 0 0 0 0 0
type32-branch: 3331 8 2 1 1 1 0 0 0 0 1
type32-branch: 3586 9 0 0 0 0 0 0 0 0 0
type32-branch: 3586 9 1 1 0 0 1 1 0 0 1
type32-branch: 3586 9 2 1 0 0 1 1 1 1 1
type32-branch: 3586 9 3 1 0 0 1 0 0 0 1
"""


class RetailType32BindTest(unittest.TestCase):
    def test_original_bind_and_segment_receive_distinct_credit_paths(self):
        data = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(data["cases"], binding.decode_report(REPORT))
        self.assertEqual([r["raw_power"] for r in data["sources"]], [30, 10])
        _, _, saucer, _, no_shape, child, no_carrier = data["cases"]
        self.assertTrue(saucer["carrier_shape_bound"])
        self.assertFalse(saucer["active_segment_requested"])
        self.assertEqual(saucer["group_bind_attempted"], 1)
        self.assertFalse(child["carrier_shape_attempt"])
        self.assertTrue(child["segment_carrier_allocated"])
        self.assertTrue(child["segment_shape_bound"])
        self.assertEqual(child["segment_samples"], {"first_endpoint": [100, 200, 300],
                                                    "updated_second_endpoint": [250, 270, 410]})
        self.assertTrue(no_shape["segment_carrier_allocated"])
        self.assertFalse(no_shape["segment_shape_bound"])
        self.assertTrue(no_carrier["active_segment_requested"])
        self.assertEqual(no_carrier["segment_key_requested"], 1)
        self.assertFalse(no_carrier["segment_carrier_allocated"])
        self.assertIsNone(no_carrier["group_bind_attempted"])
        for row in data["cases"]:
            self.assertTrue(row["carrier_already_published"])
            self.assertIs(row["returned_null_from_factory"], True if row["case"] == "scratch-empty" else None)

    def test_report_refuses_merged_ownership_and_snapshotted_endpoints(self):
        for old, new in (("8 2 1 1 1 0 0 0 0 1", "8 2 1 0 0 1 1 1 1 1"),
                         ("9 2 1 0 0 1 1 1 1 1", "9 2 1 0 0 1 1 1 0 1"),
                         ("9 3 1 0 0 1 0 0 0 1", "9 3 1 0 0 1 1 0 0 1")):
            self.assertIn(old, REPORT)
            with self.assertRaises(ValueError):
                binding.decode_report(REPORT.replace(old, new, 1))

    def test_existing_receipt_is_not_overwritten(self):
        with TemporaryDirectory() as tmp:
            out = Path(tmp) / "receipt.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                binding.export(None, None, None, None, out)
            self.assertEqual(out.read_text(), "keep")
