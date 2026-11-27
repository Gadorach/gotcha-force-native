"""Keep type-55 callback timing distinct from segment admission and HP."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type55_segment as segment


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type55-segment.json"
REPORT = """\
type55-shapes: 0 0 0 0 0 0
type55-shapes: 1 1 0 0 180 0
type55-shapes: 2 2 0 0 180 0
type55-shapes: 3 3 0 0 180 0
type55-shapes: 4 4 1 4 176 1
type55-shapes: 5 2 2 4 176 1
type55-shapes: 6 4 2 4 176 1
type55-window: 7 10 59 70
type55-window: 8 10 59 70
"""


class RetailType55SegmentTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())

    def test_shape_budget_selects_prefix_and_never_tops_up(self):
        binds, windows = segment.decode_report(REPORT)
        self.assertEqual((binds, windows), (self.data["bind_cases"], self.data["empty_carrier_windows"]))
        for slots, row in enumerate(binds[:5]):
            self.assertEqual(row["allocation_order"], list(segment.STEPS[:slots]))
            self.assertEqual(row["attack_registry_order"], list(reversed(segment.STEPS[:slots])))
            self.assertEqual(row["later_supplied_shape_cells"], 4-slots)
            self.assertFalse(row["retried_bind"])
        self.assertIsNone(binds[0]["receiver"])
        self.assertEqual([r["receiver"]["hp_loss"] for r in binds[1:5]], [0, 0, 0, 4])

    def test_two_overlapping_descriptors_use_one_token(self):
        self.assertEqual(self.data["source"]["raw_power"], 10)
        for row in self.data["bind_cases"][5:]:
            self.assertEqual(row["overlapping_shapes"], 2)
            self.assertEqual(row["receiver"], {"hp_after": 176, "hp_loss": 4, "segment_token": 1,
                                               "same_segment_token_refused_on_repeat": True})

    def test_empty_carrier_pool_does_not_delay_gate_or_change_clock_domain(self):
        unit, half = self.data["empty_carrier_windows"]
        self.assertEqual((unit["owner_clock_units_per_visit"], half["owner_clock_units_per_visit"]), (1, 0.5))
        self.assertEqual({k: v for k, v in unit.items() if k != "owner_clock_units_per_visit"},
                         {k: v for k, v in half.items() if k != "owner_clock_units_per_visit"})
        self.assertEqual((unit["first_request_visit"], unit["last_request_visit"], unit["requests"], unit["phase_two_visit"]),
                         (11, 69, 59, 70))
        self.assertEqual(unit["segments_allocated"], 0)

    def test_refuses_extra_visit_or_duplicate_damage(self):
        for old, new in (("8 10 59 70", "8 10 60 71"), ("6 4 2 4 176 1", "6 4 2 8 172 2"),
                         ("3 3 0 0 180 0", "3 4 1 4 176 1")):
            self.assertIn(old, REPORT)
            with self.assertRaises(ValueError):
                segment.decode_report(REPORT.replace(old, new, 1))

    def test_existing_output_is_not_overwritten(self):
        with TemporaryDirectory() as tmp:
            out = Path(tmp) / "segment.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                segment.export(None, None, None, None, out)
            self.assertEqual(out.read_text(), "keep")
