"""Exercise the type-89 selector handoff and keep bind, overlap, and HP separate."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_type89_area as area, retail_nu_iii as nu


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-type89-area.json"
REPORT = "".join(f"""\
type89-area: {model} 0 0 0 0 0 0 0 5
type89-area: {model} 1 1 16 0 0 0 0 5
type89-area: {model} 2 1 16 1 1 1 110 5
type89-area: {model} 3 1 16 1 2 1 110 5
type89-area: {model} 4 1 16 1 1 1 110 5
type89-area: {model} 5 1 16 1 1 0 0 5
type89-area: {model} 6 1 16 1 1 0 0 5
""" for model in (3843, 1578))


class RetailType89AreaTest(unittest.TestCase):
    def test_area_cache_requires_opening_bind_and_fixed_key_is_not_a_hit(self):
        data = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(data["source_dol_sha256"], area.SOURCE_SHA256)
        self.assertEqual(data["source_afs_sha256"], area.AFS_SHA256)
        self.assertEqual(data["area_source"]["common_group"], 110)
        self.assertEqual(nu.CHILD_UPDATE, data["entries"]["carrier_update"])
        for pilot, expected in zip(data["pilots"], area.decode_report(REPORT), strict=True):
            self.assertEqual(pilot["cases"], expected["cases"])
            refusal, neither, contact, flag, both, no_object, short_descriptors = pilot["cases"]
            self.assertIsNone(refusal["copied_area_key"])
            self.assertEqual(neither["copied_area_key"], 16)
            self.assertFalse(neither["area_factory_requested"])
            self.assertEqual(contact["origin"], both["origin"])
            self.assertEqual(contact["origin"]["carrier_field"], "0x2A0")
            self.assertEqual(flag["origin"]["carrier_field"], "0x20")
            self.assertTrue(contact["area_allocated"])
            self.assertEqual(contact["area_bound_common_group"], 110)
            for failed in (no_object, short_descriptors):
                self.assertEqual(failed["area_requested_key"], 16)
                self.assertFalse(failed["area_allocated"])
                self.assertIsNone(failed["area_bound_common_group"])
                self.assertEqual(failed["owner_stock_after"], 5)

    def test_refuses_seeded_cache_wrong_origin_or_faked_allocation(self):
        for report in (
            REPORT*2,
            REPORT.replace("0 0 0 0 0 0 0 5", "0 0 16 0 0 0 0 5"),
            REPORT.replace("4 1 16 1 1 1 110", "4 1 16 1 2 1 110"),
            REPORT.replace("6 1 16 1 1 0 0", "6 1 16 1 1 1 110"),
        ):
            with self.subTest(report=report), self.assertRaises(ValueError):
                area.decode_report(report)

    def test_existing_receipt_is_not_overwritten(self):
        with TemporaryDirectory() as tmp:
            out = Path(tmp) / "receipt.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                area.export(None, None, None, None, out)
            self.assertEqual(out.read_text(), "keep")
