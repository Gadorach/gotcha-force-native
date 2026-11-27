"""Refuse treating a pending unlink as a free shape cell."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_bomb_actor_area as actor_area


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-bomb-actor-area.json"
REPORT = "".join(f"""\
bomb-actor-area: {model} 0 1 0 0 0 1 0 172 1 0 1
bomb-actor-area: {model} 1 1 1 1 1 1 0 155 1 2 1
""" for model in (0, 10))


class RetailBombActorAreaTest(unittest.TestCase):
    def test_both_casters_need_a_spare_before_the_opening_shape_is_recycled(self):
        data = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(data["source_dol_sha256"], actor_area.SOURCE_SHA256)
        self.assertEqual(data["source_afs_sha256"], actor_area.AFS_SHA256)
        self.assertEqual(data["entries"]["request_unlink"], "8008AFF0")
        self.assertEqual(data["entries"]["recycle_cell"], "80088E50")
        for pilot, expected in zip(data["pilots"], actor_area.decode_report(REPORT), strict=True):
            self.assertEqual((pilot["borg"], pilot["cases"]), (expected["borg"], expected["cases"]))
            empty, spare = pilot["cases"]
            for case in (empty, spare):
                self.assertEqual(case["opening_hit"]["damage"], 8)
                self.assertTrue(case["same_bucket_area_bind"]["opening_unlink_requested"])
                self.assertTrue(case["same_bucket_area_bind"]["opening_shape_still_linked"])
                self.assertTrue(case["first_class4_pass_after_impact"]["opening_shape_returned"])
                self.assertEqual(case["first_class4_pass_after_impact"]["shape_slots_free"], 1)
                self.assertEqual(case["first_class4_pass_after_impact"]["registered_attacks"], 0)
                self.assertEqual(case["later_area_bind_attempts"], 0)
                self.assertEqual(case["stock_after"], 1)
            self.assertFalse(empty["same_bucket_area_bind"]["area_bound"])
            self.assertIsNone(empty["same_bucket_area_bind"]["audio_request"])
            self.assertEqual(empty["receiver_hp_after_two_growth_visits"], 172)
            self.assertEqual(empty["receiver_hit_tokens"], [1])
            self.assertTrue(spare["same_bucket_area_bind"]["area_bound"])
            self.assertEqual(spare["growth_visit_one"]["amount_record"], "802C4778")
            self.assertEqual(spare["receiver_hp_after_two_growth_visits"], 155)
            self.assertEqual(spare["receiver_hit_tokens"], [1, 2])

    def test_rejects_an_early_recycle_an_area_retry_or_misattributed_damage(self):
        for report in (
            REPORT*2,
            REPORT.replace("0 1 0 0 0 1", "0 1 1 0 0 1"),
            REPORT.replace("1 0 172", "1 1 172"),
            REPORT.replace("172 1 0 1", "155 1 2 1", 1),
            REPORT.replace("155 1 2 1", "155 2 1 1"),
        ):
            with self.subTest(report=report), self.assertRaises(ValueError):
                actor_area.decode_report(report)

    def test_existing_receipt_is_never_overwritten(self):
        with TemporaryDirectory() as temp:
            out = Path(temp) / "receipt.json"
            out.write_text("unchanged")
            with self.assertRaises(FileExistsError):
                actor_area.export(None, None, None, None, out)
            self.assertEqual(out.read_text(), "unchanged")
