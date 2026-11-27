"""Protect terrain-only clipping and the wall effect's first claim on descriptors."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type5_sibling_stage as stage


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type5-sibling-stage.json"


class RetailType5SiblingStageTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())
        cls.rows = cls.data["cases"]

    def test_wall_outside_a_clipped_probe_does_not_correct_the_carrier(self):
        self.assertEqual(len(self.rows), 42)
        for clear, wall, clipped in zip(self.rows[::7], self.rows[1::7], self.rows[4::7], strict=True):
            self.assertEqual(clear["probe"]["far"], [200, 100, 1100])
            self.assertEqual(wall["probe"]["intersection"], [200, 100, 1000])
            self.assertEqual(wall["carrier_root_after"], wall["probe"]["intersection"])
            self.assertEqual(clipped["probe"]["far"], [200, 100, 950])
            self.assertIsNone(clipped["probe"]["intersection"])
            self.assertEqual(clipped["carrier_root_after"], [200, 100, 1100])
            self.assertIsNone(clipped["area"])
            self.assertEqual((clear["parent_phase_after"], clipped["parent_phase_after"]), (1, 1))

    def test_wall_effect_and_area_compete_for_descriptors_not_carrier_classes(self):
        self.assertEqual((self.data["wall_effect"]["class"], self.data["wall_effect"]["fixed_bucket"]), (2, 31))
        for both, only_effect, only_area, neither in zip(
            self.rows[1::7], self.rows[2::7], self.rows[5::7], self.rows[6::7], strict=True
        ):
            self.assertEqual([(r["wall_effect"]["allocated"], r["area"]["allocated"])
                              for r in (both, only_effect, only_area, neither)],
                             [(True, True), (True, False), (False, True), (False, False)])
            self.assertEqual([r["free_descriptors_after"] for r in (both, only_effect, only_area, neither)], [0, 1, 0, 1])
            for row in (both, only_effect, only_area, neither):
                self.assertEqual(row["area"]["position"], row["carrier_root_after"])
                self.assertEqual(row["parent_phase_after"], 2)

    def test_actor_contact_latch_bypasses_wall_kernel(self):
        for row in self.rows[3::7]:
            self.assertTrue(row["wall_enabled"])
            self.assertIsNone(row["probe"])
            self.assertIsNone(row["wall_effect"])
            self.assertEqual(row["carrier_root_after"], [200, 100, 900])
            self.assertEqual(row["area"]["position"], [200, 100, 900] if row["parent_type"] == 77 else [260, 120, 880])

    def test_report_rejects_probe_motion_aliasing_or_reversed_allocation_priority(self):
        lines = []
        for row in self.rows:
            case = next(i for i, rule in enumerate(stage.CASES) if rule[0] == row["case"])
            probe, effect, area = (row[f] or {} for f in ("probe", "wall_effect", "area"))
            values = (int(row["borg"][-4:], 16), case, row["parent_type"], row["parent_row"],
                      probe.get("far", (0, 0, 0))[2], bool(probe.get("intersection")), row["carrier_root_after"][2],
                      bool(effect), effect.get("allocated", False), bool(area), area.get("allocated", False),
                      area.get("position", (0, 0, 0))[2], row["free_descriptors_after"])
            lines.append("area-stage: " + " ".join(str(int(x)) for x in values))
        report = "\n".join(lines)
        self.assertEqual(stage.decode_report(report), self.rows)
        for old, new in (("262 4 33 2 950 0 1100", "262 4 33 2 950 0 950"),
                         ("262 2 33 2 1100 1 1000 1 1 1 0 1000 1", "262 2 33 2 1100 1 1000 1 0 1 1 1000 0")):
            self.assertIn(old, report)
            with self.assertRaises(ValueError):
                stage.decode_report(report.replace(old, new, 1))

    def test_existing_output_is_preserved(self):
        with TemporaryDirectory() as tmp:
            target = Path(tmp) / "receipt.json"
            target.write_text("keep")
            with self.assertRaises(FileExistsError):
                stage.export(None, None, None, None, target)
            self.assertEqual(target.read_text(), "keep")
