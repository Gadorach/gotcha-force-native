"""Keep the common type-4 effect separate from damaging parent routes."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type4_roster as effect


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type4-roster.json"
REPORT = "\n".join(f"type4-life: {i} 9 39 40 41 2.00000 2.50000 3.12500" for i in range(4))


class RetailType4RosterTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())

    def test_static_census_does_not_assign_all_calls_to_borgs(self):
        self.assertEqual(self.data["counts"], {"direct_sites": 33, "functions": 20, "scoped_sites": 13,
                                               "candidate_rows": 15, "models": 9, "lifetime_cases": 4, "visits": 164})
        for row in self.data["call_sites"]:
            self.assertEqual((row["key"], row["position_offset"]), (0, "0x20"))
            self.assertIn(row["position_base_register"], (30, 31))
        charged = [r for r in self.data["candidates"] if r["borg"] == "gg4e08:0101"]
        self.assertEqual([r["parent"]["key"] for r in charged], [8, 9, 6, 7])
        self.assertTrue(all(r["descendant_sites"] == ["8007E298", "8007E610"] for r in charged))
        normal = [r for r in self.data["candidates"] if r["parent"]["type"] == 76]
        self.assertEqual({r["borg"] for r in normal}, {"gg4e08:0d01", "gg4e08:0d04"})
        self.assertTrue(all(r["descendant_sites"] == ["800FD144", "800FD16C"] for r in normal))

    def test_extended_joins_pin_constructor_to_later_exit_not_contact_area(self):
        chains = {r["type"]: r for r in self.data["parent_dispatch_links"]}
        self.assertEqual({kind: r["phase"] for kind, r in chains.items()}, {33: 1, 52: 1, 74: 1, 77: 1, 98: 2})
        extended = [r for r in self.data["candidates"] if r["parent"]["type"] in chains]
        self.assertEqual(len(extended), 8)
        seen = {(r["borg"], r["action"], r["parent"]["type"], tuple(r["parent"]["keys"])) for r in extended}
        self.assertEqual(seen, {
            ("gg4e08:0106", "B-shot", 33, (2,)), ("gg4e08:0106", "X", 33, (2,)),
            ("gg4e08:0601", "B-shot", 52, (0,)), ("gg4e08:0617", "B-shot", 52, (1,)),
            ("gg4e08:0d00", "X", 74, (0,)), ("gg4e08:0d01", "X", 77, (0,)),
            ("gg4e08:0d05", "X", 77, (1,)), ("gg4e08:0e03", "X", 98, (0, 1)),
        })
        type5 = json.loads((ARTIFACT.parent / "gg4e08-type5-roster.json").read_bytes())
        area_sites = {r["site"] for r in type5["call_sites"]}
        for row in extended:
            kind = row["parent"]["type"]
            self.assertEqual((row["action_phase"], row["parent"]["constructor"]), (1, chains[kind]["constructor"]))
            self.assertEqual(row["descendant_sites"], chains[kind]["type4_sites"])
            self.assertTrue(area_sites.isdisjoint(row["descendant_sites"]))
            self.assertNotIn("hp_loss", row)
            if kind == 52:
                self.assertIn("not bound", row["parent"]["selector_scope"])
            elif kind == 98:
                self.assertIn("+0x15B clear", row["scope"])

    def test_position_proof_requires_a_local_unclobbered_pointer(self):
        words = {0x100: 0x389E0020, 0x104: 0x38A00000}
        incoming = {0x104: {0x100}, 0x108: {0x104}}
        self.assertEqual(effect.position_register(words, incoming, 0x108), 30)
        with self.assertRaises(ValueError):
            effect.position_register({**words, 0x104: 0x38800000}, incoming, 0x108)
        with self.assertRaises(ValueError):
            effect.position_register(words, {**incoming, 0x108: {0x104, 0x10C}}, 0x108)

    def test_key_zero_changes_visual_stage_and_retires_without_shape_use(self):
        self.assertEqual(self.data["lifetime_cases"], effect.decode_report(REPORT))
        self.assertEqual([(r["visual_record"], r["active_visits"]) for r in self.data["factory_rows"][0]["stages"]],
                         [(67, 8), (23, 30)])
        for case in self.data["lifetime_cases"]:
            self.assertEqual(case["stage_scales"], [2.0, 2.5, 3.125])
            self.assertEqual((case["visual_switch_visit"], case["class2_and_descriptor_return_visit"]), (9, 41))
            self.assertEqual((case["collision_allocation_calls"], case["attack_registry_entries"]), (0, 0))
        self.assertFalse(self.data["lifetime_cases"][-1]["parent_active_at_init"])
        self.assertEqual(self.data["lifetime_cases"][2]["offered_shape_cells"], 0)

    def test_report_refuses_frozen_scale_or_early_retirement(self):
        for old, new in (("1 9 39 40 41", "1 9 38 39 40"),
                         ("2 9 39 40 41 2.00000", "2 9 39 40 41 0.75000"),
                         ("3 9 39 40 41 2.00000 2.50000", "3 9 39 40 41 2.00000 7.00000")):
            self.assertIn(old, REPORT)
            with self.assertRaises(ValueError):
                effect.decode_report(REPORT.replace(old, new, 1))

    def test_existing_receipt_is_preserved(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "type4.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                effect.export(None, None, None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
