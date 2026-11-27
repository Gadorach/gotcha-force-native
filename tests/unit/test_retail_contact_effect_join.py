"""Preserve the accepted hit, its saved point, and downstream visual refusal."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_contact_effect_join as join


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


def report(slot):
    lines = []
    for row in slot["cases"]:
        roots = [m["root_offset"] for m in row["first_update_models"]] + [0, 0]
        fields = (slot["pzz_slot"], row["attack_type"], int(row["credited_borg"].split(":")[1], 16),
                  int(row["type3_initially_supplied"]), row["selector"], row["hp_lost"], row["receiver_hp_after"],
                  row["accepted_token"], row["visual_gate_after"], row["parent_lane_byte"], row["scene_mode_byte"],
                  row["first_update_phase"], row["lane_5_descriptor_count"], roots[0], roots[1],
                  row["refused_emitter_return_visit"] or 0, int(row["type3_claimed"]))
        lines.append("impact-art:" + "".join(f" {n}" for n in fields) + "".join(f" {p:.5f}" for p in row["hit_point"]))
    return "\n".join(lines)


class RetailContactEffectJoinTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.slots = json.loads((MAPS / "gg4e08-contact-effect-join.json").read_bytes())["slots"]
        cls.art = json.loads((MAPS / "gg4e08-contact-effect-source-art.json").read_bytes())["slots"]

    def test_same_hp_point_and_token_through_a_late_visual_refusal(self):
        for slot in self.slots:
            rows = slot["cases"]
            for ready, empty in zip(rows[::2], rows[1::2], strict=True):
                for field in ("accepted_token", "hp_lost", "receiver_hp_after", "damage_ledger_deltas", "hit_point"):
                    self.assertEqual(ready[field], empty[field])
                self.assertEqual(ready["visual_gate_after"], empty["visual_gate_after"])
                if ready["selector"] == 0:
                    self.assertEqual((empty["first_update_phase"], empty["lane_5_descriptor_count"]), (1, 1))
                    self.assertFalse(ready["type3_claimed"])
                else:
                    self.assertTrue(ready["type3_claimed"])
                    self.assertEqual((empty["first_update_phase"], empty["lane_5_descriptor_count"]), (2, 0))
                    self.assertTrue(empty["late_type3_offer_did_not_retry"])
                    self.assertEqual(empty["refused_emitter_return_visit"], 3)

    def test_only_the_hit_selected_records_are_admitted_in_each_archive(self):
        for slot, art in zip(self.slots, self.art, strict=True):
            roots = join.archive_records(art)
            self.assertEqual(join.decode(report(slot), slot["pzz_slot"], roots), slot["cases"])
            ready = [r for r in slot["cases"] if r["type3_initially_supplied"]]
            self.assertEqual([[m["record"] for m in r["first_update_models"]] for r in ready],
                             [[2], [2], [53, 54], [21]])
            self.assertEqual([r["hp_lost"] for r in ready], [35, 35, 26, 17])

    def test_decoder_refuses_hit_point_drift_and_rebound_models(self):
        slot = self.slots[0]
        roots = join.archive_records(self.art[0])
        good = report(slot)
        for bad in (good.replace("200.00000 157.60001", "220.00000 157.60001", 1),
                    good.replace("168168 171016", "171016 168168", 1)):
            with self.assertRaises(ValueError):
                join.decode(bad, 1, roots)

    def test_existing_receipt_is_not_replaced(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "join.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                join.export(None, None, None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
