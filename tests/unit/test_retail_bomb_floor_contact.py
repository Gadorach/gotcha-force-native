"""Distinguish the growing area's floor-triggered damage from the opening hit."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_bomb_floor_contact as contact


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-bomb-floor-contact.json"
REPORT = "".join(f"""\
bomb-floor-contact: {model} 0 56 35 0 180 180 0 0 1
bomb-floor-contact: {model} 1 56 35 36 163 163 2 2 1
bomb-floor-contact: {model} 1 57 35 37 180 163 2 1 1
""" for model in (0, 10))


class RetailBombFloorContactTest(unittest.TestCase):
    def test_both_owners_damage_only_on_the_first_qualified_growth_visit(self):
        receipt = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(receipt["source_dol_sha256"], contact.floor.SOURCE_SHA256)
        self.assertEqual(receipt["source_afs_sha256"], contact.floor.AFS_SHA256)
        self.assertEqual(receipt["receiver"]["radius"], 22)
        for pilot, hp, parsed in zip(receipt["pilots"], (150, 180), contact.decode_report(REPORT), strict=True):
            self.assertEqual(pilot["source"]["actor_level_one_hp"], hp)
            self.assertEqual(pilot["cases"], parsed["cases"])
            refused, near, far = pilot["cases"]
            for row in pilot["cases"]:
                self.assertTrue(row["impact"]["opening_attack_registered"])
                self.assertFalse(row["impact"]["opening_attack_hit"])
                self.assertEqual(row["impact"]["receiver_hp"], 180)
                self.assertEqual(row["stock_after"], 1)
            self.assertEqual([r["receiver_hp"] for r in refused["growth_visits"]], [180, 180])
            self.assertEqual(refused["receiver_hit_tokens_after_area"], [])
            for row, visit in ((near, 36), (far, 37)):
                self.assertEqual(row["first_hp_write_visit"], visit)
                self.assertEqual(row["receiver_hit_tokens_after_area"], [2])
                self.assertEqual(row["credited_and_received_damage"], 17)
                self.assertEqual([r["registered_area_radius"] for r in row["growth_visits"]], [34.1, 65.1])
                writes = [r for r in row["growth_visits"] if r["hp_written"]]
                self.assertEqual([(r["visit"], r["amount_record"]) for r in writes], [(visit, "802C4778")])

    def test_refuses_premature_damage_parent_token_and_refunded_stock(self):
        for report in (
            REPORT*2,
            REPORT.replace("57 35 37 180 163", "57 35 36 163 163"),
            REPORT.replace("163 163 2 2 1", "163 163 1 2 1"),
            REPORT.replace("180 180 0 0 1", "180 180 0 0 2"),
            REPORT.replace("163 163", "163 146"),
        ):
            with self.subTest(report=report), self.assertRaises(ValueError):
                contact.decode_report(report)

    def test_existing_receipt_is_never_overwritten(self):
        with TemporaryDirectory() as temp:
            out = Path(temp) / "receipt.json"
            out.write_text("unchanged")
            with self.assertRaises(FileExistsError):
                contact.export(None, None, None, None, out)
            self.assertEqual(out.read_text(), "unchanged")
