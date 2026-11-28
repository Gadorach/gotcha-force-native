"""Keep the exact three-package opt-in separate from the ordinary roster."""
from copy import deepcopy
import json
from pathlib import Path
import struct
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_projectile_contact_tail as tail
from tools.extract.retail_borg_roster import convert


GAMEPLAY = b"""GFBORG-GAMEPLAY 1
variant=normal
body-record=0
projectile=shot|bullet|1|32|0|0|0|12|70|8|ignore-floor
move=basic-shot|b|10|grounded|28|3:11
release=basic-shot|12|shot-stock|1|shot
"""


class Source:
    sha256 = tail.SOURCE_SHA256

    def __init__(self, corrupt=None):
        self.words = {}
        for route in tail.ROUTES.values():
            for site, value, target in ((route.release, 0x38800000 | route.key, 0x80082824),
                                       (route.debit, 0x38C00001, 0x8006DBE0)):
                branch = 0x48000001 | ((target-site) & 0x03FFFFFC)
                self.words[site-4, 8, "text"] = struct.pack(">II", value, branch if site != corrupt else 0)

    def read(self, at, size, kind):
        return self.words[at, size, kind]


def receipt(model):
    route = tail.ROUTES[model]
    return {
        "body_binding": "strict record zero", "source_level": 1, "source_release_site": f"{route.release:08X}",
        "source_debit_site": f"{route.debit:08X}", "source_normal_factory_family": "type-0",
        "source_normal_factory_key": route.key, "reaction_releases": {"basic-shot": [("type-0", route.key)]},
        "retail_behavior_verified": False, "native_timing": {"release_tick": 12},
        **({"source_release_entry": "8013D600", "source_model_gated_muzzle": "only model 0802; form 0803 would request 34"}
           if model == 0x0802 else {}),
    }


class ProjectileContactTailBindingTest(unittest.TestCase):
    def setUp(self):
        self.evidence = tail.qualify(Source())

    def test_all_three_exact_routes_add_only_the_timing_policy(self):
        self.assertEqual({m: proof["source_factory_key"] for m, proof in self.evidence.items()},
                         {0x0800: 12, 0x0801: 125, 0x0802: 45})
        for model, route in tail.ROUTES.items():
            r = receipt(model)
            self.assertEqual(tail.bind(model, route.member, GAMEPLAY, r, self.evidence), GAMEPLAY + tail.POLICY)
            self.assertFalse(r["projectile_contact_tail_test"]["retail_behavior_verified"])
            self.assertEqual(r["projectile_contact_tail_test"]["carrier_return_visits"], 3)
            with self.assertRaises(ValueError):
                tail.bind(model, route.member, GAMEPLAY, r, self.evidence)

    def test_other_models_and_fusion_forms_are_untouched(self):
        r = receipt(0x0802)
        old = deepcopy(r)
        for model in (0x0803, 0x0600, 0x0100):
            self.assertEqual(tail.bind(model, tail.ROUTES[0x0802].member, GAMEPLAY, r, self.evidence), GAMEPLAY)
        self.assertEqual(r, old)

    def test_wrong_member_route_or_conflicting_rows_refuse(self):
        m = 0x0802
        member = tail.ROUTES[m].member
        for proof in ({**receipt(m), "source_model_gated_muzzle": None},
                      {**receipt(m), "source_normal_factory_key": 34},
                      {**receipt(m), "source_release_entry": "8013D77C"}):
            with self.assertRaises(ValueError):
                tail.bind(m, member, GAMEPLAY, proof, self.evidence)
        for rows in (GAMEPLAY.replace(b"variant=normal", b"variant=alternate"),
                     GAMEPLAY.replace(b"|1|shot\n", b"|1|other\n"),
                     GAMEPLAY+b"actor-contact=shot|continue-once-per-actor\n",
                     GAMEPLAY+b"release-followup=basic-shot|12|other\n"):
            with self.assertRaises(ValueError):
                tail.bind(m, member, rows, receipt(m), self.evidence)
        with self.assertRaises(ValueError):
            tail.bind(m, "00"*32, GAMEPLAY, receipt(m), self.evidence)

    def test_source_pin_flag_and_output_refusals(self):
        with self.assertRaises(ValueError):
            tail.qualify(Source(tail.ROUTES[0x0801].release))
        with TemporaryDirectory() as tmp:
            folder = Path(tmp)
            fake = folder / "receipt.json"
            fake.write_text("{}")
            with self.assertRaises(ValueError):
                tail.qualify(Source(), fake)
            with self.assertRaisesRegex(ValueError, "requires --basic-shot-test"):
                convert(None, None, None, folder/"new", projectile_contact_tail_test=True)
            with self.assertRaises(FileExistsError):
                convert(None, None, None, folder, projectile_contact_tail_test=True)
            self.assertEqual(fake.read_text(), "{}")

    def test_published_native_trace_distinguishes_observed_lifetimes_and_source_amounts(self):
        report = json.loads((Path(__file__).resolve().parents[2] /
                             "docs/api-map/native-projectile-contact-comparison.json").read_bytes())
        self.assertEqual(report["original_type0_sha256"], tail.EVIDENCE_SHA256)
        cases = {row["case"]: row for row in report["native_authored_cases"]}
        self.assertEqual([cases[n]["native_slot_return_updates_after_last_hit"] for n in
                          ("ordinary-default", "ordinary-held-slot")], [0, 3])
        self.assertEqual(cases["ordinary-miss"]["hp_removed"], [])
        self.assertEqual(cases["rifle-continuation"]["recipients_in_order"], [1, 2])
        self.assertEqual(cases["rifle-second-miss"]["recipients_in_order"], [1])
