"""Keep original rifle contact, attack removal, and pool return separate."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_rifle_lifetime as rifle


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-rifle-lifetime.json"
OBSERVATIONS = """\
0 0 1 0 0 0 300 1 0 0 0 1 365.00000 1.62338
0 1 1 0 0 0 299 1 0 0 0 1 445.00000 1.62338
0 300 2 2 1 0 0 0 1 0 0 1 41925.00000 1.62338
0 334 3 2 1 0 0 0 1 0 0 1 47365.00000 0.04515
0 335 3 2 1 0 0 0 1 1 1 1 47365.00000 0.04515

1 0 1 0 0 16 300 1 0 0 0 1 365.00000 1.62338
1 1 1 0 0 16 299 1 0 0 0 1 445.00000 1.62338

2 0 1 0 0 2 300 1 0 0 0 1 365.00000 1.62338
2 1 2 1 1 2 300 0 1 0 0 1 445.00000 1.62338
2 3 3 1 1 2 300 0 1 0 0 1 445.00000 1.62338
2 4 3 1 1 2 300 0 1 1 1 1 445.00000 1.62338

3 0 2 0 0 0 300 0 0 0 0 1 0.00000 0.00000
3 1 3 0 0 0 300 0 0 0 0 1 0.00000 0.00000
3 2 3 0 0 0 300 0 0 1 1 1 0.00000 0.00000

4 0 1 0 0 0 300 1 0 0 0 1 365.00000 1.62338
4 1 1 0 0 0 299 1 0 0 0 1 445.00000 1.62338
4 237 2 2 1 0 64 0 1 0 0 1 31845.00000 1.62338
4 271 3 2 1 0 64 0 1 0 0 1 37285.00000 0.04515
4 272 3 2 1 0 64 0 1 1 1 1 37285.00000 0.04515
"""


def report():
    lines = []
    for i, rows in enumerate(OBSERVATIONS.strip().split("\n\n")):
        lines.extend((rifle.RELEASE, rifle.EMPTY if i == 3 else rifle.PUBLISHED))
        if i == 1:
            lines.append(rifle.HIT)
        lines.extend(f"rifle-life: {row}" for row in rows.splitlines())
    return "\n".join(lines) + "\n"


class RetailRifleLifetimeTest(unittest.TestCase):
    def test_exported_cases_keep_attack_unlink_and_pool_returns_separate(self):
        result = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(result["source_dol_sha256"], rifle.SOURCE_SHA256)
        self.assertEqual(result["source_afs_sha256"], rifle.AFS_SHA256)
        self.assertEqual(result["cases"], rifle.decode_report(report()))
        expiry, hit, stop, failed_bind, bounds = (c["visits"] for c in result["cases"])
        self.assertEqual((expiry[2]["visit"], expiry[-1]["visit"]), (300, 335))
        self.assertEqual((expiry[2]["shape_free_count"], expiry[2]["carrier_free_count"]), (1, 0))
        self.assertEqual(expiry[-1]["descriptor_free_count"], 1)
        self.assertEqual((hit[-1]["contact_flags"], hit[-1]["attack_registry_count"]), (16, 1))
        self.assertEqual((stop[-1]["visit"], failed_bind[-1]["visit"]), (4, 2))
        self.assertEqual((bounds[2]["visit"], bounds[2]["carrier_timer"]), (237, 64))

    def test_rejects_early_return_and_contact_stopping(self):
        valid = report()
        for altered in (
            "",
            valid * 2,
            valid.replace("300 2 2 1 0 0 0 1 0 0 1", "300 2 2 1 0 0 0 1 1 0 1"),
            valid.replace("1 1 1 0 0 16 299 1", "1 1 1 0 1 16 299 0"),
            valid.replace("237 2 2 1 0 64", "237 2 2 1 0 0"),
            valid.replace("2 4 3 1 1 2 300 0 1 1 1", "2 4 3 1 1 2 300 0 1 1 0"),
        ):
            with self.subTest(altered=altered), self.assertRaises(ValueError):
                rifle.decode_report(altered)

    def test_export_refuses_existing_path(self):
        with TemporaryDirectory() as tmp:
            output = Path(tmp) / "receipt.json"
            output.write_text("preserved")
            with self.assertRaises(FileExistsError):
                rifle.export(None, None, None, None, output)
            self.assertEqual(output.read_text(), "preserved")
