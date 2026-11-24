"""Generate a small GFFONT demo from this repository's own bitmap glyphs."""
from argparse import ArgumentParser
from pathlib import Path
import re

from tools.extract.demo_borgs import write_new


LEGACY_ASCII = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ.,:!?/-'()%+"
# Lowercase and example non-ASCII glyphs authored for this fixture. Each
# pair of hex digits is one top-to-bottom row; bit zero is the leftmost dot.
EXTRA = {
    "a": "00000E100E111E", "b": "01010D1311110F", "c": "00000E1101110E",
    "d": "1010161911111E", "e": "00000E111F010E", "f": "0C12020F020202",
    "g": "00001E11111E100E", "h": "01010D13111111", "i": "0400060404040E",
    "j": "08000C0808080906", "k": "01011109070911", "l": "0604040404040E",
    "m": "00000B15151515", "n": "00000D13111111", "o": "00000E1111110E",
    "p": "00000F11110F0101", "q": "00001E11111E1010", "r": "00000D13010101",
    "s": "00001E010E100F", "t": "02020F0202120C", "u": "00001111111916",
    "v": "00001111110A04", "w": "0000111515150A", "x": "0000110A040A11",
    "y": "00001111111E100E", "z": "00001F0804021F", "_": "0000000000001F",
    "é": "08040E111F010E", "É": "08041F010F011F", "ö": "0A000E1111110E",
    "Ö": "0A000E1111110E", "Ω": "0E1111110A0A1B", "界": "7F49497F141422",
    "�": "1F11151415111F",
}


def legacy_glyphs():
    source = Path(__file__).resolve().parents[2] / "engine/graphics/text.hpp"
    with source.open(encoding="utf-8") as file:
        text = file.read(32 * 1024 + 1)
    if len(text) > 32 * 1024:
        raise ValueError("owned glyph table source exceeds expected size")
    table = text.split("static constexpr Glyph kGlyphs[kGlyphCount] = {", 1)[1].split("};", 1)[0]
    rows = re.findall(r"\{\{((?:0x[0-9A-F]{2},\s*){6}0x[0-9A-F]{2})\}\}", table)
    if len(rows) != len(LEGACY_ASCII):
        raise ValueError("owned glyph order or format changed")
    return {char: tuple(int(field, 16) for field in row.split(",")) for char, row in zip(LEGACY_ASCII, rows)}


def prepare(output):
    glyphs = legacy_glyphs()
    glyphs.update({char: bytes.fromhex(hex_rows) for char, hex_rows in EXTRA.items()})
    del glyphs[" "]
    stride, columns, row_height = 8, 16, 9
    width = stride * columns
    height = row_height * ((len(glyphs) + columns - 1) // columns)
    alpha = bytearray(width * height)
    metrics = ["GFFONT 1", "# Generated from repository-owned bitmap shapes.",
               "atlas=ui.a8", f"size={width}|{height}", "line-height=10", "ascent=7",
               "glyph=U+0020|0|0|0|0|0|0|4"]
    for index, (char, rows) in enumerate(sorted(glyphs.items())):
        x, y = index % columns * stride, index // columns * row_height
        occupied = 0
        for row in rows:
            occupied |= row
        if not occupied or occupied.bit_length() >= stride or len(rows) >= row_height:
            raise ValueError("fixture glyph does not fit its atlas cell")
        left = (occupied & -occupied).bit_length() - 1
        ink_width = occupied.bit_length() - left
        for dy, row in enumerate(rows):
            for dx in range(ink_width):
                if row & (1 << (dx + left)):
                    alpha[(y + dy) * width + x + dx] = 255
        metrics.append(f"glyph=U+{ord(char):04X}|{x}|{y}|{ink_width}|{len(rows)}|0|7|{ink_width+1}")
    write_new(Path(output), {"font.manifest": "\n".join(metrics) + "\n", "ui.a8": alpha})


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    prepare(parser.parse_args().output)
