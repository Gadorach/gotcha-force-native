"""Direct native compiler/test runner for systems without CMake; max two jobs."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def device_test(source):
    return bool(re.search(r'platform/pc/sdl_platform.hpp|platform/gl/gl_renderer.hpp|SDL.h|SDL2/',
                          source.read_text()))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", action="store_true")
    parser.add_argument("--headers", action="store_true")
    parser.add_argument("--tests", default="", help="comma-separated names, or all (CPU tests)")
    parser.add_argument("--output", type=Path, default=ROOT / "build/direct")
    parser.add_argument("--jobs", type=int, choices=(1, 2), default=2)
    parser.add_argument("--opt", choices=("0", "1", "2"), default="1")
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    if not (args.host or args.headers or args.tests):
        parser.error("choose --host, --headers or --tests")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    flags = ["-std=c++20", "-O" + args.opt, "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-I", str(ROOT)]
    if args.sanitize:
        flags += ["-g1", "-fsanitize=address,undefined,float-cast-overflow", "-fno-omit-frame-pointer"]
    jobs = []
    if args.host:
        platform = subprocess.check_output(["pkg-config", "--cflags", "--libs", "sdl2", "gl"], text=True)
        jobs.append(("gf_native", [*compiler, *flags, str(ROOT / "platform/pc/native_main.cpp"),
                    str(ROOT / "platform/pc/sdl_platform.cpp"), str(ROOT / "platform/gl/gl_renderer.cpp"),
                    "-o", str(output / "gf_native"), *shlex.split(platform)], False))
    if args.headers:
        headers = sorted((ROOT / "engine").rglob("*.hpp"))
        source = output / "all-native-headers.cpp"
        source.write_text("".join(f'#include "{p.relative_to(ROOT).as_posix()}"\n' for p in headers) + "int main(){}\n")
        jobs.append(("all-native-headers", [*compiler, *flags, "-fsyntax-only", str(source)], False))
    if args.tests:
        tests = sorted((ROOT / "tests/unit").glob("*.cpp"))
        if args.tests == "all":
            tests = [p for p in tests if not device_test(p)]
        else:
            names = args.tests.split(",")
            if len(set(names)) != len(names) or any(not re.fullmatch(r"[a-zA-Z0-9_]+", n) for n in names):
                parser.error("invalid/duplicate test name")
            tests = [ROOT / "tests/unit" / (name + ".cpp") for name in names]
            if any(not p.is_file() for p in tests):
                parser.error("test source not found")
            if any(device_test(p) for p in tests):
                parser.error("device tests require the opt-in CMake configuration")
        for source in tests:
            extra = [str(ROOT / "platform/pc/account_save_storage.cpp")] if 'platform/pc/account_save_storage.hpp' in source.read_text() else []
            if source.stem.startswith("account_save_storage"):
                extra += ["-pthread"]
            if source.stem == "account_save_storage":
                extra += ["-Wl,--wrap=fsync"]
            jobs.append((source.stem, [*compiler, *flags, str(source), *extra, "-o", str(output / source.stem)], True))

    def run(job):
        name, command, execute = job
        (output / (name + ".command.json")).write_text(json.dumps(command))
        with (output / (name + ".build.log")).open("w") as log:
            result = subprocess.run(command, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, timeout=1200)
        row = {"name": name, "compile_exit": result.returncode, "main_ran": False}
        if result.returncode == 0 and execute:
            run_command = [str(output / name)]
            if name in ("account_save_storage", "account_save_storage_edges"):
                run_command = [sys.executable, "-B", str(ROOT / "tests/unit/run_account_save_storage.py"), *run_command]
            elif name in ("startup_sound_timebase", "render_copy_compat_gl"):
                run_command = [sys.executable, "-B", str(ROOT / "tests/unit/run_output_test.py"), *run_command]
            with (output / (name + ".run.log")).open("w") as log:
                result = subprocess.run(run_command, cwd=ROOT, stdout=log,
                                        stderr=subprocess.STDOUT, timeout=360)
            row.update(main_ran=True, run_exit=result.returncode)
        (output / (name + ".result.json")).write_text(json.dumps(row))
        print(json.dumps(row), flush=True)
        return row

    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(run, jobs))
    (output / "results.json").write_text(json.dumps(results, indent=2))
    return int(any(r["compile_exit"] or r.get("run_exit", 0) not in (0, 77) for r in results))


if __name__ == "__main__":
    sys.exit(main())
