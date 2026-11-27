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
            "literal_site_keys": 30, "indexed_parent_sites": 13, "cached_parent_sites": 2,
            "resolved_area_keys": 27, "candidate_rows": 30, "known_ancestor_models": 24,
        })
        self.assertEqual(data["area_keys_not_resolved_to_any_site"], [])
        calls = {r["site"]: r for r in data["call_sites"]}
        self.assertEqual(calls["801AE790"]["factory"], "80085F28")
        self.assertEqual(calls["801AE790"]["key_source"], {"kind": "literal-r5", "key": 17})
        self.assertEqual({s for s, r in calls.items() if r["key_source"]["kind"] == "cached-parent-byte"},
                         {"8016D30C", "8016D32C"})
        for site in ("8016D30C", "8016D32C"):
            source = calls[site]["key_source"]
            self.assertEqual((source["bounded_parent_rows"], source["possible_area_keys"], source["write_site"]),
                             (17, [16], "8016CE48"))
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
        time, remote = (next(r for r in parents if r["borg"] == f"gg4e08:{m:04x}" and r["parent"].get("family") == "type-41")
                        for m in (0x106, 0x107))
        self.assertEqual((time["descendant"]["key"], remote["descendant"]["key"]), (19, 20))
        self.assertNotIn("800E0AAC", time["descendant"]["sites"])
        self.assertIn("800E0AAC", remote["descendant"]["sites"])
        nu, black = (next(r for r in parents if r["borg"] == borg) for borg in ("gg4e08:0f03", "gg4e08:062a"))
        self.assertEqual((nu["parent"]["key"], black["parent"]["key"]), (11, 16))
        self.assertEqual([r["descendant"]["key"] for r in (nu, black)], [16, 16])

    def test_floor_sweeps_do_not_turn_periodic_segments_into_areas(self):
        rows = [r for r in self.data["candidates"] if r["borg"] in {"gg4e08:0e00", "gg4e08:0e03", "gg4e08:0e05"}]
        self.assertEqual(len(rows), 6)
        for row in rows:
            if row["borg"] == "gg4e08:0e03":
                self.assertIn(row["action"], ("B-shot", "B-close"))
                self.assertEqual(row["descendant"]["sites"], ["801F3D50"])
            else:
                self.assertIn(row["action"], ("X", "X-charge"))
                self.assertEqual(row["descendant"]["sites"], ["801F3274"])
            self.assertEqual(row["descendant"]["key"], 26)
            self.assertIn("terrain-crossing", row["descendant"]["scope"])
        self.assertEqual(self.data["sweep_census_sha256"], area.SWEEP_CENSUS_SHA256)

    def test_sibling_area_ancestry_preserves_type77_root_exception(self):
        rows = [r for r in self.data["candidates"] if "branches" in r["descendant"]]
        self.assertEqual(len(rows), 7)
        expected = {"gg4e08:0106": (33, 19), "gg4e08:0601": (52, 11), "gg4e08:0617": (52, 11),
                    "gg4e08:0d00": (74, 6), "gg4e08:0d01": (77, 7), "gg4e08:0d05": (77, 7)}
        self.assertEqual({r["borg"] for r in rows}, expected.keys())
        for row in rows:
            kind, key = expected[row["borg"]]
            self.assertEqual((row["parent"]["type"], row["descendant"]["key"]), (kind, key))
            hit, stage = row["descendant"]["branches"]
            self.assertEqual((hit["position_offset"], stage["position_offset"]), ("0x20" if kind == 77 else "0x2A0", "0x20"))
            self.assertEqual([hit["site"], stage["site"]], row["descendant"]["sites"])
        self.assertEqual(self.data["type4_census_sha256"], area.TYPE4_CENSUS_SHA256)

    def test_existing_output_remains_unmodified(self):
        with TemporaryDirectory() as tmp:
            output = Path(tmp) / "receipt.json"
            output.write_text("keep")
            with self.assertRaises(FileExistsError):
                export_census(None, None, None, None, output, area.recover)
            self.assertEqual(output.read_text(), "keep")
