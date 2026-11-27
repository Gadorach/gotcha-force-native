"""Verify the two opt-in Workshop X bombs without relying on retail stage geometry."""
from argparse import ArgumentParser
import json
from pathlib import Path
import re
from shutil import copytree
from subprocess import run
from tempfile import TemporaryDirectory

from tools.analyze.retail_ninja_bomb_test import EVIDENCE, PILOTS, ROWS
from tools.extract.demo_borgs import prepare
from tools.extract.portable_stage_example import prepare_stage


def verify_profiles(baseline, opt_in):
    before = {p.parent.name for p in baseline.glob("gg4e08-*/source-index.json")}
    after = {p.parent.name for p in opt_in.glob("gg4e08-*/source-index.json")}
    changed, expected = set(), {f"gg4e08-{model:04x}" for model in PILOTS}
    assert len(before) == 206 and before == after, (len(before), len(after))
    addition = ("\n".join(ROWS)+"\n").encode("ascii")
    for name in sorted(before):
        old, new = baseline/name, opt_in/name
        b, a = old/"gameplay.txt", new/"gameplay.txt"
        assert b.is_file() == a.is_file(), name
        if not a.is_file() or b.read_bytes() == a.read_bytes():
            continue
        changed.add(name)
        assert name in expected and a.read_bytes() == b.read_bytes()+addition, name
        data = json.loads((new/"source-index.json").read_bytes())
        proof = data["basic_shot_battle_test"]["ninja_bomb_test"]
        assert data["selectable_variants"] == ["normal"] and not proof["retail_behavior_verified"], name
        assert proof["source_factory_key"] == PILOTS[int(name[-4:], 16)][4], name
        assert proof["source_evidence"] == {f"docs/api-map/gg4e08-bomb-{key}.json": digest
                                            for key, digest in EVIDENCE.items()}, name
        old_assets = json.loads((old/"source-index.json").read_bytes())["asset_sha256"]
        ignored = {"gameplay.txt", "catalog.txt"}
        assert {k: v for k, v in old_assets.items() if k not in ignored} == {
            k: v for k, v in data["asset_sha256"].items() if k not in ignored}, name
    assert changed == expected, changed


def play(native, package, model, wall, opengl):
    with TemporaryDirectory(prefix="gf-ninja-bomb-smoke-") as temp:
        root = Path(temp)/"content"
        prepare(root)
        workshop = prepare_stage(root)
        with (workshop/"stage.manifest").open("a", encoding="ascii") as stream:
            # Declared native placements, never restored original coordinates.
            stream.write(f"spawn=thrower|0|0|{-360 if wall else -800}\n"
                         f"spawn=receiver|0|0|{700 if wall else -380}\n"
                         "spawn=standby|1000|0|-1200\n")
        copytree(package, root/"borgs/ninja")
        (root/"borgs/flight-stick/gameplay.txt").write_text(
            "GFBORG-GAMEPLAY 1\nvariant=normal\nhealth=500\n"
            "rig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\nmovement=stationary-v1\n",
            encoding="ascii")
        roster = root/"ninja.roster"
        roster.write_text("GFROSTER 1\nlocals=2\nstage=example:workshop\n"
                          f"fighter=0|gg4e08:{model:04x}|normal|thrower\n"
                          "fighter=1|example:flight-stick|normal|receiver\n"
                          "fighter=0|example:flight-stick|normal|standby\n", encoding="ascii")
        argv = [native, "--battle-roster", roster, "--demo-package-battle", "--trace-package-battle",
                "--no-audio", "--frames", "83"]
        if not opengl:
            argv.append("--headless")
        result = run(argv, cwd=root, capture_output=True, text=True, timeout=50)
        log = result.stderr+result.stdout
        assert result.returncode == 0, log[-5000:]
        events = re.findall(r"round=0 tick=(\d+) actor=0 move=(basic-shot|ninja-bomb) event=([\w-]+)"
                            r" frame=(\d+) object=(\d+) recipient=(\d+) value=(\d+)", log)
        return [
            (move, kind, int(tick), int(frame), int(obj), int(target), int(value))
            for tick, move, kind, frame, obj, target, value in events
        ]


def smoke(native, baseline, opt_in, opengl=False):
    native = native.resolve(strict=True)
    verify_profiles(baseline, opt_in)
    for model in PILOTS:
        name = f"gg4e08-{model:04x}"
        old = play(native, baseline/name, model, False, opengl)
        assert old and all(e[0] == "basic-shot" for e in old), old
        for wall in (False, True):
            events = play(native, opt_in/name, model, wall, opengl)
            if not wall:
                early = [e for e in old if e[2] <= 40]
                assert early and early == [e for e in events if e[2] <= 40], events
            bomb = [e[1:] for e in events if e[0] == "ninja-bomb"]
            one = lambda kind: [e for e in bomb if e[0] == kind]
            debit, = one("debit")
            sampled, = one("socket-launch")
            area, = one("impact-hazard-placed")
            assert debit[2] == sampled[2] == 12 and debit[5] == 4 and sampled[5] == 11 and area[3] != sampled[3], bomb
            if wall:
                stage, = one("stage-impact")
                assert stage[1] == area[1] and stage[3] == sampled[3] and stage[5] == 2 and not one("hit"), bomb
            else:
                direct, child = one("hit")
                assert direct[1] == area[1] < child[1] and direct[3] == sampled[3] and child[3] == area[3], bomb
                assert direct[4] == child[4] == 1 and (direct[5], child[5]) == (9, 19), bomb
                print(f"{model:04x}: native direct hit tick {direct[1]} ({direct[5]} HP), "
                      f"area hit tick {child[1]} ({child[5]} HP)")
            assert not one("impact-hazard-refused") and not one("projectile-shape-refused"), bomb
    print(f"Ninja bomb opt-in: 2 of 206 profiles changed; actor and static wall routes passed "
          f"({'OpenGL' if opengl else 'headless'})")


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    for arg in ("native", "baseline", "opt_in"):
        parser.add_argument(arg, type=Path)
    parser.add_argument("--opengl", action="store_true")
    args = parser.parse_args()
    smoke(args.native, args.baseline, args.opt_in, args.opengl)
