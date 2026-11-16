import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
from urllib.error import HTTPError
from urllib.request import Request, urlopen
from tools.animation_browser import CATEGORIES, motion_catalog, validate_annotation


def motion_fixture():
    raw = bytearray(256)
    struct.pack_into(">I", raw, 0, 64)
    struct.pack_into(">I", raw, 4, 84)
    struct.pack_into(">5I", raw, 64, 128, 0, 160, 0xFFFFFFFF, 0)
    struct.pack_into(">3I", raw, 84, 128, 192, 0xFFFFFFFF)
    for at in (128, 160, 192):
        struct.pack_into(">I", raw, at, 32)
    return raw


class AnimationBrowserTests(unittest.TestCase):
    def test_stable_raw_ids_null_slots_and_aliases(self):
        entries = motion_catalog(motion_fixture())
        self.assertEqual([e["id"] for e in entries], [0, 2, 257])
        self.assertEqual(entries[0]["aliases"], [[0, 0], [1, 0]])
        self.assertEqual([e["id"] for e in motion_catalog(motion_fixture(), True)],
                         [10000, 10002, 10257])
        with self.assertRaises(ValueError):
            motion_catalog(b"short")
        edited = motion_fixture()
        struct.pack_into(">I", edited, 64, 0xFFFFF000)
        with self.assertRaises(ValueError):
            motion_catalog(edited)

    def test_requested_categories_and_validation(self):
        keys = {key for key, _ in CATEGORIES}
        self.assertTrue({"air_dash", "other", "melee_chain_3", "x_attack_charged",
                         "b_attack_normal", "combine", "separate", "transform"} <= keys)
        catalog = {"sources": [{"borg": "g-red", "entries": [{"id": 2}]}]}
        self.assertEqual(validate_annotation(catalog, {"borg": "g-red", "id": 2,
                                                      "category": "other", "notes": "uncertain"}),
                         ("g-red:2", {"category": "other", "notes": "uncertain"}))
        for change in ({"id": 3}, {"borg": "unknown"}, {"category": "bad"},
                       {"notes": "x" * 2001}, {"id": True}):
            item = {"borg": "g-red", "id": 2, "category": "air_dash", "notes": ""}
            item.update(change)
            with self.assertRaises(ValueError):
                validate_annotation(catalog, item)

    def test_local_annotation_round_trip_and_invalid_save(self):
        with tempfile.TemporaryDirectory() as name:
            directory = Path(name)
            catalog = {"fingerprint": "test-dataset",
                       "sources": [{"borg": "g-red", "entries": [{"id": 2}]}]}
            (directory / "catalog.json").write_text(json.dumps(catalog))
            process = subprocess.Popen(
                [sys.executable, "-B", "tools/animation_browser.py", "serve",
                 str(directory), "--port", "0"], stdout=subprocess.PIPE,
                stderr=subprocess.PIPE, text=True)
            try:
                url = process.stdout.readline().strip().split()[-1]
                self.assertTrue(url.startswith("http://127.0.0.1:"))
                payload = {"borg": "g-red", "id": 2, "category": "air_dash",
                           "notes": "direction / timing note"}
                request = Request(url + "api/annotations", json.dumps(payload).encode(),
                                  {"Content-Type": "application/json"})
                with urlopen(request, timeout=5) as response:
                    self.assertEqual(json.load(response)["saved"], "g-red:2")
                saved = (directory / "annotations.json").read_bytes()
                self.assertEqual(json.loads(saved)["entries"]["g-red:2"]["category"], "air_dash")
                payload["category"] = "invalid"
                with self.assertRaises(HTTPError) as error:
                    urlopen(Request(url + "api/annotations", json.dumps(payload).encode(),
                                    {"Content-Type": "application/json"}), timeout=5)
                self.assertEqual(error.exception.code, 400)
                error.exception.close()
                self.assertEqual((directory / "annotations.json").read_bytes(), saved)
            finally:
                process.terminate()
                process.communicate(timeout=5)


if __name__ == "__main__":
    unittest.main()
