"""Pin the proved countdown, participant ownership and native hold mapping."""
import json
import os
from pathlib import Path
import struct
import tempfile
import unittest

from tools.analyze import contact_pose_pause as pause

ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/contact-pose-pause.json"
DOL = Path(os.environ.get("GF_RETAIL_ROOT", "original/extracted")) / "sys/main.dol"


class ContactPosePauseTest(unittest.TestCase):
    def test_source_timer_is_not_a_world_freeze(self):
        source = json.loads(ARTIFACT.read_bytes())["source"]
        self.assertEqual((source["attack_record_offset"], source["body_timer_offset"]), (8, 1820))
        self.assertTrue(source["attacking_object"]["does_not_follow_credit_link"])
        self.assertEqual(source["slowdown"]["factor"], .03)
        self.assertIn("after decrement", source["slowdown"]["condition"])

    def test_only_agreeing_positive_normal_body_routes_bind(self):
        class Image:
            def read(self, address, count, kind):
                index, offset = divmod(address, 24)
                row = bytearray(24)
                row[1] = 0 if index == 4 else 20
                row[8] = (1, 4, 8, 4, 4, 9)[index]
                if index == 3:
                    struct.pack_into(">H", row, 18, 0x40)
                return row[offset:offset + count]

        def route(*indices, scope="releases", proof=None):
            return {"table": "0", scope: [{"steps": [{"index": index} for index in indices]}],
                    "allocator_call": proof}

        proof = {"status": "one-static-slot-0-record", "selected_record": 0}
        routes = {"seed1": route(0), "shot": route(2), "melee": route(1, scope="contacts", proof="8004C6F0"),
                  "mixed": route(1, 2), "zero": route(4), "bypass": route(3), "large": route(5),
                  "no-carrier": route(2, scope="contacts")}
        result = pause.profile_rows(Image(), routes, proof, 432, native=True)
        self.assertEqual(result["lines"], ["contact-pose-hold-receive=normal", "contact-pose-hold=melee|3|bodies",
                                           "contact-pose-hold=shot|7|receiver"])
        self.assertFalse(pause.profile_rows(Image(), routes, proof, 864, native=True)["receiver"])
        self.assertEqual(pause.profile_rows(Image(), routes, proof, 432, native=False)["lines"], [])

    @unittest.skipUnless(DOL.is_file(), "original DOL not available")
    def test_regeneration_refuses_to_replace_output(self):
        with tempfile.TemporaryDirectory() as scratch:
            output = Path(scratch) / ARTIFACT.name
            pause.export(DOL, output)
            self.assertEqual(output.read_bytes(), ARTIFACT.read_bytes())
            with self.assertRaises(FileExistsError):
                pause.export(DOL, output)


if __name__ == "__main__":
    unittest.main()
