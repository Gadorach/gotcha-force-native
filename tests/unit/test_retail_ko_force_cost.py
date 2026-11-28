"""Bind the verified normal Tau cost only to its own authored melee package."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_ko_force_cost as ko
from tools.extract.retail_borg_roster import convert


ROWS = b"GFBORG-GAMEPLAY 1\nvariant=normal\nhealth=60\nmelee=opening-slash|9|14|88|38|52|44|12\n"
SOURCE = {"cost": ko.COST, "variant": 0, "member_sha256": ko.MEMBER_SHA256, "receipt_sha256": ko.EVIDENCE_SHA256}


class KoForceBindingTest(unittest.TestCase):
    def test_normal_cost_is_an_optional_ledger_operand_only(self):
        metadata = {"retail_behavior_verified": False}
        for model, source in ((ko.MODEL, None), (0x0806, SOURCE), (0x0808, SOURCE)):
            self.assertEqual(ko.bind(model, -1, "", ROWS, metadata, source), ROWS)
        self.assertEqual(metadata, {"retail_behavior_verified": False})
        actual = ko.bind(ko.MODEL, ko.MEMBER, ko.MEMBER_SHA256, ROWS, metadata, SOURCE)
        self.assertEqual(actual, ROWS+b"ko-force-cost=90\n")
        self.assertEqual(metadata["ko_force_cost_test"], SOURCE)
        self.assertFalse(metadata["retail_behavior_verified"])

    def test_variant_member_and_duplicate_refuse_without_mutating_receipt(self):
        for member, digest, gameplay, source in (
            (-1, ko.MEMBER_SHA256, ROWS, SOURCE),
            (ko.MEMBER, "", ROWS, SOURCE),
            (ko.MEMBER, ko.MEMBER_SHA256, ROWS.replace(b"normal", b"alternate"), SOURCE),
            (ko.MEMBER, ko.MEMBER_SHA256, ROWS.replace(b"health=60", b"health=1"), SOURCE),
            (ko.MEMBER, ko.MEMBER_SHA256, ROWS+b"ko-force-cost=90\n", SOURCE),
            (ko.MEMBER, ko.MEMBER_SHA256, ROWS, {**SOURCE, "variant": 1}),
            (ko.MEMBER, ko.MEMBER_SHA256, ROWS, {**SOURCE, "receipt_sha256": ""}),
        ):
            meta = {}
            with self.assertRaisesRegex(ValueError, "exact normal melee package"):
                ko.bind(ko.MODEL, member, digest, gameplay, meta, source)
            self.assertFalse(meta)

    def test_known_cost_and_extractor_preconditions(self):
        raw = (Path(__file__).resolve().parents[2]/ko.EVIDENCE).read_bytes()
        self.assertEqual(ko.sha256(raw).hexdigest(), ko.EVIDENCE_SHA256)
        self.assertEqual(json.loads(raw)["cached_force_cost_per_actor"], ko.COST)
        with TemporaryDirectory() as tmp:
            root = Path(tmp)
            with self.assertRaisesRegex(ValueError, "requires --basic-melee-test"):
                convert(None, None, None, root/"out", ko_force_cost_test=True)
            with self.assertRaises(FileExistsError):
                convert(None, None, None, root, ko_force_cost_test=True)
