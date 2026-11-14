"""Bounded read-only source access for offline extraction.

No game execution, mounting, downloads, emulator or output-path inference.
Raw GameCube ISO/GCM only; compressed containers must be handled separately.
Game-specific identities, addresses and extraction recipes belong in presets,
not in this reusable reader. An unverified source is not release qualification.
"""
import hashlib
import os
import struct
from pathlib import Path, PurePosixPath

from tools.analyze.formats import dol_layout


MAX_READ = 64 << 20


def _range(offset, size, length, limit=MAX_READ):
    if (type(offset) is not int or type(size) is not int or offset < 0 or size < 0
            or size > limit or offset > length or size > length - offset):
        raise ValueError("source range/budget")


def _stamp(stat):
    return (stat.st_dev, stat.st_ino, stat.st_size, stat.st_mtime_ns, stat.st_ctime_ns)


class ReadOnlyFile:
    def __init__(self, path):
        self.path = Path(path)
        self._file = self.path.open("rb")
        self._identity = _stamp(os.fstat(self._file.fileno()))
        self.size = self._identity[2]

    def read(self, offset, size):
        _range(offset, size, self.size)
        if _stamp(os.fstat(self._file.fileno())) != self._identity:
            raise ValueError("source changed during extraction")
        self._file.seek(offset)
        data = self._file.read(size)
        if len(data) != size or _stamp(os.fstat(self._file.fileno())) != self._identity:
            raise ValueError("short/changed source read")
        return data

    def slice(self, offset, size):
        _range(offset, size, self.size, self.size)
        return SourceSlice(self, offset, size)

    def close(self):
        self._file.close()

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()


class SourceSlice:
    def __init__(self, parent, offset, size):
        self.parent, self.offset, self.size = parent, offset, size

    def read(self, offset, size):
        _range(offset, size, self.size)
        return self.parent.read(self.offset + offset, size)

    def slice(self, offset, size):
        _range(offset, size, self.size, self.size)
        return SourceSlice(self.parent, self.offset + offset, size)


class DolImage:
    """Initialized DOL ranges only; never BSS, RAM snapshots or guessed gaps."""
    def __init__(self, data, expected_sha256=None):
        if len(data) > MAX_READ:
            raise ValueError("DOL byte budget")
        self.data = bytes(data)
        self.sha256 = hashlib.sha256(self.data).hexdigest()
        if expected_sha256 is not None and self.sha256 != expected_sha256:
            raise ValueError("DOL revision mismatch")
        self.layout = dol_layout(self.data)
        # The shared layout validator checks virtual overlap; also refuse file
        # overlap, which would make revision/source provenance ambiguous.
        sections = self.layout["sections"]
        for i, a in enumerate(sections):
            for b in sections[:i]:
                if a["offset"] < b["offset"] + b["size"] and b["offset"] < a["offset"] + a["size"]:
                    raise ValueError("DOL file sections overlap")

    def read(self, address, size, kind=None):
        _range(address, size, 1 << 32)
        for section in self.layout["sections"]:
            delta = address - section["address"]
            if delta >= 0 and delta <= section["size"] and size <= section["size"] - delta:
                if kind is not None and section["kind"] != kind:
                    raise ValueError("DOL section kind mismatch")
                start = section["offset"] + delta
                return self.data[start:start + size]
        raise ValueError("unmapped initialized DOL range")


class GcmImage:
    """Read-only raw-disc FST file views; no extraction writes or path joining."""
    def __init__(self, source):
        self.source = source
        header = source.read(0, 0x440)
        if struct.unpack_from(">I", header, 0x1c)[0] != 0xc2339f3d:
            raise ValueError("not a supported raw GameCube image")
        self.disc_id = header[:6].hex()
        self.dol_offset, fst_offset, fst_size = struct.unpack_from(">III", header, 0x420)
        if fst_offset < 0x440 or fst_size < 12 or fst_size > 16 << 20:
            raise ValueError("FST header/budget")
        fst = source.read(fst_offset, fst_size)
        first, parent, count = struct.unpack_from(">III", fst)
        if first != 0x01000000 or parent or not 1 <= count <= 100000 or count * 12 > len(fst):
            raise ValueError("FST root/count")
        names = count * 12
        self.files = {}
        paths = set()
        stack = [(count, 0, "")]
        for index in range(1, count):
            while index >= stack[-1][0]:
                stack.pop()
            word, a, b = struct.unpack_from(">III", fst, index * 12)
            tag, name_offset = word >> 24, word & 0xffffff
            at = names + name_offset
            if tag not in (0, 1) or at >= len(fst):
                raise ValueError("FST tag/name offset")
            end = fst.find(b"\0", at, min(len(fst), at + 256))
            if end < 0:
                raise ValueError("FST unterminated/oversize name")
            try:
                name = fst[at:end].decode("shift_jis")
            except UnicodeError as exc:
                raise ValueError("unsupported FST name encoding") from exc
            if not name or name in (".", "..") or any(c in name for c in "/\\:\0"):
                raise ValueError("unsafe FST component")
            path = stack[-1][2] + name
            if path in paths:
                raise ValueError("duplicate FST path")
            paths.add(path)
            if tag:
                if a != stack[-1][1] or not index < b <= stack[-1][0] or len(stack) >= 64:
                    raise ValueError("FST directory topology/budget")
                stack.append((b, index, path + "/"))
            else:
                _range(a, b, source.size, source.size)
                self.files[path] = (a, b)

    def file(self, name):
        if (not isinstance(name, str) or str(PurePosixPath(name)) != name
                or name.startswith("/") or any(p in (".", "..") for p in name.split("/"))):
            raise ValueError("noncanonical source file path")
        if name not in self.files:
            raise ValueError("source file absent")
        return self.source.slice(*self.files[name])

    def main_dol(self, expected_sha256=None):
        header = self.source.read(self.dol_offset, 256)
        end = 256
        for i in range(18):
            offset, size = (struct.unpack_from(">I", header, base + i * 4)[0] for base in (0, 0x90))
            if size:
                if offset < 256 or offset + size > MAX_READ:
                    raise ValueError("disc DOL extent/budget")
                end = max(end, offset + size)
        return DolImage(self.source.read(self.dol_offset, end), expected_sha256)
