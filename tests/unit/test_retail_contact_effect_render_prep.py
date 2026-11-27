"""Keep dynamic visual update, matrix preparation, and view queues separate."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_contact_effect_render_prep as render


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-contact-effect-render-prep.json"


def report():
    lines = []
    for selector, masks in render.CASES:
        for visit, mask in enumerate(masks, 1):
            collected = int(visit < len(masks)-1)
            remaining = int(bool(mask)) if selector == 1 else mask
            alternate_prepared = mask if selector == 8 else 0
            lines.append(f"effect-render: {selector} {visit} 1 {mask} {alternate_prepared} {collected} 0 0")
            lines.append(f"effect-render: {selector} {visit} 0 {mask} {mask} {collected} "
                         f"{remaining.bit_count()} {remaining}")
    return "\n".join(lines)


class RetailContactEffectRenderPrepTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())
        cls.rows = {(r["selector"], r["visit"], r["view_context_byte"]): r for r in cls.data["view_visits"]}

    def test_view_zero_prepares_a_group_for_selector_one_but_only_queues_selector_eight(self):
        for selector, visit, active, descriptors in ((1, 6, 65535, 1), (8, 5, 63, 6)):
            no_match, match = self.rows[selector, visit, 1], self.rows[selector, visit, 0]
            self.assertEqual((match["effect_active_mask"], match["prepared_joint_mask"]), (active, active))
            self.assertEqual(match["lane_5_descriptor_count"], descriptors)
            self.assertEqual(no_match["prepared_joint_mask"], 0 if selector == 1 else active)
            self.assertEqual(no_match["lane_5_descriptor_count"], 0)
            self.assertFalse(no_match["reached_render_state_frontier"])

    def test_zero_mask_stops_queueing_before_visual_collection_ends(self):
        for selector, first_empty in ((1, 18), (8, 11)):
            self.assertTrue(self.rows[selector, first_empty, 0]["in_view_zero_effect_list"])
            self.assertEqual(self.rows[selector, first_empty, 0]["lane_5_descriptor_count"], 0)
            self.assertFalse(self.rows[selector, first_empty+1, 0]["in_view_zero_effect_list"])
        self.assertEqual(len(self.rows), 66)
        self.assertEqual(sum(r["reached_render_state_frontier"] for r in self.rows.values()), 27)

    def test_report_rejects_wrong_view_queue_or_skipped_early_transform(self):
        replay = report()
        self.assertEqual(render.decode(replay), self.data["view_visits"])
        for bad in (replay.replace("1 6 1 65535 0 1 0 0", "1 6 1 65535 0 1 1 1", 1),
                    replay.replace("8 5 1 63 63 1 0 0", "8 5 1 63 0 1 0 0", 1)):
            with self.assertRaises(ValueError):
                render.decode(bad)

    def test_refuses_to_replace_output(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "render.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                render.export(None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
