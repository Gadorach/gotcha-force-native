"""Exercise bounded CFGs and check the published roster code atlas."""
import json
from contextlib import closing
from pathlib import Path
import sqlite3
import struct
from tempfile import TemporaryDirectory
import unittest
from unittest.mock import Mock

from tools.analyze import retail_code_atlas as atlas


ARTIFACT = Path(__file__).resolve().parents[2] / "docs/api-map/gg4e08-code-atlas.json"


class RetailCodeAtlasTest(unittest.TestCase):
    def test_entry_walk_skips_dead_words_and_keeps_conditional_fallthrough(self):
        start = 0x80003000
        # bc to offset 12; bl to offset 32; blr; bctrl; b out of span.
        words = (0x4182000C, 0x4800001D, 0x4E800020, 0x4E800421, 0x48000014, 0x48000019)
        direct, indirect, reached = atlas.scan(start, struct.pack(">6I", *words))
        self.assertEqual(direct, [(4, "80003020", "call"), (16, "80003024", "branch")])
        self.assertEqual(indirect, [(12, "ctr-call")])
        self.assertEqual(reached, 20)

    def test_local_loop_absolute_branch_and_unknown_indirect(self):
        start = 0x80003000
        words = (0x41820000, 0x4800000E, 0x4E800420)
        self.assertEqual(atlas.scan(start, struct.pack(">3I", *words)), ([(4, "0000000C", "branch")], [], 8))
        self.assertEqual(atlas.scan(start, struct.pack(">I", words[2])), ([], [(0, "ctr-branch")], 4))

    def test_reports_span_fallthrough(self):
        self.assertEqual(atlas.scan(0x80003000, struct.pack(">I", 0x60000000)),
                         ([(0, "80003004", "fallthrough")], [], 4))

    def test_validates_function_boundaries(self):
        image = Mock()
        first = b"one = .text:0x80003000; // type:function size:0xC\n"
        second = b"two = .text:0x80003004; // type:function size:0x8\n"
        self.assertEqual(atlas.boundaries(image, first), {0x80003000: 12})
        image.read.assert_called_with(0x80003000, 12, "text")
        for malformed in (first * 2, first + second, first.replace(b"0xC", b"0x0"),
                          first.replace(b"80003000", b"80003001"), b"// type:function\n"):
            with self.subTest(malformed=malformed), self.assertRaises(ValueError):
                atlas.boundaries(image, malformed)

    def test_symbol_object_disagreement_and_table_bounds(self):
        raw = b"t = .data:0x803429D4; // type:object size:0x10\n"
        self.assertEqual(atlas.object_sizes(raw + raw.replace(b"0x10", b"0x14")), {0x803429D4: 0})
        linker = Mock()
        linker.read_word.return_value = 0x801570B0
        self.assertIsNone(atlas.table_entry(linker, atlas.object_sizes(raw), 0x803429D4, 4))
        linker.read_word.assert_not_called()
        self.assertEqual(atlas.table_entry(linker, atlas.object_sizes(raw), 0x803429D4, 3), 0x801570B0)

    def test_match_metadata_requires_identical_address_and_extent(self):
        with TemporaryDirectory() as temp:
            path = Path(temp) / "match #status.db"
            with closing(sqlite3.connect(path)) as db, db:
                db.execute("create table functions(name,size,status)")
                db.executemany("insert into functions values(?,?,?)",
                               [("fn_80003000", 8, "open"), ("fn_80003008", 4, "matched"),
                                ("fn_8000300C", 4, "matched"), ("fn_80003010_suffix", 8, "matched")])
            self.assertEqual(atlas.matching_status(path, {0x80003000: 8, 0x80003008: 8, 0x80003010: 8}),
                             {0x80003000: "open"})

    def test_output_refuses_existing_file(self):
        with TemporaryDirectory() as temp:
            path = Path(temp) / "exists.json"
            path.write_text("keep")
            with self.assertRaises(FileExistsError):
                atlas.export(None, None, None, path)
            self.assertEqual(path.read_text(), "keep")

    def test_published_atlas_covers_every_borg_and_keeps_dispatch_guards(self):
        raw = ARTIFACT.read_bytes()
        result = json.loads(raw)
        self.assertEqual(atlas.encode(result), raw)
        self.assertEqual(result["source_dol_sha256"], atlas.SOURCE_SHA256)
        borgs = {b["id"]: b for b in result["borgs"]}
        self.assertEqual(len(borgs), 206)
        functions, profiles = result["functions"], result["profiles"]
        self.assertEqual(len(functions), result["counts"]["graph"]["functions"])
        self.assertEqual(result["battle_roots"]["8002F110"]["function"], "8002E2A8")
        self.assertEqual(result["battle_roots"]["8004BEB8"]["function"], "8004BEB8")
        for borg in borgs.values():
            self.assertTrue(borg["reaction_table"] and borg["profiles"])
            self.assertIn(borg["installer"], functions)
            for ident in borg["profiles"]:
                self.assertIn(profiles[ident]["main_handler"], functions)
                self.assertEqual(len(profiles[ident]["groups"]), 6)

        def routes(model):
            return [row["code_path"] for ident in borgs[model]["profiles"]
                    for group in profiles[ident]["groups"] for row in group["rows"] if "code_path" in row]

        guarded = next(r["guarded_families"] for r in routes("gg4e08:0205") if "guarded_families" in r)
        self.assertEqual([r["family_table"] for r in guarded], ["803429C4", "803429D4"])
        self.assertEqual([r["guard"] for r in guarded], ["actor+0x582 == 1", "actor+0x582 != 1"])
        self.assertIn({"direct_entry": "801AE7F4"}, routes("gg4e08:0d06"))
        self.assertIn("802D3BE8", {r.get("substate_table") for r in routes("gg4e08:000a")})
