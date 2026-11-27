"""Guard the static type-2 census against key, owner, and scope conflation."""
import json
from pathlib import Path
import struct
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import reaction_attack_table, reaction_emit, retail_type2_roster as type2
from tools.analyze import retail_projectile_census as census


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type2-roster.json"


class Type2RosterTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.result = json.loads(ARTIFACT.read_bytes())

    def test_full_domain_and_call_coverage(self):
        result = self.result
        counts = result["counts"]
        self.assertEqual(result["source_dol_sha256"], type2.SOURCE_SHA256)
        self.assertEqual(result["source_afs_sha256"], type2.AFS_SHA256)
        self.assertEqual((counts["roster_profiles_examined"], counts["models_with_candidates"]), (206, 30))
        self.assertEqual([r["key"] for r in result["factory_rows"]], list(range(123)))
        calls = {r["site"] for r in result["call_sites"]}
        joined = {r["site"] for r in result["candidates"]}
        self.assertEqual((len(calls), len(joined), len(calls - joined)), (103, 58, 45))
        self.assertLessEqual(joined, calls)
        self.assertEqual(len({r["key"] for r in result["candidates"]}), 56)
        self.assertEqual(counts["action_context_sets"], sum(len(r["action_candidates"]) for r in result["candidates"]))
        self.assertEqual(result["phase_entries"], list(map(type2.address, type2.PHASES)))
        self.assertEqual(census.encode(result), ARTIFACT.read_bytes())

    def test_same_dispatch_does_not_copy_machine_red_operands(self):
        guns = self.result["factory_rows"]
        releases = {(r["borg"], r["key"]): r for r in self.result["candidates"]}
        self.assertEqual(self.result["counts"]["speed_triplets"], 15)
        self.assertEqual(sum(row["basis"] == "parent-root-matrix" for row in guns), 13)
        self.assertEqual((guns[21]["collision_group_selector"], guns[85]["collision_group_selector"]), (0, 9))
        self.assertEqual((guns[21]["initial_speed"], guns[85]["initial_speed"]), (80, 160))
        self.assertEqual(guns[8]["bearing_helper"], "never")
        self.assertEqual(guns[8]["matrix_axis_if_no_helper"], 2)
        self.assertEqual(releases["gg4e08:0600", 21]["queue"]["steps"][0]["raw_power"], 10)
        self.assertEqual(releases["gg4e08:0615", 43]["queue"]["steps"][0]["raw_power"], 20)
        self.assertEqual(releases["gg4e08:061e", 85]["queue"]["steps"][0]["raw_power"], 50)
        self.assertEqual(releases["gg4e08:060a", 105]["queue"], {"status": "combined-form-ownership-unqualified"})
        alternate = [r for r in releases.values() if r["spec_record"] != 0]
        self.assertEqual(len(alternate), 4)
        self.assertTrue(all(r["queue"] == {"status": "alternate-spec-unqualified"} for r in alternate))
        nonadjacent = next(r for r in self.result["call_sites"] if r["site"] == "80103E14")
        self.assertIsNone(nonadjacent["adjacent_li_r4_key"])
        self.assertEqual(releases["gg4e08:0104", 26]["site"], "80103E14")

    def test_queue_classifier_does_not_promote_empty_or_mixed_groups(self):
        block = bytearray(reaction_emit.HIT_BYTES)
        block[reaction_emit.QUEUE:reaction_emit.QUEUE + 4] = bytes((0, 1, 255, 255))
        block[reaction_emit.STEPS + reaction_emit.STEP_SIZE + 5] = 1

        class DamageTable:
            def __init__(self, powers):
                self.powers = powers

            def read(self, offset, length, kind):
                if (length, kind) != (2, "data"):
                    raise AssertionError("expected a raw-power halfword")
                return struct.pack(">H", self.powers[offset // reaction_attack_table.SIZE])

        for powers, status in (((0, 0), "zero-power"), ((0, 20), "mixed-power"), ((20, 20), "uniform-positive-power")):
            with self.subTest(powers=powers):
                self.assertEqual(census.queue_metadata(DamageTable(powers), block, 0, 0)["status"], status)
        block[reaction_emit.QUEUE] = 255
        self.assertEqual(census.queue_metadata(None, block, 0, 0), {"status": "empty-source-queue"})
        self.assertEqual(census.queue_metadata(None, block, 0, -1), {"status": "negative-selector-skips-bind"})

    def test_refuses_output_overwrite_and_metadata_mismatch(self):
        with TemporaryDirectory() as tmp:
            output = Path(tmp) / "receipt.json"
            output.write_text("{}")
            with self.assertRaises(FileExistsError):
                type2.export(None, None, None, None, output)
            with self.assertRaises(ValueError):
                census.read_metadata(output, type2.ATLAS_SHA256)
            self.assertEqual(output.read_text(), "{}")
