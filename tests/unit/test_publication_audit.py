from pathlib import Path
import subprocess
import tempfile
import unittest
from tools.audit_publication import audit


class PublicationAuditTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        subprocess.run(["git", "init", "-q", str(self.root)], check=True)
        (self.root / "engine").mkdir()
        (self.root / "engine/owned.hpp").write_text("#pragma once\n")

    def test_native_text_only_and_ignored_private_tree(self):
        (self.root / ".gitignore").write_text("/local/\n")
        (self.root / "local").mkdir()
        (self.root / "local/owned-negative-fixture.bin").write_bytes(b"\0PRIVATE")
        result = audit(self.root)
        self.assertEqual({x["path"] for x in result["files"]}, {".gitignore", "engine/owned.hpp"})

    def test_binary_or_unapproved_path_refuses(self):
        (self.root / "engine/owned.hpp").write_bytes(b"\0binary")
        with self.assertRaisesRegex(ValueError, "binary"):
            audit(self.root)
        (self.root / "engine/owned.hpp").write_text("#pragma once\n")
        (self.root / "data.bin").write_bytes(b"data")
        with self.assertRaises(ValueError):
            audit(self.root)

    def test_symbolic_links_and_credential_patterns_refuse(self):
        (self.root / "engine/alias.hpp").symlink_to(self.root / "engine/owned.hpp")
        with self.assertRaises(ValueError):
            audit(self.root)
        (self.root / "engine/alias.hpp").unlink()
        (self.root / "engine/owned.hpp").write_text("ghp_" + "a" * 36)
        with self.assertRaisesRegex(ValueError, "credential"):
            audit(self.root)

    def test_staged_audit_reads_exact_index_bytes(self):
        subprocess.run(["git", "-C", str(self.root), "add", "engine/owned.hpp"], check=True)
        before = audit(self.root, staged=True)
        (self.root / "engine/owned.hpp").write_text("changed, not staged\n")
        self.assertEqual(before, audit(self.root, staged=True))
        self.assertNotEqual(before, audit(self.root))

    def test_auditor_itself_is_not_exempt_from_credentials(self):
        (self.root / "tools").mkdir()
        (self.root / "tools/audit_publication.py").write_text("ghp_" + "a" * 36)
        with self.assertRaisesRegex(ValueError, "credential"):
            audit(self.root)


if __name__ == "__main__":
    unittest.main()
