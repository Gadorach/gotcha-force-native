"""Keep bomb HP, child HP, stock and damage-tier controls distinct."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type41_parent_contact as contact


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type41-parent-contact.json"


class RetailType41ParentContactTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rows = {(r["borg"], r["case"]): r for r in json.loads(ARTIFACT.read_bytes())["cases"]}

    def test_parent_geometry_and_child_use_distinct_tokens(self):
        for model in ("gg4e08:0106", "gg4e08:0107"):
            near = self.rows[model, "outgoing-near"]
            self.assertEqual((near["parent_shape_role"], near["first_pair_token"], near["child_token"]), (3, 1, 2))
            self.assertEqual((near["first_hp_request"], near["child_hp_lost"], near["sasuke_hp_at_final_cut"]),
                             (26, 14, 140))
            self.assertEqual(self.rows[model, "outgoing-exact-tangent"]["first_hp_request"], 26)
            self.assertIsNone(self.rows[model, "outgoing-miss"]["terminal_area_site"])

    def test_shot_can_destroy_bomb_without_fighter_hp_or_ledger_change(self):
        for model in ("gg4e08:0106", "gg4e08:0107"):
            r = self.rows[model, "incoming-lethal"]
            self.assertEqual((r["first_hp_request"], r["parent_hp_after_first_pair"], r["sasuke_hp_at_final_cut"]),
                             (17, 0, 180))
            self.assertEqual((r["parent_incoming_latch"], r["parent_outgoing_latch"]), (1, 0))
            self.assertEqual(r["first_pair_ledgers"], {"owner_opponent": 0, "owner_ally": 0, "receiver": 0})
            self.assertEqual(r["terminal_area_site"], "800E0B18")
            self.assertEqual(r["stock_after_return_visit"], r["stock_after_terminal"]+1)

    def test_tier_fork_explains_both_observed_child_amount_differences(self):
        for model in ("gg4e08:0106", "gg4e08:0107"):
            for name, parent, child, cleared in (("outgoing-near", 26, 14, 17), ("outgoing-ally", 6, 3, 4)):
                row = self.rows[model, name]
                self.assertEqual((row["first_hp_request"], row["receiver_tier_at_child"], row["child_hp_lost"],
                                  row["amount_with_only_tier_cleared"]), (parent, 1, child, cleared))

    def test_remote_command_priority_does_not_change_return_count(self):
        for name in ("outgoing-and-remote-command", "incoming-and-remote-command"):
            row = self.rows["gg4e08:0107", name]
            self.assertEqual(row["terminal_area_site"], "800E0AAC")
            self.assertEqual((row["stock_after_terminal"], row["stock_after_return_visit"]), (7, 8))

    def test_refuses_incomplete_report_and_existing_output(self):
        with self.assertRaises(ValueError):
            contact.decode("type41-parent: 0")
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "parent.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                contact.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
