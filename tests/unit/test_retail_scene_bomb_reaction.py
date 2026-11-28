"""Protect the original body-clock order and same-visit second-B contact receipt."""
from hashlib import sha256
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_scene_bomb_reaction as reaction


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailSceneBombReactionTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.receipt = json.loads((MAPS / "gg4e08-scene-bomb-reaction.json").read_bytes())
        cls.cases = {(r["entry_hold_visits"], r["shape_cells_offered"]): r for r in cls.receipt["cases"]}

    def test_source_scope_and_fork_counts(self):
        for name in ("scene_bomb_area", "type41_parent_contact"):
            prior = MAPS / f"gg4e08-{name.replace('_', '-')}.json"
            self.assertEqual(self.receipt[f"{name}_sha256"], sha256(prior.read_bytes()).hexdigest())
        self.assertEqual(set(self.cases), {(d, s) for d in (0, 10) for s in (1, 2)})
        self.assertEqual(sum(r["retained_coordinator_visits"] for r in self.cases.values()), 940)
        self.assertEqual(sum(r["fresh_b_factory_forks"]["count"] for r in self.cases.values()), 208)
        self.assertEqual(sum(len(r["completed_second_placements"]) for r in self.cases.values()), 8)

    def test_rate_multiplier_changes_before_the_consumed_rate_and_without_pose_hold(self):
        for delay in (0, 10):
            samples = {s["visit_from_first_press"]: s["both_actors"]
                       for s in self.cases[delay, 2]["retained_body_samples"]}
            self.assertEqual([samples[n]["impact_visual_gate"] for n in range(185, 190)], [4, 3, 2, 1, 0])
            self.assertEqual(samples[185]["secondary_multiplier_1dcc_bits"], reaction.NEGATIVE)
            self.assertEqual(samples[185]["secondary_rate_bits"], reaction.ONE)
            self.assertEqual(samples[186]["secondary_rate_bits"], reaction.NEGATIVE)
            self.assertEqual(samples[235]["secondary_rate_bits"], reaction.NEGATIVE)
            self.assertTrue(all(s["pose_timer"] == 0 and s["primary_rate_bits"] == reaction.ONE for s in samples.values()))
            control = self.cases[delay, 1]["retained_body_samples"]
            self.assertTrue(all(s["both_actors"]["secondary_rate_bits"] == reaction.ONE for s in control))

    def test_fresh_b_after_self_hit_pays_and_publishes_before_the_new_parent_loses_its_one_hp(self):
        for delay in (0, 10):
            row = self.cases[delay, 2]
            press = next(p for p in row["fresh_b_sampled_forks"] if p["visit_from_first_press"] == 186)
            placed = next(p for p in row["completed_second_placements"] if p["visit_from_first_press"] == 186)
            self.assertEqual((press["stock_at_second_factory"], press["secondary_rate_at_second_factory_bits"]),
                             (9, reaction.NEGATIVE))
            self.assertEqual(placed["attack_shapes_in_order"], ["new-parent", "old-area"])
            self.assertEqual((placed["new_parent_token"], placed["new_parent_hp_after_world"]), (3, 0))
            self.assertEqual(placed["hp_writes_in_order"], [{
                "source": "old-area", "receiver": "new-parent", "amount_record": reaction.AREA,
                "source_token": 2, "receiver_tier_at_amount": None,
                "hp_before_write": 1, "hp_requested": 4, "hp_removed": 1,
            }])
            self.assertEqual(placed["received_damage_ledgers_by_seat"], [17, 4])

    def test_combined_contact_reads_tier_one_only_after_the_new_parent_hits(self):
        combined = self.cases[10, 2]["completed_second_placements"][0]
        same_visit = self.cases[0, 2]["completed_second_placements"][0]
        no_area = self.cases[10, 1]["completed_second_placements"][0]
        self.assertEqual(combined["sasuke_history"], [3, 2])
        hits = combined["hp_writes_in_order"]
        self.assertEqual([(r["source"], r["receiver"], r["receiver_tier_at_amount"], r["hp_removed"]) for r in hits],
                         [("new-parent", "seat-0", 0, 26), ("old-area", "seat-0", 1, 14),
                          ("old-area", "new-parent", None, 1), ("old-area", "seat-1", 0, 4)])
        self.assertEqual((combined["hp_after_world_by_seat"], combined["owner_opponent_credit"]), ([140, 216], 40))
        self.assertEqual((same_visit["hp_after_world_by_seat"], same_visit["owner_opponent_credit"]), ([163, 216], 17))
        self.assertEqual((no_area["hp_after_world_by_seat"], no_area["owner_opponent_credit"]), ([154, 220], 26))

    def test_refuses_truncated_or_changed_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            reaction.decode("")
        with self.assertRaisesRegex(ValueError, "changed at row 1"):
            reaction.decode("area-reaction-body: invalid\n"*123)
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "reaction.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                reaction.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
