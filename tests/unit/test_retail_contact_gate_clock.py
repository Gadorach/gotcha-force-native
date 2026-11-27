"""Preserve the collision-walk gate scope and fresh-token reopening boundary."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_contact_gate_clock as gate


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-contact-gate-clock.json"
PRELUDE = "\n".join(
    f"impact-prelude: {step} {visit} {g} 0 4 {g} {g} 4 4 {g} {g} {g}"
    for step in (2, 1) for visit in range(1, 6) for g in (max(0, 4-visit),)
)
PAIRS = "\n".join(
    f"impact-clock: {visits} {step} {4-visits} {4 if visits == 4 else 4-visits} {int(visits == 4)} 146 1 2"
    for step in (2, 1) for visits in range(5)
)


class RetailContactGateClockTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())

    def test_idle_walk_visits_active_objects_and_leaves_ineligible_children_alone(self):
        rows = self.data["prelude_visits"]
        self.assertEqual(gate.decode_prelude(PRELUDE), rows)
        self.assertEqual(len(rows), 10)
        for group in (rows[:5], rows[5:]):
            self.assertEqual([r["active_actor_gate"] for r in group], [3, 2, 1, 0, 0])
            for row in group:
                self.assertEqual(row["registered_child_gate"], row["active_actor_gate"])
                self.assertEqual(row["first_view_gate"], row["last_view_gate"])
                self.assertEqual((row["inactive_actor_gate"], row["unregistered_child_gate"], row["inactive_seat_child_gate"]),
                                 (4, 4, 4))
        self.assertEqual(self.data["explicit_gate_byte_accesses"], [
            {"site": "8002CE18", "kind": "load"}, {"site": "8002CE28", "kind": "store"},
            {"site": "8002F858", "kind": "load"}, {"site": "8002F86C", "kind": "store"},
        ])

    def test_fresh_hp_is_independent_of_visual_reopen(self):
        rows = self.data["fresh_pair_cases"]
        self.assertEqual(gate.decode_pairs(PAIRS), rows)
        for group in (rows[:5], rows[5:]):
            self.assertEqual([r["gate_before_second_hit"] for r in group], [4, 3, 2, 1, 0])
            self.assertEqual([r["second_effect_requested"] for r in group], [False, False, False, False, True])
            self.assertTrue(group[-1]["same_token_refused_after_natural_reopen"])
            self.assertTrue(all((r["receiver_hp_after"], r["first_token"], r["second_token"]) == (146, 1, 2) for r in group))

    def test_refuses_early_reopen_or_reset_of_unregistered_child(self):
        with self.assertRaises(ValueError):
            gate.decode_pairs(PAIRS.replace("3 2 1 1 0 146", "3 2 1 4 1 146", 1))
        with self.assertRaises(ValueError):
            gate.decode_prelude(PRELUDE.replace("2 1 3 0 4 3 3 4 4", "2 1 3 0 4 3 3 3 4", 1))

    def test_output_is_not_replaced(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "clock.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                gate.export(None, None, None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
