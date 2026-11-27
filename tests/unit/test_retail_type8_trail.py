"""Check distance markers and keep child allocation distinct from shape publication."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type8_trail as trail


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type8-trail.json"


class RetailType8TrailTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cases = json.loads(ARTIFACT.read_bytes())["cases"]

    def test_carrier_refusal_still_advances_the_next_distance_marker(self):
        self.assertEqual(len(self.cases), 12)
        self.assertEqual(sum(r["last_visit_observed"] for r in self.cases), 156)
        for full, restored, no_descriptors, one_shape in zip(*(self.cases[i::4] for i in range(4)), strict=True):
            expected = [1, 8, 15] if full["key"] == 0 else [1, 5, 9]
            for row in (full, restored, no_descriptors, one_shape):
                self.assertEqual([e["visit"] for e in row["requests"]], expected)
                self.assertEqual([e["origin"] for e in row["requests"]], [e["origin"] for e in full["requests"]])
                self.assertTrue(all(e["descendant_key"] == row["key"] for e in row["requests"]))
            self.assertEqual(restored["one_carrier_added_after_visit"], expected[1])
            self.assertEqual([e["carrier_allocated"] for e in restored["requests"]], [False, False, True])
            self.assertEqual([e["carrier_allocated"] for e in no_descriptors["requests"]], [False, False, False])

    def test_opening_child_gets_the_only_shape(self):
        for row in self.cases[3::4]:
            self.assertTrue(all(e["carrier_allocated"] for e in row["requests"]))
            self.assertEqual([e["shape_published_on_visit"] for e in row["requests"]], [True, False, False])
            self.assertEqual([e["attack_registry_count_at_end"] for e in row["requests"]], [1, 1, 1])
            self.assertEqual(row["shape_refusals_retired_in_window"], 2)

    def test_report_rejects_catchup_and_confused_pool_results(self):
        lines = []
        for row in self.cases:
            key, case = row["key"], trail.CASES.index(row["case"])
            for event in row["requests"]:
                values = (key, case, event["visit"], event["origin"][2], event["carrier_allocated"],
                          event["shape_published_on_visit"], event["attack_registry_count_at_end"],
                          event["free_carriers_at_end"], event["free_descriptors_at_end"])
                lines.append("type8-trail: " + " ".join(str(int(v)) for v in values))
            lines.append(f"type8-end: {key} {case} {row['last_visit_observed']} {row['shape_refusals_retired_in_window']} "
                         f"{row['requests'][-1]['attack_registry_count_at_end']}")
        report = "\n".join(lines)
        self.assertEqual(trail.decode_report(report), self.cases)
        for old, new in (("0 1 15 960 1 1 1 0 2", "0 1 9 720 1 1 1 0 2"),
                         ("5 2 5 600 0 0 0 3 0", "5 2 5 600 1 0 0 2 0"),
                         ("6 3 5 600 1 0 1 1 1", "6 3 5 600 1 1 2 1 1")):
            self.assertIn(old, report)
            with self.assertRaises(ValueError):
                trail.decode_report(report.replace(old, new, 1))

    def test_existing_receipt_cannot_be_overwritten(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "receipt.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                trail.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
