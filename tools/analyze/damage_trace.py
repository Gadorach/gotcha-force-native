"""Run the original headless with a read-only observer attached.

The native port cannot recover the damage amount statically. This runner builds
`damage_observer_mod.c`, attaches it with `--mods`, and reports every
`damage-observer:` line. It can also load a battle savestate and drive a short
input script through the runtime's documented automation directory, so an
original battle can be observed without any guest-memory writes.

Two details decide whether the observer is actually loaded, and both are
checked here rather than assumed:

- a dynamic mod is only discovered when its file name ends in `.mgm.so`;
- `version` must be a three-part version such as `1.0.0`.

A rejected mod is reported as an error instead of being mistaken for an
observer that saw no damage.

The run refuses a missing runtime, module, mod ABI header or `main.dol`, refuses
an existing work directory, and is bounded by a timeout.
"""
import argparse
import os
import subprocess
import time
from pathlib import Path

MOD = Path(__file__).with_name("damage_observer_mod.c")
MOD_LIBRARY = "damage_observer.mgm.so"
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


def status(root):
    path = Path(root) / "status.txt"
    if not path.exists():
        return {}
    return dict(line.split("=", 1)
                for line in path.read_text().splitlines() if "=" in line)


def send(root, counter, text, wait=300):
    root = Path(root)
    counter[0] += 1
    name = f"{counter[0]:04d}.txt"
    tmp = root / (name + ".tmp")
    tmp.write_text(text)
    os.replace(tmp, root / "commands" / name)
    started = time.time()
    while time.time() - started < wait:
        current = status(root)
        if int(current.get("processed_commands") or 0) >= counter[0]:
            return current
        if current.get("last_error"):
            raise RuntimeError(current["last_error"])
        time.sleep(0.2)
    raise RuntimeError("automation command was not processed: " + text.splitlines()[0])


def run(runtime, game, module, mods, user_dir, *, timeout=DEFAULT_TIMEOUT,
        state=None, automation=None):
    command = [str(runtime), "--game", str(game), "--module", str(module),
               "--mods", str(mods), "--graphics", "Null", "--audio", "Null",
               "--user-dir", str(user_dir)]
    if state is not None:
        command += ["--load-state", str(state)]
    if automation is not None:
        command += ["--automation-dir", str(automation)]
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
    loaded = [line for line in log.splitlines() if line.startswith("mod loaded:")]
    rejected = [line for line in log.splitlines() if line.startswith("mod rejected:")]
    return {"timed_out": timed_out, "observed": observed, "loaded": loaded,
            "rejected": rejected, "log": log}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--gotcha", type=Path, required=True,
                        help="the GotchaForce checkout that owns the recomp runtime")
    parser.add_argument("--timeout", type=int, default=DEFAULT_TIMEOUT)
    parser.add_argument("--work", type=Path, default=DEFAULT_WORK,
                        help="run directory; refuses one that already exists")
    parser.add_argument("--state", type=Path,
                        help="optional battle savestate to observe")
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
    automation = work / "automation"
    mods.mkdir(parents=True)
    user.mkdir()
    (automation / "commands").mkdir(parents=True)
    build(include, mods / MOD_LIBRARY)
    # The runtime loads the state during boot; the attack is only sent once the
    # core reports a running frame count, so it acts on the loaded battle.
    import threading
    def drive():
        counter = [0]
        started = time.time()
        while time.time() - started < 240:
            current = status(automation)
            if current.get("state") == "running" and int(current.get("frame_count") or 0) > 1000:
                break
            time.sleep(0.5)
        else:
            return
        send(automation, counter, "command=pad_frames\nport=0\nframes=30\nmain_y=1\n")
        for button in ("b", "a", "b", "a"):
            send(automation, counter, f"command=pad_frames\nport=0\nframes=5\n{button}=1\n")
    driver = None
    if args.state is not None:
        driver = threading.Thread(target=drive, daemon=True)
        driver.start()
    got = run(runtime, game, module, mods, user, timeout=args.timeout,
              state=args.state, automation=automation if driver else None)
    if got["rejected"]:
        raise SystemExit(got["rejected"][0])
    print(f"timed_out={got['timed_out']} loaded={len(got['loaded'])} "
          f"observed={len(got['observed'])}")
    for line in got["loaded"][:2]:
        print(line)
    for line in got["observed"][:20]:
        print(line)
    if not got["loaded"]:
        raise SystemExit("the observer was not loaded")
    if not got["observed"]:
        print("no damage observed in this run")


if __name__ == "__main__":
    main()
