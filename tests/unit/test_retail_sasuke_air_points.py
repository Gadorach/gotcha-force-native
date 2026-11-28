from copy import deepcopy
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_sasuke_air_point_policy as policy
from tools.extract.retail_borg_roster import convert


GAMEPLAY = (
    b"GFBORG-GAMEPLAY 1\nvariant=normal\nbody-record=0\n"
    b"movement=grounded-v1|5|13|15\nair-action-budget=3\n"
)


class Source:
    sha256 = policy.SOURCE_SHA256
    text = {
        0x8006137C: bytes.fromhex("887F07477C6007754082000C38030001981F0747"),
        0x80055DE0: bytes.fromhex("800305E05400D7FE98030087"),
        0x80055A9C: bytes.fromhex("38000000981F0747"),
    }

    def read(self, at, size, kind):
        assert kind == "text" and size == len(self.text[at])
        return self.text[at]


def receipt():
    return {
        "source_level": 1, "retail_behavior_verified": False,
        "air_action_budget": {
            "operand": "09F", "value": 3, "source": "combined jump and air-dash cap",
            "retail_behaviour_verified": False,
        },
    }


class RetailSasukeAirPointTest(unittest.TestCase):
    def setUp(self):
        self.proof = policy.qualify(Source())

    def test_only_normal_sasuke_switches_the_exact_authored_rows(self):
        r = receipt()
        before = deepcopy(r)
        for model in (0x0009, 0x000B, 0x0600):
            self.assertEqual(policy.bind(model, "wrong member", GAMEPLAY, r, self.proof), GAMEPLAY)
        self.assertEqual(r, before)
        self.assertEqual(policy.bind(policy.MODEL, policy.MEMBER_SHA256, GAMEPLAY, r, self.proof),
                         GAMEPLAY.replace(policy.OLD+b"\n", b"\n".join(policy.POINT_ROWS)+b"\n"))
        self.assertNotIn("air_action_budget", r)
        self.assertEqual(r["sasuke_air_points_test"], self.proof)
        self.assertFalse(r["sasuke_air_points_test"]["retail_behavior_verified"])

    def test_form_member_and_incompatible_package_refusals(self):
        for rows in (GAMEPLAY.replace(b"variant=normal", b"variant=alternate"),
                     GAMEPLAY.replace(b"body-record=0", b"body-record=1"),
                     GAMEPLAY.replace(b"air-action-budget=3", b"air-action-budget=2"),
                     GAMEPLAY+b"air-dash=none\n",
                     GAMEPLAY+b"air-point-reset=next-grounded-update\n"):
            r = receipt()
            with self.assertRaises(ValueError):
                policy.bind(policy.MODEL, policy.MEMBER_SHA256, rows, r, self.proof)
            self.assertEqual(r, receipt())
        with self.assertRaises(ValueError):
            policy.bind(policy.MODEL, "0"*64, GAMEPLAY, receipt(), self.proof)
        with self.assertRaises(ValueError):
            policy.bind(policy.MODEL, policy.MEMBER_SHA256, GAMEPLAY, None, self.proof)

    def test_evidence_and_extraction_fail_closed(self):
        source = Source()
        source.text = {**source.text, 0x80055A9C: b"\x00"*8}
        with self.assertRaisesRegex(ValueError, "seed or reset store"):
            policy.qualify(source)
        with TemporaryDirectory() as tmp:
            root = Path(tmp)
            fake = root/"receipt.json"
            fake.write_text("{}")
            with self.assertRaisesRegex(ValueError, "unqualified"):
                policy.qualify(Source(), fake)
            with self.assertRaisesRegex(ValueError, "requires --basic-shot-test"):
                convert(None, None, None, root/"new", sasuke_air_points_test=True)
            with self.assertRaises(FileExistsError):
                convert(None, None, None, root, sasuke_air_points_test=True)
            self.assertEqual(fake.read_text(), "{}")
