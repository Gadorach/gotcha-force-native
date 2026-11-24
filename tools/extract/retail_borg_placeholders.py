"""Generate nonplayable GG4E08 Borg packages from a qualified local GFBC export."""
import argparse
from dataclasses import dataclass
from hashlib import sha256
import os
from pathlib import Path
import re
import stat
import struct
import unicodedata

CATALOG_SHA256 = "b53213014a0ff10ed68e3de3ad59c6c8f2760c2af7bde761a29a81570ae375c8"
CATALOG_BYTES = 128 << 10
MANIFEST_BYTES = 8192
SCAN_ENTRIES = 2048  # The host counts all immediate entries, even ignored files.
RETAIL_ROWS = 206
IDENTITY = re.compile(r"[a-z0-9][a-z0-9._-]*:[a-z0-9][a-z0-9._-]*")


@dataclass(frozen=True)
class Borg:
    model: int
    ordinal: int
    tribe: int
    name: str
    normal_cost: int
    variant_costs: tuple = ()

    @property
    def id(self):
        return f"gg4e08:{self.model:04x}"

    @property
    def folder(self):
        return f"gg4e08-{self.model:04x}"


def decode_catalog(data):
    """Decode the identity and base cost fields of GFBC v1, not a controller."""
    if len(data) < 32 or len(data) > CATALOG_BYTES:
        raise ValueError("GFBC extent")
    magic, version, count, starters, pairs, reserved = struct.unpack_from(">4sI4H", data)
    name_sizes = struct.unpack_from(">3H", data, 16)
    if (magic != b"GFBC" or version != 1 or count != RETAIL_ROWS or reserved
            or not 1 <= starters <= 200 or pairs > 32 or any(not 1 <= n <= 40 for n in name_sizes)
            or struct.unpack_from(">I", data, 28)[0] != len(data) - 32):
        raise ValueError("GFBC header")
    at = 32
    models, ordinals, rows = set(), set(), []
    for _ in range(count):
        if at + 50 > len(data):
            raise ValueError("truncated GFBC row")
        model, ordinal, tribe, n, *costs = struct.unpack_from(">4H6h", data, at)
        at += 50  # Remaining fixed columns are original rarity and XP, not used here.
        if (model >= 0x8000 or model in models or ordinal >= count or ordinal in ordinals
                or tribe >= 32 or any(cost < 0 for cost in costs)
                or not 1 <= n <= 96 or at + n > len(data)):
            raise ValueError("invalid or duplicate GFBC row")
        # The English disc's name table uses full-width, encoded uppercase text.
        name = unicodedata.normalize("NFKC", data[at:at + n].decode("cp932"))
        if not name.strip() or not name.isascii() or not name.isprintable():
            raise ValueError("unsupported GG4E08 display name")
        at += n
        models.add(model)
        ordinals.add(ordinal)
        rows.append(Borg(model, ordinal, tribe, name, costs[0], tuple(costs)))
    if at + 2 * starters + 6 * pairs + sum(name_sizes) != len(data):
        raise ValueError("GFBC trailer extent")
    return tuple(sorted(rows, key=lambda row: row.model))


def read_catalog(path):
    # O_NONBLOCK ensures an unexpected FIFO refuses instead of blocking open.
    with open_regular(path) as source:
        data = source.read(CATALOG_BYTES + 1)
    if sha256(data).hexdigest() != CATALOG_SHA256:
        raise ValueError("not the qualified GG4E08 GFBC export")
    return decode_catalog(data)


def open_regular(path):
    fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK | os.O_NOFOLLOW)
    try:
        if not stat.S_ISREG(os.fstat(fd).st_mode):
            raise ValueError(f"not a regular input: {path}")
        return os.fdopen(fd, "rb")
    except BaseException:
        os.close(fd)
        raise


def read_installed(root):
    """Reserve existing IDs even when their declared assets aren't available."""
    if not root.exists() and not root.is_symlink():
        return {}, set()
    if root.is_symlink() or not root.is_dir():
        raise ValueError("installed Borg root must be a real directory")
    installed, entries = {}, set()
    for folder in root.iterdir():
        entries.add(folder.name)
        if len(entries) > SCAN_ENTRIES:
            raise ValueError("installed Borg scan entry budget")
        if folder.is_symlink() or not folder.is_dir():
            continue
        manifest = folder / "borg.manifest"
        try:
            with open_regular(manifest) as source:
                raw = source.read(MANIFEST_BYTES + 1)
        except FileNotFoundError:
            continue
        if len(raw) > MANIFEST_BYTES:
            raise ValueError(f"oversize installed manifest: {manifest}")
        lines = raw.decode("utf-8").split("\n")
        if lines[0].removesuffix("\r") not in ("GFBORG 1", "GFBORG 2"):
            raise ValueError(f"invalid installed manifest header: {manifest}")
        ids = [line[3:].removesuffix("\r") for line in lines[1:] if line.startswith("id=")]
        if len(ids) != 1 or len(ids[0]) > 96 or not IDENTITY.fullmatch(ids[0]):
            raise ValueError(f"invalid installed manifest identity: {manifest}")
        if ids[0] in installed:
            raise ValueError(f"duplicate installed package ID: {ids[0]}")
        installed[ids[0]] = folder.name
    return installed, entries


def package_text(row):
    manifest = (
        "GFBORG 2\n# Retail placeholder: discovery only; no playable actor.\n"
        f"id={row.id}\nname={row.name}\ncatalog=catalog.txt\n"
    )
    catalog = (
        "GFBORG-CATALOG 2\n"
        "# Derived identity, ordering, tribe and normal cost from local GG4E08 GFBC.\n"
        f"# Retail source: GG4E08:{row.model:04X}/0\n"
        "description=Retail placeholder. Gameplay, preview and editor art are unavailable.\n"
        f"tribe=gg4e08:tribe-{row.tribe:02d}\ndefault-variant=normal\n"
        f"variant=normal|Normal|{row.normal_cost}|unresolved\n"
        f"collection-number={row.ordinal + 1}\ncombat-role=unresolved\nrarity-grade=unresolved\n"
        "crystals=unresolved\nalternate=unresolved\nlevel-schedule=unresolved\nexperience=unresolved\n"
        "display-jump=unresolved\nhp=unresolved|unresolved\nratings=unresolved|unresolved|unresolved|unresolved\n"
        "moves=unresolved\ntraits=unresolved\nforms=unresolved\nfusions=unresolved\nacquisition=unresolved\n"
        "effects=unresolved\nvariant-overrides=unresolved\nspawn=unresolved\ncontrollers=unresolved\n"
    )
    return manifest, catalog


def prepare(catalog, root, *, dry_run=False):
    """Install only absent identities. Never open an existing package for write."""
    rows = read_catalog(catalog)
    root = Path(root)
    installed, entries = read_installed(root)
    missing = [row for row in rows if row.id not in installed]
    if len(entries) + len(missing) > SCAN_ENTRIES:
        raise ValueError("combined Borg scan entry budget")
    for row in missing:
        if row.folder in entries:
            raise FileExistsError(f"placeholder folder is occupied: {root / row.folder}")
    if dry_run or not missing:
        return len(missing), len(rows) - len(missing)
    root.mkdir(exist_ok=True)
    for row in missing:
        folder = root / row.folder
        folder.mkdir()  # Exclusive. Another writer claiming this path makes us fail.
        manifest, catalog_text = package_text(row)
        # Publish the manifest last. A failed write must not advertise a package.
        for name, contents in (("catalog.txt", catalog_text), ("borg.manifest", manifest)):
            with (folder / name).open("x", encoding="ascii", newline="\n") as stream:
                stream.write(contents)
    return len(missing), len(rows) - len(missing)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("catalog", type=Path, help="qualified local GG4E08 catalog.gfbc")
    parser.add_argument("root", type=Path, help="local Borg install root; usually borgs")
    parser.add_argument("--dry-run", action="store_true", help="preflight without writing")
    args = parser.parse_args()
    try:
        missing, kept = prepare(args.catalog, args.root, dry_run=args.dry_run)
    except (OSError, ValueError) as exc:
        parser.exit(1, f"retail Borg placeholders: {exc}\n")
    action = "would write" if args.dry_run else "wrote"
    print(f"GG4E08: {RETAIL_ROWS} identities; {action} {missing} placeholders, preserved {kept} existing packages")


if __name__ == "__main__":
    main()
