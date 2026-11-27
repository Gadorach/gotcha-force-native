"""Validate isolated installer receipts and their exact static profile joins."""
from collections import Counter
import json
from pathlib import Path
import unittest
from unittest.mock import Mock

from tools.analyze import retail_installer_execution as installed


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailInstallerExecutionTest(unittest.TestCase):
    def test_partial_ambiguous_or_misaligned_evidence_refuses(self):
        borgs = [{"id": "gg4e08:000a", "installer": "80001000", "profiles": ["p1"]}]
        profiles = {"p1": {"config": "80300000", "context_table": "80300100",
                           "selector_table": "80300200", "main_handler": "80001100",
                           "program_and_descriptor_roots": ["80301000", "80301100", "80301200", "80301300"],
                           "groups": [{"group": "B", "mode": 0}] * 6}}
        spans = {0x80001100: 4, 0x80001200: 4, 0x80001300: 4}
        line = ("000A 80001000 80300000 80300100 80300200 80001100 80001200 "
                "80001300 000001B0 80301000 80301100 80301200 80301300\n")
        self.assertEqual(installed.plan(borgs), "000A 80001000\n")
        image = Mock()
        image.read.return_value = b"\x03"
        counts, rows = installed.join(image, borgs, profiles, spans, line)
        self.assertEqual(counts["borgs"], 1)
        self.assertEqual(rows[0]["spec_record"], 1)
        self.assertEqual(rows[0]["config_lane_mask_seed"], 3)
        image.read.assert_called_with(0x80300002, 1, "data")
        for change in ("", line + line, line.replace("000A", "000B"),
                       line.replace("80301300", "80301304"), line.replace("000001B0", "000001B4")):
            with self.subTest(change=change), self.assertRaises(ValueError):
                installed.join(image, borgs, profiles, spans, change)
        borgs[0]["profiles"].append("p2")
        profiles["p2"] = profiles["p1"]
        with self.assertRaisesRegex(ValueError, "not one static profile"):
            installed.join(image, borgs, profiles, spans, line)

    def test_published_execution_map_matches_borg_and_charge_inputs(self):
        payload = json.loads((MAPS / "gg4e08-standalone-installers.json").read_bytes())
        atlas = json.loads((MAPS / "gg4e08-code-atlas.json").read_bytes())
        inputs = json.loads((MAPS / "gg4e08-action-input.json").read_bytes())
        for source in (atlas, inputs):
            self.assertEqual(payload["symbol_metadata_sha256"], source["symbol_metadata_sha256"])
            self.assertEqual(payload["source_dol_sha256"], source["source_dol_sha256"])
        models = {b["id"]: b for b in payload["borgs"]}
        charge = {b["id"]: b for b in inputs["borgs"]}
        self.assertEqual(len(models), 206)
        counts = Counter()
        for static in atlas["borgs"]:
            actual = models[static["id"]]
            self.assertIn(actual["profile"], static["profiles"])
            self.assertEqual(actual["installer"], static["installer"])
            self.assertEqual(actual["spec_record"], charge[static["id"]]["spec_record"])
            self.assertIn(actual["pre_callback"], atlas["functions"])
            self.assertIn(actual["post_callback"], atlas["functions"])
            active = next((p["enabled_modes"] for p in charge[static["id"]]["profiles_with_charge"]
                           if p["profile"] == actual["profile"]), {})
            self.assertEqual(actual["enabled_charge_modes"], active)
            counts.update(active.keys())
        self.assertEqual(counts, {"B-charge": 44, "X-charge": 25})
        self.assertEqual(models["gg4e08:000a"]["profile"], "p013")
        self.assertEqual(models["gg4e08:0600"]["profile"], "p076")
        self.assertEqual(models["gg4e08:062a"]["profile"], "p089")
