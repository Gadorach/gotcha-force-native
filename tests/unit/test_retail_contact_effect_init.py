"""Keep battle admission, visual storage and failure cleanup separate."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_contact_effect_init as init


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-contact-effect-init.json"
REPORT = "\n".join("impact-init: " + " ".join(map(str, row)) for _, row in init.CASES)


class RetailContactEffectInitTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())
        cls.cases = {r["case"]: r for r in cls.data["cases"]}

    def test_selector_1_trims_the_same_random_draw_at_the_entry_cap(self):
        names = ("last-burst-slot", "trim-to-16-entries", "trim-to-one-entry")
        rows = [self.cases[name] for name in names]
        self.assertEqual([r["selector_1_entry_split"] for r in rows], [[10, 9], [8, 8], [1, 0]])
        self.assertEqual([r["visual_request_bytes"] for r in rows], [1064, 896, 56])
        self.assertEqual([r["heap_frontier_bytes_with_type3_cell"] for r in rows], [1088, 896, 64])
        self.assertTrue(all(r["rng_after_refusal"] == 42834 for r in rows))

    def test_scene_caps_do_not_gate_selector_8s_fixed_request(self):
        for name in ("full-burst-budget", "full-entry-budget"):
            row = self.cases[name]
            self.assertIsNone(row["visual_request_bytes"])
            self.assertEqual(row["rng_before"], row["rng_after_refusal"])
        for name in ("selector-8-empty-budget", "selector-8-full-budgets"):
            row = self.cases[name]
            self.assertEqual(row["visual_request_bytes"], 256)
            self.assertEqual(row["descriptors_claimed_and_returned"], 8)
            self.assertIsNone(row["selector_1_entry_split"])
        self.assertEqual(self.data["entries"]["selector_inits"], [
            {"selector": 1, "family": 1, "entry": "80019A14"},
            {"selector": 8, "family": 3, "entry": "8001A71C"},
        ])

    def test_report_refuses_an_unrounded_heap_or_late_descriptor_return(self):
        self.assertEqual(init.decode(REPORT), self.data["cases"])
        for changed in (REPORT.replace("56 64", "56 56", 1), REPORT.replace("8 8 3", "8 8 4", 1)):
            with self.assertRaises(ValueError):
                init.decode(changed)
        self.assertTrue(all(r["class_2_and_descriptor_return_visit"] == 3 for r in self.cases.values()))

    def test_output_is_not_overwritten(self):
        with TemporaryDirectory() as tmp:
            dest = Path(tmp) / "init.json"
            dest.write_text("keep")
            with self.assertRaises(FileExistsError):
                init.export(None, None, None, dest)
            self.assertEqual(dest.read_text(), "keep")
