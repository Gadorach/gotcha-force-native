import unittest
from pathlib import Path

from tools.extract import borg_api_map as api

ROSTER = Path("local/retail-roster-dive-v17/borgs")


class BorgApiMapTest(unittest.TestCase):
    def test_operands_select_the_bound_record_and_keep_labelled_values(self):
        summary = {"movement_operands": {
            "class_installer_binding": {"selected_record": 1},
            "records": [{"record_sha256": "a", "operands": {"004": {"value": 1}}},
                        {"record_sha256": "b", "operands": {"02C": {"value": 10.0}}}]}}
        self.assertEqual(api._operands(summary),
                         {"record": 1, "record_sha256": "b", "values": {"02C": 10.0}})

    def test_operands_refuse_an_unbound_record(self):
        self.assertIsNone(api._operands({"movement_operands": {"class_installer_binding": {}}}))

    def test_actions_keep_only_the_neutral_rows(self):
        summary = {"action_candidates": {"profiles": [{
            "config": "1", "context_table": "2", "main_handler": "3",
            "groups": [{"group": "B-shot", "rows": [
                {"state": 0, "substate": 0, "selector": 44, "handler_pair": [61, 0],
                 "class_callback_candidate": "AA", "contexts": [[0, 0], [1, 1]]},
                {"state": 1, "substate": 2, "selector": 45, "handler_pair": [61, 0],
                 "class_callback_candidate": None, "contexts": [[3, 0]]}]}]}]}}
        groups = api._actions(summary)
        self.assertEqual(len(groups), 1)
        self.assertEqual(groups[0]["group"], "B-shot")
        self.assertEqual([row["selector"] for row in groups[0]["neutral_rows"]], [44])

    def test_capability_prefers_the_structured_sources(self):
        self.assertEqual(api._capability({"flying_field_source": {}}), "flying-field")
        self.assertEqual(api._capability({"staged_field_source": {}}), "staged-field")
        self.assertEqual(api._capability({"basic_melee_battle_test": {}}), "basic-melee")
        self.assertEqual(api._capability(
            {"basic_shot_battle_test": {"kind": "authored shot"}}), "authored shot")
        self.assertIsNone(api._capability({}))

    @unittest.skipUnless(ROSTER.is_dir(), "local roster export")
    def test_map_states_that_it_carries_no_asset_bytes(self):
        data = api.build(ROSTER)
        self.assertGreaterEqual(data["borg_count"], 206)
        self.assertIn("no model, texture or audio bytes", data["kind"])
        entry = next(item for item in data["borgs"] if item["id"] == "gg4e08:0e01")
        self.assertEqual(entry["movement"]["operands"]["values"]["004"], 10000)
        self.assertEqual(entry["capability"], "flying-field")
        self.assertTrue(entry["actions"])


if __name__ == "__main__":
    unittest.main()
