"""Pin the receiver-owned tier data and the scoped contact selector."""
import hashlib
import json
import os
import struct
import tempfile
import unittest
from pathlib import Path

from tools.analyze import damage_contact_tiers

DOL = Path(os.environ.get("GF_RETAIL_ROOT", "original/extracted")) / "sys/main.dol"
ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/damage-contact-tiers.json"


class DamageContactTiersTest(unittest.TestCase):
    def test_physical_table_and_counter_evidence(self):
        data = json.loads(ARTIFACT.read_bytes())
        self.assertEqual(data["table"], "802C7CA0")
        self.assertEqual(data["factors"][:7], [1.0, 0.8, 0.5, 0.25, 0.1, 0.05, 0.02])
        self.assertEqual(len(data["factors"]), 64)
        self.assertEqual(data["factors"][-1], 0.01)
        source = data["source"]
        self.assertEqual((source["flag_offset"], source["bypass_mask"], source["charge_offset"]), (18, 0x4000, 5))
        self.assertEqual((source["threshold"], source["cap"], source["idle_value"]), (100, 63, 60.0))
        reset = data["reset"]
        self.assertEqual((reset["wear"]["record_offset"], reset["capacity"]["record_offset"]), (4, 0))
        self.assertEqual(reset["gate"]["bypass_mask"], 0x80)

    def test_zero_charge_is_valid_but_zero_power_or_disagreeing_routes_are_not(self):
        class Image:
            rows = ((20, 0, 0, 0), (40, 25, 60, 0), (10, 25, 60, 0x4000), (0, 25, 60, 0), (40, 30, 60, 0))

            def read(self, address, size, kind):
                index, offset = divmod(address, 24)
                base, wear, charge, bypass = self.rows[index]
                row = bytearray(24)
                struct.pack_into(">H", row, 0, base)
                row[4:6] = bytes((wear, charge))
                struct.pack_into(">H", row, 18, bypass)
                return bytes(row[offset:offset + size])

        def route(*indices):
            return {"table": "0", "releases": [{"steps": [{"index": i} for i in indices]}]}

        report = json.loads(ARTIFACT.read_bytes())
        result = damage_contact_tiers.profile_rows(report, Image(),
            {"zero-charge": route(0), "bypass": route(2), "mixed": route(1, 2), "zero-power": route(3)},
            receiver=True)
        self.assertEqual(len(result["lines"][0].split("|")), 65)
        self.assertEqual(result["lines"][1:], ["contact-tier=bypass|60|bypass", "contact-tier=zero-charge|0|scale"])
        self.assertIsNone(result["moves"]["mixed"]["charge"])
        self.assertIsNone(result["moves"]["zero-power"]["charge"])
        result = damage_contact_tiers.profile_rows(report, Image(),
            {"mixed-wear": route(1, 4), "zero": route(0), "bypass": route(2), "zero-power": route(3)},
            receiver=True, reset={"capacity": 500})
        self.assertEqual(result["lines"][1:], ["damage-tier-reset=500", "contact-tier=bypass|60|bypass",
            "contact-tier-reset=bypass|25", "contact-tier=mixed-wear|60|scale",
            "contact-tier=zero|0|scale", "contact-tier-reset=zero|0"])
        self.assertIsNone(result["moves"]["mixed-wear"]["reset_wear"])
        self.assertIsNone(result["moves"]["zero-power"]["reset_wear"])

    def test_reset_capacity_requires_exact_single_record(self):
        actor = struct.pack(">h", 700) + bytes(430)
        proof = {"status": "one-static-slot-0-record", "selected_record": 0,
                 "return_candidates": [{"byte_offset": 0, "record_sha256": hashlib.sha256(actor).hexdigest()}]}
        get = damage_contact_tiers.receiver_reset
        self.assertEqual(get(actor, proof, receiver=True)["capacity"], 700)
        self.assertIsNone(get(actor * 2, proof, receiver=True)["capacity"])
        self.assertIsNone(get(actor, proof, receiver=False)["capacity"])
        self.assertIsNone(get(actor, {}, receiver=True)["capacity"])
        with self.assertRaisesRegex(ValueError, "does not match"):
            get(bytes(432), proof, receiver=True)
        zero = bytes(432)
        proof["return_candidates"][0]["record_sha256"] = hashlib.sha256(zero).hexdigest()
        self.assertEqual(get(zero, proof, receiver=True), {"capacity": None, "status": "nonpositive source capacity"})

    @unittest.skipUnless(DOL.is_file(), "original DOL not available")
    def test_regenerates_without_replacing_output(self):
        with tempfile.TemporaryDirectory() as scratch:
            output = Path(scratch) / ARTIFACT.name
            damage_contact_tiers.export(DOL, output)
            self.assertEqual(output.read_bytes(), ARTIFACT.read_bytes())
            with self.assertRaises(FileExistsError):
                damage_contact_tiers.export(DOL, output)


if __name__ == "__main__":
    unittest.main()
