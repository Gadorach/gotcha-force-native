"""Decode BNR1 or an explicitly selected bounded TPL image to lossless RGBA PNG.

Supported: I4, IA4, RGB565, RGB5A3, RGBA8, C4 and C8 (IA8/RGB565/RGB5A3 palettes).
Other formats fail explicitly. No runtime parsing is needed on Vita.
"""
import argparse
import binascii
import struct
import sys
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/"analyze"))
from formats import afs_entries, region


def expand(value, bits):
    # GX expands packed channels by bit replication, not floor(v*255/max).
    result, width = value, bits
    while width < 8:
        result = (result << bits) | value
        width += bits
    return result >> (width-8)


def color(word, fmt):
    if fmt not in (4, 5):
        raise ValueError("unsupported packed color format")
    if fmt == 4:
        return bytes((expand(word >> 11, 5), expand((word >> 5) & 63, 6), expand(word & 31, 5), 255))
    if word & 0x8000:
        return bytes((expand((word >> 10) & 31, 5), expand((word >> 5) & 31, 5), expand(word & 31, 5), 255))
    return bytes((expand((word >> 8) & 15, 4), expand((word >> 4) & 15, 4), expand(word & 15, 4), expand(word >> 12, 3)))


def tiled16(data, width, height, fmt):
    if not (0 < width <= 4096 and 0 < height <= 4096) or fmt not in (4, 5):
        raise ValueError("unsupported texture dimensions or encoding")
    needed = ((width+3)//4) * ((height+3)//4) * 32
    region(data, 0, needed)
    out = bytearray(width*height*4)
    offset = 0
    for ty in range(0, height, 4):
        for tx in range(0, width, 4):
            for y in range(4):
                for x in range(4):
                    word = struct.unpack_from(">H", data, offset)[0]
                    offset += 2
                    if tx+x < width and ty+y < height:
                        i = ((ty+y)*width + tx+x)*4
                        out[i:i+4] = color(word, fmt)
    return width, height, bytes(out)


def tiled_indexed(data, width, height, fmt, palette=None):
    if not (0 < width <= 4096 and 0 < height <= 4096) or fmt not in (0, 2, 8, 9):
        raise ValueError("unsupported indexed/intensity dimensions or encoding")
    if fmt in (8, 9) and not palette:
        raise ValueError("indexed texture requires a palette")
    block_height = 4 if fmt in (2, 9) else 8
    needed = ((width+7)//8)*((height+block_height-1)//block_height)*32
    region(data, 0, needed)
    out = bytearray(width*height*4)
    offset = 0
    for ty in range(0, height, block_height):
        for tx in range(0, width, 8):
            for y in range(block_height):
                for x in range(8):
                    if fmt in (2, 9):
                        index = data[offset]
                        offset += 1
                    else:
                        index = (data[offset] >> 4) if x % 2 == 0 else (data[offset] & 15)
                        offset += x % 2
                    if tx+x >= width or ty+y >= height:
                        continue
                    if fmt == 0:
                        # GX intensity expands to all four channels, including alpha.
                        pixel = bytes((expand(index, 4),))*4
                    elif fmt == 2:
                        pixel = bytes((expand(index & 15, 4),)*3+(expand(index >> 4, 4),))
                    else:
                        if index >= len(palette):
                            raise ValueError("pixel index outside declared palette")
                        pixel = palette[index]
                    i = ((ty+y)*width+tx+x)*4
                    out[i:i+4] = pixel
    return width, height, bytes(out)


def tiled_rgba8(data, width, height):
    if not (0 < width <= 4096 and 0 < height <= 4096):
        raise ValueError("unsupported RGBA8 dimensions")
    region(data, 0, ((width+3)//4)*((height+3)//4)*64)
    out = bytearray(width*height*4)
    tile = 0
    for ty in range(0, height, 4):
        for tx in range(0, width, 4):
            for y in range(4):
                for x in range(4):
                    if tx+x >= width or ty+y >= height:
                        continue
                    p = (y*4+x)*2
                    a, r = data[tile+p:tile+p+2]
                    g, b = data[tile+32+p:tile+34+p]
                    i = ((ty+y)*width+tx+x)*4
                    out[i:i+4] = bytes((r, g, b, a))
            tile += 64
    return width, height, bytes(out)


def tpl_layout(data, image_index=None):
    if data[:4] != b"\x00\x20\xaf\x30":
        raise ValueError("not TPL")
    count, table = struct.unpack(">II", region(data, 4, 8))
    if count < 1:
        raise ValueError("empty TPL image table")
    region(data, table, count*8)
    if image_index is None:
        if count != 1:
            raise ValueError("multi-image TPL requires an explicit image index")
        image_index = 0
    if type(image_index) is not int or not 0 <= image_index < count:
        raise ValueError("TPL image index out of bounds")
    image, palette_header = struct.unpack(">II", region(data, table+image_index*8, 8))
    height, width, fmt, offset = struct.unpack(">HHII", region(data, image, 12))
    if not (0 < width <= 4096 and 0 < height <= 4096):
        raise ValueError("unsupported texture dimensions")
    if fmt not in (0, 2, 4, 5, 6, 8, 9):
        raise ValueError("unsupported texture encoding")
    block_width, block_height = (4, 4) if fmt in (4, 5, 6) else (8, 4 if fmt in (2, 9) else 8)
    pixel_size = ((width+block_width-1)//block_width)*((height+block_height-1)//block_height)*(64 if fmt == 6 else 32)
    ranges = [(0, 12), (table, count*8), (image, 36), (offset, pixel_size)]
    palette, palette_format, palette_offset, palette_bytes = None, None, None, 0
    if fmt in (8, 9):
        if not palette_header:
            raise ValueError("missing indexed texture palette")
        entries, unpacked, padding, palette_format, palette_offset = struct.unpack(">HBBII", region(data, palette_header, 12))
        if not 0 < entries <= 256 or palette_format not in (0, 1, 2):
            raise ValueError("unsupported palette count/format")
        palette_bytes = entries*2
        raw = region(data, palette_offset, palette_bytes)
        palette = []
        for (word,) in struct.iter_unpack(">H", raw):
            if palette_format == 0:
                palette.append(bytes((word & 255,)*3+(word >> 8,)))
            else:
                palette.append(color(word, 4 if palette_format == 1 else 5))
        ranges += [(palette_header, 12), (palette_offset, palette_bytes)]
    elif palette_header:
        raise ValueError("unexpected palette for non-indexed texture")
    for index, (start, size) in enumerate(ranges):
        region(data, start, size)
        if any(start < other+length and other < start+size for other, length in ranges[:index]):
            raise ValueError("overlapping TPL header/table/payload ranges")
    return {"width": width, "height": height, "format": fmt, "pixel_offset": offset,
            "image_count": count, "image_index": image_index,
            "pixel_bytes": pixel_size, "palette": palette, "palette_format": palette_format,
            "palette_offset": palette_offset, "palette_bytes": palette_bytes,
            "block_width": block_width, "block_height": block_height}


def decode(data, image_index=None):
    if data[:4] == b"BNR1":
        if image_index not in (None, 0):
            raise ValueError("BNR1 has one image")
        if len(data) != 0x1960:
            raise ValueError("invalid BNR1 length")
        return tiled16(region(data, 0x20, 0x1800), 96, 32, 5)
    layout = tpl_layout(data, image_index)
    pixels = region(data, layout["pixel_offset"], layout["pixel_bytes"])
    if layout["format"] in (4, 5):
        return tiled16(pixels, layout["width"], layout["height"], layout["format"])
    if layout["format"] == 6:
        return tiled_rgba8(pixels, layout["width"], layout["height"])
    return tiled_indexed(pixels, layout["width"], layout["height"], layout["format"], layout["palette"])


def png(width, height, pixels):
    if len(pixels) != width*height*4:
        raise ValueError("RGBA byte count")
    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", binascii.crc32(kind+data))
    rows = b"".join(b"\0"+pixels[y*width*4:(y+1)*width*4] for y in range(height))
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)) +
            chunk(b"IDAT", zlib.compress(rows)) + chunk(b"IEND", b""))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("input", type=Path)
    p.add_argument("output", type=Path)
    p.add_argument("--afs-index", type=int)
    p.add_argument("--image-index", type=int, help="required for multi-image TPL")
    a = p.parse_args()
    if a.afs_index is None:
        data = a.input.read_bytes()
    else:
        import mmap
        with a.input.open("rb") as f, mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ) as archive:
            entries = afs_entries(archive)
            if not 0 <= a.afs_index < len(entries):
                raise ValueError("AFS index out of bounds")
            entry = entries[a.afs_index]
            data = region(archive, entry["offset"], entry["size"])
            print("member", entry["name"], "index", entry["index"])
    width, height, pixels = decode(data, a.image_index)
    a.output.parent.mkdir(parents=True, exist_ok=True)
    with a.output.open("xb") as output:
        output.write(png(width, height, pixels))
    print(f"decoded {width}x{height} RGBA -> {a.output}")


if __name__ == "__main__":
    main()
