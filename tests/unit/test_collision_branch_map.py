"""Pin the collision-walk conditional branch map."""
import json
import unittest
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ARTIFACT = ROOT / "docs/api-map/collision-branch-map.json"
WALK = ROOT / "docs/api-map/collision-walk-map.json"


class CollisionBranchMapTest(unittest.TestCase):
    def test_phases(self):
        data = json.loads(ARTIFACT.read_text())
        self.assertEqual(len(data["branches"]), 98)
        self.assertEqual(data["phases"],
                         {"broad-phase": 10, "geometry": 14,
                          "apply": 27, "post": 47})

    def test_known_branch(self):
        data = json.loads(ARTIFACT.read_text())
        calls = {b["site"]: b for b in data["branches"]}
        branch = calls["8002BBA0"]
        self.assertEqual(branch["cond"], "eq")
        self.assertEqual(branch["sense"], "true")
        self.assertEqual(branch["condition"]["kind"], "extsb.")
        self.assertEqual(branch["target"], "8002BD2C")
        self.assertEqual(branch["gate"], "actor-active")

    def test_named_broad_phase_gates(self):
        data = json.loads(ARTIFACT.read_text())
        broad = [b for b in data["branches"] if b["phase"] == "broad-phase"]
        self.assertEqual(len(broad), 10)
        self.assertTrue(all("gate" in b for b in broad))
        gates = {b["site"]: b["gate"] for b in broad}
        self.assertEqual(gates["8002BD3C"], "actor-loop-6")
        self.assertEqual(gates["8002BD94"], "effect-slot-loop-12")

    def test_named_geometry_gates(self):
        data = json.loads(ARTIFACT.read_text())
        geometry = [b for b in data["branches"] if b["phase"] == "geometry"]
        self.assertEqual(len(geometry), 14)
        self.assertTrue(all("gate" in b for b in geometry))
        gates = {b["site"]: b["gate"] for b in geometry}
        self.assertEqual(gates["8002BED4"], "geometry-actor-loop")
        self.assertEqual(gates["8002BFDC"], "pair-model-table")

    def test_apply_effect_scale(self):
        data = json.loads(WALK.read_text())
        scale = data["apply_effect_scale"]
        self.assertEqual(scale["function"], "8002CAA8")
        self.assertEqual(scale["scaled"], "+0xB4")
        self.assertEqual(scale["timer"], "+0x1E8")
        self.assertIn("+0x58", scale["copies"])
        self.assertIn("+0x1CC", scale["cleared"])

    def test_field_usage(self):
        data = json.loads(ARTIFACT.read_text())
        usage = data["field_usage"]
        self.assertEqual(usage["apply"]["const:-31400"], 22)
        self.assertEqual(usage["apply"]["f32:+180"], 16)
        self.assertEqual(usage["apply"]["call"], 6)
        self.assertEqual(usage["post"]["s16:+1000"], 6)
        self.assertEqual(usage["post"]["call"], 5)
        self.assertEqual(usage["post"]["indexed"], 4)

    def test_operand_provenance(self):
        data = json.loads(ARTIFACT.read_text())
        calls = {b["site"]: b for b in data["branches"]}
        actor = calls["8002BBA0"]["condition"]["operand"]
        self.assertEqual(actor["kind"], "field")
        self.assertEqual(actor["offset"], 0)
        self.assertTrue(actor["sign_extended"])
        self.assertEqual(actor["via"]["offset"], 20100)
        peak = calls["8002BC10"]["condition"]["operands"]
        self.assertEqual(peak["r3"]["offset"], 1268)
        self.assertEqual(peak["r0"]["offset"], 1276)
        linked = calls["8002BE38"]["condition"]["operand"]
        self.assertEqual(linked["offset"], 477)
        self.assertEqual(linked["via"]["offset"], 484)
        flag = calls["8002BF10"]["condition"]["operand"]
        self.assertEqual(flag["offset"], 628)
        self.assertEqual(flag["mask"], "0x00000002")
        table = calls["8002BFDC"]["condition"]["operand"]
        self.assertEqual(table["kind"], "indexed")
        self.assertEqual(table["op"], "lwzx")

    def test_all_conditions_decoded(self):
        data = json.loads(ARTIFACT.read_text())
        kinds = Counter(b["condition"]["kind"] for b in data["branches"])
        self.assertNotIn("unknown", kinds)
        self.assertEqual(kinds["fcmpo"], 28)
        self.assertEqual(kinds["cmpi"], 29)


if __name__ == "__main__":
    unittest.main()
