"""Bounded, read-only structural survey of HSD archives (JOBJ/DOBJ/MOBJ/TOBJ).

This is NOT a renderer and NOT a geometry decoder. It parses the HSD archive
container exactly as tools/analyze/hsd_archive.py does (both use the same
`archive_tables` helper), then classifies the structures that relocation fields
point at by validating the *field layouts only* against the field offsets
documented in noclip.website's SYSDOLPHIN.ts at the revision pinned in
dependencies.lock.json (fd973ff240233c63883750a557c62baf6ff2a6c7, MIT;
read-only reference, no code copied into this repository).

Layouts used here (all big-endian, all offsets relative to the archive data
block; the block starts 0x20 bytes into the container):

  HSD_ImageDesc  (0x18): 0x00 data*, 0x04 w:u16, 0x06 h:u16, 0x08 format:u32,
                         0x0C mipmaps, 0x10 minLOD:f32, 0x14 maxLOD:f32
  HSD_TlutDesc   (0x10): 0x00 paletteData*, 0x04 paletteFormat:u32,
                         0x08 tlutName, 0x0C numEntries:u16
  HSD_TObj       (0x5C): 0x00 class*, 0x04 nextSibling*, 0x08 animID, 0x0C src,
                         0x10 rotation[3]:f32, 0x1C scale[3]:f32,
                         0x28 translation[3]:f32, 0x34 wrapS, 0x38 wrapT,
                         0x3C repeatS:u8, 0x3D repeatT:u8, 0x40 flags,
                         0x44 blending:f32, 0x48 magFilt, 0x4C imageDesc*,
                         0x50 tlutDesc*, 0x54 texLODDesc*, 0x58 tevDesc*
  HSD_Material   (0x14): 0x00 ambient, 0x04 diffuse, 0x08 specular,
                         0x0C alpha:f32, 0x10 shininess:f32
  HSD_MObj       (0x18): 0x00 class*, 0x04 renderMode, 0x08 texDesc*,
                         0x0C material*, 0x10 renderDesc*, 0x14 peDesc*
  HSD_PObj       (0x18): 0x00 class*, 0x04 nextSibling*, 0x08 vtxDescList*,
                         0x0C flags:u16, 0x0E nDisp:u16, 0x10 disp*,
                         0x14 contents
  HSD_DObj       (0x10): 0x00 class*, 0x04 nextSibling*, 0x08 mobj*, 0x0C pobj*
  HSD_JObj       (0x40): 0x00 class*, 0x04 flags, 0x08 firstChild*,
                         0x0C nextSibling*, 0x10 contents*, 0x14 rotation[3],
                         0x20 scale[3], 0x2C translation[3], 0x38 inverseBindPose*,
                         0x3C robj*
  HSD vtx-desc    (0x18): 0x00 attr, 0x04 attrType, 0x08 compCnt, 0x0C compType,
                         0x10 compShift:u8, 0x12 stride:u16, 0x14 arrayOffs*
                          (list is terminated by attr == 0)

Recorded deviations from the reference, not "fixed": the class-name pointer at
offset 0x00 is NULL in every Gotcha Force archive observed so far, so class names
cannot be used for identification; classification is purely structural. The
reference also does not document the layout of the public `scene_data` symbol,
so this tool never assumes one: it follows the archive's own relocation pointers
as an untyped graph and reports which classified nodes are reachable from it.

All counts are capped and every offset/size is validated against the declared
data region; anything that does not validate is reported as unknown/unreadable,
never guessed. Outputs are refused outside local/.
"""
import argparse
import hashlib
import json
import math
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from formats import afs_entries, region  # noqa: E402
from hsd_archive import archive_tables  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
MAX_INPUT_BYTES = 64 * 1024 * 1024
MAX_RELOCATIONS = 4 * 1024 * 1024
MAX_GRAPH_NODES = 20000      # reachability walk from scene_data
MAX_GRAPH_DEPTH = 12
MAX_JOBJ_NODES = 20000       # per archive scene walk
MAX_CHAIN = 8192             # sibling chains (JOBJ/DOBJ/TOBJ/POBJ)
MAX_VTXDESC = 32
MAX_DISP = 65536
MAX_ROOTS_REPORTED = 128
MAX_IMAGES_REPORTED = 256

JOBJ_FLAG_MASK = 0x73FFFFFF          # bits 0..25, 28..30 from HSD_JObjFlags
JOBJ_BILLBOARD_MASK = 0xF00          # BILLBOARD | VBILLBOARD | HBILLBOARD | RBILLBOARD
MOBJ_RENDERMODE_MASK = 0x6C00600F    # diffuse/alpha mode + DIFFUSE/SPECULAR/SHADOW/ZMODE/NO_ZUPDATE/XLU
TOBJ_FLAG_MASK = 0x01FF01FF          # COORD(bits0-3)/LIGHTMAP(4-8)/COLORMAP(16-19)/ALPHAMAP(20-23)/BUMP(24)
POBJ_FLAG_MASK = 0xF001              # type/cull + retained low bit in source envelope POBJs
TEX_FORMATS = {0: "I4", 1: "I8", 2: "IA4", 3: "IA8", 4: "RGB565", 5: "RGB5A3",
               6: "RGBA8", 8: "C4", 9: "C8", 10: "C14X2", 14: "CMPR"}
TLUT_FORMATS = {0: "IA8", 1: "RGB565", 2: "RGB5A3"}


# --------------------------------------------------------------------------
# container
# --------------------------------------------------------------------------
def load_container(data, label):
    """Validate the container through the reviewed helper; expose the raw block."""
    try:
        tables = archive_tables(data)
    except ValueError as exc:
        return {"label": label, "hsd": False, "reason": str(exc)}
    if tables["relocation_count"] > MAX_RELOCATIONS:
        raise ValueError("relocation count exceeds offline bound")
    block = bytes(region(data, 0x20, tables["data_bytes"]))
    reloc = tables["relocations"]
    targets = {entry["target_offset"] for entry in reloc}
    # The reference also treats public symbol offsets as valid structure starts.
    targets.update(public["offset"] for public in tables["publics"])
    targets.discard(0)
    fields = {entry["field_offset"]: entry["target_offset"] for entry in reloc}
    return {
        "label": label,
        "hsd": True,
        "tables": tables,
        "block": block,
        "targets": targets,
        "fields": fields,
    }


def probe_bytes(data, size):
    """Cheaply name a non-HSD container from its first bytes (no guessing beyond magic)."""
    head = bytes(data[:16])
    if head.startswith(b"Yaz0"):
        return "Yaz0-compressed (not parsed: decompressor is out of scope)"
    if head.startswith(b"Yay0"):
        return "Yay0-compressed (not parsed: decompressor is out of scope)"
    if head.startswith(b"\x1f\x8b"):
        return "gzip stream (not parsed)"
    if head[:1] == b"\x78":
        return "possible zlib stream (not parsed)"
    return "unrecognised container; not an HSD archive at offset 0 (not parsed)"


# --------------------------------------------------------------------------
# field readers
# --------------------------------------------------------------------------
class Reader:
    def __init__(self, ctx):
        self.block = ctx["block"]
        self.size = len(self.block)
        self.targets = ctx["targets"]
        self.fields = ctx["fields"]

    def has(self, off, width):
        return 0 <= off and off + width <= self.size

    def u32(self, off):
        return struct.unpack(">I", self.block[off:off + 4])[0] if self.has(off, 4) else None

    def u16(self, off):
        return struct.unpack(">H", self.block[off:off + 2])[0] if self.has(off, 2) else None

    def u8(self, off):
        return self.block[off] if self.has(off, 1) else None

    def f32(self, off):
        return struct.unpack(">f", self.block[off:off + 4])[0] if self.has(off, 4) else None

    def ptr(self, off):
        """A relocated pointer field: 0/0xFFFFFFFF (null) or a known target offset."""
        value = self.u32(off)
        if value is None:
            return None
        if value in (0, 0xFFFFFFFF):
            return value
        return value if value in self.targets else None

    def finite(self, values, limit=1e6):
        return all(v is not None and math.isfinite(v) and abs(v) <= limit for v in values)


def _class_ok(reader, off):
    """Class-name field: NULL everywhere in this game, but accept a valid pointer."""
    value = reader.u32(off)
    return value == 0 or value in reader.targets


# --------------------------------------------------------------------------
# structural classifiers (positive evidence only)
# --------------------------------------------------------------------------
def as_image_desc(r, off):
    if not r.has(off, 0x18):
        return None
    data_ptr = r.ptr(off)
    if data_ptr is None or data_ptr in (0, 0xFFFFFFFF):
        return None
    width, height = r.u16(off + 4), r.u16(off + 6)
    fmt, mips = r.u32(off + 8), r.u32(off + 0xC)
    if not width or not height or width > 2048 or height > 2048:
        return None
    if fmt not in TEX_FORMATS or mips > 16:
        return None
    if not r.finite([r.f32(off + 0x10), r.f32(off + 0x14)], limit=64):
        return None
    return {"data_offset": data_ptr, "width": width, "height": height,
            "format": TEX_FORMATS[fmt], "mipmaps": mips}


def as_tlut_desc(r, off):
    if not r.has(off, 0x10):
        return None
    data_ptr = r.ptr(off)
    if data_ptr is None or data_ptr in (0, 0xFFFFFFFF):
        return None
    fmt = r.u32(off + 4)
    entries = r.u16(off + 0xC)
    if fmt not in TLUT_FORMATS or not entries or entries > 256:
        return None
    return {"palette_data_offset": data_ptr, "format": TLUT_FORMATS[fmt],
            "entries": entries}


def _colorish(word):
    """RGBA8 colour words here store alpha in the low byte (r,g,b,a byte order)."""
    return word is not None and (word & 0xFF) in (0x00, 0x40, 0x80, 0xC0, 0xFF)


def as_material(r, off):
    if not r.has(off, 0x14):
        return None
    colors = [r.u32(off), r.u32(off + 4), r.u32(off + 8)]
    if not all(_colorish(word) for word in colors):
        return None
    alpha, shininess = r.f32(off + 0xC), r.f32(off + 0x10)
    if not r.finite([alpha, shininess], limit=1e5):
        return None
    if not (0.0 <= alpha <= 2.0):
        return None
    return {"ambient": colors[0], "diffuse": colors[1], "specular": colors[2],
            "alpha": round(alpha, 6), "shininess": round(shininess, 6)}


def as_tobj(r, off):
    if not r.has(off, 0x5C):
        return None
    if not _class_ok(r, off):
        return None
    if r.ptr(off + 4) is None:                     # nextSibling
        return None
    if not r.finite([r.f32(off + 0x10 + 4 * i) for i in range(3)] +
                    [r.f32(off + 0x1C + 4 * i) for i in range(3)] +
                    [r.f32(off + 0x28 + 4 * i) for i in range(3)]):
        return None
    wrap_s, wrap_t = r.u32(off + 0x34), r.u32(off + 0x38)
    if wrap_s not in (0, 1, 2) or wrap_t not in (0, 1, 2):
        return None
    repeat_s, repeat_t = r.u8(off + 0x3C), r.u8(off + 0x3D)
    if not repeat_s or not repeat_t:
        return None
    flags = r.u32(off + 0x40)
    if flags is None or flags & ~TOBJ_FLAG_MASK:
        return None
    blending = r.f32(off + 0x44)
    if not r.finite([blending], limit=1e4):
        return None
    mag_filt = r.u32(off + 0x48)
    if mag_filt is None or mag_filt > 5:
        return None
    image = r.ptr(off + 0x4C)
    if image is None or image in (0, 0xFFFFFFFF):
        return None
    for field in (0x50, 0x54, 0x58):
        if r.ptr(off + field) is None:
            return None
    return {"offs": off, "flags": flags, "src": r.u32(off + 0xC),
            "anim_id": r.u32(off + 8), "wrap_s": wrap_s, "wrap_t": wrap_t,
            "repeat_s": repeat_s, "repeat_t": repeat_t,
            "blending": round(blending, 6), "mag_filt": mag_filt,
            "next_sibling": r.ptr(off + 4),
            "image_desc": image, "tlut_desc": r.ptr(off + 0x50)}


def as_mobj(r, off):
    if not r.has(off, 0x18):
        return None
    if not _class_ok(r, off):
        return None
    render_mode = r.u32(off + 4)
    if render_mode is None:
        return None
    tex, material = r.ptr(off + 8), r.ptr(off + 0xC)
    if tex is None or material is None:
        return None
    if r.ptr(off + 0x10) is None or r.ptr(off + 0x14) is None:
        return None
    if not tex and not material:
        return None
    # Mutual validation: the referenced sub-structures must actually validate.
    if material and as_material(r, material) is None:
        return None
    if tex and as_tobj(r, tex) is None:
        return None
    if not tex and not material:
        return None
    # The reference's HSD_RenderModeFlags does not cover every bit this game
    # sets, so the raw word is recorded instead of rejecting the structure.
    return {"offs": off, "render_mode": render_mode, "tex_desc": tex,
            "material": material, "render_desc": r.u32(off + 0x10),
            "pe_desc": r.u32(off + 0x14),
            "render_mode_unknown_bits": render_mode & ~MOBJ_RENDERMODE_MASK}


def as_pobj(r, off):
    if not r.has(off, 0x18):
        return None
    if not _class_ok(r, off):
        return None
    if r.ptr(off + 4) is None:                     # nextSibling
        return None
    vtx_desc = r.ptr(off + 8)
    if vtx_desc is None or vtx_desc in (0, 0xFFFFFFFF):
        return None
    flags, n_disp = r.u16(off + 0xC), r.u16(off + 0xE)
    if flags is None or flags & ~POBJ_FLAG_MASK:
        return None
    if not n_disp or n_disp > MAX_DISP:
        return None
    disp = r.ptr(off + 0x10)
    if disp is None or disp in (0, 0xFFFFFFFF):
        return None
    entries, reason = vtx_desc_layout(r, vtx_desc)
    if reason or not entries:
        return None
    return {"offs": off, "flags": flags, "n_disp": n_disp,
            "vtx_desc": vtx_desc, "disp": disp,
            "next_sibling": r.ptr(off + 4),
            "obj_type": {0: "skin", 0x1000: "shapeanim", 0x2000: "envelope"}.get(flags & 0x3000, "unknown"),
            "contents": r.u32(off + 0x14)}


def as_dobj(r, off):
    if not r.has(off, 0x10):
        return None
    if not _class_ok(r, off):
        return None
    if r.ptr(off + 4) is None:
        return None
    mobj, pobj = r.ptr(off + 8), r.ptr(off + 0xC)
    if mobj is None or pobj is None:
        return None
    # Require at least one child to validate, otherwise four near-zero words
    # would be enough to look like a DObj.
    if not ((mobj and as_mobj(r, mobj)) or (pobj and as_pobj(r, pobj))):
        return None
    return {"offs": off, "next_sibling": r.ptr(off + 4), "mobj": mobj, "pobj": pobj}


def as_jobj(r, off):
    if not r.has(off, 0x40):
        return None
    if not _class_ok(r, off):
        return None
    flags = r.u32(off + 4)
    if flags is None or flags & ~JOBJ_FLAG_MASK:
        return None
    child, sibling, contents = r.ptr(off + 8), r.ptr(off + 0xC), r.ptr(off + 0x10)
    if child is None or sibling is None or contents is None:
        return None
    if contents and as_dobj(r, contents) is None:
        return None
    rot = [r.f32(off + 0x14 + 4 * i) for i in range(3)]
    scale = [r.f32(off + 0x20 + 4 * i) for i in range(3)]
    trans = [r.f32(off + 0x2C + 4 * i) for i in range(3)]
    if not r.finite(rot + scale + trans):
        return None
    if not all(abs(v) > 1e-9 for v in scale):
        return None
    if r.ptr(off + 0x38) is None or r.ptr(off + 0x3C) is None:
        return None
    return {"offs": off, "flags": flags, "first_child": child,
            "next_sibling": sibling, "contents": contents,
            "rotation": [round(v, 5) for v in rot],
            "scale": [round(v, 5) for v in scale],
            "translation": [round(v, 5) for v in trans],
            "billboard": (flags & JOBJ_BILLBOARD_MASK) >> 9,
            "hidden": bool(flags & 0x10),
            "inverse_bind_pose": r.ptr(off + 0x38), "robj": r.ptr(off + 0x3C)}


CLASSIFIERS = (
    ("jobj", as_jobj),
    ("tobj", as_tobj),
    ("mobj", as_mobj),
    ("dobj", as_dobj),
    ("pobj", as_pobj),
    ("image_desc", as_image_desc),
    ("tlut_desc", as_tlut_desc),
    ("material", as_material),
)


def classify(r, off):
    """All layouts that validate at this offset (ambiguity is reported, not hidden)."""
    hits = {}
    for name, fn in CLASSIFIERS:
        value = fn(r, off)
        if value is not None:
            hits[name] = value
    return hits


# --------------------------------------------------------------------------
# untyped reachability from the public symbol
# --------------------------------------------------------------------------
def reachable_from(reader, start):
    """BFS over relocation pointer fields, treating every structure as untyped.

    The span of a structure is [start, next valid structure start), which is the
    reference's own GetStructSize heuristic; no artificial per-struct cap is
    applied, so pointer arrays inside large descriptors are still followed.
    """
    if start not in reader.targets:
        return set()
    import bisect
    ordered = sorted(reader.targets)
    field_offsets = sorted(reader.fields)
    seen = {start}
    frontier = [start]
    for _ in range(MAX_GRAPH_DEPTH):
        nxt = []
        for node in frontier:
            idx = bisect.bisect_right(ordered, node)
            end = ordered[idx] if idx < len(ordered) else reader.size
            low = bisect.bisect_left(field_offsets, node)
            high = bisect.bisect_left(field_offsets, end)
            for index in range(low, high):
                value = reader.fields[field_offsets[index]]
                if value in reader.targets and value not in seen:
                    seen.add(value)
                    nxt.append(value)
                    if len(seen) > MAX_GRAPH_NODES:
                        return seen
        if not nxt:
            break
        frontier = nxt
    return seen


# --------------------------------------------------------------------------
# typed scene walk
# --------------------------------------------------------------------------
def vtx_desc_layout(reader, off):
    """Walk the GX vtx-desc list; returns (entries, unreadable_reason).

    GX_VA_NULL (0xFF) terminates the list; attribute 0 (position-matrix index)
    is a legal entry, so a zero word is NOT a terminator.
    """
    entries = []
    for index in range(MAX_VTXDESC):
        base = off + index * 0x18
        if not reader.has(base, 0x18):
            return entries, "vtx-desc runs past data region"
        attr = reader.u32(base)
        if attr is None:
            return entries, "vtx-desc read failed"
        if attr == 0xFF:
            return entries, None
        attr_type, comp_cnt, comp_type = (reader.u32(base + 4), reader.u32(base + 8),
                                          reader.u32(base + 0xC))
        comp_shift, stride = reader.u8(base + 0x10), reader.u16(base + 0x12)
        array_offs = reader.u32(base + 0x14)
        if attr > 25 or attr_type > 3 or comp_cnt > 3 or comp_type > 5:
            return entries, "vtx-desc field out of reviewable range"
        if attr_type in (2, 3):
            # 0 means "array at the start of the data block", which is legitimate.
            if array_offs and array_offs not in reader.targets:
                return entries, "indexed vtx array offset is not a relocation target"
        entries.append({"attr": attr, "attr_type": attr_type, "comp_cnt": comp_cnt,
                        "comp_type": comp_type, "comp_shift": comp_shift,
                        "stride": stride,
                        "array_offset": array_offs})
    return entries, "vtx-desc exceeded %d entries" % MAX_VTXDESC


def walk_chain(reader, start, cap=MAX_CHAIN):
    """Yield (offset, classification-dict) along a nextSibling chain."""
    seen = set()
    chain = []
    node = start
    while node and node not in seen and len(chain) < cap:
        seen.add(node)
        hits = classify(reader, node)
        if "dobj" in hits:
            data = hits["dobj"]
        elif "pobj" in hits:
            data = hits["pobj"]
        elif "tobj" in hits:
            data = hits["tobj"]
        else:
            return chain, node
        chain.append(data)
        node = data.get("next_sibling") or 0
    if node in seen:
        return chain, node
    if len(chain) >= cap:
        return chain, node
    return chain, None


def walk_scene(reader, roots):
    """Walk JObj trees and everything reachable through their typed fields."""
    stats = {
        "jobj": 0, "jobj_with_contents": 0, "jobj_empty": 0,
        "jobj_hidden": 0, "jobj_billboard": 0, "jobj_robject": 0,
        "max_depth": 0,
        "dobj": 0, "dobj_without_mobj": 0, "mobj": 0, "tobj": 0, "pobj": 0,
        "material": 0, "image_desc": 0, "tlut_desc": 0,
        "pobj_unreadable": 0, "dobj_unclassified": 0, "tobj_unclassified": 0,
    }
    flag_hist = {}
    billboard_hist = {}
    child_hist = {}
    objtype_hist = {}
    vtx_multiset = {}
    pobj_flag_multiset = {}
    rendermode_unknown = {}
    images = {}
    tluts = {}
    world = {"min": [0.0, 0.0, 0.0], "max": [0.0, 0.0, 0.0], "seen": 0}
    root_reports = []
    unreadable = []
    seen_jobj = set()
    seen_dobj = set()
    seen_mobj = set()
    seen_tobj = set()
    seen_pobj = set()

    def bump(bucket, key):
        bucket[key] = bucket.get(key, 0) + 1

    for root in sorted(roots):
        if stats["jobj"] >= MAX_JOBJ_NODES:
            unreadable.append("JObj node cap reached")
            break
        j = classify(reader, root).get("jobj")
        if j is None:
            continue
        root_report = {"offs": root, "flags": j["flags"],
                       "translation": j["translation"], "billboard": j["billboard"],
                       "depth": 0, "jobj": 0, "dobj": 0, "tobj": 0, "pobj": 0}
        stack = [(root, 0)]
        while stack:
            node, depth = stack.pop()
            if node in seen_jobj or stats["jobj"] >= MAX_JOBJ_NODES:
                continue
            jobj = classify(reader, node).get("jobj")
            if jobj is None:
                unreadable.append("JObj field chain hit unclassified structure at %#x" % node)
                continue
            seen_jobj.add(node)
            stats["jobj"] += 1
            root_report["jobj"] += 1
            stats["max_depth"] = max(stats["max_depth"], depth)
            root_report["depth"] = max(root_report["depth"], depth)
            bump(flag_hist, "%08x" % jobj["flags"])
            if jobj["hidden"]:
                stats["jobj_hidden"] += 1
            if jobj["billboard"]:
                stats["jobj_billboard"] += 1
                bump(billboard_hist, str(jobj["billboard"]))
            if jobj["robj"]:
                stats["jobj_robject"] += 1
            if depth == 0:
                for axis, value in enumerate(jobj["translation"]):
                    if world["seen"] == 0:
                        world["min"][axis] = world["max"][axis] = value
                    else:
                        world["min"][axis] = min(world["min"][axis], value)
                        world["max"][axis] = max(world["max"][axis], value)
                world["seen"] += 1

            children = 0
            child = jobj["first_child"]
            child_seen = set()
            while child and child not in child_seen and child in reader.targets and children < MAX_CHAIN:
                child_seen.add(child)
                children += 1
                stack.append((child, depth + 1))
                child_jobj = classify(reader, child).get("jobj")
                if child_jobj is None:
                    break
                child = child_jobj["next_sibling"]
            bump(child_hist, str(children))

            contents = jobj["contents"]
            if not contents:
                stats["jobj_empty"] += 1
                continue
            stats["jobj_with_contents"] += 1
            dobj_chain, residue = walk_chain(reader, contents)
            if residue:
                stats["dobj_unclassified"] += 1
                unreadable.append("DObj chain residue at %#x (from JObj %#x)" % (residue, node))
            for dobj in dobj_chain:
                if dobj["offs"] in seen_dobj:
                    continue
                seen_dobj.add(dobj["offs"])
                stats["dobj"] += 1
                root_report["dobj"] += 1
                if not dobj["mobj"]:
                    stats["dobj_without_mobj"] += 1
                if dobj["mobj"]:
                    mobj = classify(reader, dobj["mobj"]).get("mobj")
                    if mobj is None:
                        unreadable.append("MObj field of DObj %#x unclassified" % dobj["offs"])
                    elif mobj["offs"] not in seen_mobj:
                        seen_mobj.add(mobj["offs"])
                        stats["mobj"] += 1
                        if mobj["render_mode_unknown_bits"]:
                            bump(rendermode_unknown, "%08x" % mobj["render_mode_unknown_bits"])
                        if mobj["material"]:
                            if as_material(reader, mobj["material"]):
                                stats["material"] += 1
                        tobj_chain, t_residue = walk_chain(reader, mobj["tex_desc"]) if mobj["tex_desc"] else ([], None)
                        if t_residue:
                            stats["tobj_unclassified"] += 1
                            unreadable.append("TObj chain residue at %#x" % t_residue)
                        for tobj in tobj_chain:
                            if tobj["offs"] in seen_tobj:
                                continue
                            seen_tobj.add(tobj["offs"])
                            stats["tobj"] += 1
                            root_report["tobj"] += 1
                            image = as_image_desc(reader, tobj["image_desc"]) if tobj["image_desc"] else None
                            if image:
                                key = (image["width"], image["height"], image["format"])
                                bump(images, key)
                                if stats["image_desc"] < MAX_IMAGES_REPORTED:
                                    stats["image_desc"] += 1
                            else:
                                unreadable.append("ImageDesc of TObj %#x unreadable" % tobj["offs"])
                            if tobj["tlut_desc"]:
                                tlut = as_tlut_desc(reader, tobj["tlut_desc"])
                                if tlut:
                                    bump(tluts, (tlut["entries"], tlut["format"]))
                                    stats["tlut_desc"] += 1
                if dobj["pobj"]:
                    pobj_chain, p_residue = walk_chain(reader, dobj["pobj"])
                    if p_residue:
                        unreadable.append("PObj chain residue at %#x" % p_residue)
                    for pobj in pobj_chain:
                        if pobj["offs"] in seen_pobj:
                            continue
                        seen_pobj.add(pobj["offs"])
                        stats["pobj"] += 1
                        root_report["pobj"] += 1
                        bump(objtype_hist, pobj["obj_type"])
                        bump(pobj_flag_multiset, "%04x" % pobj["flags"])
                        entries, reason = vtx_desc_layout(reader, pobj["vtx_desc"])
                        if reason:
                            stats["pobj_unreadable"] += 1
                            unreadable.append("PObj %#x vtx-desc: %s" % (pobj["offs"], reason))
                        for entry in entries:
                            bump(vtx_multiset, "attr%d/%d/%d/%d" % (
                                entry["attr"], entry["attr_type"],
                                entry["comp_cnt"], entry["comp_type"]))
        if len(root_reports) < MAX_ROOTS_REPORTED:
            root_reports.append(root_report)

    fingerprint = {
        "jobj_flags": flag_hist,
        "jobj_billboard_kinds": billboard_hist,
        "jobj_child_counts": child_hist,
        "pobj_flags": pobj_flag_multiset,
        "pobj_obj_types": objtype_hist,
        "vtx_attrs": vtx_multiset,
        "mobj_render_mode_unknown_bits": rendermode_unknown,
        "textures": {"%dx%d/%s" % k: v for k, v in images.items()},
        "tluts": {"%dx%s" % k: v for k, v in tluts.items()},
    }
    digest = hashlib.sha256(json.dumps(fingerprint, sort_keys=True).encode()).hexdigest()
    return {"stats": stats, "fingerprint": fingerprint, "fingerprint_sha256": digest,
            "root_world_bounds": world if world["seen"] else None,
            "roots": root_reports, "unreadable": unreadable[:64],
            "unreadable_count": len(unreadable)}


def analyse(data, label):
    result = {"input": label, "input_bytes": len(data),
              "input_sha256": hashlib.sha256(data).hexdigest()}
    ctx = load_container(data, label)
    if not ctx["hsd"]:
        result.update(hsd=False, container=probe_bytes(data, len(data)),
                      container_reason=ctx["reason"])
        return result
    tables = ctx["tables"]
    reader = Reader(ctx)
    result.update(hsd=True, container={
        "declared_bytes": tables["declared_bytes"],
        "data_bytes": tables["data_bytes"],
        "relocation_count": tables["relocation_count"],
        "publics": tables["publics"],
        "externs": tables["externs"],
        "zero_padding_bytes": tables["zero_padding_bytes"],
    })

    ordered_targets = sorted(ctx["targets"])
    classification = {}
    ambiguous = []
    for target in ordered_targets:
        hits = classify(reader, target)
        if not hits:
            continue
        if len(hits) > 1:
            ambiguous.append({"offs": target, "matches": sorted(hits)})
        for name in hits:
            classification[name] = classification.get(name, 0) + 1
    result["target_classification"] = {
        "targets_total": len(ordered_targets),
        "classified": classification,
        "ambiguous": ambiguous[:32],
        "ambiguous_count": len(ambiguous),
    }

    public = tables["publics"]
    scene_public = next((p for p in public if p["name"] == "scene_data"), None)
    if scene_public is None and len(public) == 1:
        scene_public = public[0]
    scene = None
    if scene_public is not None:
        start = scene_public["offset"]
        reachable = reachable_from(reader, start)
        reach_counts = {}
        reach_ambiguous = 0
        for target in sorted(reachable):
            hits = classify(reader, target)
            for name in hits:
                reach_counts[name] = reach_counts.get(name, 0) + 1
            if len(hits) > 1:
                reach_ambiguous += 1
        # Roots: JObj nodes reachable from the public symbol that are not another
        # JObj's child or sibling.
        jobjs = {t for t in reachable if "jobj" in classify(reader, t)}
        linked = set()
        for jobj_off in jobjs:
            jobj = as_jobj(reader, jobj_off)
            for pointer in (jobj["first_child"], jobj["next_sibling"]):
                if pointer:
                    linked.add(pointer)
        roots = sorted(jobjs - linked)
        unknown_roots = sorted(reachable - jobjs - linked)
        walk = walk_scene(reader, roots)
        scene = {
            "public_symbol": scene_public["name"],
            "public_offset": start,
            "publics_total": len(public),
            "public_names_sample": [p["name"] for p in public[:12]],
            "public_target_classification": sorted(classify(reader, start)),
            "reachable_nodes": len(reachable),
            "reachable_classification": reach_counts,
            "reachable_ambiguous": reach_ambiguous,
            "jobj_candidates": len(jobjs),
            "root_count": len(roots),
            "roots_not_classified": len(unknown_roots),
            "root_offsets": roots[:MAX_ROOTS_REPORTED],
        }
        scene.update(walk)
    result["scene"] = scene
    return result


# --------------------------------------------------------------------------
# AFS member access
# --------------------------------------------------------------------------
def extract_member(afs_data, entry, destination):
    blob = bytes(region(afs_data, entry["offset"], entry["size"]))
    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.exists():
        existing = destination.read_bytes()
        if existing != blob:
            raise ValueError("private extract differs from archive member: %s" % destination)
        return blob
    with destination.open("xb") as stream:
        stream.write(blob)
    return blob


def private_path(path):
    resolved = path.resolve()
    if not resolved.is_relative_to(ROOT / "local") or resolved == ROOT / "local":
        raise ValueError("analysis output must stay in private local storage: %s" % path)
    return resolved


def looks_like_hsd(blob, size):
    """Cheap container plausibility test used by the sweep (header words only)."""
    if size < 0x20:
        return False
    try:
        declared, data, reloc, public, extern, version, reserved0, reserved1 = \
            struct.unpack(">8I", blob[:0x20])
    except struct.error:
        return False
    if reserved0 or reserved1 or version != 0:
        return False
    if declared < 0x40 or declared > size:
        return False
    # Every real HSD archive in this game exports at least one symbol and has a
    # non-degenerate relocation table; that rules out zero-field false positives.
    if reloc < 4 or public < 1:
        return False
    if data < 0x40:
        return False
    if 0x20 + data > declared or 0x20 + data + reloc * 4 > declared:
        return False
    if 0x20 + data + reloc * 4 + (public + extern) * 8 > declared:
        return False
    return True


def embedded_slices(data, limit=64):
    """Find HSD containers stored at 4-byte-aligned offsets inside a container.

    The only accepted evidence is that the header words validate AND the exact
    declared slice passes the reviewed table validator. Anything else is ignored.
    """
    found = []
    for off in range(0, len(data) - 0x20, 4):
        if not looks_like_hsd(data[off:off + 0x20], len(data) - off):
            continue
        declared = struct.unpack(">I", data[off:off + 4])[0]
        try:
            archive_tables(data[off:off + declared])
        except ValueError:
            continue
        found.append((off, declared))
        if len(found) >= limit:
            break
    return found


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("inputs", nargs="*", type=Path, help="archive files to analyse")
    parser.add_argument("--afs", type=Path, help="AFS volume to pull members from")
    parser.add_argument("--member", action="append", default=[], help="AFS member name (repeatable)")
    parser.add_argument("--sweep", action="store_true",
                        help="analyse every AFS member whose header validates as HSD")
    parser.add_argument("--scan-embedded", action="store_true",
                        help="analyse each HSD container found inside each input container")
    parser.add_argument("--limit", type=int, default=0, help="cap the number of sweep/scan members")
    parser.add_argument("--extract-dir", type=Path, default=Path("local/analysis/arc"),
                        help="private directory for extracted members")
    parser.add_argument("--output", type=Path, help="directory for one JSON summary per input")
    parser.add_argument("--json", type=Path, help="single JSON with every summary")
    parser.add_argument("--print", dest="do_print", action="store_true",
                        help="print the compact per-archive table")
    args = parser.parse_args(argv)

    jobs = [(path, path.name, None) for path in args.inputs]
    afs_data = None
    extract_dir = private_path(args.extract_dir)
    if args.member or args.sweep:
        if args.afs is None:
            raise SystemExit("--member/--sweep require --afs")
        afs_data = args.afs.read_bytes()
        entries = afs_entries(afs_data)
        by_name = {entry["name"]: entry for entry in entries}
        if args.sweep:
            selected = []
            for entry in entries:
                if not entry["size"]:
                    continue
                head = afs_data[entry["offset"]:entry["offset"] + 0x20]
                if looks_like_hsd(head, entry["size"]):
                    selected.append(entry)
            if args.limit:
                selected = selected[:args.limit]
            jobs.extend((None, entry["name"], entry) for entry in selected)
        for name in args.member:
            if name not in by_name or not by_name[name]["size"]:
                raise SystemExit("member not present in AFS: %s" % name)
            jobs.append((None, name, by_name[name]))
    if not jobs:
        raise SystemExit("nothing to analyse")

    summaries = []
    for path, label, entry in jobs:
        if entry is not None:
            destination = extract_dir / Path(label).name
            data = extract_member(afs_data, entry, destination)
            source = "afs:%s#%d" % (args.afs, entry["index"])
        else:
            if path.stat().st_size > MAX_INPUT_BYTES:
                raise SystemExit("input exceeds offline bound: %s" % path)
            data = path.read_bytes()
            source = str(path)
        if args.scan_embedded:
            slices = embedded_slices(data, limit=args.limit or 64)
            if not slices:
                summary = {"input": label, "source": source, "hsd": False,
                           "container": "no HSD container found at any 4-byte-aligned offset"}
                summaries.append(summary)
                if args.do_print:
                    print_table_row(summary)
            for offset, declared in slices:
                slice_label = "%s@%#x" % (label, offset)
                summary = analyse(data[offset:offset + declared], slice_label)
                summary["source"] = source
                summary["container_offset"] = offset
                summaries.append(summary)
                if args.do_print:
                    print_table_row(summary)
            continue
        summary = analyse(data, label)
        summary["source"] = source
        summaries.append(summary)
        if args.do_print:
            print_table_row(summary)
        if args.output:
            target = private_path(args.output / (Path(label).stem + ".json"))
            target.parent.mkdir(parents=True, exist_ok=True)
            with target.open("w") as stream:
                json.dump(summary, stream, indent=1, sort_keys=True)
                stream.write("\n")
    if args.json:
        target = private_path(args.json)
        target.parent.mkdir(parents=True, exist_ok=True)
        with target.open("w") as stream:
            json.dump(summaries, stream, indent=1, sort_keys=True)
            stream.write("\n")
    return 0


def print_table_row(summary):
    name = summary["input"]
    if not summary["hsd"]:
        print("%-22s NOT-HSD  %s" % (name, summary["container"]))
        return
    scene = summary.get("scene") or {}
    stats = scene.get("stats", {})
    tex = scene.get("fingerprint", {}).get("textures", {})
    bbk = scene.get("fingerprint", {}).get("jobj_billboard_kinds", {})
    print("%-22s roots=%-3d jobj=%-4d(d%d,bb%d%s) dobj=%-4d(bare %d) mobj=%-4d "
          "tobj=%-3d pobj=%-3d img=%-3d tlut=%-3d tex=%s" % (
              name, scene.get("root_count", 0), stats.get("jobj", 0),
              stats.get("max_depth", 0), stats.get("jobj_billboard", 0),
              (":" + "/".join("%sx%d" % (k, v) for k, v in sorted(bbk.items()))) if bbk else "",
              stats.get("dobj", 0), stats.get("dobj_without_mobj", 0),
              stats.get("mobj", 0), stats.get("tobj", 0),
              stats.get("pobj", 0), stats.get("image_desc", 0),
              stats.get("tlut_desc", 0),
              ",".join(sorted(tex)) or "-"))


if __name__ == "__main__":
    raise SystemExit(main())
