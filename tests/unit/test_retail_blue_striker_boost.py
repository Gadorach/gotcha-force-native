"""Refuse any change outside normal Blue Striker's staged scalar contract."""
from copy import deepcopy
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest
from unittest.mock import patch

from tools.analyze import retail_blue_striker_boost as boost
from tools.extract.retail_borg_roster import convert


ROWS = (b"GFBORG-GAMEPLAY 1\nvariant=normal\nbody-motion=static-cruise\nbody-record=0\n"
        +boost.row(boost.OLD)+b"move=charge-shot|b|10|airborne|20|none\n")


def receipt():
    return {
        "source_level": 1, "retail_behavior_verified": False, "native_cruise": boost.OLD,
        "source_charge_factory": {"key": 20, "site": "800F80CC"},
    }


class BlueStrikerBoostBindingTest(unittest.TestCase):
    def test_scope_and_exact_row(self):
        meta = receipt()
        with patch.object(boost, "verify_record") as verify:
            for other in (0x0D04, 0x0F00, 0x061B):
                self.assertEqual(boost.bind(other, "", b"", ROWS, meta, {}), ROWS)
            self.assertEqual(meta, receipt())
            verify.assert_not_called()
            self.assertEqual(boost.bind(boost.MODEL, boost.MEMBER_SHA256, b"spec", ROWS, meta, {}),
                             ROWS.replace(boost.row(boost.OLD), boost.row(boost.SCOPED)))
            verify.assert_called_once_with(b"spec")
        self.assertEqual(meta["native_cruise"], (240, 10000, 50, 50, 16, 31))
        self.assertIn("blue_striker_boost_test", meta)
        self.assertFalse(meta["retail_behavior_verified"])

    def test_form_member_record_and_action_context_refuse(self):
        with self.assertRaisesRegex(ValueError, "unqualified Blue Striker row"):
            boost.bind(boost.MODEL, boost.MEMBER_SHA256, bytes(432), ROWS, receipt(), {})
        with patch.object(boost, "verify_record"):
            for rows in (ROWS.replace(b"variant=normal", b"variant=alternate"),
                         ROWS.replace(b"body-record=0", b"body-record=1"),
                         ROWS.replace(b"body-motion=static-cruise", b"body-motion=animated"),
                         ROWS+boost.row(boost.SCOPED)):
                meta = receipt()
                with self.assertRaises(ValueError):
                    boost.bind(boost.MODEL, boost.MEMBER_SHA256, b"", rows, meta, {})
                self.assertEqual(meta, receipt())
            for digest, meta in (("", receipt()),
                                 (boost.MEMBER_SHA256, {**receipt(), "source_charge_factory": {"key": 56}})):
                before = deepcopy(meta)
                with self.assertRaises(ValueError):
                    boost.bind(boost.MODEL, digest, b"", ROWS, meta, {})
                self.assertEqual(meta, before)

    def test_evidence_and_extractor_fail_closed(self):
        with TemporaryDirectory() as tmp:
            root = Path(tmp)
            receipt_path = root/"unqualified.json"
            receipt_path.write_text("{}")
            with self.assertRaisesRegex(ValueError, "unqualified Blue Striker boost receipt"):
                boost.qualify(None, receipt_path)
            with self.assertRaisesRegex(ValueError, "requires --basic-shot-test and --cruise-lab"):
                convert(None, None, None, root/"new", blue_striker_boost_test=True)
            with self.assertRaises(FileExistsError):
                convert(None, None, None, root, blue_striker_boost_test=True)
            self.assertEqual(receipt_path.read_text(), "{}")

    def test_published_unit_rate_trace(self):
        path = Path(__file__).resolve().parents[2]/boost.EVIDENCE
        raw = path.read_bytes()
        self.assertEqual(boost.sha256(raw).hexdigest(), boost.EVIDENCE_SHA256)
        p = json.loads(raw)
        self.assertEqual((p["source_clock_seed"], p["native_minimum_handler_visits"],
                          p["source_native_matched_visits"]), (30, 31, 353))
        self.assertEqual([(r["minimum"], r["exit_visit"]) for r in p["native_controls"]], [(4, 5), (30, 31)])
