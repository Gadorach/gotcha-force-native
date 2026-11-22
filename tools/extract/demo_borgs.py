"""Generate two original example Borgs and a flat arena; no game inputs."""
from argparse import ArgumentParser
from pathlib import Path
import struct


class Archive:
    def __init__(self):
        self.block = bytearray(b"\0" * 16)
        self.reloc = []

    def reserve(self, size):
        offset = len(self.block)
        self.block.extend(bytes((size + 3) & ~3))
        return offset

    def word(self, offset, *values):
        struct.pack_into(">" + "I" * len(values), self.block, offset, *values)

    def real(self, offset, *values):
        struct.pack_into(">" + "f" * len(values), self.block, offset, *values)

    def pointer(self, offset, target):
        self.word(offset, target)
        self.reloc.append(offset)

    def finish(self, symbols):
        names = bytearray()
        table = bytearray()
        for offset, name in symbols:
            table += struct.pack(">II", offset, len(names))
            names += name.encode("ascii") + b"\0"
        reloc = b"".join(struct.pack(">I", n) for n in sorted(self.reloc))
        size = 32 + len(self.block) + len(reloc) + len(table) + len(names)
        return struct.pack(">8I", size, len(self.block), len(self.reloc), len(symbols), 0, 0, 0, 0) + self.block + reloc + table + names


def box_vertices(center, half):
    # Clockwise render winding; each face supplies its own flat normal.
    for axis in range(3):
        j, k = (axis + 1) % 3, (axis + 2) % 3
        for sign in (-1, 1):
            normal = [0, 0, 0]
            normal[axis] = sign
            corners = []
            for u, v in ((-1, -1), (-1, 1), (1, 1), (1, -1)):
                xyz = list(center)
                xyz[axis] += sign * half[axis]
                xyz[j] += u * half[j]
                xyz[k] += v * half[k]
                corners.append(xyz)
            for n in ((0, 1, 2, 0, 2, 3) if sign > 0 else (0, 2, 1, 0, 3, 2)):
                yield (*corners[n], *normal)


def rigid_part(a, joint, boxes, color, vtx):
    dobj, mobj, material, pobj = (a.reserve(n) for n in (16, 32, 20, 24))
    display = bytearray()
    for center, half in boxes:
        vertices = list(box_vertices(center, half))
        display += struct.pack(">BH", 0x90, len(vertices))
        display += b"".join(struct.pack(">6f", *v) for v in vertices)
    words = (len(display) + 32) // 32
    data = a.reserve(words * 32)
    a.block[data:data + len(display)] = display
    a.pointer(joint + 16, dobj)
    a.pointer(dobj + 8, mobj)
    a.pointer(dobj + 12, pobj)
    a.word(mobj + 4, 0x201D)
    a.pointer(mobj + 12, material)
    a.word(material, color, color, 0x303030FF)
    a.real(material + 12, 1, 0)
    a.pointer(pobj + 8, vtx)
    a.word(pobj + 12, 0x80000000 | words)
    a.pointer(pobj + 16, data)


def model(records, static_stage=False):
    a = Archive()
    scene, table = a.reserve(16), a.reserve(4 * (len(records) + 1))
    a.pointer(scene, table)
    attrib = a.reserve(72)
    a.word(attrib, 9, 1, 1, 4, 0, 0)
    a.word(attrib + 24, 10, 1, 0, 4, 0, 0)
    a.word(attrib + 48, 255)
    offsets = []
    for i, parts in enumerate(records):
        header = a.reserve(12)
        joints = [a.reserve(64) for _ in parts]
        offsets.append(joints)
        a.pointer(table + 4 * i, header)
        a.pointer(header, joints[0])
        for j, (translation, child, sibling, _, boxes, color) in enumerate(parts):
            at = joints[j]
            a.word(at + 4, 0x10040080 if static_stage else 0x80)
            a.real(at + 32, 1, 1, 1)
            a.real(at + 44, *translation)
            if child is not None:
                a.pointer(at + 8, joints[child])
            if sibling is not None:
                a.pointer(at + 12, joints[sibling])
            rigid_part(a, at, boxes, color, attrib)
    return a.finish([(scene, "scene_data")]), offsets


def mannequin(color, accent):
    # Root at feet. Chest, head and arms are the upper/temporary aim lane.
    return [
        ((0, 0, 0), 1, None, None, [((0, 65, 0), (12, 5, 5))], color),
        ((0, 66, 0), 2, 5, 0, [((0, 17, 0), (7, 19, 5))], color),
        ((0, 40, 0), None, 3, 1, [((0, 0, 0), (12, 12, 8)), ((0, 2, 8), (10, 2, 1))], accent),
        ((-15, 28, 0), None, 4, 1, [((-8, -12, 0), (4, 15, 4))], color),
        ((15, 28, 0), None, None, 1, [((8, -12, 0), (4, 15, 4))], color),
        ((-9, 62, 0), None, 6, 0, [((-4, -28, 0), (4, 28, 4)), ((-4, -58, 4), (5, 4, 8))], color),
        ((9, 62, 0), None, None, 0, [((4, -28, 0), (4, 28, 4)), ((4, -58, 4), (5, 4, 8))], color),
    ]


def bindings(parts, joints):
    n = len(parts)
    prefix = bytearray(256)
    struct.pack_into(">III", prefix, 0, 256, 32, 192)
    for i, (_, child, sibling, parent, _, _) in enumerate(parts):
        prefix[32 + 4 * i:36 + 4 * i] = bytes(255 if p is None else p for p in (child, sibling, parent)) + bytes(1)
        prefix[192 + i] = i
    at = (32 + 2 * n + 3) & ~3
    layout = struct.pack(">4sHH4B5I", b"GFCP", 1, n, 3, 4, n, n, 32, 32+n, at, at+4*n, joints[0])
    layout += bytes((1, 2, 3, 4, 0, 5, 6)) + bytes(range(n))
    layout += bytes(at - len(layout)) + struct.pack(">" + "I" * n, *joints)
    return bytes(prefix), layout


def clip(angles):
    a = Archive()
    nodes = [a.reserve(20) for _ in range(7)]
    for bone, values in angles.items():
        obj, track = a.reserve(16), a.reserve(20)
        stream = b"".join(struct.pack("<BfB", 2, x, wait) for x, wait in zip(values, (8, 8, 8, 1), strict=True))
        data = a.reserve(len(stream))
        a.block[data:data + len(stream)] = stream
        a.pointer(nodes[bone] + 8, obj)
        a.real(obj + 4, 24)
        a.pointer(obj + 8, track)
        a.word(track + 4, len(stream))
        a.block[track + 12] = 1
        a.pointer(track + 16, data)
    return a.finish([(node, f"bone_{i}") for i, node in enumerate(nodes)])


def motion_bank():
    idle = (0., 0., 0., 0.)
    a, b = (0., .36, -.36, 0.), (0., -.36, .36, 0.)
    clips = (clip({3: idle}), clip({3: b, 4: a, 5: b, 6: a}),
             clip({5: (.3, .3, .3, .3), 6: (-.3, -.3, -.3, -.3)}),
             clip({1: (.18, .18, .18, .18)}), clip({4: (1.5, 1.5, .9, 0.)}))
    bank = bytearray(64 + 4 * (len(clips) + 1))
    struct.pack_into(">I", bank, 0, 64)
    for i, data in enumerate(clips):
        struct.pack_into(">I", bank, 64 + 4*i, len(bank))
        bank += data
        bank += bytes((-len(bank)) & 3)
    struct.pack_into(">I", bank, 64 + 4*len(clips), 0xFFFFFFFF)
    return bank


def arena():
    def part(y, half, color):
        return ((0, 0, 0), None, None, None, [((0, y, 0), half)], color)
    stage, _ = model([[part(-12, (1800, 12, 1800), 0x314255FF)],
                      [part(-18, (1850, 12, 1850), 0x242C3BFF)],
                      [part(-1, (3, 2, 1670), 0x88A9BBFF)]], static_stage=True)
    # Dedicated synthetic floor with an 8×8 table sharing two plane triangles.
    cells, table = 64, 40
    cell_list = table + 4*cells
    triangles = cell_list + 12
    length = triangles + 112
    floor = struct.pack(">6I2i2I", 0x53544948, length, 500, 500, 8, 8, -2000, -2000, table-8, triangles-8)
    floor += struct.pack(">I", cell_list-8) * cells + struct.pack(">3I", 0, 56, 0xFFFFFFFF)
    lo, hi = -2000., 2000.
    for points in ((lo,0,lo,lo,0,hi,hi,0,lo),(hi,0,lo,lo,0,hi,hi,0,hi)):
        floor += struct.pack(">I13f", 0, *points, 0,1,0,0)
    return stage, floor


def write_new(folder, files):
    folder.mkdir(parents=True, exist_ok=False)
    for name, data in files.items():
        with (folder / name).open("xb") as f:
            f.write(data.encode() if isinstance(data, str) else data)


def prepare(output):
    output = Path(output)
    output.mkdir(parents=True, exist_ok=False)
    stage, floor = arena()
    write_new(output/"arena", {"stage.hsd": stage, "floor.stih": floor})
    for name, label, color, accent, policy, effect in (
        ("flight-stick", "Flight Stick", 0xD55848FF, 0xFFD263FF, "powered-flight=7|32",
         "movement-effect=jet|rise|jet|1|0|-9|-14|44|0"),
        ("jump-stick", "Jump Stick", 0x4BA6A3FF, 0xAEF1D5FF, "air-jump-points=3",
         "movement-effect=jump|air-jump|puff|0|0|0|0|66|18\nmovement-effect=land|landing|puff|0|0|0|0|72|20"),
    ):
        parts = mannequin(color, accent)
        body, [joints] = model([parts])
        prefix, pose = bindings(parts, joints)
        write_new(output/"borgs"/name, {
            "body.hsd": body, "body.prefix": prefix, "body.pose": pose, "body.motions": motion_bank(),
            "catalog.txt": f"GFBORG-CATALOG 1\ndescription=Original {label} example.\ntribe=none\ndefault-variant=normal\nvariant=normal|Normal|100|placeholder\n",
            "gameplay.txt": f"GFBORG-GAMEPLAY 1\nvariant=normal\nmovement=grounded-v1|7|11|14\nhealth=160\n{policy}\n{effect}\n"
                            "rig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\nwalk=0:1\nair=0:2\ndash=0:3\n"
                            "stock=paint|5|90\nprojectile=paint|bullet|1|55|0|0|0|8|110|8|ignore-floor\n"
                            "origin=paint|4|8|-15|5|target-height\nlook=paint|18|255|229|142\n"
                            "move=paint-shot|b|10|any|14|0:4\nrelease=paint-shot|3|paint|1|paint\n",
            "borg.manifest": f"GFBORG 2\nid=example:{name}\nname={label}\ncatalog=catalog.txt\n"
                             "model=body.hsd\nanimations=body.motions\ngameplay=gameplay.txt\n",
        })


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    prepare(parser.parse_args().output)
