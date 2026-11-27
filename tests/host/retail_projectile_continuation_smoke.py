"""Play the exact extracted Machine Red B profile with and without continuation."""
from argparse import ArgumentParser
import json
from pathlib import Path
import re
from shutil import copytree
from subprocess import run
from tempfile import TemporaryDirectory

from tools.analyze.retail_projectile_continuation import EVIDENCE_SHA256, POLICY
from tools.extract.demo_borgs import prepare
from tools.extract.portable_stage_example import prepare_stage


def play(native, package, opengl):
    with TemporaryDirectory(prefix="gf-retail-shot-continuation-") as temp:
        root = Path(temp) / "content"
        prepare(root)
        workshop = prepare_stage(root)
        with (workshop / "stage.manifest").open("a", encoding="ascii") as stream:
            # Declared Workshop placement for the existing authored pose,
            # origin and cylinder sweep; not restored original coordinates.
            stream.write("spawn=shooter|0|0|-770\nspawn=first|0|0|-260\nspawn=second|0|0|-172\n")
        copytree(package, root / "borgs/machine-red")
        (root / "borgs/flight-stick/gameplay.txt").write_text(
            "GFBORG-GAMEPLAY 1\nvariant=normal\nhealth=500\n"
            "rig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\nmovement=stationary-v1\n",
            encoding="ascii",
        )
        preset = root / "machine-red.roster"
        preset.write_text(
            "GFROSTER 1\nlocals=2\nstage=example:workshop\n"
            "fighter=0|gg4e08:0600|normal|shooter\n"
            "fighter=1|example:flight-stick|normal|first\n"
            "fighter=1|example:flight-stick|normal|second\n",
            encoding="ascii",
        )
        args = [native, "--battle-roster", preset, "--demo-package-battle", "--trace-package-battle",
                "--no-audio", "--frames", "50"]
        if not opengl:
            args.append("--headless")
        result = run(args, cwd=root, capture_output=True, text=True, timeout=50)
        assert result.returncode == 0, result.stderr[-4000:]
        hits = re.findall(r"round=0 tick=(\d+) actor=0 move=basic-shot event=hit frame=\d+ object=(\d+)"
                          r" recipient=(\d+) value=(\d+)", result.stderr)
        assert result.stderr.count("actor=0 move=basic-shot event=debit ") == 1, result.stderr[-4000:]
        assert result.stderr.count("actor=0 move=basic-shot event=socket-launch ") == 1, result.stderr[-4000:]
        return [tuple(map(int, hit)) for hit in hits]


def smoke(native, baseline, opt_in, opengl):
    native = native.resolve(strict=True)
    ordinary, checked = (path.resolve(strict=True) / "gg4e08-0600" for path in (baseline, opt_in))
    before, after = ((path / "gameplay.txt").read_bytes() for path in (ordinary, checked))
    assert after.count(POLICY + b"\n") == 1 and after.replace(POLICY + b"\n", b"") == before
    summary = json.loads((checked / "source-index.json").read_bytes())
    proof = summary["basic_shot_battle_test"]["projectile_continuation_test"]
    assert proof["source_evidence_sha256"] == EVIDENCE_SHA256 and not proof["retail_behavior_verified"]
    first = play(native, ordinary, opengl)
    both = play(native, checked, opengl)
    assert len(first) == 1 and first[0][2] == 1, first
    assert len(both) == 2 and both[0] == first[0], (first, both)
    assert both[1][0] > both[0][0] and both[1][1:] == (both[0][1], 2, both[0][3]) and both[0][3] > 0, both
    print(f"Extracted Machine Red: same paid shot, {len(first)} default and {len(both)} opted-in recipients "
          f"({'OpenGL' if opengl else 'headless'})")


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    for arg in ("native", "baseline", "opt_in"):
        parser.add_argument(arg, type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    smoke(args.native, args.baseline, args.opt_in, args.opengl)
