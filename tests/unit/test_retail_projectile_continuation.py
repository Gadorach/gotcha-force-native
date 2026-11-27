"""Guard the narrow Workshop binding and its pinned evidence."""
from copy import deepcopy
from pathlib import Path
import struct
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_projectile_continuation as continuation
from tools.extract.retail_borg_roster import convert


GAMEPLAY = b"""GFBORG-GAMEPLAY 1
variant=normal
body-record=0
projectile=shot|bullet|1|32|0|0|0|12|70|8|ignore-floor
move=basic-shot|b|10|grounded|6|3:11
release=basic-shot|3|shot-stock|1|shot
"""


def branch(site, target):
    return 0x48000001 | ((target-site) & 0x03FFFFFC)


class Source:
    sha256 = continuation.SOURCE_SHA256

    def __init__(self, factory=0x800C3BE0):
        self.factory = factory

    def read(self, at, size, kind):
        release, debit = continuation.RELEASE_SITE, continuation.DEBIT_SITE
        spans = {
            (release-4, 8, "text"): struct.pack(">2I", 0x38800015, branch(release, self.factory)),
            (debit, 4, "text"): struct.pack(">I", branch(debit, 0x8006DBE0)),
        }
        return spans[at, size, kind]


class ProjectileContinuationBindingTest(unittest.TestCase):
    def setUp(self):
        self.evidence = continuation.qualify(Source())
        self.receipt = {
            "body_binding": "strict record zero", "source_level": 1, "source_release_site": "800C9F98",
            "source_debit_site": "800C9F78", "source_normal_factory_family": "type-2",
            "source_normal_factory_key": 21, "reaction_releases": {"basic-shot": [("type-2", 21)]},
            "retail_behavior_verified": False, "native_timing": {"release_tick": 3},
        }

    def bind(self, model=continuation.MODEL, member=continuation.MEMBER_SHA256, gameplay=GAMEPLAY, receipt=None):
        return continuation.bind(model, member, gameplay, self.receipt if receipt is None else receipt, self.evidence)

    def test_changes_only_the_reviewed_contact_row(self):
        self.assertEqual(self.bind(), GAMEPLAY + continuation.POLICY + b"\n")
        receipt = self.receipt["projectile_continuation_test"]
        self.assertEqual(receipt["source_evidence_sha256"], continuation.EVIDENCE_SHA256)
        self.assertEqual((receipt["move"], receipt["projectile"]), ("basic-shot", "shot"))
        self.assertFalse(receipt["retail_behavior_verified"])
        with self.assertRaises(ValueError):
            self.bind()

    def test_other_models_are_untouched_even_if_given_the_same_key(self):
        for model in (0x0615, 0x061E, 0x0501, 0x0802):
            original = deepcopy(self.receipt)
            self.assertEqual(self.bind(model), GAMEPLAY)
            self.assertEqual(self.receipt, original)

    def test_refuses_wrong_member_key_release_and_variants(self):
        with self.assertRaises(ValueError):
            self.bind(member="00"*32)
        for rows in (
            GAMEPLAY.replace(b"variant=normal", b"variant=alternate"),
            GAMEPLAY.replace(b"|1|shot\n", b"|1|other\n"),
            GAMEPLAY + b"release-followup=basic-shot|3|other\n",
            GAMEPLAY + b"actor-contact=shot|ignore\n",
        ):
            with self.subTest(rows=rows), self.assertRaises(ValueError):
                self.bind(gameplay=rows)
        with self.assertRaises(ValueError):
            self.bind(receipt={**self.receipt, "source_normal_factory_key": 43})

    def test_requires_the_original_binding_and_refuses_existing_output(self):
        with self.assertRaises(ValueError):
            continuation.qualify(Source(0x80082824))
        with TemporaryDirectory() as tmp:
            folder = Path(tmp)
            fake = folder / "receipt.json"
            fake.write_text("{}")
            with self.assertRaises(ValueError):
                continuation.qualify(Source(), fake)
            with self.assertRaises(ValueError, msg="opt-in must require the basic-shot test"):
                convert(None, None, None, folder / "new", projectile_continuation_test=True)
            with self.assertRaises(FileExistsError):
                convert(None, None, None, folder, projectile_continuation_test=True)
            self.assertEqual(fake.read_text(), "{}")
