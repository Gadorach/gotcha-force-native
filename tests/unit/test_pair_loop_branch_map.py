"""Pin the pair-loop conditional branch map."""
import json
import unittest
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ARTIFACT = ROOT / "docs/api-map/pair-loop-branch-map.json"


class PairLoopBranchMapTest(unittest.TestCase):
    def test_phases(self):
        data = json.loads(ARTIFACT.read_text())
        self.assertEqual(len(data["branches"]), 32)
        self.assertEqual(data["phases"],
                         {"prepare": 2, "inner": 20, "select": 4,
                          "publish": 6})

    def test_mode_and_carrier_flags(self):
        data = json.loads(ARTIFACT.read_text())
        calls = {b["site"]: b for b in data["branches"]}
        self.assertEqual(calls["8002DBC8"]["condition"]["text"], "r0 cmp 2")
        self.assertEqual(calls["8002DBC8"]["sense"], "false")
        self.assertEqual(calls["8002DC04"]["condition"]["text"],
                         "(r4 & 0x00000800)")

    def test_named_gates(self):
        data = json.loads(ARTIFACT.read_text())
        calls = {b["site"]: b for b in data["branches"]}
        self.assertEqual(calls["8002DBC8"]["gate"], "direct-category")
        self.assertEqual(calls["8002DC60"]["gate"], "validity-8002FD7C")
        self.assertEqual(calls["8002DD94"]["gate"], "handler-80030490")
        self.assertEqual(calls["8002DD20"]["gate"], "matrix-copy")
        self.assertTrue(all("gate" in b for b in data["branches"]))

    def test_category_dispatch(self):
        data = json.loads(ARTIFACT.read_text())
        dispatch = data["category_dispatch"]
        self.assertEqual(dispatch["index_table"], "802CFF68")
        self.assertEqual(dispatch["handler_table"], "802CFF2C")
        self.assertEqual(len(dispatch["categories"]), 48)
        self.assertEqual(len(dispatch["handlers"]), 15)
        self.assertEqual(dispatch["handlers"][0], "80030500")
        self.assertEqual(dispatch["handlers"][14], "80032E44")
        self.assertEqual(dispatch["categories"][5]["handler"], 12)
        self.assertEqual(dispatch["categories"][5]["address"], "80032DA4")
        swapped = dispatch["categories"][8]
        self.assertTrue(swapped["swap"])
        self.assertEqual(swapped["handler"], 1)
        self.assertEqual(swapped["address"], "800305D8")
        self.assertIsNone(dispatch["categories"][6]["handler"])
        self.assertIsNone(dispatch["categories"][47]["address"])

    def test_operand_provenance(self):
        data = json.loads(ARTIFACT.read_text())
        calls = {b["site"]: b for b in data["branches"]}
        direct = calls["8002DBC8"]["condition"]["operand"]
        self.assertEqual(direct["offset"], 88)
        self.assertEqual(direct["via"]["offset"], 0)
        carrier = calls["8002DC04"]["condition"]["operand"]
        self.assertEqual(carrier["offset"], 2)
        self.assertEqual(carrier["op"], "lhz")
        self.assertEqual(carrier["mask"], "0x00000800")
        self.assertEqual(carrier["via"]["offset"], 40)
        receiver = calls["8002DC48"]["condition"]["operand"]
        self.assertEqual(receiver["offset"], 474)
        self.assertEqual(receiver["via"]["offset"], 32)
        validity = calls["8002DC60"]["condition"]["operand"]
        self.assertEqual(validity["kind"], "call")
        team = calls["8002DC98"]["condition"]["operands"]
        self.assertEqual(team["r5"]["offset"], 136)
        self.assertEqual(team["r0"]["offset"], 136)
        owner = calls["8002DEBC"]["condition"]["operand"]
        self.assertEqual(owner["offset"], 256)
        aim = calls["8002DE4C"]["condition"]["operand"]
        self.assertEqual(aim["kind"], "indexed")
        self.assertEqual(aim["op"], "lhax")
        self.assertEqual(aim["mask"], "0x000003C0")

    def test_ctr_copies(self):
        data = json.loads(ARTIFACT.read_text())
        kinds = Counter(b["condition"]["kind"] for b in data["branches"])
        self.assertEqual(kinds["bdnz"], 2)
        self.assertNotIn("unknown", kinds)


if __name__ == "__main__":
    unittest.main()
