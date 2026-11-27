"""Do not merge the three type-32 key domains or their collision ownership."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type32_roster as roster


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type32-roster.json"


class RetailType32RosterTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())

    def test_three_domains_cover_selectors_without_inventing_action_joins(self):
        self.assertEqual(self.data["counts"], {
            "factory_domains": 3, "factory_rows": 31, "direct_call_sites": 24, "resolved_key_sites": 24,
            "installer_profiles_examined": 206, "candidate_rows": 13, "candidate_models": 12,
            "joined_sites": 8, "unjoined_sites": 16, "literal_key_sites": 22, "model_guarded_key_sites": 2,
            "queue_statuses": {"uniform-positive-power": 13},
        })
        domains = {d["factory"]: (d["stride"], d["rows"], d["class"]) for d in self.data["domains"]}
        self.assertEqual(domains, {"800E19A8": (32, 20, 3), "801DEB68": (40, 10, 3), "80165184": (24, 1, 2)})
        self.assertEqual(self.data["rows_without_resolved_call_sources"],
                         [{"factory": "800E19A8", "key": k} for k in (3, 5, 7, 8, 9)])
        calls = {r["site"]: r for r in self.data["call_sites"]}
        joined = {r["site"] for r in self.data["candidates"]}
        self.assertEqual({pc for pc, row in calls.items() if row["installed_action_join"]}, joined)
        for site, model, dflt, eq in (("800B9450", "050F", 0, 7), ("80109798", "0510", 5, 6)):
            self.assertEqual(calls[site]["key_source"],
                             {"kind": "model-guard", "model": model, "otherwise_key": dflt, "equal_key": eq})
            self.assertNotIn(site, joined)
        self.assertNotIn("80160E20", joined)

    def test_normal_saucer_binds_the_carrier_not_a_segment(self):
        rows = {(r["factory"], r["key"]): r for r in self.data["factory_rows"]}
        self.assertEqual(rows["801DEB68", 8]["collision_path"],
                         {"kind": "carrier-opening-bind", "site": "801DED2C", "selector": 1})
        self.assertEqual(rows["801DEB68", 9]["collision_path"],
                         {"kind": "type-11-child-request", "site": "801DF2B0", "selector": 1})
        self.assertEqual(rows["800E19A8", 16]["collision_path"],
                         {"kind": "type-11-child-request", "site": "800E20C0", "selector": 1})
        self.assertEqual(rows["80165184", 0]["collision_path"]["kind"], "unqualified")
        self.assertEqual(rows["800E19A8", 1]["source_socket_offset"], [0, 25, 350])

    def test_same_child_selector_does_not_make_actor_queues_equal(self):
        actions = {(r["borg"], r["action"]): r for r in self.data["candidates"]}
        for model, key, power in (("0d03", 8, 30), ("0505", 16, 4), ("0511", 19, 2)):
            row = actions[f"gg4e08:{model}", "X"]
            self.assertEqual(row["key"], key)
            self.assertEqual(row["collision_path"]["selector"], 1)
            self.assertEqual(row["normal_group_queue"]["steps"][0]["raw_power"], power)
        self.assertEqual(actions["gg4e08:0615", "B-charge"]["key"], 15)
        self.assertEqual(actions["gg4e08:0629", "B-charge"]["key"], 15)
        self.assertEqual(actions["gg4e08:062a", "X-charge"]["key"], 17)
        self.assertNotIn(("gg4e08:062a", "B-charge"), actions)
        self.assertEqual(actions["gg4e08:0e02", "B-close"]["site"], actions["gg4e08:0e02", "B-shot"]["site"])

    def test_existing_output_is_not_overwritten(self):
        with TemporaryDirectory() as tmp:
            out = Path(tmp) / "roster.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                roster.export_census(None, None, None, None, out, roster.recover)
            self.assertEqual(out.read_text(), "keep")
