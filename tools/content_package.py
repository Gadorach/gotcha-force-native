"""Native loose-content distribution: deterministic ZIP_STORED + JSON manifest.

No AFS offsets, compression, game conversion, runtime archive reader or installer
dependency. Install only into a NEW destination; existing content is never replaced.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import tempfile
import zipfile

MAX_FILES = 65536
MAX_FILE = 256 << 20
MAX_TOTAL = 4 << 30
MAX_MANIFEST = 8 << 20
MANIFEST = "manifest.json"


def asset_path(name: str) -> str:
    if not isinstance(name, str) or len(name) > 240 or not name:
        raise ValueError("invalid asset path")
    parts = name.split("/")
    reserved = {"con", "prn", "aux", "nul", *(f"com{i}" for i in range(1, 10)), *(f"lpt{i}" for i in range(1, 10))}
    for part in parts:
        if not re.fullmatch(r"[a-z0-9][a-z0-9._-]{0,127}", part) or part.endswith(".") or part.split(".")[0] in reserved:
            raise ValueError("asset paths must be portable lowercase relative paths")
    if str(PurePosixPath(name)) != name or name == MANIFEST:
        raise ValueError("noncanonical/reserved asset path")
    return name


def _json_pairs(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError("duplicate JSON key")
        result[key] = value
    return result


def _manifest(data: bytes):
    if len(data) > MAX_MANIFEST:
        raise ValueError("manifest exceeds budget")
    m = json.loads(data, object_pairs_hook=_json_pairs)
    if not isinstance(m, dict) or set(m) != {"schema", "layout", "files"} or type(m["schema"]) is not int or m["schema"] != 1 or m["layout"] != "loose-v1":
        raise ValueError("unsupported native content manifest")
    if not isinstance(m["files"], list) or not 1 <= len(m["files"]) <= MAX_FILES:
        raise ValueError("manifest file budget")
    paths = set()
    total = 0
    for row in m["files"]:
        if not isinstance(row, dict) or set(row) != {"path", "bytes", "sha256"}:
            raise ValueError("invalid asset record")
        name = asset_path(row["path"])
        if name in paths or type(row["bytes"]) is not int or not 0 <= row["bytes"] <= MAX_FILE or not re.fullmatch("[0-9a-f]{64}", str(row["sha256"])):
            raise ValueError("duplicate/invalid asset record")
        paths.add(name)
        total += row["bytes"]
    if total > MAX_TOTAL:
        raise ValueError("content exceeds total budget")
    for name in paths:
        if any(str(parent) in paths for parent in PurePosixPath(name).parents if str(parent) != "."):
            raise ValueError("asset path is also a directory")
    return m


def _digest_file(path: Path, expected: int) -> str:
    digest = hashlib.sha256()
    count = 0
    with path.open("rb") as f:
        while chunk := f.read(1 << 20):
            count += len(chunk)
            if count > expected:
                raise ValueError("source changed while hashing")
            digest.update(chunk)
    if count != expected:
        raise ValueError("source changed while hashing")
    return digest.hexdigest()


def _info(name: str):
    info = zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0))
    info.create_system = 3
    info.external_attr = 0o100644 << 16
    info.compress_type = zipfile.ZIP_STORED
    return info


def pack(root: Path, output: Path):
    root, output = Path(root), Path(output)
    if root.is_symlink() or not root.is_dir() or root.resolve() in output.resolve().parents:
        raise ValueError("source must be a directory; output must be outside it")
    rows = []
    total = 0
    # Bound discovery before sorting or hashing; do not collect an unbounded tree.
    paths = []
    for path in root.rglob("*"):
        paths.append(path)
        if len(paths) > MAX_FILES * 8:
            raise ValueError("source tree discovery budget")
    for path in sorted(paths):
        if path.is_symlink():
            raise ValueError("source symlinks are not assets")
        if path.is_dir():
            continue
        if not path.is_file():
            raise ValueError("source contains a nonregular file")
        name = path.relative_to(root).as_posix()
        if name == MANIFEST:
            continue  # regenerate when repacking editable installed content
        size = path.stat().st_size
        total += size
        if size > MAX_FILE or total > MAX_TOTAL or len(rows) >= MAX_FILES:
            raise ValueError("source file/total/count budget")
        rows.append({"path": asset_path(name), "bytes": size, "sha256": _digest_file(path, size)})
    data = (json.dumps({"schema": 1, "layout": "loose-v1", "files": rows}, indent=2, sort_keys=True) + "\n").encode()
    manifest = _manifest(data)
    with zipfile.ZipFile(output, "x", compression=zipfile.ZIP_STORED, allowZip64=True) as z:
        z.writestr(_info(MANIFEST), data)
        for row in rows:
            digest = hashlib.sha256()
            count = 0
            with (root / row["path"]).open("rb") as source, z.open(_info(row["path"]), "w", force_zip64=True) as target:
                while chunk := source.read(1 << 20):
                    count += len(chunk)
                    if count > row["bytes"]:
                        raise ValueError("source changed while packing")
                    digest.update(chunk)
                    target.write(chunk)
            if count != row["bytes"] or digest.hexdigest() != row["sha256"]:
                raise ValueError("source changed while packing")
    return manifest


def install(package: Path, destination: Path):
    destination = Path(destination)
    if destination.exists() or destination.is_symlink() or not destination.parent.is_dir() or destination.parent.is_symlink():
        raise ValueError("installation requires a new destination under an existing real directory")
    with zipfile.ZipFile(package) as z, tempfile.TemporaryDirectory(prefix="gf-content-", dir=destination.parent) as scratch:
        infos = z.infolist()
        if not 2 <= len(infos) <= MAX_FILES + 1 or len({i.filename for i in infos}) != len(infos):
            raise ValueError("duplicate names/file budget")
        table = {i.filename: i for i in infos}
        for info in infos:
            if info.filename != MANIFEST:
                asset_path(info.filename)
            mode = info.external_attr >> 16
            if (mode & 0o170000) not in (0, 0o100000) or info.compress_type != zipfile.ZIP_STORED or info.compress_size != info.file_size or info.flag_bits & 1:
                raise ValueError("only unencrypted regular stored entries are supported")
        if MANIFEST not in table or table[MANIFEST].file_size > MAX_MANIFEST:
            raise ValueError("missing/oversized manifest")
        data = z.read(MANIFEST)
        manifest = _manifest(data)
        if set(table) != {MANIFEST, *(r["path"] for r in manifest["files"])}:
            raise ValueError("archive/manifest file sets disagree")
        stage = Path(scratch)
        for row in manifest["files"]:
            if table[row["path"]].file_size != row["bytes"]:
                raise ValueError("asset size mismatch")
            path = stage / row["path"]
            path.parent.mkdir(parents=True, exist_ok=True)
            digest = hashlib.sha256()
            count = 0
            with z.open(row["path"]) as source, path.open("xb") as target:
                while chunk := source.read(1 << 20):
                    count += len(chunk)
                    if count > row["bytes"]:
                        raise ValueError("asset exceeds declared budget")
                    digest.update(chunk)
                    target.write(chunk)
            if count != row["bytes"] or digest.hexdigest() != row["sha256"]:
                raise ValueError("asset integrity mismatch")
        # All validation precedes destination creation. Per-file exclusive writes
        # refuse collisions; manifest publishes LAST. Not a power-loss-atomic tree
        # installation. On an I/O failure, retain the incomplete new destination.
        destination.mkdir()
        for row in manifest["files"]:
            target = destination / row["path"]
            target.parent.mkdir(parents=True, exist_ok=True)
            with (stage / row["path"]).open("rb") as source, target.open("xb") as output:
                while chunk := source.read(1 << 20):
                    output.write(chunk)
        with (destination / MANIFEST).open("xb") as output:
            output.write(data)
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("operation", choices=["pack", "install"])
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    result = (pack if args.operation == "pack" else install)(args.source, args.destination)
    print(json.dumps({"operation": args.operation, "files": len(result["files"]), "compression": "stored", "runtime_layout": "loose-v1"}))


if __name__ == "__main__":
    main()
