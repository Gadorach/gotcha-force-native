"""Exercise an exact Collection scroll/preview path on disposable example packages."""
from argparse import ArgumentParser
from pathlib import Path
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.demo_borgs import prepare
from tools.extract.portable_ui_font_example import prepare as prepare_font


def smoke(binary, opengl=False):
    binary = binary.resolve(strict=True)
    with TemporaryDirectory(prefix="gf-borg-collection-") as scratch:
        root = Path(scratch) / "examples"
        prepare(root)
        font_root = Path(scratch) / "font"
        prepare_font(font_root)
        invalid_font = Path(scratch) / "invalid-font"
        invalid_font.mkdir()
        (invalid_font / "font.manifest").write_text("GFFONT 99\n", encoding="utf-8")
        for name in ("flight-stick", "jump-stick"):
            folder = root / "borgs" / name
            manifest = folder / "borg.manifest"
            contents = manifest.read_text(encoding="utf-8")
            if name == "flight-stick":
                contents = contents.replace("name=Flight Stick", "name=Café 界")
            manifest.write_text(contents.rstrip() + "\npreview=preview.txt\n", encoding="utf-8")
            (folder / "preview.txt").write_text(
                "GFBORG-PREVIEW 2\nmodel-format=hsd-body-v1\n"
                "variant=normal\nrig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\n"
            )
        catalog = root / "borgs/flight-stick/catalog.txt"
        catalog.write_text(catalog.read_text().replace("|placeholder", "|missing.tpl").rstrip() +
                           "\nvariant=silver|Silver|21|unresolved\n")
        diagnostic = root / "borgs" / "diagnostic"
        diagnostic.mkdir()
        (diagnostic / "borg.manifest").write_text(
            "GFBORG 2\nid=example:z-broken\nname=Catalogue Missing\ncatalog=no-card.txt\n"
        )
        inputs = {p.relative_to(root): p.read_bytes() for p in root.rglob("*") if p.is_file()}
        for width, height, font in ((480, 360, None), (960, 540, font_root),
                                    (1100, 440, None), (300, 500, font_root)):
            result = run([binary, "--demo-borg-browser", "--frames", "26", "--size", str(width), str(height),
                          "--no-audio", *([] if font is None else ["--collection-font", font]),
                          *([] if opengl else ["--headless"])],
                         cwd=root, capture_output=True, text=True, timeout=25)
            log = result.stdout + result.stderr
            assert result.returncode == 0, log
            assert "inspected example:flight-stick variant=silver" in log, log
            assert "nondefault variant needs its own declared preview profile" in log, log
            assert "opened body preview example:flight-stick variant=normal" in log, log
            assert "opened body preview example:jump-stick variant=normal" in log, log
            assert log.count("returned from body preview") == 2, log
            assert "selected=example:z-broken variant=unavailable view=picker rows=4" in log, log
            assert ("Collection font bound" in log) == (font is not None), log
        for font in (root / "missing-font", diagnostic, invalid_font):
            result = run([binary, "--browse-borgs", "--collection-font", font, "--headless", "--frames", "2"],
                         cwd=root, capture_output=True, text=True, timeout=10)
            log = result.stdout + result.stderr
            assert result.returncode == 0 and "Collection font unavailable; using bitmap fallback" in log, log
        for mode in (["--preview-borg", "example:flight-stick"], ["--cheat-offer-picker"],
                     ["--browse-owned-borgs", root, "1", "1", "--practice-from-picker"]):
            result = run([binary, *mode, "--collection-font", font_root, "--headless"],
                         cwd=root, capture_output=True, text=True, timeout=10)
            log = result.stdout + result.stderr
            assert result.returncode != 0 and "backend=" not in log, log
        assert inputs == {p.relative_to(root): p.read_bytes() for p in root.rglob("*") if p.is_file()}
    print(f"Borg Collection: read-only exact navigation and shared body preview PASS ({'OpenGL' if opengl else 'headless'})")


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    smoke(args.binary, args.opengl)
