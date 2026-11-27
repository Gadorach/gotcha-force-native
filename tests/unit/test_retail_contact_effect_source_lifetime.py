"""Preserve exact-model retirement, resource return, and reuse observations."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_contact_effect_source_lifetime as lifetime


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-contact-effect-source-lifetime.json"


def report(slot):
    lines = []
    for selector, records, loads, peak, last, retire, descriptors, block in lifetime.CASES:
        for generation, first in enumerate(records, 1):
            extra = first[1] if len(first) == 2 else 0
            lines.append(f"effect-source-lifetime: {slot} {selector} {generation} {first[0]} {extra} "
                         f"{loads} {peak} {last} {retire} {descriptors} {block} 2")
    return "\n".join(lines)


class RetailContactEffectSourceLifetimeTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())
        cls.slots = cls.data["slots"]

    def test_both_archives_match_through_two_generations(self):
        single, split = self.slots
        self.assertNotEqual(single["archive_sha256"], split["archive_sha256"])
        self.assertEqual(single["cases"], split["cases"])
        for slot in self.slots:
            self.assertEqual(lifetime.decode(report(slot["pzz_slot"]), slot["pzz_slot"]), slot["cases"])

    def test_last_draw_list_visit_is_distinct_from_retirement(self):
        for row in self.slots[0]["cases"]:
            want = {0: (21, 23, None), 1: (16, 19, 96), 8: (10, 13, 288)}[row["selector"]]
            self.assertEqual((row["last_lane_5_visit"], row["retirement_visit"],
                              row["returned_type3_heap_block_bytes"]), want)
            self.assertEqual(row["pre_retirement_refusal_visits"], [want[1]-2, want[1]-1])
            self.assertEqual(row["scene_budget_return_visit"], 17 if row["selector"] == 1 else None)

    def test_reuse_rebinds_selector_one_and_selector_eight_installs_later_pieces(self):
        _, _, first, second, *pieces = self.slots[0]["cases"]
        self.assertEqual((first["first_update_models"], second["first_update_models"]), ([2], [3]))
        for row in pieces:
            self.assertEqual(row["first_update_models"], [53, 54])
            self.assertEqual(row["later_model_loads"], [{"visit": i, "record": 54} for i in range(2, 6)])
            self.assertEqual((row["total_model_loads"], row["peak_lane_5_descriptors"], row["returned_descriptors"]),
                             (6, 6, 8))

    def test_decoder_rejects_stale_rebind_or_early_slot_release(self):
        trace = report(1)
        for bad in (trace.replace("1 1 2 3 0", "1 1 2 2 0", 1), trace.replace("16 19 1 96 2", "16 18 1 96 1", 1)):
            with self.assertRaises(ValueError):
                lifetime.decode(bad, 1)

    def test_existing_output_is_preserved(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "lifetime.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                lifetime.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
