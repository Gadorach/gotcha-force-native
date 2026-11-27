"""Keep type-9 unbind, phase changes and pool returns separate."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type9_lifetime as lifetime


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type9-lifetime.json"
SUFFIXES = ("30 31 32 31 31 33 33 800FC9F4", "30 31 32 31 31 33 33 800FC9F4",
            "0 1 2 0 0 3 3 00000000", "1 0 2 2 2 3 3 80084090")
REPORT = "\n".join(f"type9-lifetime: {key} {n} {tail}" for key in range(7) for n, tail in enumerate(SUFFIXES))


class RetailType9LifetimeTest(unittest.TestCase):
    def test_normal_and_half_clock_have_separate_shape_and_carrier_returns(self):
        data = json.loads(ARTIFACT.read_bytes())
        rows = data["cases"]
        self.assertEqual(rows, lifetime.decode_report(REPORT))
        self.assertEqual(sum(r["carrier_return_visit"] for r in rows), 504)
        for one, half in zip(rows[::4], rows[1::4], strict=True):
            self.assertEqual((one["owner_clock_step"], half["owner_clock_step"]), (1.0, 0.5))
            for row in (one, half):
                self.assertEqual((row["published_attack_visits"], row["radius_during_publication"],
                                  row["token_during_publication"]), (30, 150, 1))
                self.assertEqual((row["first_post_visit_phase_two"], row["first_post_visit_phase_three"]), (31, 32))
                self.assertEqual((row["shape_return_visit"], row["carrier_return_visit"],
                                  row["descriptor_return_visit"]), (31, 33, 33))

    def test_shape_refusal_and_early_owner_exit_are_not_the_same_transition(self):
        for refused, owner in zip(lifetime.decode_report(REPORT)[2::4], lifetime.decode_report(REPORT)[3::4], strict=True):
            self.assertEqual((refused["published_attack_visits"], owner["published_attack_visits"]), (0, 1))
            self.assertEqual((refused["shape_return_visit"], owner["shape_return_visit"]), (None, 2))
            self.assertEqual((refused["first_post_visit_phase_two"], owner["first_post_visit_phase_two"]), (1, None))
            self.assertIsNone(refused["shape_unbind_site"])
            self.assertEqual((owner["first_post_visit_phase_three"], owner["shape_unbind_site"]), (2, "80084090"))
            self.assertEqual((refused["carrier_return_visit"], owner["carrier_return_visit"]), (3, 3))

    def test_decoder_refuses_early_carrier_return_or_invented_shape_return(self):
        for old, new in (
            ("4 1 30 31 32 31 31 33 33", "4 1 30 31 32 31 31 31 33"),
            ("4 2 0 1 2 0 0 3 3", "4 2 0 1 2 0 1 3 3"),
            ("4 3 1 0 2 2 2 3 3 80084090", "4 3 1 0 2 2 2 3 3 800FC9F4"),
        ):
            self.assertIn(old, REPORT)
            with self.assertRaises(ValueError):
                lifetime.decode_report(REPORT.replace(old, new, 1))

    def test_existing_output_is_preserved(self):
        with TemporaryDirectory() as tmp:
            target = Path(tmp) / "out.json"
            target.write_text("keep")
            with self.assertRaises(FileExistsError):
                lifetime.export(None, None, None, None, target)
            self.assertEqual(target.read_text(), "keep")
