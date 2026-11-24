"""Drive the real Story handoff with a generated duel and a disposable saved Force."""
from argparse import ArgumentParser
from pathlib import Path
from shutil import copytree
from struct import pack_into
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.demo_borgs import prepare
from tools.extract.portable_stage_example import prepare_stage


def smoke(adapter, seed, binary=None):
    adapter = adapter.resolve(strict=True)
    seed = seed.resolve(strict=True)
    if binary is not None:
        binary = binary.resolve(strict=True)
    with TemporaryDirectory(prefix="gf-story-handoff-") as scratch:
        root = Path(scratch) / "content"
        prepare(root)
        prepare_stage(root)
        second = root / "stages/second-workshop"
        copytree(root / "stages/workshop", second)
        stage = second / "stage.manifest"
        stage.write_text(stage.read_text(encoding="utf-8").replace("example:workshop", "example:second-workshop"),
                         encoding="utf-8")
        # Qualify a deterministic real AI defeat, as in the shared host tests.
        profile = root / "borgs/flight-stick/gameplay.txt"
        original = profile.read_text(encoding="utf-8")
        assert "health=160" in original
        profile.write_text(original.replace("health=160", "health=1"), encoding="utf-8")
        save = root / "save"
        save.mkdir()
        run([seed, save], cwd=root, check=True, timeout=20)

        challenge = root / "challenges/trials/challenge.manifest"
        challenge.parent.mkdir(parents=True)
        challenge.write_text(
            "GFCHALLENGE 1\nid=example:trials\nname=Story encounters\n"
            "encounter=one-tick|example:second-workshop|west|east|duel-ai-v1|defeat-opponent|1\n"
            "enemy=one-tick|example:jump-stick|normal\n"
            "encounter=detour|example:workshop|west|east|duel-ai-v1|defeat-opponent|1000\n"
            "enemy=detour|example:jump-stick|normal\n",
            encoding="utf-8")
        story = root / "stories/map/story.manifest"
        story.parent.mkdir(parents=True)
        graph = (
            "GFSTORY 1\nid=example:story\nname=Story\nstart=arrival\nmap-art=map.tpl\n"
            "node=arrival|text.arrival|200|400|example:trials|detour\n"
            "node=retry|text.retry|420|540|example:trials|one-tick\n"
            "node=terminal|text.terminal|620|240|none|none\n"
            "edge=arrival|win|terminal\nedge=arrival|loss|retry\nedge=retry|loss|arrival\n"
        )
        story.write_text(graph, encoding="utf-8")
        art = bytearray(56 + 64)
        pack_into(">5I2H2I", art, 0, 0x0020AF30, 1, 12, 20, 0, 3, 4, 6, 56)
        art[56:] = bytes([255, 40] * 16 + [80, 96] * 16)
        (story.parent / "map.tpl").write_bytes(art)
        missing = root / "stories/missing/story.manifest"
        missing.parent.mkdir(parents=True)
        missing.write_text(graph.replace("example:story", "example:missing-encounter")
                           .replace("|one-tick", "|missing"), encoding="utf-8")

        def snapshot():
            return {p.relative_to(root): p.read_bytes() if p.is_file() else None for p in root.rglob("*")}

        before = snapshot()
        run([adapter, save], cwd=root, check=True, timeout=35)
        if binary is not None:
            def launch(*options):
                out = run([binary, *options], cwd=root, capture_output=True, text=True, timeout=15)
                return out.returncode, out.stdout + out.stderr

            command = ("--headless", "--frames", "2", "--run-story", "example:story", save, "1", "1", "1")
            code, log = launch(*command)
            assert code == 0 and "closed current=arrival map-drawn-frames=2" in log and "actors=" not in log, log
            code, log = launch(*command, "--story-map", "example:story")
            assert code == 2 and "map-drawn-frames" not in log, log
            code, log = launch(*command[:-2], "5", "1")
            assert code == 2 and "requested save unavailable" in log, log
            code, log = launch(*command[:-1], "0")
            assert code == 2 and "map-drawn-frames" not in log, log
        assert snapshot() == before, "Story handoff altered content, save files, or save directory contents"


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("adapter", type=Path)
    parser.add_argument("seed", type=Path, help="test_portable_battle_host")
    parser.add_argument("binary", nargs="?", type=Path, help="optional gf_native CLI check")
    args = parser.parse_args()
    smoke(args.adapter, args.seed, args.binary)
    print("portable Story handoff smoke: exact admission, two stages, map return and no writes PASS")
