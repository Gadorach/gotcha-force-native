from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import struct
from tempfile import TemporaryDirectory
import unittest

from tools.analyze import retail_stage_side_census as side


MAPS = Path(__file__).resolve().parents[2] / "docs/api-map"


class RetailStageSideCensusTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((MAPS / "gg4e08-stage-side-census.json").read_bytes())

    def test_full_hex_named_side_archive_set(self):
        archives = self.report["archives"]
        self.assertEqual([row["filename"] for row in archives], list(side.NAMES))
        self.assertEqual([row["member"] for row in archives], list(range(2419, 2480)))
        self.assertEqual(Counter(len(row["placement_rows"]) for row in archives), {4: 24, 6: 20, 8: 16, 16: 1})
        self.assertEqual(sum(len(row["banks"]) for row in archives), 116)

    def test_placement_variants_allocate_different_objects_and_roots(self):
        self.assertEqual(self.report["totals"], {
            "class2_model_only": 180, "class3_collision_owners": 180, "model_roots": 540,
            "published_ordinary_receiver_entries": 265, "published_common_role20_entries": 181,
        })
        types = {x["type"]: x for x in self.report["types"]}
        for archive in self.report["archives"]:
            rows = archive["placement_rows"]
            dynamic = [types[t] for t, _, variant, _ in rows if variant == 0]
            shapes = [s for t in dynamic for s in t["collision_shapes"]]
            self.assertEqual(archive["class3_collision_owners"], len(dynamic))
            self.assertEqual(archive["model_roots"], len(rows)+len(dynamic))
            self.assertEqual(archive["published_ordinary_receiver_entries"], len(shapes))
            self.assertEqual(archive["published_common_role20_entries"], sum(bool(s["descriptor_role"] & 0x20) for s in shapes))

    def test_common_role_does_not_mean_plane(self):
        types = self.report["types"]
        plane_visits = sum(t["class3_visits"] for t in types for s in t["collision_shapes"] if s["shape_kind"] == 4)
        self.assertEqual(plane_visits, 84)
        self.assertEqual([t["type"] for t in types if any(s["shape_kind"] == 4 for s in t["collision_shapes"])], [0, 2, 4, 6])
        self.assertEqual([s["descriptor"] for s in types[11]["collision_shapes"]], [242, 243])
        self.assertEqual([t["collision_owner_hp"] for t in types], [100]*8+[300]*4)
        self.assertEqual([t["break_duration_operand"] for t in types], [600]*8+[-1]*4)

    def test_original_stage_prop_receipt_is_a_subset(self):
        raw = (MAPS / "gg4e08-stage-prop-support.json").read_bytes()
        self.assertEqual(self.report["stage_prop_support_sha256"], sha256(raw).hexdigest())
        prior = json.loads(raw)
        row = next(x for x in self.report["archives"] if x["member"] == prior["side_member"])
        special = next(x for x in self.report["archives"] if x["filename"] == "set0904.arc")
        self.assertEqual(row["sha256"], prior["side_sha256"])
        self.assertEqual(row["placement_rows"][0], [4, 0, 0, 0])
        self.assertEqual(row["banks"], [9, 8])
        self.assertEqual(special["banks"], [7, 7, 7, 7])
        self.assertEqual(special["model_roots"], 24)

    def test_side_parser_rejects_bad_ranges_and_rows(self):
        raw = bytearray(6144)
        struct.pack_into(">III", raw, 0, 2, 1, 1)
        struct.pack_into(">I", raw, 2048, 1)
        struct.pack_into(">I", raw, 4096, 32)
        self.assertEqual(side.read_side(raw)[1], [(0, 0, 0, 0)])
        with self.assertRaisesRegex(ValueError, "archive extent"):
            side.read_side(raw[:-1])
        raw[2055] = 1
        with self.assertRaisesRegex(ValueError, "placement row"):
            side.read_side(raw)
        raw[2055] = 0
        struct.pack_into(">f", raw, 2056, float("inf"))
        with self.assertRaisesRegex(ValueError, "placement row"):
            side.read_side(raw)
        struct.pack_into(">I", raw, 4096, 2049)
        with self.assertRaisesRegex(ValueError, "HSD bank extent"):
            side.read_side(raw)

    def test_refuses_truncated_report_and_existing_output(self):
        with self.assertRaisesRegex(ValueError, "report extent"):
            side.decode("", [(2419, "set0000.arc", "digest", ["bank"], [(0, 0, 0, 0)])], [])
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "census.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                side.export(None, None, None, None, path)
            self.assertEqual(path.read_text(), "keep")
