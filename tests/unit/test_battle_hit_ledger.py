"""Pin the source observer count and its separate per-actor history layout."""
import json
import os
from pathlib import Path
import tempfile
import unittest

from tools.analyze import battle_hit_ledger

DOL = Path(os.environ.get("GF_RETAIL_ROOT", "original/extracted")) / "sys/main.dol"
ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/battle-hit-ledger.json"


class BattleHitLedgerTest(unittest.TestCase):
    def test_observer_layout(self):
        report = json.loads(ARTIFACT.read_bytes())
        history = report["observers"]
        self.assertEqual((history["slots"], history["stride"]), (6, 0x1E00))
        self.assertEqual((history["records"], history["record_bytes"]), (8, 4))
        self.assertEqual(history["history_offset"] + history["records"] * history["record_bytes"],
                         history["cursor_offset"])
        self.assertEqual(history["actor_array_pointer"], "803C4E84")
        self.assertEqual((history["active_byte_offset"], history["group_offset"], history["recipient_slot_offset"]),
                         (0, 136, 996))
        self.assertTrue(history["exclude_recipient"])

    def test_cached_force_cost_and_relationships(self):
        report = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(report["amount"]["recipient_cache_offset"], 0x4AA)
        self.assertIn("Force cost", report["amount"]["note"])
        self.assertEqual(report["recipient_totals"], {"count_offset": 0x435, "amount_offset": 0x424})
        history = report["observers"]
        self.assertEqual(history["different_player"]["relation"], 3)
        self.assertEqual(history["same_player"]["relation"], 4)
        self.assertEqual((history["same_player"]["count_offset"], history["same_player"]["amount_offset"]),
                         (0x437, 0x430))

    @unittest.skipUnless(DOL.is_file(), "original DOL not available")
    def test_regenerates_without_overwrite(self):
        with tempfile.TemporaryDirectory() as name:
            out = Path(name) / ARTIFACT.name
            battle_hit_ledger.export(DOL, out)
            self.assertEqual(out.read_bytes(), ARTIFACT.read_bytes())
            with self.assertRaises(FileExistsError):
                battle_hit_ledger.export(DOL, out)


if __name__ == "__main__":
    unittest.main()
