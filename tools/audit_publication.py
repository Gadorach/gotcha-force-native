"""Audit the native-only publication tree without reading ignored local data."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
ROOT_FILES = {"README.md", "AGENTS.md", "CMakeLists.txt", ".gitignore"}
SUFFIXES = {".hpp", ".h", ".cpp", ".c", ".py", ".md", ".json"}
ALLOWED_DIRS = {"engine", "platform", "tests", "tools", "docs"}
DISALLOWED_TOOLS = {"decomp_control", "build_endpoint", "ghidra", "verify"}


def audit(root=ROOT, staged=False):
    root = Path(root)
    args = ["git", "-C", str(root), "ls-files", "-z"]
    if staged:
        args += ["--cached"]
    else:
        args += ["--cached", "--others", "--exclude-standard"]
    names = sorted(set(subprocess.check_output(args).decode().split("\0")) - {""})
    report = []
    for name in names:
        p = root / name
        rel = Path(name)
        if rel.is_absolute() or ".." in rel.parts or p.is_symlink() or not p.is_file():
            raise ValueError("not a regular publication path: " + name)
        if name not in ROOT_FILES and (rel.parts[0] not in ALLOWED_DIRS or p.suffix not in SUFFIXES):
            raise ValueError("not native source/tooling/documentation: " + name)
        if rel.parts[0] == "tools" and any(x in DISALLOWED_TOOLS for x in rel.parts[1:]):
            raise ValueError("research/control tool excluded: " + name)
        data = (subprocess.check_output(["git", "-C", str(root), "show", ":" + name])
                if staged else p.read_bytes())
        if b"\0" in data:
            raise ValueError("binary payload: " + name)
        text = data.decode("utf-8")
        patterns = [
            r"^<{7} |^>{7} ", r"/home/", r"/tmp/opencode",
            r"BEGIN (?:OPENSSH |RSA |EC )?PRIVATE KEY",
            r"gh[pousr]_[A-Za-z0-9]{20,}",
            r"github_pat_[A-Za-z0-9_]{30,}",
        ]
        for pattern in patterns:
            # Only these two literal path-pattern definitions are intentional
            # here. The auditor itself still receives the credential checks.
            if name == "tools/audit_publication.py" and pattern in (r"/home/", r"/tmp/opencode"):
                continue
            if re.search(pattern, text, re.M):
                raise ValueError("private path, merge marker or credential pattern: " + name)
        report.append({"path": name, "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()})
    if not report:
        raise ValueError("empty publication")
    return {"files": report, "file_count": len(report), "bytes": sum(x["bytes"] for x in report)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--staged", action="store_true")
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    result = audit(staged=args.staged)
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(result, indent=2))
    print(json.dumps({k: v for k, v in result.items() if k != "files"}))


if __name__ == "__main__":
    main()
