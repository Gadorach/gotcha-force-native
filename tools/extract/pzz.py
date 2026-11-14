"""Bounded read-only PZZ block directory and word-LZ decompression.

No filename/role inference, padding removal, extraction writes or game presets.
The caller supplies a ranged source exposing size/read(offset, size).
"""
import struct


def decompress(data, max_output=16 << 20):
    """Return (decoded bytes, consumed bytes), preserving decoded padding.

    Control bits are MSB-first; distances/counts are measured in two-byte words.
    A compressed command with distance zero ends the stream. Unread source block
    padding is not decoded and is not inferred to be part of the logical file.
    """
    if type(max_output) is not int or not 0 <= max_output <= 64 << 20:
        raise ValueError("PZZ output budget")
    source = memoryview(data)
    if len(source) > 64 << 20 or len(source) % 2:
        raise ValueError("PZZ input extent/alignment")
    cursor = 0
    result = bytearray()

    def word():
        nonlocal cursor
        if cursor + 2 > len(source):
            raise ValueError("truncated PZZ word")
        value = (source[cursor] << 8) | source[cursor + 1]
        cursor += 2
        return value

    while cursor < len(source):
        control = word()
        for bit in range(15, -1, -1):
            command = word()
            if control & (1 << bit):
                distance = (command & 0x7ff) * 2
                if not distance:
                    return bytes(result), cursor
                count = (command >> 11) * 2
                if not count:
                    count = word() * 2
                if distance > len(result) or count > max_output - len(result):
                    raise ValueError("PZZ back-reference/output budget")
                for _ in range(count):
                    result.append(result[-distance])
            else:
                if max_output - len(result) < 2:
                    raise ValueError("PZZ literal output budget")
                result.extend((command >> 8, command & 255))
    raise ValueError("PZZ stream has no explicit terminator")


class PzzArchive:
    """Original 0x800-byte block layout; only compression bit0x40000000 allowed."""
    def __init__(self, source):
        self.source = source
        if type(source.size) is not int or not 0x800 <= source.size <= 64 << 20 or source.size % 0x800:
            raise ValueError("PZZ archive extent/alignment")
        header = source.read(0, 0x800)
        if len(header) != 0x800:
            raise ValueError("short PZZ directory")
        count = struct.unpack_from(">I", header)[0]
        if not 1 <= count <= 511:
            raise ValueError("PZZ directory count")
        self.entries = []
        cursor = 0x800
        for index in range(count):
            word = struct.unpack_from(">I", header, 4 + index * 4)[0]
            size = (word & 0x3fffffff) * 0x800
            if word & 0x80000000 or size > source.size - cursor:
                raise ValueError("PZZ descriptor flags/range")
            self.entries.append(dict(index=index, offset=cursor, size=size,
                                     compressed=bool(word & 0x40000000)))
            cursor += size
        if cursor != source.size:
            raise ValueError("PZZ undeclared trailing blocks")

    def read(self, index, max_output=16 << 20):
        if type(index) is not int or not 0 <= index < len(self.entries):
            raise ValueError("PZZ member index")
        if type(max_output) is not int or not 0 <= max_output <= 64 << 20:
            raise ValueError("PZZ member budget")
        entry = self.entries[index]
        if not entry["size"]:
            return b"", 0
        if not entry["compressed"] and entry["size"] > max_output:
            raise ValueError("PZZ raw member budget")
        raw = self.source.read(entry["offset"], entry["size"])
        if len(raw) != entry["size"]:
            raise ValueError("short PZZ member")
        if entry["compressed"]:
            return decompress(raw, max_output)
        return raw, len(raw)
