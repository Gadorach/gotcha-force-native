import unittest

from tools.analyze.retail_ground_pace import cruise_pace_candidate


def _operands(value, *, record_sha="ab", status="one-static-slot-0-record", selected=0):
    return {"class_installer_binding": {"status": status, "selected_record": selected},
            "records": [{"record_sha256": record_sha,
                         "operands": {"02C": {"value": value, "bits": "41700000"}}}]}


class CruisePaceTest(unittest.TestCase):
    def test_bound_record_uses_the_shared_normalisation(self):
        pace = cruise_pace_candidate(_operands(15.0))
        self.assertEqual(pace["native_units_per_tick_x100"], 625)
        self.assertEqual(pace["source_value"], 15.0)
        self.assertEqual(pace["operand"], "02C")
        self.assertEqual(pace["native_policy"], "source-relative-cruise-v1")
        self.assertFalse(pace["retail_speed_verified"])

    def test_half_the_operand_is_half_the_pace(self):
        self.assertEqual(cruise_pace_candidate(_operands(7.5))["native_units_per_tick_x100"], 313)

    def test_zero_target_is_refused_rather_than_defaulted(self):
        self.assertIsNone(cruise_pace_candidate(_operands(0.0)))

    def test_unbound_record_is_refused(self):
        self.assertIsNone(cruise_pace_candidate(_operands(15.0, status="unbound")))
        self.assertIsNone(cruise_pace_candidate(_operands(15.0, selected=3)))

    def test_missing_operand_or_table_is_refused(self):
        self.assertIsNone(cruise_pace_candidate(None))
        self.assertIsNone(cruise_pace_candidate({}))
        self.assertIsNone(cruise_pace_candidate({"class_installer_binding":
                                                 {"status": "one-static-slot-0-record",
                                                  "selected_record": 0},
                                                 "records": [{"operands": {}}]}))


if __name__ == "__main__":
    unittest.main()
