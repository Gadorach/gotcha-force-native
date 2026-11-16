"""Local classification UI for numbered loops rendered by the native engine.

No game renderer is implemented here: PNG sprite sheets are cropped from actual
native GL viewports. Assets remain local. Annotation saves are per Borg/clip.
"""
import argparse
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import struct
import subprocess
import threading
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer


CATEGORIES = [
    ("unclassified", "Unclassified"),
    ("idle", "Idle / Neutral"),
    ("move_forward_slow", "Move forward - low speed (walk / slow float)"),
    ("move_forward_fast", "Move forward - high speed (run / fast float)"),
    ("strafe_ground_dash", "Strafe / Ground dash"),
    ("air_dash", "Air Dash"),
    ("jump_fly_up", "Jump / Fly upward"),
    ("jump_fly_forward", "Jump / Fly forward"),
    ("land", "Land"),
    ("hit_fall_back", "Fall back / Taking damage"),
    ("melee_chain_1", "Melee Chain Position 1"),
    ("melee_chain_2", "Melee Chain Position 2"),
    ("melee_chain_3", "Melee Chain Position 3"),
    ("x_attack_normal", "X Attack (normal)"),
    ("x_attack_charged", "X Attack (charged)"),
    ("b_attack_normal", "B Attack (normal)"),
    ("b_attack_charged", "B Attack (charged)"),
    ("combine", "Combine (fusion units)"),
    ("separate", "Separate (fusion units)"),
    ("transform", "Transform (transformer units)"),
    ("other", "Other"),
]


def u32(raw, at):
    if at < 0 or at + 4 > len(raw):
        raise ValueError("motion table extent")
    return struct.unpack_from(">I", raw, at)[0]


def motion_catalog(raw, common=False):
    if not 64 <= len(raw) <= 32 << 20:
        raise ValueError("motion bank byte budget")
    found, entries = {}, []
    for group in range(16):
        table = u32(raw, group * 4)
        if not table:
            continue
        if table < 64 or table & 3:
            raise ValueError("motion group offset")
        for choice in range(257):
            offset = u32(raw, table + 4 * choice)
            if offset == 0xFFFFFFFF:
                break
            if choice == 256:
                raise ValueError("motion choice budget")
            if not offset:
                continue
            if offset < 64 or offset & 3:
                raise ValueError("motion archive offset")
            size = u32(raw, offset)
            if not 32 <= size <= 4 << 20 or offset + size > len(raw):
                raise ValueError("motion archive extent")
            if offset not in found:
                found[offset] = len(entries)
                entries.append({"id": (10000 if common else 0) + group * 256 + choice,
                                "group": group, "choice": choice, "archive": len(entries),
                                "common": common, "aliases": [], "ready": False})
            entries[found[offset]]["aliases"].append([group, choice])
        else:
            raise ValueError("motion group terminator")
    return entries


def read_bounded(path, maximum=32 << 20):
    with Path(path).open("rb") as stream:
        size = os.fstat(stream.fileno()).st_size
        if not 0 < size <= maximum:
            raise ValueError(f"input budget: {path}")
        raw = stream.read(maximum + 1)
    if len(raw) != size:
        raise ValueError(f"changed/short input: {path}")
    return raw


def atomic_json(path, value):
    temporary = path.with_name(path.name + ".tmp")
    with temporary.open("w", encoding="utf8") as stream:
        json.dump(value, stream, ensure_ascii=False, indent=2)
        stream.flush()
        os.fsync(stream.fileno())
    os.replace(temporary, path)


def prepare(args):
    from PIL import Image
    assets = args.assets.resolve(strict=True)
    binary = args.binary.resolve(strict=True)
    sources, hashes = [], {}
    for borg in ("g-red", "sasuke"):
        for suffix in ("hsd", "prefix", "pose", "motions"):
            name = borg + "." + suffix
            raw = read_bounded(assets / name)
            hashes[name] = hashlib.sha256(raw).hexdigest()
        entries = motion_catalog(read_bounded(assets / (borg + ".motions")))
        if (assets / "common.motions").exists():
            raw = read_bounded(assets / "common.motions")
            hashes["common.motions"] = hashlib.sha256(raw).hexdigest()
            entries += motion_catalog(raw, True)
        sources.append({"borg": borg, "name": "G-Red" if borg == "g-red" else "Sasuke",
                        "entries": entries})
    if not 2 <= args.frames <= 60:
        raise ValueError("capture frames must be2..60")
    fingerprint = hashlib.sha256(json.dumps(hashes, sort_keys=True).encode()).hexdigest()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    (output / "media").mkdir()
    (output / ".raw").mkdir()
    template = Path(__file__).with_name("animation_browser.html")
    with (output / "index.html").open("xb") as stream:
        stream.write(template.read_bytes())
    catalog = {"schema": 1, "fingerprint": fingerprint, "asset_hashes": hashes,
               "categories": CATEGORIES, "status": "preparing", "sources": sources}
    atomic_json(output / "catalog.json", catalog)
    columns, rows, width, height = 3, 2, 1152, 648
    try:
        for source in sources:
            borg, entries = source["borg"], source["entries"]
            pages = (len(entries) + columns * rows - 1) // (columns * rows)
            for page in range(pages):
                raw_dir = output / ".raw" / f"{borg}-{page:03}"
                command = [str(binary), "--animation-mosaic", "--battle-mvp-assets", str(assets),
                           "--animation-borg", borg, "--animation-page", str(page + 1),
                           "--animation-grid", str(columns), str(rows), "--size", str(width), str(height),
                           "--no-audio", "--animation-export", str(raw_dir),
                           "--animation-export-frames", str(args.frames), "--animation-export-page-only"]
                with (output / "generation.log").open("ab") as log:
                    subprocess.run(command, stdout=log, stderr=log, check=True, timeout=300)
                with (raw_dir / "previews.tsv").open() as stream:
                    metadata = list(csv.DictReader(stream, delimiter="\t"))
                expected = entries[page * columns * rows:(page + 1) * columns * rows]
                if [int(row["id"]) for row in metadata] != [entry["id"] for entry in expected]:
                    raise ValueError("native/Python catalog IDs disagree")
                sheets = []
                sheet_columns = math.ceil(math.sqrt(args.frames))
                sheet_rows = math.ceil(args.frames / sheet_columns)
                for item in metadata:
                    w, h = int(item["width"]), int(item["height"])
                    if not 0 < w <= width or not 0 < h <= height:
                        raise ValueError("native viewport crop budget")
                    sheets.append(Image.new("RGB", (w * sheet_columns, h * sheet_rows), "#212833"))
                for frame in range(args.frames):
                    path = raw_dir / f"page-{page}" / f"frame-{frame}.ppm"
                    with Image.open(path) as image:
                        if image.width > 4096 or image.height > 4096:
                            raise ValueError("native capture pixel budget")
                        image = image.convert("RGB")
                        for row, sheet in zip(metadata, sheets):
                            x, y, w, h = (int(row[key]) for key in ("x", "y", "width", "height"))
                            crop = image.crop((round(x * image.width / width), round(y * image.height / height),
                                               round((x + w) * image.width / width), round((y + h) * image.height / height))).resize((w, h))
                            sheet.paste(crop, ((frame % sheet_columns) * w, (frame // sheet_columns) * h))
                    path.unlink()  #only this tool's individually named generated frames
                for entry, row, sheet in zip(expected, metadata, sheets):
                    name = f"{borg}-{entry['id']:05}.png"
                    with (output / "media" / name).open("xb") as stream:
                        sheet.save(stream, format="PNG", optimize=True)
                    end = float(row["end_frame"])
                    if not math.isfinite(end) or not 0 <= end <= 8192:
                        raise ValueError("animation duration budget")
                    entry.update({"ready": True, "supported": row["supported"] == "1",
                                  "error": row["error"], "end_frame": end,
                                  "media": "media/" + name, "frame_count": args.frames,
                                  "sheet_columns": sheet_columns, "width": int(row["width"]),
                                  "height": int(row["height"]), "seconds": max(.5, (end + 1) / 60)})
                atomic_json(output / "catalog.json", catalog)
                print(f"{borg}: native preview batch {page + 1}/{pages} ready", flush=True)
        catalog["status"] = "ready"
    except Exception as error:
        catalog["status"], catalog["error"] = "failed", str(error)
        atomic_json(output / "catalog.json", catalog)
        raise
    atomic_json(output / "catalog.json", catalog)
    print(f"Animation classification assets ready: {output}", flush=True)


def validate_annotation(catalog, payload):
    borg, number = payload.get("borg"), payload.get("id")
    sources = {source["borg"]: source for source in catalog["sources"]}
    if borg not in sources or type(number) is not int or number not in {e["id"] for e in sources[borg]["entries"]}:
        raise ValueError("unknown Borg/animation ID")
    category, notes = payload.get("category"), payload.get("notes", "")
    if category not in {key for key, _ in CATEGORIES} or not isinstance(notes, str) or len(notes) > 2000:
        raise ValueError("invalid category or notes")
    return f"{borg}:{number}", {"category": category, "notes": notes}


def serve(args):
    directory = args.directory.resolve(strict=True)
    lock = threading.Lock()

    class Handler(SimpleHTTPRequestHandler):
        def __init__(self, *items, **kwargs):
            super().__init__(*items, directory=str(directory), **kwargs)

        def reply(self, code, value):
            raw = json.dumps(value, ensure_ascii=False).encode()
            self.send_response(code)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Content-Length", str(len(raw)))
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            self.wfile.write(raw)

        def do_GET(self):
            if self.path == "/api/catalog":
                self.reply(200, json.loads(read_bounded(directory / "catalog.json", 8 << 20)))
            elif self.path == "/api/annotations":
                path = directory / "annotations.json"
                catalog = json.loads(read_bounded(directory / "catalog.json", 8 << 20))
                self.reply(200, json.loads(read_bounded(path, 8 << 20)) if path.exists()
                           else {"schema": 1, "dataset": catalog["fingerprint"], "entries": {}})
            else:
                super().do_GET()

        def do_POST(self):
            if self.path != "/api/annotations":
                self.reply(404, {"error": "not found"})
                return
            try:
                if self.headers.get("Content-Type", "").split(";")[0].strip() != "application/json":
                    raise ValueError("JSON content type required")
                size = int(self.headers.get("Content-Length", "0"))
                if not 0 < size <= 16384:
                    raise ValueError("annotation request budget")
                payload = json.loads(self.rfile.read(size))
                if not isinstance(payload, dict):
                    raise ValueError("annotation object required")
                with lock:
                    catalog = json.loads(read_bounded(directory / "catalog.json", 8 << 20))
                    key, value = validate_annotation(catalog, payload)
                    path = directory / "annotations.json"
                    current = json.loads(read_bounded(path, 8 << 20)) if path.exists() else {"schema": 1, "dataset": catalog["fingerprint"], "entries": {}}
                    if current["dataset"] != catalog["fingerprint"]:
                        raise ValueError("annotation dataset mismatch")
                    current["entries"][key] = value
                    atomic_json(path, current)
                self.reply(200, {"saved": key})
            except (ValueError, KeyError, TypeError) as error:
                self.reply(400, {"error": str(error)})

    server = ThreadingHTTPServer(("127.0.0.1", args.port), Handler)
    print(f"Animation classifier: http://127.0.0.1:{server.server_port}/", flush=True)
    print(f"Selections save only to {directory / 'annotations.json'}", flush=True)
    server.serve_forever()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    build = commands.add_parser("prepare")
    build.add_argument("--binary", type=Path, required=True)
    build.add_argument("--assets", type=Path, required=True)
    build.add_argument("--output", type=Path, required=True)
    build.add_argument("--frames", type=int, default=36)
    host = commands.add_parser("serve")
    host.add_argument("directory", type=Path)
    host.add_argument("--port", type=int, default=8765)
    args = parser.parse_args()
    (prepare if args.command == "prepare" else serve)(args)


if __name__ == "__main__":
    main()
