"""Test native portable Force arrangement and cheat offers on disposable data."""
from argparse import ArgumentParser
from pathlib import Path
import struct
from subprocess import run
from tempfile import TemporaryDirectory


def catalogue(path):
    name, force, unnamed, owner = b"RETAIL FIXTURE", b"FIRST FORCE", b"FORCE A", b"PLAYER"
    body = (struct.pack(">4H", 0x1001, 0, 0, len(name)) + struct.pack(">6h", *([100] * 6)) +
            struct.pack(">6h", *([0] * 6)) + struct.pack(">9h", *range(100, 1000, 100)) + name +
            struct.pack(">H", 0x1001) + force + unnamed + owner)
    header = bytearray(32)
    struct.pack_into(">II8H", header, 0, 0x47464243, 1, 1, 1, 0, 0, len(force), len(unnamed), len(owner), len(unnamed) - 1)
    struct.pack_into(">I", header, 28, len(body))
    path.write_bytes(bytes(header) + body)


def smoke(binary, replay):
    binary, replay = binary.resolve(strict=True), replay.resolve(strict=True)
    with TemporaryDirectory(prefix="gf-borg-arrange-") as scratch:
        root = Path(scratch)
        borgs = root / "borgs"
        for ident, name, variants in (
            ("gred", "Current G-Red", "variant=normal|Normal|23|placeholder\nvariant=shadow|Shadow|46|unresolved\n"),
            ("support", "Support Borg", "variant=normal|Normal|17|placeholder\n"),
            ("missing", "Missing Catalogue", ""),
        ):
            folder = borgs / ident
            folder.mkdir(parents=True)
            (folder / "borg.manifest").write_text(f"GFBORG 2\nid=fixture:{ident}\nname={name}\ncatalog=catalog.txt\n")
            if variants:
                (folder / "catalog.txt").write_text(
                    "GFBORG-CATALOG 1\ndescription=Arrangement fixture.\ntribe=none\ndefault-variant=normal\n" + variants
                )
        accounts = root / "disposable-save-store"
        retail = root / "retail.gfbc"
        catalogue(retail)
        result = run([replay, "--replay-portable-arrange", accounts, retail], cwd=root, capture_output=True, text=True, timeout=30)
        log = result.stdout + result.stderr
        assert result.returncode == 0, log
        assert "one checked placement, and offer-only cheat picker PASS" in log, log
        before = {p.relative_to(root): p.read_bytes() for p in root.rglob("*") if p.is_file()}

        def host(*args):
            return run([binary, *args, "--headless", "--no-audio"], cwd=root, capture_output=True, text=True, timeout=15)

        result = host("--browse-borg-force", accounts, "7", "11", "2", "--force-slot", "3", "--retail-catalog", retail)
        assert result.returncode == 0, result.stdout + result.stderr
        assert "force=2 slot=3 claims=package owned-instance=8 retail-record=present overlaps=0" in result.stdout + result.stderr
        result = host("--place-owned-borg", accounts, "7", "11", "2", "4")
        assert result.returncode == 2 and "retail Force record is unverified" in result.stdout + result.stderr
        result = host("--cheat-offer-picker", "--size", "300", "500")
        assert result.returncode == 0, result.stdout + result.stderr
        for args in (
            ("--arrange-from-force",),
            ("--browse-borg-force", accounts, "7", "11", "2", "--arrange-from-force", "--practice-from-picker"),
            ("--cheat-offer-picker", "--browse-owned-borgs", accounts, "7", "11"),
        ):
            result = host(*args)
            assert result.returncode == 2 and "backend=" not in result.stdout + result.stderr
        assert before == {p.relative_to(root): p.read_bytes() for p in root.rglob("*") if p.is_file()}
    print("Borg Force host: exact arrange, read-only routes, and explicit offer-only picker PASS")


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--replay", type=Path)
    args = parser.parse_args()
    replay = args.replay
    if replay is None:
        for candidate in (args.binary.with_name("gred_practice_replay"),
                          args.binary.parent.parent / "direct" / "gred_practice_replay"):
            if candidate.is_file():
                replay = candidate
                break
    if replay is None:
        parser.error("practice replay binary not found; pass --replay")
    smoke(args.binary, replay)
