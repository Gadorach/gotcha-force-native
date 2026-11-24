"""Check the extracted normal roster with the native preview, not the original engine."""
import argparse
from collections import Counter
import json
from pathlib import Path
import re
import subprocess


def check(executable, root, frames, *, source_variants=False, role_clips=False, action_clips=False):
    executable = executable.resolve(strict=True)
    rows = sorted(path.name for path in (root / "borgs").iterdir()
                  if re.fullmatch(r"gg4e08-[0-9a-f]{4}", path.name))
    ids = [name.replace("-", ":", 1) for name in rows]
    if len(ids) != 206:
        raise ValueError("expected 206 extracted normal Borg packages")

    def run(*args):
        return subprocess.run([executable, *args], cwd=root, text=True,
                              capture_output=True, timeout=15, check=False)

    listing = run("--list-borgs")
    # The effects archive directory is offline evidence, not a Borg. Allow
    # only the known single rejection, with the exact evidence-only layout.
    effects = root/"borgs"/"effects"
    expected_stderr = ""
    if effects.is_dir() and not effects.is_symlink():
        entries = list(effects.iterdir())
        if ({p.name for p in entries}=={"common-atlas.tpl","common-single.hsd","common-split.hsd"} and
                all(not p.is_symlink() and p.is_file() for p in entries)):
            expected_stderr = "1 Borg package directories were rejected\n"
    if (listing.returncode or listing.stderr!=expected_stderr or
            [line.split("\t")[0] for line in listing.stdout.splitlines()] != ids):
        raise ValueError(f"native discovery differs from the extracted roster: {listing.stderr.strip()}")
    requests = [(ident,"normal",None) for ident in ids]
    action_requests = 0
    if source_variants or role_clips or action_clips:
        for ident, folder in zip(ids,rows,strict=True):
            path=root/"borgs"/folder/"source-index.json"
            with path.open("rb") as stream:
                data=stream.read((2<<20)+1)
            if len(data)>2<<20:
                raise ValueError("source index byte budget")
            index=json.loads(data)
            variants=index["selectable_variants"]
            if not isinstance(variants,list) or not 1<=len(variants)<=6 or variants[0]!="normal":
                raise ValueError(f"invalid exact variant list for {ident}")
            alternate=variants[1:]
            if any(not isinstance(v,str) or not re.fullmatch(r"source-[1-5]",v) for v in alternate) or len(set(alternate))!=len(alternate):
                raise ValueError(f"invalid or duplicated source variant for {ident}")
            if alternate and source_variants:
                override=run("--check-borg-variants",ident)
                if override.returncode:
                    raise ValueError(f"invalid package override for {ident}: {override.stderr.strip()}")
                requests.extend((ident,v,None) for v in alternate)
            if role_clips:
                clips=index["role_preview_clips"]
                if len(clips)>14 or any(len(row)!=3 or not re.fullmatch(r"[a-z-]{1,32}", row[0]) for row in clips):
                    raise ValueError(f"invalid role clip list for {ident}")
                requests.extend((ident,"normal",row[0]) for row in clips)
            if action_clips:
                clips=index.get("action_preview_clips",[])
                if len(clips)>7 or len({c["id"] for c in clips})!=len(clips) or any(
                        not re.fullmatch(r"(?:(?:b-shot|b-close|x|b-charge|x-charge)-c[0-5]-s[0-3]|b-shot-c[0-5]-s[0-3]-p[01]|b-shot-release-pose)",c["id"]) or
                        c["runtime_input_verified"] for c in clips):
                    raise ValueError(f"invalid action clip list for {ident}")
                alternatives={c["id"]:c for c in clips if c["id"].endswith(("-p0","-p1"))}
                if alternatives and (len(alternatives)!=2 or len({key[:-1] for key in alternatives})!=1 or
                                     any(c.get("request_choice_condition",{}).get("set") is not key.endswith("1")
                                         for key,c in alternatives.items())):
                    raise ValueError(f"invalid conditional action clip pair for {ident}")
                requests.extend((ident,"normal",row["id"]) for row in clips)
                action_requests+=len(clips)
    if action_clips and not action_requests:
        raise ValueError("no action clip declarations; extract with --action-clips")
    results = {}
    for ident,variant,role in requests:
        label=f"{ident}/{variant}" if source_variants or role_clips or action_clips else ident
        args=["--preview-borg",ident,"--borg-variant",variant,"--headless","--frames",str(frames)]
        if role:
            label+=f"/{role}"
            args.extend(("--preview-clip",role))
        preview = run(*args)
        if preview.returncode == 0:
            results[label] = "passed"
            continue
        for line in reversed(preview.stderr.splitlines()):
            if line.startswith("[error] borg-preview: "):
                results[label] = line.removeprefix("[error] borg-preview: ")
                break
        else:
            raise ValueError(f"unexpected host failure for {ident}: exit {preview.returncode}")
    refusals = Counter(result for result in results.values() if result != "passed")
    return {"discovered": len(ids), "preview_requests": len(requests), "frames": frames,
            "passed": len(requests) - sum(refusals.values()),
            "refusal_counts": dict(sorted(refusals.items())), "results": results}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("root", type=Path, help="launch directory whose borgs/ contains only this extraction")
    parser.add_argument("--frames", type=int, default=1)
    parser.add_argument("--source-variants", action="store_true", help="validate each declared exact source look")
    parser.add_argument("--role-clips", action="store_true", help="validate each declared normal movement role candidate")
    parser.add_argument("--action-clips", action="store_true", help="validate each declared normal action-entry candidate")
    parser.add_argument("--report", type=Path, help="new private output file; never overwritten")
    args = parser.parse_args()
    if not 1 <= args.frames <= 240:
        parser.error("--frames must be in 1..240")
    if args.report:
        if args.report.exists() or args.report.is_symlink():
            parser.error("--report already exists")
        repo = Path(__file__).resolve().parents[2]
        report = args.report.resolve()
        if report.is_relative_to(repo) and not report.is_relative_to(repo / "local"):
            parser.error("in-repository original roster reports must stay under local/")
    result = check(args.executable, args.root, args.frames, source_variants=args.source_variants,
                   role_clips=args.role_clips, action_clips=args.action_clips)
    if args.report:
        with args.report.open("x", encoding="ascii") as stream:
            json.dump(result, stream, indent=2)
            stream.write("\n")
    print(json.dumps({k: v for k, v in result.items() if k != "results"}, indent=2))


if __name__ == "__main__":
    main()
