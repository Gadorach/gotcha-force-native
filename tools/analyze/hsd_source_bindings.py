"""Published HSD model/animation binding metadata for offline extraction.

Reuses the container/relocation validator; no heuristic tree matching, sampling,
rendering or proprietary presets. Texture/shape references remain explicit,
not silently treated as implemented animation consumers.
"""
import hashlib
import math
import struct

from tools.analyze.hsd_scene import load_container


class BindingReader:
    def __init__(self, data):
        if len(data) > 64 << 20:
            raise ValueError("HSD budget")
        self.context = load_container(data, "source-binding")
        if not self.context["hsd"]:
            raise ValueError("invalid HSD container")
        self.block = self.context["block"]
        self.fields = self.context["fields"]

    def read(self, at, size):
        if at < 0 or size < 0 or at > len(self.block) or size > len(self.block) - at:
            raise ValueError("HSD binding range")
        return self.block[at:at + size]

    def u32(self, at):
        return int.from_bytes(self.read(at, 4), "big")

    def pointer(self, at):
        value = self.u32(at)
        if at not in self.fields:
            if value in (0, 0xffffffff):
                return None
            raise ValueError("unrelocated HSD binding pointer")
        if value == 0:
            raise ValueError("descriptor-at-zero not admitted; payload-at-zero separate")
        self.read(value, 1)
        return value

    def pointer_list(self, at, cap=256):
        if at is None:
            return []
        result = []
        for i in range(cap + 1):
            value = self.pointer(at + i * 4)
            if value is None:
                return result
            if i == cap:
                raise ValueError("unterminated HSD pointer-list budget")
            result.append(value)
        raise ValueError("pointer-list budget")

    def aobj(self, at):
        if at is None:
            return None
        flags = self.u32(at)
        end = struct.unpack(">f", self.read(at + 4, 4))[0]
        if not math.isfinite(end) or not 0 <= end <= 100000:
            raise ValueError("AObj end domain")
        track = self.pointer(at + 8)
        obj = self.pointer(at + 12)
        tracks = []
        seen = set()
        while track is not None:
            if track in seen or len(seen) >= 256:
                raise ValueError("FObj cycle/budget")
            seen.add(track)
            length = self.u32(track + 4)
            start = struct.unpack(">f", self.read(track + 8, 4))[0]
            metadata = self.read(track + 12, 4)
            if not math.isfinite(start) or not 0 <= start <= 100000 or not 0 < length <= 65536 or not metadata[0]:
                raise ValueError("FObj metadata domain")
            # Payload offset0 is REAL when this field is relocated.
            if track + 16 not in self.fields:
                raise ValueError("unrelocated FObj payload")
            payload = self.u32(track + 16)
            raw = self.read(payload, length)
            tracks.append(dict(address=track, channel=metadata[0], frac_value=metadata[1], frac_slope=metadata[2], start=start, payload=payload, bytes=length, sha256=hashlib.sha256(raw).hexdigest()))
            track = self.pointer(track)
        return dict(address=at, flags=flags, end=end, object=obj, tracks=tracks)

    def animation_tree(self, root, material=False):
        work = [(root, 0)]
        nodes = []
        seen = set()
        while work:
            at, depth = work.pop()
            if at in seen or len(seen) >= 4096 or depth >= 64:
                raise ValueError("animation tree topology/budget")
            seen.add(at)
            child, sibling, data = [self.pointer(at + i * 4) for i in range(3)]
            if material:
                items = []
                materials = set()
                while data is not None:
                    if data in materials or len(materials) >= 256:
                        raise ValueError("material animation chain topology/budget")
                    materials.add(data)
                    items.append(dict(address=data, aobj=self.aobj(self.pointer(data + 4)), texture_reference=self.pointer(data + 8), render_reference=self.pointer(data + 12)))
                    data = self.pointer(data)
                node = dict(address=at, depth=depth, materials=items)
            else:
                node = dict(address=at, depth=depth, aobj=self.aobj(data), robj=self.pointer(at + 12), flags=self.u32(at + 16))
            nodes.append(node)
            if sibling is not None:
                work.append((sibling, depth))
            if child is not None:
                work.append((child, depth + 1))
        return nodes


def model_animation_bindings(data, record_ids):
    reader = BindingReader(data)
    scene = [p for p in reader.context["tables"]["publics"] if p["name"] == "scene_data"]
    if len(scene) != 1:
        raise ValueError("unique published scene_data required")
    table = reader.pointer(scene[0]["offset"])
    if table is None:
        raise ValueError("missing model table")
    records = reader.pointer_list(table, 4096)
    if len(record_ids) > 4096 or len(set(record_ids)) != len(record_ids):
        raise ValueError("record selection budget/duplicates")
    result = []
    for index in record_ids:
        if type(index) is not int or not 0 <= index < len(records):
            raise ValueError("record index")
        record = records[index]
        root = reader.pointer(record)
        if root is None:
            raise ValueError("missing model root")
        joint, material, shape = [reader.pointer_list(reader.pointer(record + i * 4)) for i in range(1, 4)]
        result.append(dict(index=index, descriptor=record, model_root=root,
                           joint_clips=[dict(root=p, nodes=reader.animation_tree(p)) for p in joint],
                           material_clips=[dict(root=p, nodes=reader.animation_tree(p, True)) for p in material],
                           shape_references=shape))
    return dict(archive_sha256=hashlib.sha256(data).hexdigest(), published_records=len(records), records=result)
