"""Verify the receipt distinguishes actual terminal order from declared controls."""
from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_scene_bomb_terminal as terminal


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailSceneBombTerminalTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-scene-bomb-terminal.json").read_bytes())
        cls.cases = {r["entry_hold_visits"]: r for r in cls.report["cases"]}

    def test_exact_zero_hp_and_mixed_inlets(self):
        prior = MAPS / "gg4e08-scene-bomb-cascade.json"
        self.assertEqual(self.report["scene_bomb_cascade_sha256"], sha256(prior.read_bytes()).hexdigest())
        for delay, latch, site in ((0, 0, terminal.ZERO_HP), (10, 16, terminal.OUTGOING)):
            row = self.cases[delay]
            observed = row["controls"][0]
            self.assertEqual((row["second_press_visit"], row["second_bomb_token"],
                              row["recorded_second_bomb_hp"], row["recorded_outgoing_latch"]), (185, 3, 0, latch))
            self.assertEqual(observed["changed_operands"], [])
            self.assertEqual((observed["request_site"], observed["request_key"]), (site, 19))
            self.assertEqual(row["second_press_hp_writes_in_order"][-2]["hp_removed"], 1)

    def test_operand_controls_do_not_masquerade_as_retained_paths(self):
        controls = self.cases[10]["controls"]
        self.assertEqual([r["case"] for r in controls], list(terminal.RULES))
        self.assertEqual([r["changed_operands"] for r in controls], [
            [], ["outgoing_latch"], ["carrier_hp"], ["outgoing_latch", "carrier_hp"],
        ])
        self.assertEqual([(r["outgoing_latch_at_inlet"], r["carrier_hp_at_inlet"], r["request_site"])
                          for r in controls], [
            (16, 0, terminal.OUTGOING), (0, 0, terminal.ZERO_HP), (16, 1, terminal.OUTGOING), (0, 1, None),
        ])
        neither = controls[-1]
        self.assertEqual(neither["incoming_latch_at_inlet"], 1)
        self.assertIsNone(neither["stock_return_visit"])
        sample, = neither["samples"]
        self.assertTrue(sample["second_bomb_active"])
        self.assertEqual((sample["second_bomb_phase_field"], sample["second_bomb_lifetime_field"], sample["stock"]),
                         (1, 179, 9))

    def test_exit_sites_converge_on_same_observed_followup_but_preserve_actor_tier(self):
        mixed = self.cases[10]["controls"][:3]
        self.assertTrue(all(r["samples"] == mixed[0]["samples"] for r in mixed[1:]))
        for delay, damage, tier in ((0, 17, 0), (10, 14, 1)):
            for row in self.cases[delay]["controls"]:
                if row["request_site"] is None:
                    continue
                bind, publish = row["samples"][:2]
                self.assertTrue(bind["new_area_shape_bound"])
                self.assertFalse(bind["new_area_shape_published"])
                self.assertTrue(publish["new_area_shape_published"])
                self.assertEqual(row["stock_return_visit"], 187)
                self.assertEqual([(r["receiver_seat"], r["source_token"], r["hp_removed"], r["receiver_tier_at_amount"])
                                  for r in publish["hp_writes_in_order"]], [(0, 4, damage, tier), (1, 4, 4, 0)])
        self.assertEqual(sum(len(r["samples"]) for c in self.cases.values() for r in c["controls"]), 21)

    def test_refuses_malformed_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            terminal.decode("terminal-inlet: 0\n")
        with self.assertRaisesRegex(ValueError, "changed at row 1"):
            terminal.decode("terminal-inlet: invalid\n"*43)
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "terminal.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                terminal.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
