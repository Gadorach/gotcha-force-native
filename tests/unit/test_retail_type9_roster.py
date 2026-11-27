"""Protect the type-9 source keys and the boundary between admission and HP."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type9_roster as type9


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type9-roster.json"
REPORT = "\n".join(f"type9-bind: {key} {spare} 84 {spare} {1 if spare else 2} {122 if spare else 0} 0"
                   for key in range(7) for spare in (0, 1))


class RetailType9RosterTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())

    def test_source_keys_do_not_imply_borg_assignments(self):
        sites = {r["site"]: r["key_source"] for r in self.data["call_sites"]}
        self.assertEqual(len(sites), 8)
        self.assertEqual(self.data["counts"]["type8_call_sites"], 3)
        self.assertEqual(self.data["counts"]["type9_call_sites"], 5)
        for site in ("800FC428", "800FC678"):
            self.assertEqual(sites[site]["kind"], "signed-type8-object-key")
            self.assertEqual(sites[site]["possible_keys_through_reviewed_type8_calls"], [0, 5, 6])
        self.assertEqual(sites["800FD314"]["eligible_rows"], [0, 2])
        self.assertEqual(sites["800FD314"]["possible_keys"], [0])
        self.assertEqual(sites["800FD324"]["eligible_rows"], [1, 3, 4, 5])
        self.assertEqual(sites["800FD324"]["possible_keys"], [1])
        self.assertEqual(sites["801DF69C"]["type32_borrowed_rows"], [0, 7])
        self.assertEqual(sites["801DF69C"]["possible_keys"], [3])
        self.assertEqual(sites["801DF8CC"]["type32_borrowed_rows"], [8])
        self.assertEqual(self.data["rows_without_resolved_type9_call_keys"], [2])
        self.assertEqual([(r["borg"], r["action"], r["parent_key"], r["descendant_key"]) for r in self.data["candidates"]],
                         [("gg4e08:0d03", "X", 8, 4)])

    def test_one_common_shape_and_zero_raw_power_do_not_prove_contact(self):
        group = self.data["common_collision"]
        self.assertEqual((group["group"], group["descriptor"], group["raw_radius"]), (84, 122, 150))
        self.assertEqual((group["amount_record"], group["raw_power"]), ("802C4760", 0))
        self.assertEqual(self.data["cases"], type9.decode_report(REPORT))
        for refused, bound in zip(self.data["cases"][::2], self.data["cases"][1::2], strict=True):
            self.assertTrue(refused["allocated"])
            self.assertIsNone(refused["bound_descriptor"])
            self.assertEqual(refused["phase_at_cut"], 2)
            self.assertEqual(bound["bound_descriptor"], 122)
            self.assertEqual(bound["phase_at_cut"], 1)
            self.assertEqual(bound["attack_registry_count_at_cut"], 0)
            self.assertNotIn("hp_loss", bound)

    def test_decoder_refuses_refused_shape_or_registry_changes(self):
        for old, new in (("4 0 84 0 2 0 0", "4 0 84 1 1 122 0"),
                         ("4 1 84 1 1 122 0", "4 1 84 1 1 122 1"),
                         ("4 1 84 1 1 122 0", "4 1 3 1 1 4 0")):
            self.assertIn(old, REPORT)
            with self.assertRaises(ValueError):
                type9.decode_report(REPORT.replace(old, new, 1))

    def test_existing_output_is_not_replaced(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "receipt.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                type9.export(None, None, None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
