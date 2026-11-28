"""Verify extracted type-0 packages keep the same hit but return the slot later."""
from argparse import ArgumentParser
import json
from pathlib import Path
from shutil import copytree
from subprocess import run
from tempfile import TemporaryDirectory

from tools.analyze.native_projectile_contact_compare import EVENT
from tools.analyze.retail_projectile_contact_tail import EVIDENCE_SHA256, POLICY, ROUTES
from tools.extract.demo_borgs import prepare
from tools.extract.portable_stage_example import prepare_stage


def play(native, package, model):
    with TemporaryDirectory(prefix="gf-type0-contact-tail-") as tmp:
        root = Path(tmp) / "content"
        prepare(root)
        stage = prepare_stage(root)
        with (stage/"stage.manifest").open("a", encoding="ascii") as f:
            f.write("spawn=type0-start|0|0|-550\nspawn=type0-first|0|0|-410\nspawn=type0-second|0|0|-270\n")
        copytree(package, root/"borgs/shot-owner")
        (root/"borgs/flight-stick/gameplay.txt").write_text(
            "GFBORG-GAMEPLAY 1\nvariant=normal\nhealth=180\nrig-prefix=body.prefix\n"
            "pose-profile=body.pose\nidle=0:0\nmovement=stationary-v1\n", encoding="ascii")
        preset = root/"type0.roster"
        preset.write_text(
            f"GFROSTER 1\nlocals=2\nstage=example:workshop\nfighter=0|gg4e08:{model:04x}|normal|type0-start\n"
            "fighter=1|example:flight-stick|normal|type0-first\n"
            "fighter=1|example:flight-stick|normal|type0-second\n", encoding="ascii")
        result = run([native, "--battle-roster", preset, "--demo-package-battle", "--trace-package-battle",
                      "--frames", "35", "--headless", "--no-audio"], cwd=root, capture_output=True, text=True, timeout=45)
        assert result.returncode == 0, result.stderr[-4000:]
        events = [(int(t), e, int(o), int(r), int(v)) for t, a, e, o, r, v in EVENT.findall(result.stderr) if a == "0"]
        launches = [x for x in events if x[1] == "emit"]
        assert launches and len([x for x in events if x[1] == "debit" and x[0] <= launches[0][0]]) == 1, events
        return [x for x in events if x[2] == launches[0][2] and x[1] in ("hit", "projectile-contact-closed", "retire")]


def smoke(native, baseline, opt_in):
    native = native.resolve(strict=True)
    for model, route in ROUTES.items():
        base, changed = (p.resolve(strict=True)/f"gg4e08-{model:04x}" for p in (baseline, opt_in))
        assert (changed/"gameplay.txt").read_bytes() == (base/"gameplay.txt").read_bytes()+POLICY
        index = json.loads((changed/"source-index.json").read_bytes())
        proof = index["basic_shot_battle_test"]["projectile_contact_tail_test"]
        assert (index["member_sha256"], proof["source_evidence_sha256"], proof["source_factory_key"],
                proof["retail_behavior_verified"]) == (route.member, EVIDENCE_SHA256, route.key, False)
        before, after = play(native, base, model), play(native, changed, model)
        assert [x[1] for x in before] == ["hit", "retire"] and before[0][0] == before[1][0], before
        assert [x[1] for x in after] == ["hit", "projectile-contact-closed", "retire"], after
        assert before[0] == after[0] and before[0][3] == 1 and before[0][4] > 0, (before, after)
        assert after[0][0] == after[1][0] and after[-1][0]-after[0][0] == 3, after
        print(f"{model:04x} key {route.key}: same authored hit, slot return 0 -> 3 updates (headless)")


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    for name in ("native", "baseline", "opt_in"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    smoke(args.native, args.baseline, args.opt_in)
