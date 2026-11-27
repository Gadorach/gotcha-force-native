"""Check the source sound selectors without substituting an unproved hit route."""
from hashlib import sha256
import json
import os
from pathlib import Path
import struct
import tempfile
from types import SimpleNamespace
import unittest

from tools.analyze import contact_sound
from tools.analyze import retail_normal_receiver_sound as normal
from tools.extract.retail_borg_roster import NORMAL_SOUND_RECORDS, NORMAL_SOUND_RECORDS_SHA256, _normal_receiver_proofs

DOL = Path(os.environ.get("GF_RETAIL_ROOT", "original/extracted")) / "sys/main.dol"
ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/contact-sound.json"


class ContactSoundTest(unittest.TestCase):
    def setUp(self):
        self.report = json.loads(ARTIFACT.read_bytes())

    def test_pairs_and_fixed_code(self):
        pairs = lambda code: contact_sound.normal_pairs(self.report, code)
        self.assertEqual(pairs(0x80), [(0, 48), (1, 50), (2, 52), (3, 54)])
        self.assertEqual(pairs(0x84), [(4, 56), (5, 58), (6, 60), (7, 62)])
        self.assertEqual(pairs(0x88), [(8, 64), (9, 66), (10, 68), (11, 70)])
        self.assertEqual(pairs(0), [(0, 48)] * 4)
        self.assertEqual(pairs(0xC0), [])

    def test_banded_size_requires_exact_record_and_closed_unanimous_configurations(self):
        class Image:
            def read(self, address, length, kind):
                return struct.pack(">2f", 100, {76: 420, 176: 421, 276: 110}[address])

        data = b"\0"*432 + b"\1"*432
        proof = {"installer": "8006F4F8", "spec_record": 1, "record_sha256": sha256(data[432:]).hexdigest(),
                 "config": "00000000", "band": 2,
                 "size_bits": struct.pack(">f", 420/self.report["source"]["divisor"]).hex().upper()}

        def configs(*pointers):
            return {"profiles": [{"config": f"{p:08X}"} for p in pointers]}

        def get(actions, row=proof, records=data):
            return contact_sound.normal_receiver_band(self.report, Image(), actions, row, 0x8006F4F8, records)

        result = get(configs(0, 100))
        self.assertEqual((result["band"], result["spec_record"]), (2, 1))
        for actions in (configs(0, 200), configs(100), {**configs(0), "open_path_boundaries": 1}):
            self.assertIsNone(get(actions)["band"])
        self.assertIsNone(get(configs(0), {**proof, "size_bits": "00000000"})["band"])
        for wrong in (None, {**proof, "spec_record": 0}, {**proof, "installer": "8006F4FC"}):
            with self.assertRaises(ValueError):
                get(configs(0), wrong)
        with self.assertRaises(ValueError):
            get(configs(0), records=data[:-1])

    def test_roster_proof_covers_only_the_exact_normal_records(self):
        raw = NORMAL_SOUND_RECORDS.read_bytes()
        self.assertEqual(sha256(raw).hexdigest(), NORMAL_SOUND_RECORDS_SHA256)
        proof = json.loads(raw)
        self.assertEqual(proof["contact_sound_replay_sha256"], normal.REPLAY_SHA256)
        self.assertEqual(proof["band_counts"], {"0": 136, "1": 26, "2": 19, "3": 25})
        amounts = json.loads(ARTIFACT.with_name("gg4e08-normal-receiver-factors.json").read_bytes())["records"]
        self.assertEqual(len(proof["records"]), 206)
        catalog = [SimpleNamespace(id=row["borg"]) for row in proof["records"]]
        with self.assertRaisesRegex(ValueError, "roster mismatch"):
            _normal_receiver_proofs(NORMAL_SOUND_RECORDS, NORMAL_SOUND_RECORDS_SHA256, proof["kind"], catalog[:-1])
        for row, other in zip(proof["records"], amounts, strict=True):
            for key in ("borg", "installer", "spec_record", "record_sha256"):
                self.assertEqual(row[key], other[key])
            self.assertEqual(row["lanes_and_even_tones"],
                             [list(contact_sound.normal_pairs(self.report, code)[row["band"]])
                              for code in proof["codes"]])

    def test_refuses_bad_report_and_preserves_existing_proof(self):
        with self.assertRaises(ValueError):
            normal.decode("receiver-sound:", None, None, [None])
        with tempfile.TemporaryDirectory() as name:
            path = Path(name) / "proof.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                normal.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")

    def test_routes_require_agreement_and_refuse_special_status(self):
        class Image:
            def read(self, address, count, kind):
                index, at = divmod(address, 24)
                row = bytearray(24)
                row[1] = 0 if index == 5 else 20
                row[9] = 1
                row[10] = (0x80, 0x80, 0x84, 0xC0, 0x80, 0x80)[index]
                if index == 4:
                    struct.pack_into(">H", row, 18, 0x0020)
                return row[at:at + count]

        def route(*indices):
            return {"table": "0", "contacts": [{"steps": [{"index": i} for i in indices]}]}
        use = lambda *indices: contact_sound.move_sound(Image(), route(*indices))
        self.assertEqual(use(0, 1)["code"], 0x80)
        for indices in ((0, 2), (3,), (4,), (5,)):
            self.assertIsNone(use(*indices)["code"])

    @unittest.skipUnless(DOL.is_file(), "original DOL not available")
    def test_regeneration_and_refusal_to_replace(self):
        with tempfile.TemporaryDirectory() as name:
            out = Path(name) / ARTIFACT.name
            contact_sound.export(DOL, out)
            self.assertEqual(out.read_bytes(), ARTIFACT.read_bytes())
            with self.assertRaises(FileExistsError):
                contact_sound.export(DOL, out)


if __name__ == "__main__":
    unittest.main()
