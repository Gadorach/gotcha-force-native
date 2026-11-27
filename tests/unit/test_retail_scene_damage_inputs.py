"""Pin separation of source port enrollment, active slots, and rank writes."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_scene_damage_inputs as inputs


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-scene-damage-inputs.json"
REPORT = """\
automatic: 2 0 15 4 7 7 0 1
selected-slot: 0 2 0 5 32 1 7 0
selected-slot: 0 2 1 5 33 2 3 0
selected-slot: 0 3 0 5 32 1 7 0
selected-slot: 0 3 1 5 33 2 3 1
selected-slot: 3 2 0 5 32 1 7 0
selected-slot: 3 2 1 5 40 2 3 0
selected-slot: 3 3 0 5 32 1 7 0
selected-slot: 3 3 1 5 40 2 3 1
ports: 0 1 2 1 3 2 12
ports: 1 3 2 2 7 3 8
ports: 1 3 4 255 3 2 12
ports: 1 3 2 255 3 2 12
ports: 2 132 2 1 134 2 9
ports: 2 7 2 1 7 3 8
"""


class RetailSceneDamageInputsTest(unittest.TestCase):
    def test_receipt_preserves_distinct_masks_and_rejected_rank(self):
        result = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(result["source_dol_sha256"], inputs.SOURCE_SHA256)
        automatic, slots, ports = inputs.decode_report(REPORT)
        self.assertEqual({k: result["automatic"][k] for k in automatic}, automatic)
        self.assertEqual(result["selected_slot"]["cases"], slots)
        self.assertEqual(result["port_enrollment"]["cases"], ports)
        self.assertEqual((automatic["mode_input_mask"], automatic["scene_active_mask"]), (0, 15))
        self.assertTrue(all(row["mode_input_mask"] == 5 for row in slots))
        self.assertEqual([row["scene_rank_after"] for row in slots], [7, 3] * 4)
        self.assertEqual([row["picked_port"] for row in ports], [1, 2, None, None, 1, 1])
        self.assertEqual(ports[4]["mode_input_port_count"], 2)
        self.assertEqual(ports[4]["mode_input_mask"], 0x86)

    def test_refuses_missing_controls_and_mask_or_rank_disagreement(self):
        for report in (REPORT.replace("automatic: 2 0 15", "automatic: 2 15 15"),
                       REPORT.replace("0 2 1 5 33 2 3 0", "0 2 1 33 33 2 3 0"),
                       REPORT.replace("3 3 0 5 32 1 7 0", "3 3 0 5 32 1 3 0"),
                       REPORT.replace("132 2 1 134 2 9", "132 2 1 134 3 9"),
                       REPORT.removesuffix("ports: 2 7 2 1 7 3 8\n")):
            with self.subTest(report=report), self.assertRaises(ValueError):
                inputs.decode_report(report)

    def test_export_refuses_an_existing_file(self):
        with TemporaryDirectory() as temp:
            output = Path(temp) / "preserved.json"
            output.write_text("preserved")
            with self.assertRaises(FileExistsError):
                inputs.export(None, None, None, output)
            self.assertEqual(output.read_text(), "preserved")
