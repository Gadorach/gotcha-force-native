"""Run a native Package Battle Test on the generated Workshop, with no retail assets."""
from argparse import ArgumentParser
from pathlib import Path
from subprocess import run
from tempfile import TemporaryDirectory

from tools.extract.demo_borgs import prepare
from tools.extract.portable_stage_example import prepare_stage


def smoke(binary):
    binary = binary.resolve(strict=True)
    with TemporaryDirectory(prefix="gf-workshop-smoke-") as scratch:
        root = Path(scratch) / "content"
        prepare(root, swept_beam=True)
        prepare_stage(root)
        listing = run([binary, "--list-stages"], cwd=root, capture_output=True, text=True, timeout=35)
        assert listing.returncode == 0 and "example:workshop | Workshop | discovered; battle not validated" in listing.stdout
        command = [binary, "--headless", "--no-audio", "--demo-package-battle",
                   "--trace-package-battle", "--frames", "100",
                   "--battle-borg", "example:flight-stick", "--borg-variant", "normal",
                   "--against-borg", "example:jump-stick", "--against-variant", "normal", "--battle-stage"]

        def launch(stage, first="west", second="east", extra=()):
            args = [*command, stage, "--stage-spawns", first, second, *extra]
            result = run(args, cwd=root, capture_output=True, text=True, timeout=35)
            return result.returncode, result.stdout + result.stderr

        code, log = launch("example:workshop")
        assert code == 0 and "stage=example:workshop player-spawn=west enemy-spawn=east" in log, log
        assert "event=stage-impact" in log and "stage-view: viewer=0" in log and "actors=2 restarts=1" in log, log

        code, log = launch("example:missing")
        assert code == 2 and "missing stage ID" in log, log
        code, log = launch("example:workshop", "missing")
        assert code == 2 and "missing stage spawn marker" in log, log
        code, log = launch("example:workshop", "west", "west")
        assert code == 2 and "markers must be distinct" in log, log
        code, log = launch("example:workshop", extra=("--battle-mvp-assets", "arena"))
        assert code == 2 and "actors=2" not in log, log
        code, log = launch("example:workshop", extra=("--gred-movement",))
        assert code == 2 and "actors=2" not in log, log
        no_variant = command.copy()
        at = no_variant.index("--against-variant")
        del no_variant[at:at + 2]
        result = run([*no_variant, "example:workshop", "--stage-spawns", "west", "east"],
                     cwd=root, capture_output=True, text=True, timeout=35)
        assert result.returncode == 2 and "actors=2" not in result.stdout + result.stderr


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    smoke(parser.parse_args().binary)
    print("portable stage host smoke: authored duel, restart, and refusal PASS")
