"""Pin the descriptor-to-common record join without inferring particle behavior."""
import json
import os
from pathlib import Path
import struct
import tempfile
import unittest

from tools.analyze import contact_impact_visuals as impact

DOL = Path(os.environ.get("GF_RETAIL_ROOT", "original/extracted")) / "sys/main.dol"
AFS = DOL.parents[1] / "files/afs_data.afs"
ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/contact-impact-visuals.json"


class ContactImpactVisualsTest(unittest.TestCase):
    def setUp(self):
        self.report = json.loads(ARTIFACT.read_bytes())

    def test_layers_and_real_template_span(self):
        self.assertEqual(impact.layers(self.report, 1, 0), [(2, 16)])
        self.assertEqual(impact.layers(self.report, 1, 1), [(3, 16)])
        for selector in (2, 3, 4):
            self.assertEqual(impact.layers(self.report, selector, 0), [(144, 7), (145, 10), (146, 13)])
        self.assertEqual(impact.layers(self.report, 8, 1), [(53, 10), (54, 6)])
        with self.assertRaises(ValueError):
            impact.layers(self.report, 0, 0)

    def test_refuses_mixed_and_special_but_does_not_infer_from_sound(self):
        class Image:
            def read(self, address, size, kind):
                index, at = divmod(address, 24)
                row = bytearray(24)
                row[1] = (20, 20, 20, 20, 0, 20)[index]
                row[9] = (1, 2, 8, 1, 1, 0)[index]
                row[10] = 0xC0
                if index == 3:
                    struct.pack_into(">H", row, 16, 0x100)
                return row[at:at + size]

        def route(*indices):
            return {"table": "0", "releases": [{"steps": [{"index": i} for i in indices]}]}
        source_binding = {"status": "one-static-slot-0-record", "selected_record": 0}
        bindings = {"ordinary": route(2), "mixed": route(0, 1), "status": route(3), "zero": route(4),
                    "unknown": route(5)}
        result = impact.profile_rows(self.report, Image(), bindings, source_binding, 432)
        self.assertTrue(result["receiver"])
        self.assertEqual(result["lines"], ["contact-impact-receive=normal", "contact-impact=ordinary|0|53|100|10",
            "contact-impact=ordinary|0|54|100|6", "contact-impact=ordinary|1|53|100|10",
            "contact-impact=ordinary|1|54|100|6"])
        self.assertFalse(impact.profile_rows(self.report, Image(), bindings, source_binding, 864)["receiver"])
        self.assertFalse(impact.profile_rows(self.report, Image(), bindings, None, 432)["receiver"])
        legacy = impact.profile_rows(self.report, Image(), {"ordinary": route(2)}, source_binding, 432,
                                     actor_retirement_art=True)
        self.assertFalse(legacy["requires_records"])
        self.assertEqual(legacy["moves"]["ordinary"]["status"], "package retains authored actor-retirement art")

    @unittest.skipUnless(DOL.is_file(), "original DOL not available")
    def test_regenerates_without_replacing(self):
        with tempfile.TemporaryDirectory() as name:
            out = Path(name) / ARTIFACT.name
            impact.export(DOL, out)
            self.assertEqual(out.read_bytes(), ARTIFACT.read_bytes())
            with self.assertRaises(FileExistsError):
                impact.export(DOL, out)

    @unittest.skipUnless(AFS.is_file(), "original AFS not available")
    def test_common_bank_contains_all_exact_record_roots(self):
        from io import BytesIO
        from tools.extract.retail_borg_roster import MEMBER_LIMIT, PzzArchive, RangeFile, RetailArchive

        binding = self.report["source"]["normal_archive"]
        with AFS.open("rb") as fp:
            archive = RetailArchive(fp)
            self.assertEqual(archive.names[binding["member"]], b"efct.pzz")
            member = archive.payload(binding["member"], MEMBER_LIMIT)
        pzz = PzzArchive(RangeFile(BytesIO(member), 0, len(member)))
        data, _ = pzz.read(binding["slot"], 1 << 20)
        art = impact.qualify_archive(self.report, data)
        self.assertEqual(set(art["records"]), {2, 3, 53, 54, 144, 145, 146})
        with self.assertRaisesRegex(ValueError, "archive identity"):
            impact.qualify_archive(self.report, data[:-1])

if __name__ == "__main__":
    unittest.main()
