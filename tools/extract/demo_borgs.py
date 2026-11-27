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
                yield (*corners[n], *normal, ((0, 0), (0, 1), (1, 1), (1, 0))[n])


def striped_texture(a):
    tobj, image, pixels = (a.reserve(n) for n in (0x5C, 24, 64))
    a.word(tobj + 12, 4)  # TEX0
    a.real(tobj + 0x1C, 1, 1, 1)
    a.word(tobj + 0x34, 1, 1)
    a.block[tobj + 0x3C:tobj + 0x3E] = b"\1\1"
    a.word(tobj + 0x40, 0x00040010)  # diffuse, modulate
    a.real(tobj + 0x44, 1)
    a.pointer(tobj + 0x4C, image)
    a.pointer(image, pixels)
    a.word(image + 4, 0x00040004, 6)  # 4x4 RGBA8 tile (AR and GB planes)
    for i in range(16):
        r, g, b = (255, 245, 193) if (i % 4) < 2 else (79, 42, 25)
        a.block[pixels + 2*i:pixels + 2*i + 2] = bytes((255, r))
        a.block[pixels + 32 + 2*i:pixels + 32 + 2*i + 2] = bytes((g, b))
    return tobj


def rigid_part(a, joint, boxes, color, vtx, texture=0, translucent=False):
    dobj, mobj, material, pobj = (a.reserve(n) for n in (16, 32, 20, 24))
    display = bytearray()
    for center, half in boxes:
        vertices = list(box_vertices(center, half))
        display += struct.pack(">BH", 0x90, len(vertices))
        for *position_normal, uv in vertices:
            display += struct.pack(">6f", *position_normal)
            if texture:
                display += struct.pack(">2f", *uv)
    words = (len(display) + 32) // 32
    data = a.reserve(words * 32)
    a.block[data:data + len(display)] = display
    a.pointer(joint + 16, dobj)
    a.pointer(dobj + 8, mobj)
    a.pointer(dobj + 12, pobj)
    a.word(mobj + 4, 0x6000201D if translucent else 0x201D)
    if texture:
        a.pointer(mobj + 8, texture)
    a.pointer(mobj + 12, material)
    a.word(material, color, color, 0x000000FF if translucent else 0x303030FF)
    a.real(material + 12, 1, 0)
    a.pointer(pobj + 8, vtx)
    a.word(pobj + 12, 0x80000000 | words)
    a.pointer(pobj + 16, data)


def badge_material_clip(a, record):
    table, joint, material, obj, red, green = (a.reserve(n) for n in (8, 12, 16, 16, 20, 20))
    a.pointer(record + 8, table)
    a.pointer(table, joint)
    a.pointer(joint + 8, material)
    a.pointer(material + 4, obj)
    a.real(obj + 4, 5)
    a.pointer(obj + 8, red)
    a.pointer(red, green)
    for track, channel, values in ((red, 4, (1., .22, 1.)), (green, 5, (.33, 1., .33))):
        stream = b"".join(struct.pack("<BfB", 2, value, wait)
                          for value, wait in zip(values, (3, 3, 1), strict=True))
        data = a.reserve(len(stream))
        a.block[data:data + len(stream)] = stream
        a.word(track + 4, len(stream))
        a.block[track + 12] = channel
        a.pointer(track + 16, data)


def model(records, static_stage=False, striped_bone=None, material_record=None, translucent_records=()):
    a = Archive()
    scene, table = a.reserve(16), a.reserve(4 * (len(records) + 1))
    a.pointer(scene, table)
    attrib = a.reserve(72)
    a.word(attrib, 9, 1, 1, 4, 0, 0)
    a.word(attrib + 24, 10, 1, 0, 4, 0, 0)
    a.word(attrib + 48, 255)
    if striped_bone is not None:
        striped_attrib = a.reserve(96)
        a.block[striped_attrib:striped_attrib + 48] = a.block[attrib:attrib + 48]
        a.word(striped_attrib + 48, 13, 1, 1, 4, 0, 0)
        a.word(striped_attrib + 72, 255)
        texture = striped_texture(a)
    offsets = []
    for i, parts in enumerate(records):
        header = a.reserve(16)
        joints = [a.reserve(64) for _ in parts]
        offsets.append(joints)
        a.pointer(table + 4 * i, header)
        a.pointer(header, joints[0])
        for j, (translation, child, sibling, _, boxes, color) in enumerate(parts):
            at = joints[j]
            a.word(at + 4, 0x10040080 if static_stage or i else 0x80)
            a.real(at + 32, 1, 1, 1)
            a.real(at + 44, *translation)
            if child is not None:
                a.pointer(at + 8, joints[child])
            if sibling is not None:
                a.pointer(at + 12, joints[sibling])
            if not boxes:
                continue
            if j == striped_bone:
                rigid_part(a, at, boxes, color, striped_attrib, texture)
            else:
                rigid_part(a, at, boxes, color, attrib, translucent=i in translucent_records)
        if i == material_record:
            badge_material_clip(a, header)
    return a.finish([(scene, "scene_data")]), offsets


def mannequin(color, accent):
    # Root at feet. Chest, head and arms are the upper/temporary aim lane.
    return [
        ((0, 0, 0), 1, None, None, [((0, 65, 0), (12, 5, 5))], color),
        ((0, 66, 0), 2, 6, 0, [((0, 17, 0), (7, 19, 5))], color),
        ((0, 40, 0), None, 3, 1, [((0, 0, 0), (12, 12, 8)), ((0, 2, 8), (10, 2, 1))], accent),
        ((-15, 28, 0), None, 4, 1, [((-8, -12, 0), (4, 15, 4))], color),
        ((15, 28, 0), None, 5, 1, [((8, -12, 0), (4, 15, 4))], color),
        ((0, 20, -12), None, None, 1, [((0, 0, 0), (3, 15, 2)), ((8, 7, 0), (8, 2, 2))], accent),
        ((-9, 62, 0), None, 7, 0, [((-4, -28, 0), (4, 28, 4)), ((-4, -58, 4), (5, 4, 8))], color),
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
    layout = struct.pack(">4sHH4B5I", b"GFCP", 1, n, 3, 5, n, n, 32, 32+n, at, at+4*n, joints[0])
    layout += bytes((1, 2, 3, 4, 5, 0, 6, 7)) + bytes(range(n))
    layout += bytes(at - len(layout)) + struct.pack(">" + "I" * n, *joints)
    return bytes(prefix), layout


def clip(angles):
    a = Archive()
    nodes = [a.reserve(20) for _ in range(8)]
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
    clips = (clip({3: idle}), clip({3: b, 4: a, 6: b, 7: a}),
             clip({6: (.3, .3, .3, .3), 7: (-.3, -.3, -.3, -.3)}),
             clip({1: (.18, .18, .18, .18)}), clip({4: (1.5, 1.5, .9, 0.)}),
             clip({5: (0., .85, -.85, 0.)}))
    bank = bytearray(64 + 4 * (len(clips) + 1))
    struct.pack_into(">I", bank, 0, 64)
    for i, data in enumerate(clips):
        struct.pack_into(">I", bank, 64 + 4*i, len(bank))
        bank += data
        bank += bytes((-len(bank)) & 3)
    struct.pack_into(">I", bank, 64 + 4*len(clips), 0xFFFFFFFF)
    return bank


def collision_grid(planes):
    # Shared list is small and covers every cell in this authored fixture.
    cells, table = 64, 40
    cell_list = table + 4*cells
    triangles = cell_list + 4*(len(planes)+1)
    length = triangles + 56*len(planes)
    data = struct.pack(">6I2i2I", 0x53544948, length, 500, 500, 8, 8, -2000, -2000, table-8, triangles-8)
    data += struct.pack(">I", cell_list-8) * cells
    data += b"".join(struct.pack(">I", 56*i) for i in range(len(planes))) + struct.pack(">I", 0xFFFFFFFF)
    return data + b"".join(struct.pack(">I13f", 0, *points, *plane) for points, plane in planes)


def arena(wall=False, projectile_geometry=False):
    def part(y, half, color):
        return ((0, 0, 0), None, None, None, [((0, y, 0), half)], color)
    scenery = part(260, (620, 260, 6), 0x7A8794FF) if wall else part(-1, (3, 2, 1670), 0x88A9BBFF)
    records = [[part(-12, (1800, 12, 1800), 0x314255FF)],
                      [part(-18, (1850, 12, 1850), 0x242C3BFF)],
                      [scenery]]
    if wall:
        records.append([part(260, (620, 260, 6), 0x000000FF)])
        for color in (0x547860FF, 0x203029FF):
            records.append([((0, 0, 0), None, None, None, [((230, 200, 400), (35, 65, 25))], color)])
    stage, _ = model(records, static_stage=True, translucent_records=(3, 5) if wall else ())
    lo, hi = -2000., 2000.
    floor = collision_grid([((lo,0,lo,lo,0,hi,hi,0,lo), (0,1,0,0)),
                            ((hi,0,lo,lo,0,hi,hi,0,hi), (0,1,0,0))])
    files = {"stage.hsd": stage, "floor.stih": floor}
    if wall:
        files["view-fade.txt"] = (
            "GFSTAGE-FADE 3\n"
            "obstacle=occlusion|2|3|0|0.25|0.5|0.025|-620|0|-6|620|520|6\n"
            "obstacle=occlusion|4|5|0|0.30|0.6|0.025|195|135|375|265|265|425\n"
        )
    if wall or projectile_geometry:
        files["projectile-stage.gfsr"] = struct.pack(">4I5f3I", 0x47465352, 1, 48, 0,
                                                     -1999, 1999, -1999, 1999, 500, 0, 0x80000000, 0)
        files["secondary.stih"] = collision_grid([
            ((-620,0,0,620,0,0,-620,520,0), (0,0,1,0)),
            ((620,0,0,620,520,0,-620,520,0), (0,0,1,0)),
        ] if wall else [])
        files["tertiary.stih"] = collision_grid([])
    return files


def write_new(folder, files):
    folder.mkdir(parents=True, exist_ok=False)
    for name, data in files.items():
        with (folder / name).open("xb") as f:
            f.write(data.encode() if isinstance(data, str) else data)


def prepare(output, clipped_beam=False, swept_beam=False, proximity_bomb=False, attracting_bomb=False,
            fragmenting_bomb=False, smoke_bomb=False, ink_bomb=False, power_field=False, slow_field=False, field_demo=False,
            accuracy_ink=False, elemental_ball=False, impact_ball=False, impact_trail=False, impact_hazard=False,
            walk_pulse=False, gravity_field=False, magnet_field=False, repel_field=False, electric_fence=False,
            cruise_boost=False, impact_area=False):
    if sum((clipped_beam, swept_beam, power_field, slow_field, field_demo, gravity_field, magnet_field, repel_field, electric_fence))>1:
        raise ValueError("select one Flight Stick X policy")
    if walk_pulse and cruise_boost:
        raise ValueError("select walk pulses or continuous flight")
    if impact_area and any((elemental_ball, impact_ball, impact_trail, impact_hazard)):
        raise ValueError("select one paint shot impact policy")
    output = Path(output)
    output.mkdir(parents=True, exist_ok=False)
    write_new(output/"arena", arena())
    write_new(output/"arena-wall", arena(wall=True))
    if attracting_bomb:
        write_new(output/"arena-bomb", arena(projectile_geometry=True))
    declarations = Path(__file__).resolve().parents[2] / "examples" / "borgs"
    for name, color, accent in (
        ("flight-stick", 0xD55848FF, 0xFFD263FF),
        ("jump-stick", 0x4BA6A3FF, 0xAEF1D5FF),
    ):
        parts = mannequin(color, accent)
        records = [parts]
        if name == "jump-stick":
            # Draw the badge as an independent record on the spinning bone.
            # Swap the visible badge during the shot without touching the pose.
            parts[5] = (*parts[5][:4], [], accent)
            def badge(boxes, tint):
                return [((0, 0, 0), None, None, None, boxes, tint)]
            records += [badge([((0, 0, 0), (7, 13, 2))], accent),
                        badge([((0, 0, 0), (4, 21, 2)), ((0, 0, 0), (21, 4, 2))], 0xFFD263FF)]
        body, [joints, *_] = model(records, striped_bone=5 if name == "flight-stick" else None,
                                   material_record=2 if name == "jump-stick" else None)
        prefix, pose = bindings(parts, joints)
        package = {file: (declarations / name / file).read_bytes()
                   for file in ("borg.manifest", "catalog.txt", "gameplay.txt")}
        if (clipped_beam or swept_beam) and name == "flight-stick":
            row = b"|18|28|3|ignore-floor"
            if package["gameplay.txt"].count(row) != 1:
                raise ValueError("example beam policy row changed")
            package["gameplay.txt"] = package["gameplay.txt"].replace(row, b"|18|28|3|clip-on-stage")
            if swept_beam:
                original = b"sustained-beam=paint-stream|780|4|0.035\norigin=paint-stream|4|8|-15|5|target-height\n"
                if package["gameplay.txt"].count(original) != 1:
                    raise ValueError("example sweep rows changed")
                package["gameplay.txt"] = package["gameplay.txt"].replace(original,
                    b"sustained-beam=paint-stream|460|4|0\nbeam-sweep=paint-stream|700|420|36\norigin=paint-stream|4|8|-15|5|fixed\n")
        if (power_field or slow_field or field_demo or gravity_field or magnet_field or repel_field or electric_fence) and name == "flight-stick":
            gameplay_prefix, marker, _ = package["gameplay.txt"].partition(b"projectile=paint-stream|")
            if not marker:
                raise ValueError("example beam rows changed")
            if electric_fence:
                scope = (b"anchor-set=fence|4\nplace-anchor=paint-scope|3|fence|none|0\n"
                         b"place-anchor=paint-scope|6|fence|none|0\nplace-anchor-offset=paint-scope|3|90|50\n"
                         b"place-anchor-offset=paint-scope|6|230|50\nanchor-fence=fence|26|8|90\nfence-recoil=fence|12\n")
            elif magnet_field or repel_field:
                scope = (b"field=paint-scope|3|sphere|360|360|360|60|0|50|82|190|215|85\n"
                         b"field-actor-pull=paint-scope|enemies|7|0.55\n")
                if repel_field:
                    scope += b"field-actor-polarity=paint-scope|away\n"
            elif gravity_field:
                scope = (b"field=paint-scope|3|sphere|275|275|275|95|180|30|112|64|220|120\n"
                         b"field-shot-pull=paint-scope|enemies|12\n"
                         b"field-core-damage=paint-scope|enemies|96|5|16\n")
            elif field_demo:
                scope = (b"field=paint-scope|3|sphere|210|210|210|105|0|0|135|220|255|130\n"
                         b"field-socket=paint-scope|1|0|4|120\n"
                         b"field-effect=paint-scope|actor-speed|source|130\nfield-effect=paint-scope|actor-speed|enemies|60\n"
                         b"field-effect=paint-scope|shot-damage|allies|200\nfield-effect=paint-scope|shot-speed|enemies|60\n"
                         b"field-texture=paint-scope|scope.rgba\n")
                pixels = bytearray()
                for y in range(32):
                    for x in range(32):
                        d2 = (2*x-31)**2+(2*y-31)**2
                        pixels.extend((255, 255, 255, 0 if d2>961 else 230 if d2>729 else 96))
                package["scope.rgba"] = pixels
            elif power_field:
                scope = b"power-field=paint-scope|3|170|105|160|75|200|145\n"
            else:
                scope = b"slow-field=paint-scope|3|210|105|200|65|50|50|break-guidance\n"
            package["gameplay.txt"] = gameplay_prefix + b"move=paint-scope|x|20|any|14|0:4\n" + scope
        if (proximity_bomb or attracting_bomb or fragmenting_bomb or smoke_bomb or ink_bomb or accuracy_ink) and name == "jump-stick":
            gameplay_prefix, marker, _ = package["gameplay.txt"].partition(b"projectile=long-paint|")
            if not marker:
                raise ValueError("example X beam row changed")
            package["gameplay.txt"] = gameplay_prefix + (
                b"stock=bomb|1|on-return\nprojectile=charge|bullet|1|25|-6|1|0|12|52|0|land-on-floor\n"
                b"actor-contact=charge|ignore\nblast=charge|expiry|440|12|opponent\n"
                b"bomb-proximity=charge|560|16|blast\nreturn-stock=charge|expiry-or-proximity\n"
                b"look=charge|33|255|178|84\nmove=drop-charge|x|20|any|22|0:4\nrelease=drop-charge|4|bomb|1|charge\n"
            )
            if attracting_bomb:
                package["gameplay.txt"] = package["gameplay.txt"].replace(b"charge|560|16|blast", b"charge|340|16|blast")
                package["gameplay.txt"] += b"bomb-attraction=charge|900|3|12\n"
            if fragmenting_bomb:
                package["gameplay.txt"] += (
                    b"projectile=shard|bullet|6|38|8|0.25|0|6|24|3|stop-on-floor\nlook=shard|18|247|215|91\n"
                    b"bomb-fuse-tail=charge|8\nblast-children=charge|shard\n"
                )
            if smoke_bomb:
                package["gameplay.txt"] = package["gameplay.txt"].replace(b"expiry|440|12|opponent", b"expiry|440|0|opponent")
                package["gameplay.txt"] += b"blast-smoke=charge|210|75|block-aim\n"
            if ink_bomb or accuracy_ink:
                package["gameplay.txt"] += b"blast-ink=charge|24|60|225\n"
            if accuracy_ink:
                package["gameplay.txt"] = package["gameplay.txt"].replace(b"expiry|440|12|opponent", b"expiry|570|12|opponent")
                package["gameplay.txt"] += b"blast-accuracy=charge|52|0.30\n"
        if accuracy_ink and name == "flight-stick":
            package["gameplay.txt"] += b"accuracy-sensitive=paint\n"
        if walk_pulse and name == "flight-stick":
            package["gameplay.txt"] += b"walk-pulse=40|12|130|25|10|6\n"
        if cruise_boost and name == "flight-stick":
            package["gameplay.txt"] = package["gameplay.txt"].replace(
                b"powered-flight=7|32\n", b"cruise-boost=80|18|4|2|7|4\n")
        if (impact_trail or impact_hazard or impact_area) and name == "flight-stick":
            package["gameplay.txt"] = package["gameplay.txt"].replace(b"|8|110|8|ignore-floor", b"|8|110|0|stop-on-stage")
            if impact_area:
                package["gameplay.txt"] += (
                    b"impact-area=paint|stage|3.1|31|160|8|17\n"
                    b"impact-trail=paint|stage|3|2|0|0|0|1|96|10\n"
                )
            elif impact_hazard:
                package["gameplay.txt"] += (
                    b"impact-hazard=paint|stage|12|2|24|12|16|3|56|120|12|5\n"
                    b"impact-trail=paint|stage|12|2|24|12|16|3|56|12\n"
                )
            else:
                package["gameplay.txt"] += b"impact-trail=paint|stage|24|1|24|6|8|3|21|12\n"
        elif (elemental_ball or impact_ball) and name == "flight-stick":
            package["gameplay.txt"] += (
                b"projectile=sparks|bullet|2|32|0|0|0.32|6|28|3|ignore-floor\n"
                b"look=sparks|10|255|165|80\n"
            )
            if impact_ball:
                package["gameplay.txt"] = package["gameplay.txt"].replace(b"|8|110|8|ignore-floor", b"|8|110|8|stop-on-stage")
                package["gameplay.txt"] += b"impact-children=paint|sparks|stage|forward\n"
            else:
                package["gameplay.txt"] += b"flight-children=paint|sparks|4|6|3\n"
        write_new(output/"borgs"/name, {
            "body.hsd": body, "body.prefix": prefix, "body.pose": pose, "body.motions": motion_bank(),
            **package,
        })


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--clipped-beam", action="store_true", help="enable Flight Stick clipping for arena-wall")
    group.add_argument("--swept-beam", action="store_true", help="enable Flight Stick's authored terrain sweep for arena-wall")
    group.add_argument("--power-field", action="store_true", help="replace Flight Stick's X with an authored projectile booster")
    group.add_argument("--slow-field", action="store_true", help="replace Flight Stick's X with an authored slowing field")
    group.add_argument("--field-demo", action="store_true", help="replace Flight Stick's X with one textured field registering several effects")
    group.add_argument("--gravity-field", action="store_true", help="replace Flight Stick's X with a projectile-pulling field and damaging core")
    group.add_argument("--magnet-field", action="store_true", help="replace Flight Stick's X with a forward actor-pull field")
    group.add_argument("--repel-field", action="store_true", help="replace Flight Stick's X with a forward actor-repulsion field")
    group.add_argument("--electric-fence", action="store_true", help="place two connected, damaging anchors on Flight Stick's X")
    movement = parser.add_mutually_exclusive_group()
    movement.add_argument("--walk-pulse", action="store_true", help="give Flight Stick a passive shockwave while walking")
    movement.add_argument("--cruise-boost", action="store_true", help="give Flight Stick continuous flight with airborne boost refill")
    bombs = parser.add_mutually_exclusive_group()
    bombs.add_argument("--proximity-bomb", action="store_true", help="give Jump Stick a fuse and proximity bomb on X")
    bombs.add_argument("--attracting-bomb", action="store_true", help="give Jump Stick an attracting bomb for arena-bomb")
    bombs.add_argument("--fragmenting-bomb", action="store_true", help="give Jump Stick an authored radial burst")
    bombs.add_argument("--smoke-bomb", action="store_true", help="give Jump Stick an authored smoke cloud on X")
    bombs.add_argument("--ink-bomb", action="store_true", help="give Jump Stick an authored viewport splatter on X")
    bombs.add_argument("--accuracy-ink", action="store_true", help="demonstrate a separate accuracy effect on Flight Stick's B")
    balls = parser.add_mutually_exclusive_group()
    balls.add_argument("--elemental-ball", action="store_true", help="give Flight Stick's B a travelling paint-ball carrier")
    balls.add_argument("--impact-ball", action="store_true", help="give Flight Stick's B a wall-triggered volley for arena-wall")
    balls.add_argument("--impact-trail", action="store_true", help="give Flight Stick's B a harmless rotating wall effect for arena-wall")
    balls.add_argument("--impact-hazard", action="store_true", help="give Flight Stick's B marked wall-impact hazards for arena-wall")
    balls.add_argument("--impact-area", action="store_true", help="give Flight Stick's B an expanding area for arena-wall")
    args = parser.parse_args()
    prepare(args.output, args.clipped_beam, args.swept_beam, args.proximity_bomb, args.attracting_bomb,
            args.fragmenting_bomb, args.smoke_bomb, args.ink_bomb, args.power_field, args.slow_field, args.field_demo,
            args.accuracy_ink, args.elemental_ball, args.impact_ball, args.impact_trail, args.impact_hazard,
            args.walk_pulse, args.gravity_field, args.magnet_field, args.repel_field, args.electric_fence, args.cruise_boost,
            args.impact_area)
