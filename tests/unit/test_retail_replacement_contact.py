"""Pin the joined replacement guard, exact HP paths, and post-hit list closure."""
from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_replacement_contact as contact


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailReplacementContactTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-replacement-contact.json").read_bytes())
        cls.rows = cls.report["cases"]

    def test_receipt_scope(self):
        for name, digest in self.report["prior_receipts"].items():
            self.assertEqual(digest, sha256((MAPS / f"gg4e08-{name}.json").read_bytes()).hexdigest())
        self.assertEqual(len(self.rows), 60)
        self.assertEqual(sum(r["accepted_token"] is not None for r in self.rows), 36)
        self.assertEqual((self.report["first_alternate_receiving_visit"],
                          self.report["first_ordinary_receiving_visit"]), (81, 139))
        self.assertTrue(all(r["repeat_hp_writes"] == 0 for r in self.rows))

    def test_original_role_mask_blocks_both_and_positive_guard_blocks_only_ordinary(self):
        for row in self.rows:
            visit = row["replacement_visit"]
            if visit == 80:
                self.assertEqual(row["receiving_cells_before"], [0, 0])
                self.assertIsNone(row["signed_hp_request"])
                continue
            guarded = visit < 139
            self.assertEqual(row["receiving_cells_before"], [0 if guarded else 9, 9])
            self.assertEqual(row["signed_hp_request"] is None, guarded and row["pair_list"] == "ordinary")
            self.assertEqual(row["ordinary_guard_before_and_after"], dict((v, g) for v, _, g in contact.VISITS)[visit])

    def test_nurse_caps_healing_then_closes_both_lists_without_ordinary_guard_spend(self):
        for row in self.rows:
            if row["probe_borg"] != "gg4e08:0900" or row["replacement_visit"] == 80:
                continue
            ally = row["relation"] == "ally"
            self.assertEqual((row["signed_hp_request"], row["hp_restored"], row["amount_calculator_calls"]),
                             (-200 if ally else -50, 110 if ally else 50, 0))
            self.assertEqual((row["role_mask_before"], row["role_mask_after"]), ("0000", "0043"))
            self.assertEqual(row["receiving_cells_on_next_rebuild"], [0, 0])
            self.assertEqual(row["probe_opponent_and_ally_damage_credit"], [0, 0])
            self.assertEqual(row["replacement_received_damage_ledger"], 16)

    def test_patra_and_tau_keep_positive_damage_ledgers_separate_from_publication(self):
        for row in self.rows:
            if row["probe_borg"] == "gg4e08:0900" or row["signed_hp_request"] is None:
                continue
            patra, ally = row["probe_borg"] == "gg4e08:0902", row["relation"] == "ally"
            expected = (2 if ally else 9) if patra else (4 if ally else 17)
            self.assertEqual((row["signed_hp_request"], row["hp_removed"], row["hp_after"]),
                             (expected, expected, 110-expected))
            self.assertEqual(row["replacement_received_damage_ledger"], 16+expected)
            self.assertEqual(row["probe_opponent_and_ally_damage_credit"], [0, expected] if ally else [expected, 0])
            self.assertEqual(row["receiving_cells_on_next_rebuild"], row["receiving_cells_before"])

    def test_refuses_changed_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            contact.decode("")
        with self.assertRaisesRegex(ValueError, "changed at 6/80"):
            contact.decode("entry-contact: invalid\n"*60)
        with TemporaryDirectory() as tmp:
            output = Path(tmp) / "contact.json"
            output.write_text("keep")
            with self.assertRaises(FileExistsError):
                contact.export(None, None, None, None, output)
            self.assertEqual(output.read_text(), "keep")
