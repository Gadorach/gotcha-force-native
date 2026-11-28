"""Replay original wall correction on the exact GG4E08 st42 collision grids."""
import argparse
from hashlib import sha256
from io import BytesIO
import json
from pathlib import Path
import struct

from tools.analyze import retail_stage_collision_walk as walk
from tools.extract.pzz import PzzArchive
from tools.extract.retail_borg_roster import RangeFile


s = walk.s
WALK_SHA256 = "8c2b53a3ccb59edfb8bcc192b56613f246dc46196e725ee00cc01830fcb16b8b"
MEMBER = 473
MEMBER_NAME = b"st42.pzz"
MEMBER_SHA256 = "d1f3b702dee565d04524ebb8c4e0a88f89426857e492432d7cce677fc8c857e4"
GRIDS = (
    (215748, "78fc77435e51e0fe4554ff97387d2f99e58d0b8673e0db3954b25d390485fca8", 2799),
    (126948, "4e82466a7718e89c67ab18fbd74ab448e7d04568a5ec5837348162b47796b4b5", 1816),
    (36484, "90f1165db758420457764a61d3576f05531004809f36265672f691acfa48f3d4", 348),
)
# Triangle ordinal, separating axis/sign, room sample for the other axes,
# exact vertex and plane anchors, and source record digest.
SITES = (
    (1549, 0, 1, (0, 202, 1649), "446C2B80", "446C2B88",
     "aa94144ae206fccf6669c3bca979b7d10222a5932638f2d358ef2293455bea6b"),
    (1505, 0, -1, (0, 202, 1557), "449E60A8", "449E60AA",
     "2ce8539123415466d20e329b168fb5b588047f5d716368668289227fcbe73bb3"),
    (1546, 2, 1, (824, 202, 0), "44DE5C48", "44DE5C4A",
     "18cd279751a5b3838ba67ce14d9e1a9aa01c176de835944d4e751a620194a041"),
)
# Name, guest-only empty secondary control, declared distances, anchor, one-step control.
CASES = (
    ("empty-secondary", True, 30, 30, False, 0),
    ("outside", False, 51, 51, False, 0),
    ("stored-plane-tangent", False, 50, 50, False, 0),
    ("front-overlap", False, 30, 30, False, 0),
    ("plane-crossing", False, 80, -10, False, 0),
    ("stationary-behind", False, -10, -10, False, 0),
    ("vertex-tangent", False, 50, 50, True, 0),
    ("vertex-one-step-out", False, 50, 50, True, 1),
    ("vertex-one-step-in", False, 50, 50, True, -1),
)


def f32(value):
    return struct.unpack(">f", struct.pack(">f", value))[0]


def scalar(bits):
    return struct.unpack(">f", bytes.fromhex(bits))[0]


def verify_source(image):
    walk.verify_source(image)
    s.verify_text(image, ((0x8003FB00, 88, "333885e9781b90d779a3a3056c414854cac154f64352ef15cf25aa8fec76b908"),))
    for site, target in ((0x80035EC8, 0x8003FB00), (0x80036128, 0x8020BA78), (0x80036134, 0x8020BB58),
                         (0x80036148, 0x8020BA78), (0x80036154, 0x8020BB58)):
        s.link(image, site, target)


def source_inputs(afs):
    with s.open_regular(afs) as stream:
        stamp = s.archive_stamp(stream)
        s.verify_archive(stream)
        archive = s.RetailArchive(stream)
        if archive.names[MEMBER] != MEMBER_NAME or archive.names.count(MEMBER_NAME) != 1:
            raise ValueError("st42 archive member identity")
        packed = archive.payload(MEMBER, 2 << 20)
        if len(packed) != 663552 or sha256(packed).hexdigest() != MEMBER_SHA256:
            raise ValueError("st42 packed source digest")
        member = PzzArchive(RangeFile(BytesIO(packed), 0, len(packed)))
        if len(member.entries) != 4:
            raise ValueError("st42 source slot count")
        grids = []
        for slot, (size, digest, count) in enumerate(GRIDS, 1):
            data, _ = member.read(slot, 1 << 20)
            if len(data) != size or sha256(data).hexdigest() != digest:
                raise ValueError(f"st42 original grid digest: {slot}")
            magic, length, dx, dz, nx, nz, _, _, _, t = struct.unpack_from(">10I", data)
            if (magic, length, dx, dz, nx, nz, size-t-8) != (0x53544948, size, 500, 500, 42, 42, 56*count):
                raise ValueError(f"st42 original grid extent: {slot}")
            grids.append(data)
        src = walk.stage.melee
        sasuke = src.opening.read_normal(archive, src.opening.RECEIVER, src.opening.RECEIVER_BLOCK_SHA256,
                                         src.ACTOR_SHA256[src.opening.RECEIVER])
        if s.archive_stamp(stream) != stamp:
            raise ValueError("st42 archive changed")
    secondary = grids[1]
    triangles = 8+struct.unpack_from(">I", secondary, 36)[0]
    for ordinal, axis, sign, _, vertex, plane, digest in SITES:
        at = triangles+ordinal*56
        data = secondary[at:at+56]
        values = struct.unpack(">I13f", data)[1:]
        normal = tuple(sign if j == axis else 0 for j in range(3))
        if (sha256(data).hexdigest() != digest or values[9:12] != normal
                or any(values[axis+3*j] != scalar(vertex) for j in range(3))
                or -sign*values[12] != scalar(plane)):
            raise ValueError(f"st42 source wall anchor: {ordinal}")
    hit, _ = sasuke
    stage_sphere = 32+80
    capsule = 32+11*80
    if (hit[stage_sphere:stage_sphere+12] != bytes.fromhex("0080000800000000FFFF0000")
            or struct.unpack_from(">4f", hit, stage_sphere+12) != (0, 50, 0, 50)
            or struct.unpack_from(">7f", hit, capsule+12) != (-12, 0, 0, 15, 0, 0, 7)):
        raise ValueError("st42 fixture body geometry changed")
    return b"".join((*sasuke, *grids))


def decode(report):
    lines = report.splitlines()
    if len(report) > 16384 or len(lines) != 2*len(SITES)*len(CASES):
        raise ValueError("st42 wall report extent")
    bits = walk.stage.pair.capsule.geometry.bits
    pos = 0
    sites = []
    for j, (ordinal, axis, sign, base, vertex, plane, digest) in enumerate(SITES):
        tangent = f32(scalar(vertex)+sign*50)
        cases = []
        for i, (name, empty, begin, end, use_vertex, step) in enumerate(CASES):
            declared = list(base)
            anchor = scalar(vertex if use_vertex else plane)
            declared[axis] = f32(anchor+sign*end)
            if step:
                # These pinned coordinates are all positive and finite.
                declared[axis] = scalar(f"{int(bits(declared[axis]), 16)+sign*step:08X}")
            distance = sign*(declared[axis]-scalar(vertex))
            contact = not empty and (i == 4 or 0 <= distance <= 50)
            root = declared.copy()
            if contact:
                root[axis] = tangent
            correction = [f32(a-b) for a, b in zip(root, declared)]
            sphere = root.copy()
            sphere[1] += 50
            stage_state = (*(bits(x) for x in (*root, *correction, *sphere, *root)), int(contact), int(contact))
            fields = (j, i, int(empty), begin, end, int(use_vertex), step,
                      *(bits(x) for x in declared), *stage_state, *stage_state, 180)
            if lines[pos] != "st42-wall: " + " ".join(map(str, fields)):
                raise ValueError(f"st42 wall geometry changed: {ordinal}/{name}")
            fields = lines[pos+1].split()
            if fields[:3] != ["st42-projection:", str(j), str(i)] or not 4 <= len(fields) <= 12:
                raise ValueError(f"st42 projection report changed: {ordinal}/{name}")
            total, *trace = map(int, fields[3:])
            if (not 0 <= total <= 64 or len(trace) != min(total, 8)
                    or any(not -1 <= n < GRIDS[1][2] for n in trace)
                    or (i in (0, 5) and total)
                    or (i == 4 and (not total or any(n != -1 for n in trace)))
                    or (contact and i != 4 and ordinal not in trace)):
                raise ValueError(f"st42 source projection changed: {ordinal}/{name}")
            pos += 2
            cases.append({
                "case": name, "secondary_lists_empty": empty, "signed_distances_declared": [begin, end],
                "pose_anchor": "triangle-vertex" if use_vertex else "stored-plane", "float_steps_outward": step,
                "pre_walk_root_bits": [bits(x) for x in declared], "post_walk_root_bits": [bits(x) for x in root],
                "post_walk_displacement_bits": [bits(x) for x in correction],
                "role08_center_bits": [bits(x) for x in sphere], "ordinary_capsule_center_bits": [bits(x) for x in root],
                "stage_contact_flag": contact, "owner_correction_flag": contact, "hp_after": 180,
                "projection_function_calls": total,
                "first_eight_projection_operands_from_face_path": [n if n >= 0 else None for n in trace],
            })
        sites.append({
            "reference_triangle_ordinal": ordinal, "reference_triangle_sha256": digest,
            "axis": "xyz"[axis], "outward_sign": sign, "vertex_anchor_bits": vertex,
            "stored_plane_anchor_bits": plane, "vertex_tangent_bits": bits(tangent), "cases": cases,
        })
    return sites


def export(dol, afs, fixture, module, output):
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise FileExistsError("st42 wall output already exists")
    dol = Path(dol).resolve(strict=True)
    image = s.DolImage(s.read_bounded(dol, 8 << 20), s.SOURCE_SHA256)
    verify_source(image)
    s.read_metadata(s.MAPS / "gg4e08-stage-collision-walk.json", WALK_SHA256)
    sites = decode(s.fixture_report(fixture, dol, module, ("--retail-stage",), source_inputs(afs)))
    raw = s.encode({
        "kind": "gf-retail-st42-wall", "revision": 1, "source_dol_sha256": s.SOURCE_SHA256,
        "source_afs_sha256": s.AFS_SHA256, "stage_collision_walk_sha256": WALK_SHA256,
        "afs_member": MEMBER, "afs_member_name": MEMBER_NAME.decode(), "afs_member_sha256": MEMBER_SHA256,
        "stih_members": [{"pzz_slot": i, "bytes": n, "sha256": digest, "triangle_count": count}
                         for i, (n, digest, count) in enumerate(GRIDS, 1)],
        "body": "gg4e08:000a", "stage_sphere_descriptor": 1, "ordinary_capsule_descriptor": 11,
        "grid_relocator": "8003D4A0", "collision_walk": "8002BB14", "stop_before": "8005A580",
        "projection_entry": "80039AEC", "secondary_face_projection_site": "8003618C",
        "sites": sites,
        "notes": [
            "The three original STIH blocks come from unedited AFS member 473. The original relocator edits only guest copies; all triangle bytes and resolved list references are checked. In the empty-secondary controls only the first reference of each secondary list is replaced with a sentinel.",
            "The stage object bounds and Sasuke rigid poses are declared. Only Sasuke's normal role-08 sphere and ordinary capsule publish. The isolated solver and the enclosing walk up to presentation agree on root, vector, flags and broad centers; allocated capsule endpoints and HP are unchanged.",
            "The three targeted sites include positive X, negative X, and positive Z. The reported face projection calls show source triangle ordinals, not a claim that every projection produced a correction. Null in the bounded sample denotes a different call site.",
            "The stored plane anchor does not reproduce vertex tangency. A sphere exactly 50 units from the vertex sets both flags with zero displacement; one positive outward binary32 step misses and one inward step is corrected. Starting at the same final pose behind the wall is distinct from crossing it.",
            "This is original static stage geometry under bounded poses, not a live retail arena tick, dynamic stage construction, drawing, original hardware or native gameplay.",
        ],
    }, expanded=("sites",))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(raw)
    return {"sha256": sha256(raw).hexdigest(), "sites": len(sites),
            "cases": sum(len(site["cases"]) for site in sites)}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("dol", "afs", "fixture", "module", "output"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    print(json.dumps(export(args.dol, args.afs, args.fixture, args.module, args.output), sort_keys=True))
