"""Preserve late eligibility projection, retained shapes, and phase-only amounts."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_actor_frame_admission as admission


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-actor-frame-admission.json"


def report(cases):
    lines = []
    for c in cases:
        model = int(c["borg"].split(":")[1], 16)
        for r in c["phase_only_controls"]:
            lines.append(f"frame-amount: {model} {r['declared_actor_phase']} {r['hp_lost']} {r['receiver_hp_after']}")
        for step, r in enumerate(c["retained_queue_sequence"]):
            values = (model, step, int(r["source_bit_set"]), r["admission_byte_before"], r["admission_byte_after_frame"],
                      r["attack_count_after_frame_before_list_rebuild"], r["attack_count_after_list_rebuild"],
                      r["retained_melee_cells"], r["unchanged_owner_token"], r["amount_calls"], r["receiver_hp_after"])
            lines.append("frame-admission: " + " ".join(map(str, values)))
    return "\n".join(lines)


class RetailActorFrameAdmissionTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(ARTIFACT.read_bytes())
        cls.cases = cls.data["cases"]

    def test_tail_updates_admission_before_the_next_list_rebuild(self):
        for case, count in zip(self.cases, (1, 2), strict=True):
            closed, opened = case["retained_queue_sequence"]
            self.assertEqual([r["admission_byte_before"] for r in (closed, opened)], [0, 1])
            self.assertEqual([r["admission_byte_after_frame"] for r in (closed, opened)], [1, 0])
            self.assertEqual([r["attack_count_after_frame_before_list_rebuild"] for r in (closed, opened)], [count, 0])
            self.assertEqual([r["attack_count_after_list_rebuild"] for r in (closed, opened)], [0, count])
            for row in (closed, opened):
                self.assertEqual((row["retained_melee_cells"], row["unchanged_owner_token"],
                                  row["receiver_shapes_after_list_rebuild"]), (count, 1, 1))

    def test_reopened_queue_hits_once_and_phase_control_explains_the_amount(self):
        for case, expected in zip(self.cases, ((20, 17), (41, 35)), strict=True):
            self.assertEqual(tuple(row["hp_lost"] for row in case["phase_only_controls"]), expected)
            closed, opened = case["retained_queue_sequence"]
            self.assertEqual((closed["amount_calls"], closed["body_timers_after_pair"]), (0, [0, 0]))
            self.assertEqual((opened["amount_calls"], opened["hp_lost"]), (1, expected[1]))

    def test_report_refuses_reallocation_or_stale_registry_assumptions(self):
        good = report(self.cases)
        self.assertEqual(admission.decode(good), self.cases)
        for bad in (good.replace("2055 0 1 0 1 1 0", "2055 0 1 0 1 0 0", 1),
                    good.replace("1 1 0 1 0 0 2 2 1", "1 1 0 1 0 0 2 2 2", 1)):
            with self.assertRaises(ValueError):
                admission.decode(bad)

    def test_explicit_byte_census_keeps_other_accesses_unreplayed(self):
        sites = {r["site"]: r["scope"] for r in self.data["explicit_eligibility_byte_accesses"]}
        self.assertEqual(len(sites), 5)
        self.assertIn("not replayed", sites["8003CC88"])
        self.assertIn("not replayed", sites["8008A5A4"])

    def test_output_is_not_replaced(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "frame.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                admission.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
