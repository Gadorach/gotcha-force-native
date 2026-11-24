"""Discover authored Stories and open a bounded headless map without retail content."""
from argparse import ArgumentParser
from pathlib import Path
from subprocess import run
from tempfile import TemporaryDirectory


def smoke(binary):
    binary = binary.resolve(strict=True)
    example = Path(__file__).resolve().parents[2] / "examples/stories/workshop-story/story.manifest"
    with TemporaryDirectory(prefix="gf-story-smoke-") as scratch:
        root = Path(scratch)

        def launch(*args):
            result = run([binary, *args], cwd=root, capture_output=True, text=True, timeout=15)
            return result.returncode, result.stdout, result.stderr

        code, stdout, stderr = launch("--list-stories")
        assert code == 0 and "No portable Stories discovered" in stdout, (stdout, stderr)

        graph = example.read_text(encoding="utf-8")

        def package(folder, text):
            manifest = root / "stories" / folder / "story.manifest"
            manifest.parent.mkdir(parents=True)
            manifest.write_text(text, encoding="utf-8")

        package("folder-is-not-identity", graph)
        package("malformed", graph + "edge=ending|win|arrival\n")
        duplicate = graph.replace("example:workshop-story", "example:duplicate")
        package("duplicate-one", duplicate)
        package("duplicate-two", duplicate)
        package("map-art-gone", graph.replace("example:workshop-story", "example:art-missing") +
                "map-art=art/map.tpl\n")

        def snapshot():
            return {path.relative_to(root): path.read_bytes() if path.is_file() else None
                    for path in root.rglob("*")}

        before = snapshot()
        code, stdout, stderr = launch("--list-stories")
        assert code == 0 and [line.split("\t")[0] for line in stdout.splitlines()] == [
            "example:art-missing", "example:workshop-story"], (stdout, stderr)
        assert "duplicate Story ID" in stderr and "presentation node has a result edge" in stderr, stderr

        flags = ("--headless", "--frames", "2", "--size", "280", "540")
        code, stdout, stderr = launch(*flags, "--story-map", "example:workshop-story")
        log = stdout + stderr
        assert code == 0 and "story=example:workshop-story start=arrival nodes=3 read-only" in log, log
        assert "read-only drawn-frames=2" in log, log
        code, stdout, stderr = launch("--story-map", "example:art-missing", *flags)
        log = stdout + stderr
        assert code == 0 and "optional art unavailable" in log and "drawn-frames=2" in log, log

        for identifier, diagnostic in (
            ("example:absent", "Story ID not found"),
            ("example:duplicate", "duplicate Story ID"),
            ("folder-is-not-identity", ""),
        ):
            code, stdout, stderr = launch("--story-map", identifier, *flags)
            log = stdout + stderr
            assert code == 2 and diagnostic in log and "drawn-frames=" not in log, log

        for conflict in (
            ("--list-stories", "--headless"),
            ("--headless", "--story-map"),
            ("--story-map", "example:workshop-story", "--story-map", "example:workshop-story"),
            ("--story-map", "example:workshop-story", "--list-borgs"),
            ("--menu", "--story-map", "example:workshop-story"),
            ("--story-map", "example:workshop-story", "--recover-borgs", "save", "1", "1"),
            ("--run-challenge", "example:trials", "save", "1", "1", "1",
             "--story-map", "example:workshop-story"),
        ):
            code, stdout, stderr = launch(*conflict)
            assert code == 2 and "drawn-frames=" not in stdout + stderr, (conflict, stdout, stderr)
        assert snapshot() == before, "read-only Story inspection changed package files or created save data"


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    smoke(parser.parse_args().binary)
    print("portable Story host smoke: discovery, headless map, refusal and no writes PASS")
