"""Protect the original defeat-to-effect shutdown evidence and its limits."""
from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_replacement_heal_exit as exit_probe


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailReplacementHealExitTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-replacement-heal-exit.json").read_bytes())
        cls.cases = {(r["previous_ko_source"], r["case"]): r for r in cls.report["cases"]}

    def test_original_amount_and_defeat_source(self):
        prior = MAPS / "gg4e08-replacement-heal-effect.json"
        self.assertEqual(self.report["replacement_heal_effect_sha256"], sha256(prior.read_bytes()).hexdigest())
        self.assertEqual((self.report["heal_visit"], self.report["tau_probe_visit"]), (138, 139))
        self.assertEqual((self.report["tau_amount_record"], self.report["zero_hp_actor_entry"],
                          self.report["original_owner_clear_site"]), ("80377AD8", "8005BBC0", "8005BC24"))
        self.assertEqual(len(self.cases), 4)
        self.assertEqual(sum(r["completed_actor_and_effect_visits"] for r in self.cases.values()), 48)

    def test_hp_writer_does_not_clear_the_effect_before_the_defeat_handler(self):
        for team in ("gg4e08:0106", "gg4e08:0107"):
            live, defeated = [self.cases[team, k] for k in ("retained-healed-hp", "declared-low-hp")]
            self.assertEqual((live["hp_requested"], live["hp_removed"], live["hp_after"]), (16, 16, 144))
            self.assertEqual((defeated["hp_requested"], defeated["hp_removed"], defeated["hp_after"]), (15, 7, 0))
            self.assertEqual((live["post_hp_handler"], defeated["post_hp_handler"]), ("8005C290", "8005BBC0"))
            self.assertEqual((live["effect_owner_8c4_at_handler_entry"], defeated["effect_owner_8c4_at_handler_entry"]),
                             (2, 2))
            self.assertEqual((live["effect_owner_8c4_after_handler"], defeated["effect_owner_8c4_after_handler"]), (2, 0))
            self.assertEqual((live["phase_after_handler"], defeated["phase_after_handler"]), (1, 2))
            self.assertEqual((defeated["receiver_damage_ledger_after"], defeated["tau_opponent_credit"]), (31, 15))
            self.assertEqual(defeated["accepted_tokens"], [3, 4])

    def test_effect_does_not_return_resources_on_the_knockout_visit(self):
        for team in ("gg4e08:0106", "gg4e08:0107"):
            live = self.cases[team, "retained-healed-hp"]
            dead = self.cases[team, "declared-low-hp"]
            self.assertIsNone(live["effect_retired_visit"])
            self.assertTrue(all(s["effect_alive"] and s["receiver_counts_ordinary_and_alternate"] == [9, 9]
                                for s in live["samples"]))
            self.assertEqual((dead["role_mask_after_handler"], dead["effect_retired_visit"]), ("0443", 20))
            samples = {s["followup_visit"]: s for s in dead["samples"]}
            self.assertEqual([samples[n]["effect_phase_field"] for n in (1, 18, 19, 20)], [2, 2, 3, 3])
            self.assertTrue(samples[19]["effect_alive"])
            self.assertFalse(samples[20]["effect_alive"])
            self.assertEqual(samples[20]["free_kind2_and_view_cells"], [1, 1])
            self.assertTrue(all(s["receiver_counts_ordinary_and_alternate"] == [0, 0] for s in samples.values()))

    def test_refuses_malformed_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            exit_probe.decode("")
        with self.assertRaisesRegex(ValueError, "changed at row 1"):
            exit_probe.decode("heal-exit-inlet: invalid\n"*26)
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "exit.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                exit_probe.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
