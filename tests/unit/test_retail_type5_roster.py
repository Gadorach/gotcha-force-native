"""Preserve common area records independently of their carrier ancestors."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type5_roster as area
from tools.analyze.retail_projectile_census import encode, export_census


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type5-roster.json"


class RetailType5RosterTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())

    def test_distinct_area_keys_and_r5_call_proofs(self):
        data = self.data
        self.assertEqual(data["source_dol_sha256"], area.SOURCE_SHA256)
        self.assertEqual(data["source_afs_sha256"], area.AFS_SHA256)
        self.assertEqual(data["counts"], {
            "factory_rows": 27, "common_groups": 21, "direct_call_sites": 45,
            "literal_site_keys": 30, "indexed_parent_sites": 13, "unassigned_runtime_sites": 2,
            "resolved_area_keys": 26, "candidate_rows": 15, "known_ancestor_models": 14,
        })
        self.assertEqual(data["area_keys_not_resolved_to_any_site"], [16])
        calls = {r["site"]: r for r in data["call_sites"]}
        self.assertEqual(calls["801AE790"]["factory"], "80085F28")
        self.assertEqual(calls["801AE790"]["key_source"], {"kind": "literal-r5", "key": 17})
        self.assertEqual({s for s, r in calls.items() if r["key_source"]["kind"] == "runtime-byte-unassigned"},
                         {"8016D30C", "8016D32C"})
        self.assertEqual(calls["800E0AAC"]["key_source"]["eligible_parent_keys"], [3])
        self.assertEqual(calls["800E0AAC"]["key_source"]["possible_area_keys"], [20])
        self.assertEqual(calls["8007E284"]["key_source"]["area_key_by_parent_key"],
                         [0, 0, 0, 0, 13, 0, None, None, None, None])
        self.assertEqual(encode(data), ARTIFACT.read_bytes())

    def test_same_shape_radius_is_not_same_power_or_growth_row(self):
        data = self.data
        groups = {r["group"]: r for r in data["common_bank"]["groups"]}
        self.assertEqual({r["raw_radius"] for r in groups.values()}, {310})
        self.assertEqual({r["raw_power"] for r in groups.values()}, {20, 30, 40, 100})
        self.assertEqual((groups[3]["reaction_record"], groups[3]["raw_power"]), ("802C4778", 20))
        self.assertEqual(groups[60]["raw_power"], 40)
        self.assertEqual(groups[177]["raw_power"], 100)
        rows = {r["key"]: r for r in data["factory_rows"]}
        self.assertEqual({rows[key]["common_group"] for key in (3, 21, 22, 23, 24, 25)}, {60})
        self.assertEqual(len({rows[key]["scale_cap"] for key in (3, 21, 22, 23, 24, 25)}), 4)

    def test_no_direct_area_cast_is_inferred_from_later_r5_key(self):
        parents = self.data["candidates"]
        self.assertNotIn("gg4e08:0101", {r["borg"] for r in parents})
        roach = next(r for r in parents if r["borg"] == "gg4e08:0f05")
        self.assertEqual(roach["parent"], {"constructor": "801FAD10", "site": "801E7FBC",
                                           "key": 0, "debit": "801E7FA8"})
        self.assertEqual((roach["descendant"]["key"], roach["descendant"]["sites"]), (26, ["801FB294"]))
        conditional = next(r for r in parents if r["borg"] == "gg4e08:0003" and r["parent"]["key"] == 2)
        self.assertIn("not qualified", conditional["ancestor_scope"])
        self.assertEqual(conditional["descendant"]["key"], 0)
        time, remote = (next(r for r in parents if r["borg"] == f"gg4e08:{m:04x}") for m in (0x106, 0x107))
        self.assertEqual((time["descendant"]["key"], remote["descendant"]["key"]), (19, 20))
        self.assertNotIn("800E0AAC", time["descendant"]["sites"])
        self.assertIn("800E0AAC", remote["descendant"]["sites"])

    def test_existing_output_remains_unmodified(self):
        with TemporaryDirectory() as tmp:
            output = Path(tmp) / "receipt.json"
            output.write_text("keep")
            with self.assertRaises(FileExistsError):
                export_census(None, None, None, None, output, area.recover)
            self.assertEqual(output.read_text(), "keep")
