"""Relationship admission is independent of HP, token history and carrier lifetime."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_contact_relationship as rel


RECEIPT = Path(__file__).resolve().parents[2]/"docs/api-map/gg4e08-contact-relationship.json"


class ContactRelationshipTest(unittest.TestCase):
    def test_stock_tether_admits_opening_and_refuses_followup(self):
        report = json.loads(RECEIPT.read_bytes())
        for tether in report["bounded_type49"]:
            opening = tether["opening"]["cases"][1]
            follow = tether["followup"]["cases"][1]
            self.assertEqual((tether["opening"]["attack_rule_flags"], tether["followup"]["attack_rule_flags"]),
                             ("0000", "00A1"))
            self.assertTrue(opening["hp_writer_reached"])
            self.assertEqual(opening["requested_and_removed_hp"], 0)
            self.assertTrue(follow["broad_overlap"])
            self.assertFalse(follow["relationship_admitted"])
            self.assertEqual(follow["narrow_attempts"], 0)
            self.assertFalse(follow["hp_writer_reached"])
            self.assertEqual(follow["receiver_history_first_two_slots"], [1, 0])
            self.assertEqual(tether["followup"]["cases"][0]["receiver_history_first_two_slots"], [1, 2])
            self.assertFalse(tether["followup_paid_again"])

    def test_ally_hits_do_not_change_the_ordinary_carrier_policy(self):
        report = json.loads(RECEIPT.read_bytes())
        for row, amounts in zip(report["bounded_ordinary_projectiles"], [(8, 2)] + [(17, 4)]*3, strict=True):
            enemy, ally = row["cases"]
            self.assertEqual((enemy["opponent_credit"], ally["ally_credit"]), amounts)
            self.assertEqual((enemy["ally_credit"], ally["opponent_credit"]), (0, 0))
            self.assertEqual(enemy["first_carrier_followup"], ally["first_carrier_followup"])
            self.assertEqual(ally["first_carrier_followup"]["stopped"], row["model"] != "0600")
        self.assertFalse(report["native_policy_changed"])

    def test_partial_report_and_existing_output_refuse(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            rel.decode("relation: 0501 0 10\n")
        with TemporaryDirectory() as tmp:
            output = Path(tmp)/"already.json"
            output.write_text("keep")
            with self.assertRaises(FileExistsError):
                rel.export(None, None, None, None, output)
            self.assertEqual(output.read_text(), "keep")
