"""Keep guard expiry, the pose clock, and active B admission separate."""
from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_replacement_heal_tail as tail


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailReplacementHealTailTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-replacement-heal-tail.json").read_bytes())
        cls.cases = {(r["previous_ko_source"], r["heal_on_replacement_visit"], r["declared_control"]): r
                     for r in cls.report["cases"]}

    def test_receipt_scope_and_effect_refusal(self):
        prior = MAPS / "gg4e08-replacement-contact.json"
        self.assertEqual(self.report["replacement_contact_sha256"], sha256(prior.read_bytes()).hexdigest())
        self.assertEqual(len(self.cases), 10)
        self.assertEqual(sum(r["neutral_actor_visits"] for r in self.cases.values()), 640)
        self.assertEqual(self.report["effect_request"], {
            "factory": "8010E158", "caller": "8002F6DC", "selector": 1, "free_kind2_cells": 0,
            "carrier_allocated": False,
        })

    def test_natural_guard_edge_reopens_before_or_after_pose_expiry(self):
        for earlier in ("gg4e08:0106", "gg4e08:0107"):
            for heal, boundary in ((81, 58), (138, 1)):
                row = self.cases[earlier, heal, None]
                samples = {s["actor_visits_after_heal"]: s for s in row["samples"]}
                opened, before = samples[boundary], samples[boundary-1]
                self.assertEqual((row["role_mask_cleared_after_actor_visits"], opened["ordinary_guard"]), (boundary, 0))
                self.assertEqual((before["ordinary_guard"], before["ordinary_and_alternate_receiving_counts"]), (1, [0, 0]))
                self.assertEqual(opened["ordinary_and_alternate_receiving_counts"], [9, 9])
                self.assertEqual(opened["pose_timer"], 14 if heal == 138 else 0)
                self.assertEqual(opened["secondary_rate_bits"], tail.SLOW if heal == 138 else tail.ONE)

    def test_zero_is_not_the_positive_to_zero_edge(self):
        for earlier in ("gg4e08:0106", "gg4e08:0107"):
            for heal, label in ((81, "clear-guard-before-update"), (139, None)):
                row = self.cases[earlier, heal, label]
                self.assertIsNone(row["role_mask_cleared_after_actor_visits"])
                self.assertTrue(all(s["ordinary_guard"] == 0 and s["role_mask"] == "0043" for s in row["samples"]))
                self.assertEqual(row["samples"][-1]["pose_timer"], 0)
                self.assertEqual(row["samples"][-1]["ordinary_and_alternate_receiving_counts"], [0, 0])
            edge = self.cases[earlier, 139, "declare-one-unit-guard"]
            self.assertEqual((edge["original_guard_at_heal"], edge["guard_at_start_of_followup"],
                              edge["role_mask_cleared_after_actor_visits"]), (0, 1, 1))
            self.assertEqual(edge["samples"][1]["ordinary_and_alternate_receiving_counts"], [9, 9])

    def test_actual_b_debit_before_allocation_even_when_receiving_is_closed(self):
        for row in self.cases.values():
            pulse = row["fresh_b_fork"]
            self.assertEqual((pulse["key"], pulse["pose"], pulse["stock_at_factory"]), (2, 10, 9))
            self.assertEqual(pulse["role_mask_at_factory"], "0000" if row["guard_at_start_of_followup"] == 1 else "0043")
            self.assertTrue(pulse["factory_entry_only"])
            self.assertTrue(all(s["hp"] == 160 and s["retained_accept_token"] == 3 for s in row["samples"]))

    def test_refuses_changed_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            tail.decode("")
        with self.assertRaisesRegex(ValueError, "changed at row 1"):
            tail.decode("heal-tail: invalid\n"*80)
        with TemporaryDirectory() as tmp:
            output = Path(tmp) / "tail.json"
            output.write_text("keep")
            with self.assertRaises(FileExistsError):
                tail.export(None, None, None, None, output)
            self.assertEqual(output.read_text(), "keep")
