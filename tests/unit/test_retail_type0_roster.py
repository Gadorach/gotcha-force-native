"""Check ordinary type-0 scope, model guards, and the original stop control."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_projectile_census as census
from tools.analyze import retail_type0_flight as flight
from tools.analyze import retail_type0_roster as type0


MAP = Path(__file__).resolve().parents[2] / "docs/api-map"
REPORT = """\
type0-flight: 0 1 1 0 299 1 0 0 0 370.00000
type0-flight: 16 1 2 1 299 0 1 0 0 350.00000
type0-flight: 16 2 3 1 299 0 1 0 0 350.00000
type0-flight: 16 3 3 1 299 0 1 1 1 350.00000
"""


class RetailType0Test(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.raw = (MAP / "gg4e08-type0-roster.json").read_bytes()
        cls.roster = json.loads(cls.raw)

    def test_census_keeps_key_alternatives_and_nonjoined_calls(self):
        result = self.roster
        self.assertEqual(result["source_dol_sha256"], type0.SOURCE_SHA256)
        self.assertEqual(result["source_afs_sha256"], type0.AFS_SHA256)
        self.assertEqual(result["factory"], "80082824")
        self.assertEqual([r["key"] for r in result["factory_rows"]], list(range(129)))
        self.assertEqual(result["factory_rows"][114]["basis"], "unresolved")
        counts = result["counts"]
        self.assertEqual((counts["roster_profiles_examined"], counts["models_with_candidates"]), (206, 27))
        self.assertEqual((counts["direct_call_sites"], counts["joined_call_sites"], counts["unjoined_call_sites"]),
                         (139, 58, 81))
        toggle = [r for r in result["candidates"] if "key_selector" in r]
        self.assertEqual([(r["site"], r["key"]) for r in toggle], [("80188CC8", 16), ("80188CC8", 17)])
        self.assertTrue(all(r["key_selector"]["actor_byte"] == "149" for r in toggle))
        self.assertEqual(census.encode(result), self.raw)

    def test_death_eye_does_not_inherit_the_other_model_arm(self):
        death = [r for r in self.roster["candidates"] if r["borg"] == "gg4e08:0f06"]
        self.assertEqual([(r["site"], r["key"], r["required_model_at_call"]) for r in death],
                         [("801E4754", 122, "0F06")])
        self.assertEqual(death[0]["queue"]["steps"][0]["raw_power"], 10)
        self.assertIn("801E4764", {r["site"] for r in self.roster["call_sites"]})
        self.assertEqual(self.roster["reviewed_model_branch"]["not_equal_call"]["key"], 123)

    def test_original_flight_control_does_not_supply_a_hit(self):
        observed = json.loads((MAP / "gg4e08-type0-flight.json").read_bytes())
        self.assertEqual(observed["samples"], flight.decode_report(REPORT))
        self.assertEqual(observed["initial_attack_registry_count"], 1)
        self.assertEqual(observed["carrier_row"], self.roster["factory_rows"][45])
        clean, stop, _, freed = observed["samples"]
        self.assertEqual((clean["carrier_z"], stop["carrier_z"]), (370, 350))
        self.assertEqual((stop["carrier_free_count"], freed["carrier_free_count"]), (0, 1))
        self.assertEqual((stop["shape_free_count"], freed["descriptor_free_count"]), (1, 1))

    def test_flight_export_and_parser_refuse_invalid_inputs(self):
        for data in ("", REPORT + REPORT, REPORT.replace("299 0 1 0 0", "299 1 0 0 0"),
                     REPORT.replace("16 3 3 1 299 0 1 1 1", "16 3 3 1 299 0 1 1 0")):
            with self.subTest(data=data), self.assertRaises(ValueError):
                flight.decode_report(data)
        with TemporaryDirectory() as tmp:
            output = Path(tmp) / "existing.json"
            output.write_text("preserved")
            with self.assertRaises(FileExistsError):
                flight.export(None, None, None, None, output)
            self.assertEqual(output.read_text(), "preserved")
