"""Pin Machine Red's original carrier, shape, and registry distinctions."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_rifle_bucket as rifle


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-rifle-bucket.json"
RELEASE = "rifle: 2 1 1 1 1 0 1 1 9 16 2 21 0\n"
REPORT = RELEASE + """\
rifle-bucket: 0 1 0 0 0 0 0 0 0 0.00000 0.00000 0.00000 0.00000 0.00000 0.00000 0.00000
""" + RELEASE + """\
rifle-bucket: 1 0 0 2 300 0 0 0 0 0.00000 0.00000 0.00000 0.00000 0.00000 0.00000 0.00000
""" + RELEASE + """\
rifle-bucket: 1 1 1 1 300 1 49 1 0 -149.39999 191.00000 365.00000 0.00000 0.00000 79.99999 32.46753
""" + RELEASE + """\
rifle-bucket: 1 1 0 1 300 1 49 1 1 -149.39999 191.00000 365.00000 0.00000 0.00000 79.99999 32.46753
"""


class RetailRifleBucketTest(unittest.TestCase):
    def test_receipt_separates_allocations_and_attack_publication(self):
        data = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(data["source_dol_sha256"], rifle.SOURCE_SHA256)
        self.assertEqual(data["source_afs_sha256"], rifle.AFS_SHA256)
        self.assertEqual(data["rifle"]["source_shape"]["hit_block_sha256"], rifle.HIT_BLOCK_SHA256)
        self.assertEqual(data["cases"], rifle.decode_report(REPORT))
        inhibited, refused, suppressed, listed = data["cases"]
        self.assertEqual([c["stock_after"] for c in data["cases"]], [1, 1, 1, 1])
        self.assertEqual([c["carrier_phase"] for c in data["cases"]], [0, 2, 1, 1])
        self.assertIsNone(inhibited["shape"])
        self.assertIsNone(refused["shape"])
        self.assertEqual(suppressed["shape"], listed["shape"])
        self.assertEqual([suppressed["attack_registry_count"], listed["attack_registry_count"]], [0, 1])

    def test_refuses_disagreement_or_unqualified_inputs(self):
        for altered in (
            "",
            REPORT + REPORT,
            REPORT.replace(RELEASE, "", 1),
            REPORT.replace("2 1 1 1 1 0 1", "2 1 1 1 2 0 1"),
            REPORT.replace("1 49 1 0 -149", "1 49 1 1 -149"),
            REPORT.replace("32.46753", "20.00000"),
            REPORT.replace("79.99999", "nan"),
        ):
            with self.subTest(altered=altered), self.assertRaises(ValueError):
                rifle.decode_report(altered)
        with self.assertRaises(ValueError):
            rifle.source_shape(bytes(rifle.HIT_BYTES))

    def test_existing_receipt_is_never_overwritten(self):
        with TemporaryDirectory() as tmp:
            output = Path(tmp) / "receipt.json"
            output.write_text("preserved")
            with self.assertRaises(FileExistsError):
                rifle.export(None, None, None, None, output)
            self.assertEqual(output.read_text(), "preserved")
