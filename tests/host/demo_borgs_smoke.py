"""Exercise the generated examples without any proprietary input or loose retail art."""
from argparse import ArgumentParser
from pathlib import Path
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.demo_borgs import prepare


def smoke(binary, opengl=False):
    binary = binary.resolve(strict=True)
    with TemporaryDirectory(prefix="gf-demo-borgs-") as scratch:
        root = Path(scratch) / "examples"
        prepare(root)
        for first, second in (("flight-stick", "jump-stick"), ("jump-stick", "flight-stick")):
            cmd = [binary, "--battle-borg", "example:" + first, "--against-borg", "example:" + second,
                   "--battle-mvp-assets", "arena", "--demo-package-battle", "--trace-package-battle",
                   "--no-audio", "--frames", "215", *([] if opengl else ["--headless"])]
            result = run(cmd, cwd=root, capture_output=True, text=True, timeout=35)
            log = result.stdout + result.stderr
            assert result.returncode == 0 and "event=socket-launch" in log and "event=hit" in log, log
            if first == "flight-stick":
                assert "package-flight: peak=32 cap=32" in log, log
            else:
                assert "actor=0 cues=10" in log and "actor=0 cues=4" in log, log
        assert not list(root.rglob("*.pcm")), "examples must not pull source audio"
    print(f"Original Flight/Jump Stick and flat arena PASS ({'OpenGL' if opengl else 'headless'})")


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    smoke(args.binary, args.opengl)
