"""Preserve archive ownership, parent-lane routing and real first-update bindings."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_contact_effect_source_art as source


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-contact-effect-source-art.json"


def fixture_rows(slot):
    report, records = [], {}
    for i, case in enumerate(slot["cases"]):
        for model in case["first_update_models"]:
            record = model["record"]
            records[record] = {key: value for key, value in model.items() if key != "record"}
            report.append(f"effect-source: {slot['pzz_slot']} {i} {case['selector']} "
                          f"{case['parent_lane_byte']} {case['credited_actor_lane_byte']} {case['scene_mode_byte']} "
                          f"{record} {model['descriptor_offset']} {model['root_offset']}")
    return "\n".join(report), records


class RetailContactEffectSourceArtTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())
        cls.slots = cls.data["slots"]

    def test_single_and_split_are_distinct_verified_archive_slots(self):
        self.assertEqual(self.data["pzz_member"], 697)
        self.assertEqual([(s["pzz_slot"], s["declared_split_selection_byte"]) for s in self.slots], [(1, 0), (2, 1)])
        self.assertEqual([s["verified_relocation_count"] for s in self.slots], [4705, 4682])
        self.assertTrue(all(s["record_count"] == 157 for s in self.slots))
        self.assertNotEqual(self.slots[0]["archive_sha256"], self.slots[1]["archive_sha256"])

    def test_selector_one_uses_parent_not_credit_with_a_separate_scene_override(self):
        for s in self.slots:
            choices = s["cases"][1:4]
            self.assertEqual([(c["parent_lane_byte"], c["credited_actor_lane_byte"], c["scene_mode_byte"]) for c in choices],
                             [(0, 1, 0), (1, 0, 0), (1, 0, 3)])
            self.assertEqual([c["first_update_models"][0]["record"] for c in choices], [2, 3, 2])

    def test_selector_zero_and_both_selector_eight_models_reach_the_queue(self):
        for slot in self.slots:
            first, *_, last = slot["cases"]
            self.assertEqual(first["first_update_models"][0]["record"], 21)
            self.assertEqual(first["first_view_zero_lane_5_descriptor_count"], 1)
            self.assertEqual([m["record"] for m in last["first_update_models"]], [53, 54])
            self.assertEqual(last["first_view_zero_lane_5_descriptor_count"], 2)

    def test_report_refuses_a_foreign_relocation_or_an_absent_second_model(self):
        for slot in self.slots:
            report, records = fixture_rows(slot)
            self.assertEqual(source.decode(report, slot["pzz_slot"], records), slot["cases"])
            for bad in (report.rsplit("\n", 1)[0], report.replace(
                    str(records[54]["root_offset"]), str(records[53]["root_offset"]))):
                with self.assertRaises(ValueError):
                    source.decode(bad, slot["pzz_slot"], records)

    def test_existing_output_is_not_overwritten(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "contact-source.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                source.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
