"""Keep carrier ownership, action arms, and secondary groups distinct."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_projectile_census as census
from tools.analyze import retail_small_projectile_roster as roster


MAP = Path(__file__).resolve().parents[2] / "docs/api-map"
ARTIFACT = MAP / "gg4e08-small-projectile-roster.json"


class SmallProjectileRosterTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.result = json.loads(ARTIFACT.read_bytes())

    def test_counts_qualify_only_the_reviewed_direct_calls(self):
        receipt = self.result
        self.assertEqual(receipt["source_dol_sha256"], roster.SOURCE_SHA256)
        self.assertEqual(receipt["source_afs_sha256"], roster.AFS_SHA256)
        self.assertEqual(receipt["counts"], {
            "roster_profiles_examined": 206, "factory_rows": 26, "direct_call_sites": 25,
            "joined_call_sites": 21, "candidate_rows": 28, "models_with_candidates": 19,
        })
        mapped = {r["site"] for r in receipt["candidates"]}
        self.assertEqual({r["site"] for r in receipt["call_sites"]} - mapped,
                         {f"{word:08X}" for word in (0x800EB270, 0x801FD024, 0x80119068, 0x80119080)})
        self.assertEqual(receipt["families"]["type-94"]["row_domain"], "reviewed prefix")
        self.assertEqual(census.encode(receipt), ARTIFACT.read_bytes())
        indexed = [receipt] + [json.loads((MAP / f"gg4e08-type{n}-roster.json").read_bytes()) for n in (0, 2)]
        self.assertEqual(sum(len(r["call_sites"]) for r in indexed), 267)
        self.assertEqual(len({r["site"] for root in indexed for r in root["candidates"]}), 137)
        self.assertEqual(len({r["borg"] for root in indexed for r in root["candidates"]}), 75)

    def test_parent_model_and_actual_bomb_action_are_both_required(self):
        bombs = [r for r in self.result["candidates"] if r["family"] == "type-15"]
        self.assertEqual(len({r["site"] for r in bombs}), 10)
        owner = {r["key"]: f'gg4e08:{r["required_parent_model"]:04x}'
                 for r in self.result["factory_rows"] if r["family"] == "type-15"}
        self.assertTrue(all(owner[row["key"]] == row["borg"] for row in bombs))
        charged = [r for r in bombs if r["borg"] == "gg4e08:0101"]
        self.assertEqual([r["key"] for r in charged], [8, 9, 6, 7])
        self.assertEqual([r["call_order_in_reviewed_arm"] for r in charged], [1, 2, 3, 4])
        for row in charged:
            self.assertEqual({(r["group"], r["state"]) for r in row["action_candidates"]}, {("X-charge", 3)})
            self.assertIsNone(row["debit_in_reviewed_arm"])
            self.assertEqual(row["release_condition"]["opposite_ordinary_X_state"], 2)
        clone = next(r for r in bombs if r["key"] == 2)
        self.assertEqual(clone["additional_guard"], {"actor_byte": "0x7C8", "required": "nonzero"})
        self.assertEqual(clone["queue"], {"status": "conditional-actor-state-unqualified"})

    def test_dragon_keys_do_not_flatten_and_worm_followups_are_not_openers(self):
        dragons = [r for r in self.result["candidates"] if r["family"] == "type-47"]
        self.assertEqual({r["site"] for r in dragons}, {"80075CA4"})
        self.assertEqual([r["key"] for r in dragons], list(range(6)))
        self.assertEqual([r["queue"]["steps"][0]["raw_power"] for r in dragons], [80]*3+[40]*3)
        self.assertEqual(len({r["key_selector"]["arm"] for r in dragons}), 6)
        strings = [r for r in self.result["candidates"] if r["family"] == "type-49"]
        self.assertEqual([r["queue"]["status"] for r in strings], ["zero-power", "zero-power"])
        self.assertEqual([r["followup_queue"]["steps"][0]["raw_power"] for r in strings], [2, 1])
        self.assertTrue(all(r["action_candidates"][0]["context_rows"] == [[0, 0]] for r in strings))

    def test_recovery_refuses_existing_output(self):
        with TemporaryDirectory() as tmp:
            output = Path(tmp) / "receipt.json"
            output.write_text("unchanged")
            with self.assertRaises(FileExistsError):
                census.export_census(None, None, None, None, output, roster.recover)
            self.assertEqual(output.read_text(), "unchanged")
