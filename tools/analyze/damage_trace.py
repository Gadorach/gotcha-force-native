"""Bounded headless run of the original with the damage observer attached.

Recovering the damage amount needs one original observation: `8003D358` is the
`HP - r4` site, has no static caller, and its address is never materialised in
main.dol. This runner builds `damage_observer_mod.c`, attaches it with
`--mods`, and reports every observed `damage-observer:` line.

What works today: the recompiled runtime boots the original headless with
`--graphics Null --audio Null` (measured: 12,394,857 native dispatches in 60 s)
and loads the observer without error. What does not: nothing scripts the pad into
a battle, so no damage is observed yet. The runner says so rather than implying a
result.

The run is bounded and refuses unsafe conditions: a missing runtime, module or
ABI header refuses; the runtime is killed at the timeout; a concurrent compile or
replay in the same output directory refuses.
"""
import argparse
import os
import shutil
import subprocess
from pathlib import Path

MOD = Path(__file__).with_name("damage_observer_mod.c")
DEFAULT_TIMEOUT = 60
DEFAULT_WORK = Path("/tmp/opencode/damage-trace")


def build(include, output):
    cc = os.environ.get("CC", "cc")
    command = [cc, "-std=c11", "-shared", "-fPIC", f"-I{include}",
               "-o", str(output), str(MOD)]
    result = subprocess.run(command, capture_output=True, text=True, timeout=120)
    if result.returncode:
        raise ValueError(f"damage observer build failed:\n{result.stdout}{result.stderr}")
    return output


def run(runtime, game, module, mods, user_dir, *, timeout=DEFAULT_TIMEOUT):
    command = [str(runtime), "--game", str(game), "--module", str(module),
               "--mods", str(mods), "--graphics", "Null", "--audio", "Null",
               "--user-dir", str(user_dir)]
    try:
        result = subprocess.run(command, capture_output=True, text=True, timeout=timeout)
        log = result.stdout + result.stderr
        timed_out = False
    except subprocess.TimeoutExpired as expired:
        def text(value):
            if value is None:
                return ""
            return value.decode("utf-8", "replace") if isinstance(value, bytes) else value
        log = text(expired.stdout) + text(expired.stderr)
        timed_out = True
    observed = [line for line in log.splitlines() if line.startswith("damage-observer:")]
    return {"timed_out": timed_out, "observed": observed, "log": log}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--gotcha", type=Path, required=True,
                        help="the GotchaForce checkout that owns the recomp runtime")
    parser.add_argument("--timeout", type=int, default=DEFAULT_TIMEOUT)
    parser.add_argument("--work", type=Path, default=DEFAULT_WORK,
                        help="run directory; refuses one that already exists")
    args = parser.parse_args(argv)
    root = args.gotcha.resolve()
    runtime = root / "build/moderngekko/moderngekko-run"
    module = root / "build/moderngekko-module/gGG4E08_recomp.so"
    include = root / "external/ModernGekko/include"
    game = root / "original/extracted"
    for path, what in ((runtime, "runtime"), (module, "module"), (include, "mod ABI header"),
                       (game / "sys/main.dol", "extracted main.dol")):
        if not path.exists():
            raise SystemExit(f"missing {what}: {path}")
    work = args.work
    if work.exists():
        raise SystemExit(f"refusing an existing run directory: {work}")
    mods, user = work / "mods", work / "user"
    mods.mkdir(parents=True)
    user.mkdir()
    build(include, mods / "damage_observer.so")
    got = run(runtime, game, module, mods, user, timeout=args.timeout)
    print(f"timed_out={got['timed_out']} observed={len(got['observed'])}")
    for line in got["observed"][:20]:
        print(line)
    if not got["observed"]:
        print("no damage observed: nothing scripts the pad into a battle yet")


if __name__ == "__main__":
    main()
