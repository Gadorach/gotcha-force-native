"""Keep type-11 child selectors separate from parent keys and raw damage."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_cannon_beam as cannon, retail_type11_roster as roster


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type11-roster.json"


class RetailType11RosterTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())

    def test_all_six_sites_use_parent_operands_and_three_endpoint_layouts(self):
        self.assertEqual(self.data["counts"], {
            "direct_segment_sites": 6, "resolved_selector_sites": 6, "candidate_rows": 21, "candidate_models": 16,
            "queue_statuses": {"uniform-positive-power": 21},
        })
        sites = {r["site"]: r for r in self.data["call_sites"]}
        self.assertEqual({tuple(r["endpoint_object_offsets"]) for r in sites.values()},
                         {("0x38", "0x9C"), ("0x174", "0x180"), ("0x150", "0x15C")})
        self.assertIsNone(sites["801DF2B0"]["selector_source"]["group_by_index"][8])
        self.assertNotIn("gg4e08:0d03", {r["borg"] for r in self.data["candidates"]})
        parent = sites["801F33A4"]
        self.assertIsNone(parent["parent_type"])
        self.assertEqual(parent["selector_source"]["row_index_object_offset"], "0x13")
        self.assertEqual(parent["selector_source"]["excluded_zero_rows"], list(range(8, 30)))

    def test_new_actor_joins_preserve_model_phase_and_non_damage_boundaries(self):
        cannon_rows = [r for r in self.data["candidates"] if r["borg"] == "gg4e08:0c04"]
        self.assertEqual({r["substate"] for r in cannon_rows}, {0, 1})
        self.assertEqual({r["parent_call"] for r in cannon_rows}, {f"{site:08X}" for site in cannon.CARRIER_SITES})
        for r in cannon_rows:
            self.assertEqual((r["action"], r["parent_row_indices"], r["segment_key"]), ("X-charge", [0], 2))
            self.assertEqual(r["normal_queue"]["steps"], [{"descriptor": 3, "raw_power": 5000, "reaction_index": 2}])
            self.assertNotIn("paid_arm", r)
            self.assertNotIn("hp_loss", r)
        narrow_gate = [r for r in self.data["candidates"] if r["segment_site"] == "8018F384"]
        self.assertEqual([r["borg"] for r in narrow_gate], ["gg4e08:0704"])
        self.assertEqual(narrow_gate[0]["segment_key"], 8)
        self.assertEqual(len(narrow_gate[0]["normal_queue"]["steps"]), 4)

    def test_nonzero_rows_and_negative_pair_suppressions_remain_distinct(self):
        rows = [r for r in self.data["candidates"] if r["segment_site"] == "801F33A4"]
        self.assertEqual({r["borg"] for r in rows}, {"gg4e08:0e00", "gg4e08:0e05"})
        self.assertTrue(all(r["parent_row_indices"] == list(range(8)) and r["segment_key"] == 1 for r in rows))
        for row in (r for r in rows if r["action"] == "X-charge"):
            self.assertEqual(row["parent_row_source"]["signed_parent_row_pairs"],
                             [[4, -1], [0, 1], [5, -1], [2, 3], [6, 7]]*2)
        even = [r for r in self.data["candidates"] if r["segment_site"] == "801F3E80"]
        self.assertEqual({(r["borg"], r["action"], r["segment_key"]) for r in even},
                         {("gg4e08:0e03", "B-shot", 2), ("gg4e08:0e03", "B-close", 2)})

    def test_existing_output_is_not_overwritten(self):
        with TemporaryDirectory() as tmp:
            out = Path(tmp) / "roster.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                roster.export_census(None, None, None, None, out, roster.recover)
            self.assertEqual(out.read_text(), "keep")
