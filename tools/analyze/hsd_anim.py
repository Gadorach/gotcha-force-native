"""Bounded, read-only identification and evaluation of HSD animation data.

This is the animation companion to ``hsd_scene.py``: that tool classifies
JOBJ/DOBJ/MOBJ/TOBJ/POBJ and their textures; this one finds the *animation*
descriptors that a Gotcha Force HSD archive also contains and evaluates them.

Why a separate tool: HSD keyframes are NOT float arrays.  A track is a packed
byte stream (one nibble-selectable opcode plus quantised values, see below), so
a scan for runs of plausible floats cannot find animation data.  The only
reliable route is structural: find the descriptor records whose pointers reach
those byte streams, exactly the way ``hsd_scene.py`` finds joints.

Field layouts used here (all big-endian, offsets relative to the archive data
block, which starts 0x20 bytes into the container).  These are the HSD
sysdolphin layouts; the class-name pointer is NULL everywhere in this game, so
identification is structural and mutually validated:

  HSD_AObjDesc    (0x10): 0x00 flags:u32, 0x04 end_frame:f32,
                          0x08 fobjdesc*, 0x0C obj_id*
  HSD_FObjDesc    (0x14): 0x00 next*, 0x04 length:u32, 0x08 startframe:f32,
                          0x0C type:u8, 0x0D frac_value:u8,
                          0x0E frac_slope:u8, 0x0F dummy:u8, 0x10 ad*(bytes)
  HSD_AnimJoint   (0x14): 0x00 child*, 0x04 next*, 0x08 aobjdesc*,
                          0x0C robjanim*, 0x10 flags:u32
  HSD_MatAnim     (0x10): 0x00 next*, 0x04 aobjdesc*, 0x08 texanim*,
                          0x0C renderanim*
  HSD_MatAnimJoint(0x0C): 0x00 child*, 0x04 next*, 0x08 matanim*
  HSD_CameraAnim  (0x0C): 0x00 aobjdesc*, 0x04 eye_anim*, 0x08 interest_anim*
  HSD_WObjAnim    (0x08): 0x00 aobjdesc*, 0x04 robjanim*
  HSD_Spline      (0x18): 0x00 type:u8, 0x02 numcv:s16, 0x04 tension:f32,
                          0x08 cv* (numcv + 2*(type!=0) Vec3), 0x0C totalLength:f32,
                          0x10 segLength* (numcv f32), 0x14 segPoly* ((numcv-1)*5 f32)
  HSD_CObjDesc perspective (0x38 used): 0x04 flags:u16, 0x06 projection:u16,
                          0x08 viewport:Rects16, 0x10 scissor, 0x18 eyepos:WObjDesc*,
                          0x1C interest:WObjDesc*, 0x20 roll:f32, 0x24 up:Vec3*,
                          0x28 near:f32, 0x2C far:f32, 0x30 fov:f32, 0x34 aspect:f32
  HSD_WObjDesc    (0x14): 0x00 class*, 0x04 position:Vec3, 0x10 robjdesc*

Track encoding (HSD_FObjData, the packed byte stream pointed at by ``ad``):
one byte holds the opcode in its low nibble and the pack count in its high
nibble (bit 7 extends the count with continuation bytes).  The opcode is one of
CON(1), LIN(2), SPL0(3), SPL(4), SLP(5), KEY(6).  Each pack then contributes a
value encoded with ``frac_value`` (0 = 32-bit float, else a signed/unsigned
8/16-bit numerator divided by 2**low5) and, for SPL/SLP, a slope encoded with
``frac_slope``; after each value follows a variable-length wait (7 bits per
byte, continuation in bit 7).  Values and waits are little-endian.  This tool
implements that state machine faithfully (see ``FObj``) and was validated by
decoding the material tracks and confirming they reproduce the authored
material colours byte-for-byte (e.g. ambient 0x80/255 = 0.50196, diffuse
(102,76,0) = (0.4, 0.298, 0.0)).

Evaluation: ``--frame N`` means "after N calls to HSD_AObjInterpretAnim with
rate 1", which is how the runtime advances one frame at a time (the first call
makes the internal time 1, not 0).  Interpolation is the runtime's own: CON is
a hold-then-jump, LIN is linear, SPL/SPL0/SLP are Hermite (``splGetHelmite``),
KEY is a step.  Nothing here is re-derived from screenshots or guessed.

What this tool deliberately does NOT do: TEV/material rendering, skinning,
JOBJ instance handling, or deciding which menu state the game chooses.  A
discovered track list is not a claim that the game plays that animation.

Known scope limit: discovery walks the relocation-target list, so a structure
that nothing points at is invisible even if it is laid out correctly (this is
why an archive may report fewer material animations than it contains).  In
``tl00_mdl.arc`` the scan is complete: 69 AObjDesc = 34 joint + 32 material +
3 camera, with zero unattributed, and the decoded material colours reproduce
31/32 of the archive's own material records.

Outputs are refused outside ``local/``.  Every pointer read goes through the
relocation table; every array and stream read is bounds-checked and capped.
"""
import argparse
import bisect
import hashlib
import json
import math
import struct
import sys
from pathlib import Path

sys.dont_write_bytecode = True   # keep the tracked tree free of __pycache__
sys.path.insert(0, str(Path(__file__).resolve().parent))
from hsd_scene import (  # noqa: E402  (reuse the validated structural layer)
    as_jobj, classify, load_container, Reader,
)

ROOT = Path(__file__).resolve().parents[2]

MAX_INPUT_BYTES = 64 * 1024 * 1024
MAX_AOBJ = 4096
MAX_ANIM_NODES = 4096
MAX_FOBJ_BYTES = 0x10000
MAX_TRACK_STEPS = 100000          # per sampled track (frame <= 100000 by cap)
MAX_SPLINE_POINTS = 4096
MAX_DEPTH = 64

JOBJ_BILLBOARD_MASK = 0xF00

# HSD_A_J_* channel ids (shared by joints and WObjs; WObj only uses 4..7).
JOINT_CHANNELS = {
    1: "rotx", 2: "roty", 3: "rotz", 4: "path",
    5: "trax", 6: "tray", 7: "traz",
    8: "scax", 9: "scay", 10: "scaz", 11: "node", 12: "branch",
}
MATERIAL_CHANNELS = {
    1: "ambient_r", 2: "ambient_g", 3: "ambient_b",
    4: "diffuse_r", 5: "diffuse_g", 6: "diffuse_b",
    7: "specular_r", 8: "specular_g", 9: "specular_b", 10: "alpha",
    11: "pe_ref0", 12: "pe_ref1", 13: "pe_dstalpha",
}
CAMERA_CHANNELS = {
    1: "eye_x", 2: "eye_y", 3: "eye_z", 4: "eye_path",
    5: "interest_x", 6: "interest_y", 7: "interest_z",
    9: "roll", 10: "fov", 11: "near", 12: "far",
}
FOBJ_OPS = {0: "NONE", 1: "CON", 2: "LIN", 3: "SPL0", 4: "SPL", 5: "SLP", 6: "KEY"}


# --------------------------------------------------------------------------
# field validation helpers
# --------------------------------------------------------------------------
def _is_fobjdesc(reader, off):
    """A track descriptor: pointer fields relocated, length ad fits in the block."""
    if not reader.has(off, 0x14):
        return False
    nxt = reader.ptr(off)
    length = reader.u32(off + 4)
    start = reader.f32(off + 8)
    kind = reader.u8(off + 0xC)
    data = reader.ptr(off + 0x10)
    if nxt is None or data is None or length is None or kind is None:
        return False
    if length > MAX_FOBJ_BYTES or kind == 0:
        return False
    if start is None or not (0.0 <= start <= 1.0e5):
        return False
    if data in (0, 0xFFFFFFFF):
        return False
    return reader.has(data, min(length, MAX_FOBJ_BYTES))


def fobjdesc_chain(reader, off, cap=MAX_ANIM_NODES):
    """Follow an HSD_FObjDesc sibling chain; every node must validate."""
    chain = []
    seen = set()
    node = off
    while node and node not in seen and len(chain) < cap:
        if not _is_fobjdesc(reader, node):
            return None
        seen.add(node)
        chain.append({
            "offs": node,
            "length": reader.u32(node + 4),
            "startframe": round(reader.f32(node + 8), 6),
            "type": reader.u8(node + 0xC),
            "frac_value": reader.u8(node + 0xD),
            "frac_slope": reader.u8(node + 0xE),
            "data": reader.ptr(node + 0x10),
            "next": reader.ptr(node),
        })
        nxt = reader.ptr(node)
        node = nxt if nxt not in (0, None) else 0
    if node and node in seen:
        return None                      # cycle: refuse rather than loop
    return chain


def is_aobjdesc(reader, off):
    """HSD_AObjDesc: flags, finite end_frame, a valid track chain, an obj key."""
    if not reader.has(off, 0x10):
        return False
    flags = reader.u32(off)
    end = reader.f32(off + 4)
    tracks = reader.ptr(off + 8)
    obj = reader.ptr(off + 0xC)
    if flags is None or flags > 0xFFFFF:
        return False
    if end is None or not (0.0 < end <= 1.0e6) or end != end:
        return False
    if tracks in (None, 0, 0xFFFFFFFF) or obj is None:
        return False
    return fobjdesc_chain(reader, tracks) is not None


def is_animjoint(reader, off, aobjs):
    """A 0x14-byte HSD_AnimJoint node (aobjdesc may be NULL for pass-throughs)."""
    if not reader.has(off, 0x14):
        return False
    child = reader.ptr(off)
    nxt = reader.ptr(off + 4)
    aobj = reader.ptr(off + 8)
    robj = reader.ptr(off + 0xC)
    flags = reader.u32(off + 0x10)
    if None in (child, nxt, aobj, robj, flags):
        return False
    if flags > 0xFFFF:
        return False
    if aobj not in (0, 0xFFFFFFFF) and aobj not in aobjs:
        return False
    # A node must not also look like a joint/model structure: those layouts do
    # contain pointer-shaped words and would otherwise be false positives.
    return not classify(reader, off)


def is_camera_anim(reader, off, aobjs):
    """HSD_CameraAnim: {aobjdesc, eye WObjAnim, interest WObjAnim}."""
    if not reader.has(off, 0xC):
        return False
    if reader.ptr(off) not in aobjs:
        return False
    for field in (4, 8):
        wobj = reader.ptr(off + field)
        if wobj in (None, 0, 0xFFFFFFFF):
            return False
        if reader.ptr(wobj) not in aobjs:
            return False
        robj = reader.ptr(wobj + 4)
        if robj is None:
            return False
    return True


def read_spline(reader, off):
    """Validate an HSD_Spline and its three arrays against the data block."""
    if not reader.has(off, 0x18):
        return None
    kind = reader.u8(off)
    numcv = reader.u16(off + 2)
    tension = reader.f32(off + 4)
    cv = reader.ptr(off + 8)
    total = reader.f32(off + 0xC)
    seglen = reader.ptr(off + 0x10)
    segpoly = reader.ptr(off + 0x14)
    if kind not in (0, 1, 2, 3) or not numcv or numcv > MAX_SPLINE_POINTS:
        return None
    if None in (tension, total) or not (math.isfinite(tension) and math.isfinite(total)):
        return None
    if cv in (None, 0) or seglen in (None, 0) or segpoly in (None, 0):
        return None
    extra = 2 if kind != 0 else 0
    if not reader.has(cv, (numcv + extra) * 12):
        return None
    if not reader.has(seglen, numcv * 4):
        return None
    if numcv > 1 and not reader.has(segpoly, (numcv - 1) * 20):
        return None
    # numcv is s16 in the reference; the big-endian u16 read above is only used
    # after the range check, and a negative value would fail it.
    return {"offs": off, "type": kind, "numcv": numcv,
            "tension": round(tension, 6), "cv": cv, "total_length": round(total, 6),
            "seg_length": seglen, "seg_poly": segpoly}


# --------------------------------------------------------------------------
# packed track evaluation (HSD_FObj state machine)
# --------------------------------------------------------------------------
def _parse_float(data, pos, frac):
    if frac == 0:
        return struct.unpack_from("<f", data, pos)[0], pos + 4
    denom = float(1 << (frac & 0x1F))
    kind = frac & 0xE0
    if kind == 0x60:                       # S8
        return struct.unpack_from("<b", data, pos)[0] / denom, pos + 1
    if kind == 0x80:                       # U8
        return data[pos] / denom, pos + 1
    if kind == 0x20:                       # S16
        return struct.unpack_from("<h", data, pos)[0] / denom, pos + 2
    if kind == 0x40:                       # U16
        return struct.unpack_from("<H", data, pos)[0] / denom, pos + 2
    raise ValueError("unsupported frac_value %#x" % frac)


def _parse_wait(data, pos):
    wait = 0
    shift = 0
    while True:
        byte = data[pos]
        pos += 1
        wait |= (byte & 0x7F) << shift
        shift += 7
        if not byte & 0x80:
            return wait, pos


def _parse_pack(data, pos):
    byte = data[pos]
    pos += 1
    count = ((byte >> 4) & 7) + 1
    shift = 3
    if byte & 0x80:
        while True:
            ext = data[pos]
            pos += 1
            count += (ext & 0x7F) << shift
            shift += 7
            if not ext & 0x80:
                break
    return count, pos


def spl_helmite(fterm, time, p0, p1, d0, d1):
    """Verbatim transcription of HSD's splGetHelmite."""
    t = time
    term = fterm
    t2 = t * t
    n2 = term * term
    t2n = t2 * term
    t3n2 = n2 * (t2 * t)
    two = 2.0 * t3n2 * term
    three = 3.0 * t2 * n2
    return (d1 * (t3n2 - t2n)) + \
           ((d0 * (t + ((t3n2 - t2n) - t2n))) +
            ((p0 * (1.0 + (two - three))) + (p1 * (-two + three))))


class FObj:
    """One HSD_FObj track, replayed exactly as HSD_FObjInterpretAnim does."""

    def __init__(self, reader, desc):
        self.length = desc["length"]
        self.ad = desc["data"]
        self.frac_value = desc["frac_value"]
        self.frac_slope = desc["frac_slope"]
        self.startframe = desc["startframe"]
        self.data = bytes(reader.block[self.ad:self.ad + self.length])
        if len(self.data) != self.length:
            raise ValueError("track data outside the data block")
        self.reset()

    def reset(self):
        self.pos = 0
        self.time = float(self.startframe)
        self.op = 0
        self.op_intrp = 0
        self.nb_pack = 0
        self.p0 = self.p1 = self.d0 = self.d1 = 0.0
        self.fterm = 0
        self.flags = 0
        self.state = 1
        self.value = 0.0
        self.finished = False
        self.float_time = False
        self.ops = []

    # -- loader states ----------------------------------------------------
    def _load_data(self):
        if self.pos >= self.length:
            return 6
        self.op_intrp = self.op
        if self.nb_pack == 0:
            # One byte carries opcode (low nibble) *and* pack count (high
            # nibble); parsePackInfo consumes that same byte.
            byte = self.data[self.pos]
            self.op = byte & 0xF
            self.nb_pack, self.pos = _parse_pack(self.data, self.pos)
        self.nb_pack -= 1
        op = self.op
        if op in (1, 2):                     # CON / LIN
            self.p0 = self.p1
            self.p1, self.pos = _parse_float(self.data, self.pos, self.frac_value)
            if self.op_intrp != 5:
                self.d0, self.d1 = self.d1, 0.0
        elif op == 3:                        # SPL0
            self.p0 = self.p1
            self.d0 = self.d1
            self.p1, self.pos = _parse_float(self.data, self.pos, self.frac_value)
            self.d1 = 0.0
        elif op == 4:                        # SPL
            self.p0 = self.p1
            self.p1, self.pos = _parse_float(self.data, self.pos, self.frac_value)
            self.d0 = self.d1
            self.d1, self.pos = _parse_float(self.data, self.pos, self.frac_slope)
        elif op == 5:                        # SLP
            self.d0 = self.d1
            self.d1, self.pos = _parse_float(self.data, self.pos, self.frac_slope)
            # Original802489F4: slope-only keys keep the loader state and
            # consume no wait; the next byte is another opcode/pack.
            return self.state
        elif op == 6:                        # KEY (step, launched on the next key)
            if self.flags & 0x40:
                self.op_intrp = self.op
                self.flags &= ~0x40
                self.flags |= 0x80
                self.p0 = self.p1
            self.p1, self.pos = _parse_float(self.data, self.pos, self.frac_value)
            self.flags |= 0x40
        else:
            return 0
        self.ops.append(op)
        return 3 if self.state == 1 else 4

    def _load_wait(self):
        if self.pos >= self.length:
            return 6
        self.fterm, self.pos = _parse_wait(self.data, self.pos)
        self.flags |= 0x20
        return 2

    # -- interpolation ----------------------------------------------------
    def _update(self):
        if self.op_intrp == 6:               # KEY
            if self.flags & 0x80:
                self.value = self.p0
                self.flags &= ~0x80
                return True
            return False
        if self.op_intrp == 1:               # CON
            self.value = self.p1 if self.time >= self.fterm else self.p0
            return True
        if self.op_intrp == 2:               # LIN
            if self.flags & 0x20:
                self.flags &= ~0x20
                if self.fterm != 0:
                    self.d0 = (self.p1 - self.p0) / self.fterm
                else:
                    self.d0 = 0.0
                    self.p0 = self.p1
            self.value = self.d0 * self.time + self.p0
            return True
        if self.op_intrp in (3, 4, 5):       # SPL0 / SPL / SLP
            if self.fterm != 0:
                self.value = spl_helmite(1.0 / self.fterm, self.time,
                                         self.p0, self.p1, self.d0, self.d1)
            else:
                self.value = self.p1
            return True
        return False

    def step(self, rate=1.0):
        """Interpret one call; explicit FIRST_PLAY requests supply rate zero."""
        self.time += rate
        if self.float_time:self.time=struct.unpack('<f',struct.pack('<f',self.time))[0]
        if self.time < 0.0:
            return False
        consumed_wait = 0.0  # call-local f31 in original80249AC4
        state = self.state
        for _ in range(MAX_TRACK_STEPS):
            if state == 6:                   # EOF return does NOT store state6
                self.time += consumed_wait
                if self.float_time:self.time=struct.unpack('<f',struct.pack('<f',self.time))[0]
                if self.flags & 0x40:
                    self.op_intrp = self.op
                    self.flags &= ~0x40
                    self.flags |= 0x80
                    self.p0 = self.p1
                self.finished = True
                return self._update()
            if state in (1, 2):
                nxt = self._load_data()
                if nxt == 6:
                    state = 6
                    continue
                if nxt == 0:
                    self.finished = True
                    return False
                state = self.state = nxt
                continue
            if state == 3:
                if self.flags & 0x80:
                    self._update()
                nxt = self._load_wait()
                if nxt == 6:
                    state = 6
                    continue
                state = self.state = nxt
                continue
            if state == 4:
                if self.fterm <= self.time:
                    consumed_wait = self.fterm
                    self.time -= self.fterm
                    if self.float_time:self.time=struct.unpack('<f',struct.pack('<f',self.time))[0]
                    state = self.state = 3
                    continue
                ok = self._update()
                self.state = 5
                return ok
            if state == 5:
                state = self.state = 4
                continue
            return False
        raise ValueError("track state machine did not settle")


def sample_track(reader, desc, frame):
    """Value of one track after ``frame`` interpret calls (frame 0 = no update)."""
    if frame > MAX_TRACK_STEPS:
        raise ValueError("frame exceeds the offline evaluation cap")
    track = FObj(reader, desc)
    for _ in range(frame):
        track.step()
    return track


def track_profile(reader, desc, frames):
    """Records a track's op sequence and its value at each requested frame."""
    profile = {"type": desc["type"], "length": desc["length"],
               "startframe": desc["startframe"], "data": desc["data"],
               "channel": JOINT_CHANNELS.get(desc["type"], "?")}
    track = FObj(reader, desc)
    samples = []
    for frame in range(0, max(frames) + 1):
        if frame:
            try:
                track.step()
            except ValueError:
                profile["error"] = "track stream ran out of bounds"
                break
        if frame in frames:
            samples.append([frame, round(track.value, 6)])
    profile["samples"] = samples
    profile["ops"] = [FOBJ_OPS.get(op, op) for op in sorted(set(track.ops))]
    return profile


# --------------------------------------------------------------------------
# spline evaluation (HSD spline.c)
# --------------------------------------------------------------------------
class Spline:
    def __init__(self, reader, info):
        self.r = reader
        self.info = info
        self.type = info["type"]
        self.numcv = info["numcv"]
        self.tension = info["tension"]
        self.cv = info["cv"]
        self.total = info["total_length"]
        self.seglen = info["seg_length"]
        self.segpoly = info["seg_poly"]

    def point_at(self, index):
        off = self.cv + 12 * index
        return (self.r.f32(off), self.r.f32(off + 4), self.r.f32(off + 8))

    @staticmethod
    def _bezier(cp, u):
        u1 = 1.0 - u
        b0 = u1 * u1 * u1
        b1 = 3.0 * u * u1 * u1
        b2 = 3.0 * u * u * u1
        b3 = u * u * u
        return tuple(cp[0][i] * b0 + cp[1][i] * b1 + cp[2][i] * b2 + cp[3][i] * b3
                     for i in range(3))

    @staticmethod
    def _bspline(cp, u):
        u2 = u * u
        u3 = u2 * u
        u1 = 1.0 - u
        k = 1.0 / 6.0
        b0 = k * u1 * u1 * u1
        b1 = k * (4.0 + (3.0 * u3 - 6.0 * u2))
        b2 = k * (3.0 * (-u3 + u2 + u) + 1.0)
        b3 = k * u3
        return tuple(cp[0][i] * b0 + cp[1][i] * b1 + cp[2][i] * b2 + cp[3][i] * b3
                     for i in range(3))

    def _cardinal(self, cp, u):
        u2 = u * u
        u3 = u2 * u
        t = self.tension
        c0 = t * (-u3 + 2.0 * u2 - u)
        c1 = (2.0 - t) * u3 + (t - 3.0) * u2 + 1.0
        c2 = (t - 2.0) * u3 + (3.0 - 2.0 * t) * u2 + t * u
        c3 = t * (u3 - u2)
        return tuple(cp[0][i] * c0 + cp[1][i] * c1 + cp[2][i] * c2 + cp[3][i] * c3
                     for i in range(3))

    def point(self, u):
        """splGetSplinePoint: parameter u in [0,1] over the control points."""
        if u < 0.0 or u > 1.0:
            return None
        if u < 1.0:
            t = u * (self.numcv - 1)
            idx = int(t)
            frac = t - idx
            if self.type == 0:
                a, b = self.point_at(idx), self.point_at(idx + 1)
                return tuple(a[i] + frac * (b[i] - a[i]) for i in range(3))
            if self.type == 1:
                return self._bezier([self.point_at(idx * 3 + k) for k in range(4)], frac)
            if self.type == 2:
                return self._bspline([self.point_at(idx + k) for k in range(4)], frac)
            return self._cardinal([self.point_at(idx + k) for k in range(4)], frac)
        idx = self.numcv - 1
        if self.type == 0:
            return self.point_at(idx)
        if self.type == 1:
            return self.point_at(idx * 3)
        if self.type == 2:
            return self._bspline([self.point_at(idx - 1 + k) for k in range(4)], 1.0)
        return self.point_at(idx + 1)

    def _poly(self, coeffs, t):
        t2 = t * t
        t3 = t2 * t
        t4 = t3 * t
        value = coeffs[0] * t4 + coeffs[1] * t3 + coeffs[2] * t2 + coeffs[3] * t + coeffs[4]
        if -0.001 < value < 0.0:
            value = 0.0
        return math.sqrt(value) if value > 0.0 else 0.0

    def _coeffs(self, index):
        off = self.segpoly + index * 20
        return [self.r.f32(off + 4 * k) for k in range(5)]

    def arc_parameter(self, arg):
        """splArcLengthGetParameter: normalised arc length -> curve parameter."""
        if arg <= 0.0:
            return 0.0
        if arg >= 1.0:
            return 1.0
        idx = 0
        while idx + 1 < self.numcv and self.r.f32(self.seglen + 4 * (idx + 1)) < arg:
            idx += 1
        if self.type == 0:
            a = self.r.f32(self.seglen + 4 * idx)
            b = self.r.f32(self.seglen + 4 * (idx + 1))
            result = (arg - a) / (b - a) if b != a else 0.0
        else:
            remaining = self.total * (arg - self.r.f32(self.seglen + 4 * idx))
            start, end, result = 0.0, 1.0, 0.0
            for _ in range(200):
                if abs(start - end) < 1e-5:
                    break
                result = (start + end) / 2.0
                dx = (result - start) / 8.0
                middle = 0.0
                t = start + dx
                for step in range(2, 9):
                    middle += (4.0 if step % 2 == 0 else 2.0) * self._poly(self._coeffs(idx), t)
                    t += dx
                simpson = dx * (middle + self._poly(self._coeffs(idx), start) +
                                self._poly(self._coeffs(idx), result)) / 3.0
                if remaining < 1e-5 + simpson:
                    end = result
                else:
                    start = result
                    remaining -= simpson
        return (result + idx) / (self.numcv - 1.0)

    def arc_point(self, arg):
        return self.point(self.arc_parameter(arg))


# --------------------------------------------------------------------------
# discovery
# --------------------------------------------------------------------------
def discover(reader):
    targets = sorted(reader.targets)
    ordered = targets

    def span(off):
        i = bisect.bisect_left(ordered, off)
        return (ordered[i + 1] if i + 1 < len(ordered) else reader.size) - off

    aobjs = sorted({t for t in targets if is_aobjdesc(reader, t)})
    aobj_set = set(aobjs)
    if len(aobjs) > MAX_AOBJ:
        raise ValueError("AObjDesc count exceeds the offline cap")

    # HSD_AnimJoint is 0x14 bytes.  The relocation table gives no struct sizes,
    # so a node is accepted only when the next relocation target is exactly
    # 0x14 bytes away (the same span heuristic hsd_scene.py uses) AND its five
    # words validate.  Every tree is then re-validated by the JOBJ shape match.
    nodes = {}
    for off in targets:
        if span(off) != 0x14:
            continue
        if is_animjoint(reader, off, aobj_set):
            nodes[off] = {
                "child": reader.ptr(off), "next": reader.ptr(off + 4),
                "aobj": reader.ptr(off + 8), "robjanim": reader.ptr(off + 0xC),
                "flags": reader.u32(off + 0x10),
            }
    if len(nodes) > MAX_ANIM_NODES:
        raise ValueError("AnimJoint count exceeds the offline cap")

    referenced = set()
    for node in nodes.values():
        for key in ("child", "next"):
            if node[key]:
                referenced.add(node[key])
    roots = [t for t in sorted(nodes) if t not in referenced]

    trees = []
    for root in roots:
        order = []

        def walk(off, depth, parent):
            seen = set()
            while off and off not in seen and off in nodes and depth < MAX_DEPTH:
                seen.add(off)
                node = nodes[off]
                order.append({"offs": off, "depth": depth, "parent": parent,
                              "node": node})
                if node["child"]:
                    walk(node["child"], depth + 1, off)
                off = node["next"]

        walk(root, 0, None)
        if not any(entry["node"]["aobj"] for entry in order):
            continue                      # pass-through fragments, not a tree
        trees.append({"root": root, "nodes": order})

    joint_aobjs = {entry["node"]["aobj"] for tree in trees for entry in tree["nodes"]
                   if entry["node"]["aobj"]}

    # Material animations: HSD_MatAnim {next, aobjdesc, texanim, renderanim}.
    matanims = []
    for off in targets:
        if not reader.has(off, 0x10):
            continue
        if reader.ptr(off + 4) not in aobj_set:
            continue
        nxt, texanim, renderanim = reader.ptr(off), reader.ptr(off + 8), reader.ptr(off + 0xC)
        if None in (nxt, texanim, renderanim):
            continue
        if texanim not in (0, 0xFFFFFFFF) or renderanim not in (0, 0xFFFFFFFF):
            continue
        matanims.append({"offs": off, "aobj": reader.ptr(off + 4), "next": nxt})

    mat_aobjs = {m["aobj"] for m in matanims}

    # Camera animation: a HSD_CameraAnim plus the {desc, anim} entry that owns it.
    # Two layouts occur: desc+4 points straight at the CameraAnim, or (as here)
    # desc+4 points at a small {camera_anim, nul} wrapper.  Both are accepted and
    # the descriptor is validated independently as a perspective/frustum/ortho
    # camera before anything is reported.
    cameras = []

    def valid_desc(d):
        if d in (None, 0) or not reader.has(d, 0x38):
            return None
        projection = reader.u16(d + 6)
        if projection not in (1, 2, 3):
            return None
        eyepos = reader.ptr(d + 0x18)
        interest = reader.ptr(d + 0x1C)
        if eyepos in (None, 0) or interest in (None, 0):
            return None
        if not reader.has(eyepos, 0x14) or not reader.has(interest, 0x14):
            return None
        near, far, fov, aspect, roll = (reader.f32(d + 0x28), reader.f32(d + 0x2C),
                                        reader.f32(d + 0x30), reader.f32(d + 0x34),
                                        reader.f32(d + 0x20))
        if None in (near, far, fov, aspect, roll) or not (0 < near < far):
            return None
        if not (0.0 < aspect <= 16.0) or not (0.0 < fov < 180.0):
            return None
        return {"desc": d, "projection": projection, "eyepos": eyepos,
                "interest": interest, "near": near, "far": far, "fov": fov,
                "aspect": aspect, "roll": roll}

    def canim_of(ptr):
        if ptr in (None, 0):
            return None
        if is_camera_anim(reader, ptr, aobj_set):
            return ptr
        inner = reader.ptr(ptr)
        if inner is not None and is_camera_anim(reader, inner, aobj_set):
            return inner
        return None

    seen_cameras = set()
    for entry in targets:
        for first, second in ((reader.ptr(entry), reader.ptr(entry + 4)),
                              (reader.ptr(entry + 4), reader.ptr(entry))):
            desc = valid_desc(first)
            if desc is None:
                continue
            canim = canim_of(second)
            if canim is None or canim in seen_cameras:
                continue
            seen_cameras.add(canim)
            cameras.append({
                "anim": canim, "entry": entry, "desc": desc["desc"],
                "aobj": reader.ptr(canim),
                "eye_anim": reader.ptr(canim + 4),
                "interest_anim": reader.ptr(canim + 8),
                "desc_eye": (reader.f32(desc["eyepos"] + 4),
                             reader.f32(desc["eyepos"] + 8),
                             reader.f32(desc["eyepos"] + 0xC)),
                "desc_interest": (reader.f32(desc["interest"] + 4),
                                  reader.f32(desc["interest"] + 8),
                                  reader.f32(desc["interest"] + 0xC)),
                "projection": desc["projection"], "near": desc["near"],
                "far": desc["far"], "fov": desc["fov"],
                "aspect": desc["aspect"], "roll": desc["roll"],
            })

    return {
        "aobjs": aobjs,
        "joint_aobjs": joint_aobjs,
        "mat_aobjs": mat_aobjs,
        "anim_trees": trees,
        "matanims": matanims,
        "cameras": cameras,
        "anim_nodes": nodes,
    }


# --------------------------------------------------------------------------
# joint tree + mapping
# --------------------------------------------------------------------------
def jobj_forest(reader, targets):
    jobjs = {t for t in targets if classify(reader, t).get("jobj")}
    linked = set()
    for off in jobjs:
        data = as_jobj(reader, off)
        for pointer in (data["first_child"], data["next_sibling"]):
            if pointer:
                linked.add(pointer)
    roots = sorted(jobjs - linked)
    order = []

    def walk(off, parent, depth):
        seen = set()
        while off and off not in seen and off in jobjs and depth < MAX_DEPTH:
            seen.add(off)
            data = as_jobj(reader, off)
            order.append({"offs": off, "parent": parent, "depth": depth, "jobj": data})
            if data["first_child"]:
                walk(data["first_child"], off, depth + 1)
            off = data["next_sibling"]

    for root in roots:
        walk(root, None, 0)
    return order, roots


def _shape(nodes, index):
    children = []
    i = index + 1
    while i < len(nodes) and nodes[i]["depth"] == nodes[index]["depth"] + 1:
        sub, i = _shape(nodes, i)
        children.append(sub)
    return tuple(children), i


def _channels(reader, tree):
    channels = set()
    for entry in tree["nodes"]:
        aobj = entry["node"]["aobj"]
        if not aobj:
            continue
        for desc in fobjdesc_chain(reader, reader.ptr(aobj + 8)) or []:
            channels.add(desc["type"])
    return channels


def _initial_scale_matches(reader, tree, jobj_nodes):
    """Count nodes whose first scale-X key equals the authored scale (tie-break)."""
    score = 0
    for entry, jobj in zip(tree["nodes"], jobj_nodes):
        aobj = entry["node"]["aobj"]
        if not aobj or "jobj" not in jobj:
            continue
        for desc in fobjdesc_chain(reader, reader.ptr(aobj + 8)) or []:
            if desc["type"] != 8:
                continue
            try:
                value = sample_track(reader, desc, 1).value
            except ValueError:
                continue
            if abs(value - jobj["jobj"]["scale"][0]) < 1.0e-4:
                score += 1
    return score


def jobj_subtree(reader, root):
    """The JOBJ subtree rooted at ``root`` in preorder (siblings included)."""
    out = []

    def walk(off, depth):
        seen = set()
        while off and off not in seen and depth < MAX_DEPTH:
            seen.add(off)
            data = as_jobj(reader, off)
            if data is None:
                return
            out.append({"offs": off, "depth": depth, "jobj": data})
            if data["first_child"]:
                walk(data["first_child"], depth + 1)
            off = data["next_sibling"]

    walk(root, 0)
    return out


def map_trees(reader, trees, jobj_order):
    """Match each AnimJoint tree to the JOBJ root whose subtree it mirrors.

    HSD loads animation joints positionally: anim.child maps to jobj.child and
    anim.next to jobj.next, so the tree *shape* must be identical.  When two
    roots share a shape, the deciding evidence is content, in this order:
    a NODE (visibility) track implies children authored hidden, a ROTY track
    implies children authored rotated in Y, and otherwise the first scale keys
    must equal the authored scales.
    """
    # HSD_JObjAddAnim can be called on any joint, not only a scene root, so the
    # candidate set is every joint whose (sibling-inclusive) subtree shape
    # matches the animation tree.  Picking the root joint is a shape match; the
    # content checks below decide between equal shapes.
    subtrees = {off: jobj_subtree(reader, off) for off in (n["offs"] for n in jobj_order)}

    result = []
    for tree in trees:
        tree_shape = _shape(tree["nodes"], 0)[0]
        candidates = [root for root, nodes in subtrees.items()
                      if len(nodes) == len(tree["nodes"]) and
                      _shape(nodes, 0)[0] == tree_shape]
        channels = _channels(reader, tree)
        scored = []
        for root in candidates:
            nodes = subtrees[root]
            score = 0
            if 11 in channels:
                score += sum(1 for n in nodes[1:] if n["jobj"]["hidden"])
            if 2 in channels:
                score += sum(1 for n in nodes[1:] if abs(n["jobj"]["rotation"][1]) > 1e-6)
            scored.append([score, root])
        if len(scored) > 1:
            best = max(score for score, _ in scored)
            tied = [item for item in scored if item[0] == best]
            if len(tied) > 1:
                for item in tied:
                    item[0] += _initial_scale_matches(reader, tree, subtrees[item[1]])
            scored = tied if len(tied) > 1 else scored
        scored.sort(key=lambda item: (-item[0], item[1]))
        root = scored[0][1] if scored else None
        node_map = {}
        if root is not None:
            for entry, jobj in zip(tree["nodes"], subtrees[root]):
                node_map[entry["offs"]] = jobj["offs"]
        result.append({
            "anim_root": tree["root"],
            "joint_root": root,
            "nodes": len(tree["nodes"]),
            "channels": sorted(channels),
            "candidates": [{"joint_root": r, "score": s} for s, r in scored],
            "unique": len(candidates) == 1,
            "node_map": node_map,
            "anim_nodes": tree["nodes"],
        })
    return result


# --------------------------------------------------------------------------
# pose + world matrices
# --------------------------------------------------------------------------
def identity():
    return [[1.0 if r == c else 0.0 for c in range(4)] for r in range(4)]


def mat_mul(a, b):
    return [[sum(a[r][k] * b[k][c] for k in range(4)) for c in range(4)] for r in range(4)]


def mat_srt(rot, scale, trans):
    cx, sx = math.cos(rot[0]), math.sin(rot[0])
    cy, sy = math.cos(rot[1]), math.sin(rot[1])
    cz, sz = math.cos(rot[2]), math.sin(rot[2])
    rx = [[1, 0, 0, 0], [0, cx, -sx, 0], [0, sx, cx, 0], [0, 0, 0, 1]]
    ry = [[cy, 0, sy, 0], [0, 1, 0, 0], [-sy, 0, cy, 0], [0, 0, 0, 1]]
    rz = [[cz, -sz, 0, 0], [sz, cz, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]]
    sc = [[scale[0], 0, 0, 0], [0, scale[1], 0, 0], [0, 0, scale[2], 0], [0, 0, 0, 1]]
    tr = [[1, 0, 0, trans[0]], [0, 1, 0, trans[1]], [0, 0, 1, trans[2]], [0, 0, 0, 1]]
    # Same convention as engine/graphics/transform.hpp: T * Rz * Ry * Rx * S.
    return mat_mul(tr, mat_mul(rz, mat_mul(ry, mat_mul(rx, sc))))


def pose_at(reader, tree_maps, jobj_order, frame):
    """Joint pose with the mapped animation tracks applied at ``frame``."""
    pose = {}
    for mapping in tree_maps:
        for anim_off, joint_off in mapping["node_map"].items():
            entry = next(n for n in jobj_order if n["offs"] == joint_off)
            data = entry["jobj"]
            state = {
                "rotation": list(data["rotation"]),
                "scale": list(data["scale"]),
                "translation": list(data["translation"]),
                "hidden": data["hidden"],
                "animated": [],
            }
            node = next(n for n in mapping["anim_nodes"] if n["offs"] == anim_off)["node"]
            aobj = node["aobj"]
            if aobj:
                for desc in fobjdesc_chain(reader, reader.ptr(aobj + 8)) or []:
                    try:
                        track = sample_track(reader, desc, frame)
                    except ValueError:
                        continue
                    value = track.value
                    kind = desc["type"]
                    if kind in (1, 2, 3):
                        state["rotation"][kind - 1] = value
                    elif kind in (8, 9, 10):
                        state["scale"][kind - 8] = value
                    elif kind in (5, 6, 7):
                        state["translation"][kind - 5] = value
                    elif kind == 11:
                        state["hidden"] = value <= 0.5
                    state["animated"].append(kind)
            pose[joint_off] = state
    return pose


def world_matrices(reader, jobj_order, pose):
    world = {}
    for entry in jobj_order:
        off = entry["offs"]
        state = pose.get(off)
        if state is None:
            data = entry["jobj"]
            state = {"rotation": data["rotation"], "scale": data["scale"],
                     "translation": data["translation"], "hidden": data["hidden"]}
        local = mat_srt([float(v) for v in state["rotation"]],
                        [float(v) for v in state["scale"]],
                        [float(v) for v in state["translation"]])
        parent = entry["parent"]
        world[off] = mat_mul(world[parent], local) if parent else local
    return world


def world_position(matrix):
    return [round(matrix[r][3], 4) for r in range(3)]


# --------------------------------------------------------------------------
# material / camera reporting
# --------------------------------------------------------------------------
def material_summary(reader, discovered, targets, frames):
    """Every material animation in this archive.

    HSD exporters bake constant colour tracks into the loop and frequently add
    short alpha fades on top, so "constant" is reported per channel instead of
    assuming the whole record is static.
    """
    # Independent reference data: the material records the MObjs actually use.
    known = set()
    for off in targets:
        hits = classify(reader, off)
        mobj = hits.get("mobj")
        if not mobj or not mobj.get("material"):
            continue
        material = mobj["material"]
        rows = []
        complete = True
        for field in ("ambient", "diffuse", "specular"):
            word = reader.u32(material + {"ambient": 0, "diffuse": 4, "specular": 8}[field])
            if word is None:
                complete = False
                continue
            rows.append(((word >> 24) & 0xFF, (word >> 16) & 0xFF, (word >> 8) & 0xFF))
        alpha = reader.f32(material + 0xC)
        if not complete or alpha is None:
            continue
        known.add((tuple(rows), round(alpha, 3)))

    known_colours = {entry[0] for entry in known}
    results = []
    matched = 0
    colour_matched = 0
    colour_near = 0
    for mat in discovered["matanims"]:
        tracks = fobjdesc_chain(reader, reader.ptr(mat["aobj"] + 8)) or []
        values = {}
        ranges = {}
        animated = []
        for desc in tracks:
            profile = track_profile(reader, desc, frames)
            samples = [v for _, v in profile["samples"]]
            if not samples:
                continue
            values[desc["type"]] = samples[-1]
            if len(samples) > 1 and max(samples) - min(samples) > 1.0e-5:
                animated.append(MATERIAL_CHANNELS.get(desc["type"], str(desc["type"])))
                ranges[desc["type"]] = [round(min(samples), 6), round(max(samples), 6)]
        entry = {"matanim": mat["offs"], "aobj": mat["aobj"],
                 "constant": not animated,
                 "animated_channels": sorted(animated),
                 "values": values,
                 "ranges": {str(k): v for k, v in ranges.items()},
                 "channels": {str(k): MATERIAL_CHANNELS.get(k, "?")
                              for k in sorted(values)}}
        # Self-check: the baked colour tracks must reproduce an authored material.
        if all(k in values for k in (1, 2, 3, 4, 5, 6, 7, 8, 9, 10)):
            colours = tuple(tuple(max(0, min(255, int(round(255.0 * values[k + i]))))
                                  for i in range(3)) for k in (1, 4, 7))
            alpha = round(1.0 - values[10], 3)
            entry["matches_authored_material"] = (colours, alpha) in known
            entry["colour_matches_authored_material"] = colours in known_colours
            entry["colour_matches_within_1_lsb"] = any(
                all(abs(a - b) <= 1 for a, b in zip(colours[i], candidate[i]))
                for candidate in known_colours for i in range(3))
            if entry["matches_authored_material"]:
                matched += 1
            if entry["colour_matches_authored_material"]:
                colour_matched += 1
            if entry["colour_matches_within_1_lsb"]:
                colour_near += 1
        results.append(entry)
    return results, {"material_animations": len(results),
                     "colour_matched_exact": colour_matched,
                     "colour_matched_within_1_lsb": colour_near,
                     "colour_and_alpha_matched": matched,
                     "note": "colour/alpha tracks decoded from the packed stream are "
                             "compared with the material records the MObjs use; a match "
                             "is independent proof the track decoder is right (alpha is "
                             "compared at the last sampled frame, so an animated fade "
                             "need not match)."}


def camera_report(reader, discovered, frames):
    reports = []
    for cam in discovered["cameras"]:
        def sample_anim(aobj_off, channels):
            tracks = fobjdesc_chain(reader, reader.ptr(aobj_off + 8)) or []
            table = {}
            for desc in tracks:
                profile = track_profile(reader, desc, frames)
                table[desc["type"]] = profile
            return table

        # sample_anim takes an HSD_AObjDesc offset; the CameraAnim and the two
        # WObjAnim records each point at one.
        base = sample_anim(cam["aobj"], CAMERA_CHANNELS)
        eye_tracks = sample_anim(reader.ptr(cam["eye_anim"]), CAMERA_CHANNELS)
        interest_tracks = sample_anim(reader.ptr(cam["interest_anim"]), CAMERA_CHANNELS)

        def param(track_table, channel, frame):
            if channel not in track_table:
                return None
            for f, value in track_table[channel]["samples"]:
                if f == frame:
                    return value
            return None

        eye_spline = None
        interest_spline = None
        # The spline lives on the joint descriptor named by the WObj animation's
        # AObjDesc.obj_id (HSD loads that descriptor as a spline joint); the
        # JOBJ_SPLINE flag selects the spline field at +0x10.
        for key, wobj_anim, spline_slot in (("eye", cam["eye_anim"], "eye"),
                                            ("interest", cam["interest_anim"], "interest")):
            aobjdesc = reader.ptr(wobj_anim)
            obj = reader.ptr(aobjdesc + 0xC) if aobjdesc else 0
            if obj and (reader.u32(obj + 4) & 0x4000):
                info = read_spline(reader, reader.ptr(obj + 0x10))
                if info:
                    if spline_slot == "eye":
                        eye_spline = Spline(reader, info)
                    else:
                        interest_spline = Spline(reader, info)

        per_frame = []
        for frame in frames:
            eye = None
            interest = None
            if eye_spline is not None and 4 in eye_tracks:
                p = param(eye_tracks, 4, frame)
                if p is not None:
                    eye = eye_spline.arc_point(p)
            if interest_spline is not None and 4 in interest_tracks:
                p = param(interest_tracks, 4, frame)
                if p is not None:
                    interest = interest_spline.arc_point(p)
            roll = param(base, 9, frame)
            fov = param(base, 10, frame)
            near = param(base, 11, frame)
            far = param(base, 12, frame)
            per_frame.append({
                "frame": frame,
                "eye": [round(v, 4) for v in eye] if eye else None,
                "interest": [round(v, 4) for v in interest] if interest else None,
                "roll": roll, "fov": fov, "near": near, "far": far,
            })
        reports.append({
            "anim": cam["anim"], "desc": cam["desc"],
            "projection": cam["projection"],
            "desc_eye": [round(v, 4) for v in cam["desc_eye"]],
            "desc_interest": [round(v, 4) for v in cam["desc_interest"]],
            "desc_fov": round(cam["fov"], 6), "desc_near": round(cam["near"], 6),
            "desc_far": round(cam["far"], 6), "desc_aspect": round(cam["aspect"], 6),
            "eye_spline": eye_spline.info if eye_spline else None,
            "interest_spline": interest_spline.info if interest_spline else None,
            "frames": per_frame,
        })
    return reports


# --------------------------------------------------------------------------
# top level
# --------------------------------------------------------------------------
def analyse(data, label, frames, stretch_threshold):
    result = {"input": label, "input_bytes": len(data),
              "input_sha256": hashlib.sha256(data).hexdigest()}
    ctx = load_container(data, label)
    if not ctx["hsd"]:
        result.update(hsd=False, reason=ctx["reason"])
        return result
    reader = Reader(ctx)
    result.update(hsd=True, container={
        "declared_bytes": ctx["tables"]["declared_bytes"],
        "data_bytes": ctx["tables"]["data_bytes"],
        "relocation_count": ctx["tables"]["relocation_count"],
        "publics": ctx["tables"]["publics"],
    })

    discovered = discover(reader)
    jobj_order, jobj_roots = jobj_forest(reader, sorted(ctx["targets"]))
    tree_maps = map_trees(reader, discovered["anim_trees"], jobj_order)

    for mapping in tree_maps:
        for entry in mapping["anim_nodes"]:
            entry["channels"] = []
            aobj = entry["node"]["aobj"]
            if not aobj:
                continue
            for desc in fobjdesc_chain(reader, reader.ptr(aobj + 8)) or []:
                entry["channels"].append({
                    "type": desc["type"],
                    "name": JOINT_CHANNELS.get(desc["type"], "?"),
                    "length": desc["length"],
                    "startframe": desc["startframe"],
                })

    # Which joints have tracks at all?  Only nodes that carry an AObjDesc.
    animated_joints = {m["node_map"][entry["offs"]] for m in tree_maps
                       for entry in m["anim_nodes"]
                       if entry["node"]["aobj"] and entry["offs"] in m["node_map"]}

    # Any non-constant translation track would mean animation can move a joint.
    translation_tracks = []
    for tree in discovered["anim_trees"]:
        for entry in tree["nodes"]:
            aobj = entry["node"]["aobj"]
            if not aobj:
                continue
            for desc in fobjdesc_chain(reader, reader.ptr(aobj + 8)) or []:
                if desc["type"] not in (5, 6, 7):
                    continue
                first = sample_track(reader, desc, 1).value
                last = sample_track(reader, desc, max(frames) if frames else 1999).value
                translation_tracks.append({
                    "joint_anim_node": entry["offs"],
                    "channel": JOINT_CHANNELS[desc["type"]],
                    "first": round(first, 6), "last": round(last, 6),
                    "constant_zero": abs(first) < 1e-6 and abs(last - first) < 1e-6,
                })

    billboard = [n["offs"] for n in jobj_order
                 if (n["jobj"]["flags"] & JOBJ_BILLBOARD_MASK)]
    stretched = [n["offs"] for n in jobj_order
                 if abs(n["jobj"]["scale"][1]) >= stretch_threshold]

    before = world_matrices(reader, jobj_order, {})
    evaluations = []
    max_position_delta = 0.0
    for frame in frames:
        pose = pose_at(reader, tree_maps, jobj_order, frame)
        after = world_matrices(reader, jobj_order, pose)
        for off in (n["offs"] for n in jobj_order):
            delta = max(abs(after[off][r][3] - before[off][r][3]) for r in range(3))
            max_position_delta = max(max_position_delta, delta)
        evaluations.append({
            "frame": frame,
            "billboard_world_positions": {
                hex(off): {"flags": next(n for n in jobj_order
                                         if n["offs"] == off)["jobj"]["flags"],
                           "before": world_position(before[off]),
                           "after": world_position(after[off])}
                for off in billboard},
            "stretched_joint_scales": {
                hex(off): {"before": list(round(v, 5) for v in
                                          next(n for n in jobj_order if n["offs"] == off)["jobj"]["scale"]),
                           "after": [round(v, 5) for v in pose.get(off, {}).get("scale",
                                        next(n for n in jobj_order if n["offs"] == off)["jobj"]["scale"])],
                           "hidden_after": pose.get(off, {}).get("hidden")}
                for off in stretched},
        })
    positions_stable = max_position_delta < 1.0e-6

    # Frame-independent argument: a joint's world position can only change if a
    # translation track changes, or an animated ancestor's rotation/scale moves
    # a descendant that has a nonzero local translation.  Check both directly
    # (the sampled check above then becomes a confirmation, not the only proof).
    children = {}
    for node in jobj_order:
        children.setdefault(node["parent"], []).append(node["offs"])
    by_off = {node["offs"]: node for node in jobj_order}
    moving_pairs = []
    for mapping in tree_maps:
        for entry in mapping["anim_nodes"]:
            if not entry["node"]["aobj"] or entry["offs"] not in mapping["node_map"]:
                continue
            channels = {c["type"] for c in entry.get("channels", [])}
            if not (channels & {1, 2, 3, 8, 9, 10, 11}):
                continue
            node = mapping["node_map"][entry["offs"]]
            stack = list(children.get(node, []))
            while stack:
                off = stack.pop()
                if any(abs(v) > 1e-9 for v in by_off[off]["jobj"]["translation"]):
                    moving_pairs.append({"animated": node, "offset_descendant": off})
                stack.extend(children.get(off, []))

    materials, material_check = material_summary(reader, discovered,
                                                 sorted(ctx["targets"]), frames)
    camera_aobjs = set()
    for camera in discovered["cameras"]:
        if camera["aobj"]:
            camera_aobjs.add(camera["aobj"])      # already an AObjDesc offset
        for key in ("eye_anim", "interest_anim"):
            record = camera[key]                  # WObjAnim -> AObjDesc
            if record:
                aobj = reader.ptr(record)
                if aobj:
                    camera_aobjs.add(aobj)
    attributed = set(discovered["joint_aobjs"]) | set(discovered["mat_aobjs"]) | camera_aobjs
    unattributed = set(discovered["aobjs"]) - attributed

    result.update({
        "counts": {
            "jobj": len(jobj_order),
            "jobj_roots": len(jobj_roots),
            "aobjdesc": len(discovered["aobjs"]),
            "animjoint_nodes": len(discovered["anim_nodes"]),
            "animjoint_trees": len(discovered["anim_trees"]),
            "joint_anim_aobjdesc": len(discovered["joint_aobjs"]),
            "material_anim_aobjdesc": len(discovered["mat_aobjs"]),
            "camera_anim": len(discovered["cameras"]),
            "camera_anim_aobjdesc": len(camera_aobjs),
            "aobjdesc_unattributed": len(unattributed),
        },
        "aobjdesc_unattributed": [hex(x) for x in sorted(unattributed)[:32]],
        "joint_animate_channels_observed": sorted({c["type"] for tree in discovered["anim_trees"]
                                                    for entry in tree["nodes"]
                                                    for c in entry["channels"]}),
        "animated_joint_count": len(animated_joints),
        "translation_tracks": translation_tracks,
        "translation_moves_anything": any(not t["constant_zero"] for t in translation_tracks),
        "trees": [{
            "anim_root": mapping["anim_root"],
            "joint_root": mapping["joint_root"],
            "unique_shape_match": mapping["unique"],
            "candidates": [{"joint_root": hex(c["joint_root"]) if c["joint_root"] else None,
                            "score": c["score"]} for c in mapping["candidates"]],
            "nodes": [{
                "anim_node": entry["offs"],
                "joint": hex(mapping["node_map"].get(entry["offs"], 0)) if mapping["node_map"].get(entry["offs"]) else None,
                "depth": entry["depth"],
                "channels": entry["channels"],
            } for entry in mapping["anim_nodes"]],
        } for mapping in tree_maps],
        "evaluations": evaluations,
        "world_positions_stable_across_frames": positions_stable,
        "max_world_position_delta": round(max_position_delta, 9),
        "animated_transform_can_move_geometry": bool(moving_pairs),
        "animated_transform_moving_pairs": moving_pairs[:32],
        "materials": materials,
        "material_check": material_check,
        "cameras": camera_report(reader, discovered, frames),
        "conclusion": {
            "has_hsd_animation": bool(discovered["aobjs"]),
            "has_joint_animation": bool(discovered["anim_trees"]),
            "animation_can_translate_joints": any(not t["constant_zero"] for t in translation_tracks),
            "evaluated_frames_keep_world_positions": positions_stable,
            "animated_transforms_keep_all_geometry_in_place": not moving_pairs,
            "note": ("Joint animation exists, but every translation track is a constant "
                     "zero and no animated rotation/scale node has a descendant with a "
                     "nonzero offset, so no frame can change any joint's world position "
                     "(sampled max delta %g). Billboard/disc placement in this archive "
                     "therefore comes from the file's static joint translations or from "
                     "game code, not from an animation track." % max_position_delta
                     if discovered["anim_trees"] and positions_stable and not moving_pairs and
                     not any(not t["constant_zero"] for t in translation_tracks) else
                     "Review translation_tracks and world position deltas above."),
        },
    })
    return result


def private_path(path):
    resolved = path.resolve()
    if not resolved.is_relative_to(ROOT / "local") or resolved == ROOT / "local":
        raise ValueError("analysis output must stay in private local storage: %s" % path)
    return resolved


def print_summary(summary):
    name = summary["input"]
    if not summary.get("hsd"):
        print("%-14s NOT-HSD %s" % (name, summary.get("reason")))
        return
    counts = summary["counts"]
    print("%-14s jobj=%-4d animjoints=%-4d trees=%-2d aobj=%-3d joint=%-3d mat=%-3d cam=%-2d "
          "unattr=%-2d translate_moves=%s" % (
              name, counts["jobj"], counts["animjoint_nodes"], counts["animjoint_trees"],
              counts["aobjdesc"], counts["joint_anim_aobjdesc"],
              counts["material_anim_aobjdesc"], counts["camera_anim"],
              counts["aobjdesc_unattributed"], summary["translation_moves_anything"]))
    for tree in summary["trees"]:
        print("   anim %#07x -> joint %s  nodes=%d channels=%s" % (
            tree["anim_root"], tree["joint_root"] and hex(tree["joint_root"]),
            len(tree["nodes"]), sorted({c["type"] for n in tree["nodes"] for c in n["channels"]})))
    for evaluation in summary["evaluations"]:
        print("   frame %d:" % evaluation["frame"])
        for off, pos in evaluation["billboard_world_positions"].items():
            print("      billboard %s world before=%s after=%s" % (
                off, pos["before"], pos["after"]))
        for off, scale in evaluation["stretched_joint_scales"].items():
            print("      stretched %s scale before=%s after=%s hidden=%s" % (
                off, scale["before"], scale["after"], scale["hidden_after"]))
    for camera in summary.get("cameras", []):
        print("   camera %#07x desc=%#07x fov=%g near=%g far=%g" % (
            camera["anim"], camera["desc"], camera["desc_fov"],
            camera["desc_near"], camera["desc_far"]))
        for frame in camera["frames"]:
            print("      frame %d eye=%s interest=%s" % (
                frame["frame"], frame["eye"], frame["interest"]))


def main(argv=None):
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("inputs", nargs="*", type=Path, help="HSD archive files")
    parser.add_argument("--frame", action="append", type=int, default=[],
                        help="frame to evaluate (repeatable). Default: 1, 30, 60, 1999")
    parser.add_argument("--stretch-threshold", type=float, default=5.0,
                        help="report joints whose authored Y scale reaches this (default 5)")
    parser.add_argument("--json", type=Path, help="single JSON with every summary")
    parser.add_argument("--output", type=Path, help="directory for one JSON per input")
    parser.add_argument("--print", dest="do_print", action="store_true",
                        help="print the compact summary")
    parser.add_argument("--summary-only", action="store_true",
                        help="skip per-track material/camera details (faster)")
    args = parser.parse_args(argv)

    frames = sorted(set(args.frame or [1, 30, 60, 1999]))
    if not frames:
        raise SystemExit("nothing to evaluate")
    if frames[-1] > MAX_TRACK_STEPS:
        raise SystemExit("frame exceeds the offline evaluation cap")

    summaries = []
    for path in args.inputs:
        if path.stat().st_size > MAX_INPUT_BYTES:
            raise SystemExit("input exceeds offline bound: %s" % path)
        data = path.read_bytes()
        summary = analyse(data, path.name, frames, args.stretch_threshold)
        summary["source"] = str(path)
        if args.summary_only:
            summary.pop("materials", None)
            summary.pop("cameras", None)
        summaries.append(summary)
        if args.do_print:
            print_summary(summary)
        if args.output:
            target = private_path(args.output / (path.stem + ".json"))
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
    if not summaries:
        raise SystemExit("nothing to analyse")
    return 0


if __name__ == "__main__":
    sys.exit(main())
