"""Verify the observed standalone receiver proof before portable emission."""
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import struct
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import damage_contact_roles as roles
from tools.analyze import retail_normal_receiver_factors as normal
from tools.extract.retail_borg_roster import NORMAL_DAMAGE_RECORDS_SHA256


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailNormalReceiverFactorsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        raw = (MAPS/"gg4e08-normal-receiver-factors.json").read_bytes()
        cls.digest = sha256(raw).hexdigest()
        cls.rows = json.loads(raw)["records"]
        cls.report = json.loads((MAPS/"damage-contact-roles.json").read_bytes())

    def test_normal_proof_tracks_all_installations_and_preserves_legacy_refusals(self):
        self.assertEqual(self.digest, NORMAL_DAMAGE_RECORDS_SHA256)
        self.assertEqual(Counter(r["spec_record"] for r in self.rows), {0: 200, 1: 5, 2: 1})
        self.assertTrue(all(len(r["receiver_only_amounts"]) == 4 for r in self.rows))
        legacy = json.loads((MAPS/"borg-damage-selectors.json").read_bytes())
        self.assertEqual(sum(r["borg"][-4:] not in legacy["models"] for r in self.rows), 18)
        self.assertNotIn("0620", legacy["models"])

    def test_nonzero_selected_record_drives_the_same_portable_rows(self):
        unused = bytearray(432)
        active = bytearray(432)
        struct.pack_into(">h", unused, 0, 500)
        struct.pack_into(">h", active, 0, 700)
        active[156:159] = bytes((2, 3, 2))
        proof = {"installer": "8006F4F8", "spec_record": 1, "record_sha256": sha256(active).hexdigest(),
                 "selectors": [2, 3, 2], "reset_capacity": 700}
        data = bytes(unused+active)
        r = roles.normal_profile_rows(self.report, {"gg4e08:0620": proof}, 0x620, 0x8006F4F8, data, image=None)
        self.assertEqual(r["receiver_record"]["spec_record"], 1)
        self.assertEqual(r["receiver_record"]["reset_capacity"], 700)
        self.assertEqual(r["receiver_selector"], [2, 3, 2])
        self.assertEqual(sum(s.startswith("damage-receive=") for s in r["lines"]), 2)
        for alternate in ({**proof, "spec_record": 0}, {**proof, "selectors": [0, 0, 0]},
                          {**proof, "spec_record": 3}, {**proof, "installer": "80000000"}):
            with self.assertRaises(ValueError):
                roles.normal_profile_rows(self.report, {"gg4e08:0620": alternate}, 0x620, 0x8006F4F8, data, image=None)
        with self.assertRaises(ValueError):
            roles.normal_profile_rows(self.report, {}, 0xF05, 0x8006F4F8, data, image=None)

    def test_refuses_replacing_artifact(self):
        with TemporaryDirectory() as tmp:
            out = Path(tmp) / "receivers.json"
            out.write_text("keep")
            with self.assertRaises(FileExistsError):
                normal.export(None, None, None, None, out)
            self.assertEqual(out.read_text(), "keep")
