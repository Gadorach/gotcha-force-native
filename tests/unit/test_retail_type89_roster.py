"""Keep type-89 selector coverage separate from qualified action joins."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type89_roster as roster


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type89-roster.json"


class RetailType89RosterTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())

    def test_all_factory_keys_and_edges_are_accounted_for(self):
        self.assertEqual(self.data["counts"], {
            "roster_profiles_examined": 206, "candidate_models": 12, "candidate_rows": 36,
            "direct_call_sites": 11, "literal_sites": 9, "indexed_sites": 2, "factory_rows": 17,
            "unjoined_sites": 4, "queue_statuses": {"uniform-positive-power": 36},
        })
        rows = self.data["factory_rows"]
        self.assertEqual([r["key"] for r in rows], list(range(17)))
        self.assertEqual({r["area_key"] for r in rows}, {16})
        self.assertEqual([r["key"] for r in rows if r["phase_duration_units"] == [25, 4, 10]], [1, 15])
        self.assertNotEqual(rows[3]["timer_program"], rows[4]["timer_program"])
        self.assertEqual(rows[3]["phase_duration_units"], rows[4]["phase_duration_units"])

    def test_first_pair_is_one_step_not_two_timed_steps(self):
        for model, charged in (("0503", True), ("0506", False), ("050f", True), ("0512", False)):
            rows = [r for r in self.data["candidates"] if r["borg"] == f"gg4e08:{model}"]
            self.assertEqual({(r["action"], r["action_state"]) for r in rows},
                             {("B-charge" if charged else "B-shot", 3)})
            self.assertEqual([r["key"] for r in rows], ([9, 10] if charged else []) + [3, 6, 4, 7, 5, 8])
            self.assertEqual([r["cursor"] for r in rows], ([0] if charged else []) + list(range(7 if charged else 6)))
            self.assertEqual([r["site"] for r in rows if r["order_at_cursor"] == 2],
                             ["800B8E9C"] if charged else [])
            self.assertTrue(all("same_leaf_debit" not in r for r in rows))
        self.assertEqual([b["first_negative_cursor"] for b in self.data["pair_selector"]["banks"]], [7, 6])

    def test_model_guards_do_not_resolve_unreached_lanes_or_siblings(self):
        calls = {r["site"]: r for r in self.data["call_sites"]}
        self.assertEqual({s: row["unjoined"]["required_model"] for s, row in calls.items() if "unjoined" in row},
                         {"800CBA78": "0600", "800CBA90": "0608", "800CBAA8": "0616", "800CBAC0": "061C"})
        candidates = self.data["candidates"]
        self.assertFalse({r["site"] for r in candidates} & {s for s, row in calls.items() if "unjoined" in row})
        self.assertFalse({r["borg"] for r in candidates} & {"gg4e08:0302", "gg4e08:0308", "gg4e08:0f01", "gg4e08:0f02"})
        self.assertEqual({r["borg"]: r["key"] for r in candidates if r["site"] in ("80134748", "80108564")},
                         {"gg4e08:0304": 1, "gg4e08:0306": 15})
        self.assertTrue(all("damage" not in r and "hp_delta" not in r for r in candidates))

    def test_existing_output_is_not_overwritten(self):
        with TemporaryDirectory() as tmp:
            out = Path(tmp) / "roster.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                roster.export_census(None, None, None, None, out, roster.recover)
            self.assertEqual(out.read_text(), "keep")
