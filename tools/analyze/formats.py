"""Read-only, bounds-checked probes. No format is inferred from its extension alone."""
import struct


def region(data, offset, size):
    if offset < 0 or size < 0 or offset > len(data) or size > len(data) - offset:
        raise ValueError(f"range outside file: offset={offset:#x} size={size:#x}")
    return data[offset:offset + size]


def dol_layout(data):
    region(data, 0, 256)
    sections = []
    for i in range(18):
        offset, address, size = (struct.unpack_from(">I", data, base + i*4)[0] for base in (0, 0x48, 0x90))
        if not size:
            continue
        region(data, offset, size)
        if offset < 256 or address < 0x80000000 or address + size > 0x81800000:
            raise ValueError("DOL section outside GameCube MEM1 or overlaps header")
        if any(address < s["address"] + s["size"] and s["address"] < address + size for s in sections):
            raise ValueError("overlapping DOL initialized sections")
        sections.append(dict(kind="text" if i < 7 else "data", index=i if i < 7 else i-7,
                             offset=offset, address=address, size=size))
    bss, bss_size, entry = struct.unpack_from(">III", data, 0xd8)
    if bss_size and (bss < 0x80000000 or bss + bss_size > 0x81800000):
        raise ValueError("DOL BSS outside MEM1")
    if entry & 3 or not any(s["kind"] == "text" and s["address"] <= entry < s["address"]+s["size"] for s in sections):
        raise ValueError("entry outside text")
    return dict(sections=sections, bss_address=bss, bss_size=bss_size, entry=entry)


def afs_entries(data):
    if region(data, 0, 4) not in (b"AFS\0", b"AFS "):
        raise ValueError("not AFS")
    count = struct.unpack("<I", region(data, 4, 4))[0]
    if not count or count > (len(data)-8)//8:
        raise ValueError("invalid AFS count")
    table_end = 8 + count*8
    table = region(data, 8, count*8)
    pairs = list(struct.iter_unpack("<II", table))
    first = min((off for off, size in pairs if size), default=len(data))
    if first < table_end:
        raise ValueError("AFS payload overlaps table")
    directory = None
    # Documented pointer locations: directly after TOC, or just before payload.
    for location in dict.fromkeys((table_end, first-8)):
        if location < table_end or location+8 > first:
            continue
        offset, size = struct.unpack("<II", region(data, location, 8))
        if offset == 0 and size == 0:
            continue
        if size != count*48 or offset < table_end:
            raise ValueError("invalid AFS filename directory")
        directory = region(data, offset, size)
        break
    result = []
    for index, (offset, size) in enumerate(pairs):
        region(data, offset, size)
        if size and offset < table_end:
            raise ValueError("AFS member overlaps table")
        name = None
        if directory is not None:
            name = directory[index*48:index*48+32].split(b"\0", 1)[0].decode("shift_jis", errors="replace")
        result.append(dict(index=index, offset=offset, size=size, name=name))
    return result
