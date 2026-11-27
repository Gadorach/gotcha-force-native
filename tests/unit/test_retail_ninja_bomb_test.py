"""Limit the authored bomb to a fresh, exact normal Workshop bind."""
from hashlib import sha256
from pathlib import Path
from tempfile import TemporaryDirectory
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from tools.analyze import retail_ninja_bomb_test as bomb
from tools.extract.retail_borg_roster import convert


ROOT = Path(__file__).resolve().parents[2]
MOTION = b"motion fixture, not game data"
GAMEPLAY = b"""\
GFBORG-GAMEPLAY 1
variant=normal
body-record=0
move=basic-shot|b|10|grounded|10|3:0
release=basic-shot|6|shot-stock|1|shot
"""


class RetailNinjaBombTest(unittest.TestCase):
    def fixture(self, model):
        member, digest, b_key, b_site, _, _ = bomb.PILOTS[model]
        receipt = {"body_binding": "strict record zero", "source_normal_factory_family": "type-14",
                   "source_normal_factory_key": b_key, "source_release_site": b_site, "retail_behavior_verified": False}
        return {"model": model, "member": member, "digest": digest, "gameplay": GAMEPLAY, "receipt": receipt,
                "evidence": {"retail_behavior_verified": False}, "motion": MOTION, "clips": [bomb.X_SLOT],
                "timing": [{"archive_offset": bomb.X_SLOT[2], "aobj_flags": [0], "authored_end_frames": [45.],
                            "animated_joint_count": 20}],
                "joints": [{"index": i} for i in range(12)], "level": [150, 10, 10, 0, 90, 5, 5, 0, 300]}

    def test_appends_only_scoped_x_with_separate_stock_and_area(self):
        with patch.object(bomb, "MOTION_SHA256", sha256(MOTION).hexdigest()):
            for model, key in ((0, 0), (10, 3)):
                args = self.fixture(model)
                bound = bomb.bind(**args)
                self.assertEqual(bound, GAMEPLAY + ("\n".join(bomb.ROWS)+"\n").encode("ascii"))
                self.assertIn(b"impact-area=ninja-bomb|impact|", bound)
                self.assertIn(b"impact-area-shapes=ninja-bomb|shared\n", bound)
                self.assertIn(b"release=ninja-bomb|12|bomb-stock|1|ninja-bomb\n", bound)
                proof = args["receipt"]["ninja_bomb_test"]
                self.assertEqual((proof["source_factory_key"], proof["pose"]), (key, "4:0"))
                self.assertFalse(proof["retail_behavior_verified"])
                self.assertFalse(proof["source_timing_converted"])
                with self.assertRaises(ValueError):
                    bomb.bind(**args)
        for model in (3, 12, 0x0600):
            args = self.fixture(0)
            args["model"] = model
            self.assertIs(bomb.bind(**args), GAMEPLAY)
            self.assertNotIn("ninja_bomb_test", args["receipt"])

    def test_requires_exact_rig_source_stock_and_no_conflicting_x(self):
        changes = ({"member": 495}, {"digest": "different"}, {"motion": MOTION+b"x"},
                   {"clips": [(4, 1, 285024)]}, {"joints": []},
                   {"level": [150, 10, 10, 0, 90, 4, 5, 0, 300]},
                   {"gameplay": GAMEPLAY+b"move=other|x|20|grounded|8|4:0\n"},
                   {"gameplay": GAMEPLAY.replace(b"variant=normal", b"variant=edited")})
        with patch.object(bomb, "MOTION_SHA256", sha256(MOTION).hexdigest()):
            for change in changes:
                with self.subTest(change=change), self.assertRaises(ValueError):
                    bomb.bind(**(self.fixture(0) | change))

    def test_receipts_pinned_and_bad_extraction_does_not_create_output(self):
        for label, digest in bomb.EVIDENCE.items():
            data = (ROOT / "docs/api-map" / f"gg4e08-bomb-{label}.json").read_bytes()
            self.assertEqual(sha256(data).hexdigest(), digest)
        with self.assertRaises(ValueError):
            bomb.qualify(SimpleNamespace(sha256="unqualified"))
        with TemporaryDirectory() as temp:
            root = Path(temp)
            (root/"gg4e08-bomb-x-cue.json").write_text("{}")
            with self.assertRaisesRegex(ValueError, "unqualified.*receipt"):
                bomb.qualify(SimpleNamespace(sha256=bomb.SOURCE_SHA256), root)
            out = root/"fresh"
            with self.assertRaisesRegex(ValueError, "--ninja-bomb-test requires --basic-shot-test"):
                convert(None, None, None, out, ninja_bomb_test=True)
            self.assertFalse(out.exists())
            with self.assertRaises(FileExistsError):
                convert(None, None, None, root, ninja_bomb_test=True)
