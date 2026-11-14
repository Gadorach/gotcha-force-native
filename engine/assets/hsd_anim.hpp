#pragma once
// HSD animation decoder: the packed keyframe clips HSD archives carry for their
// joints and their camera.
//
// Why this is a separate decoder from hsd.hpp: HSD keyframes are NOT float
// arrays. A track is a packed byte stream (HSD_FObjData) whose first byte holds
// an opcode in the low nibble and a pack count in the high nibble, followed by
// quantised values (a numerator divided by 2**frac, or a plain f32) each
// followed by a variable-length wait. Scanning for plausible floats finds none
// of it, so animation is found structurally instead: the descriptor records that
// own the byte streams are located through the archive's relocation table,
// exactly the way hsd.hpp finds joints.
//
// Ported from tools/analyze/hsd_anim.py, which was validated on real data by
// decoding the 32 material colour tracks and reproducing the archive's authored
// material colours (32/32 within one 8-bit LSB), plus a positive control on
// vsel00_mdl.arc that does find non-zero translation tracks. Every layout and
// the track state machine below follow that tool; deviations are listed at the
// end of this comment.
//
// Layouts (big-endian, offsets relative to the data block that starts 0x20 bytes
// into the container). The class-name pointer is NULL everywhere in this game,
// so identification is structural and mutually validated:
//
//   HSD_AObjDesc    (0x10): 0x00 flags:u32, 0x04 end_frame:f32,
//                           0x08 fobjdesc*, 0x0C obj_id*
//   HSD_FObjDesc    (0x14): 0x00 next*, 0x04 length:u32, 0x08 startframe:f32,
//                           0x0C type:u8, 0x0D frac_value:u8,
//                           0x0E frac_slope:u8, 0x0F dummy:u8, 0x10 ad*(bytes)
//   HSD_AnimJoint   (0x14): 0x00 child*, 0x04 next*, 0x08 aobjdesc*,
//                           0x0C robjanim*, 0x10 flags:u32
//   HSD_CameraAnim  (0x0C): 0x00 aobjdesc*, 0x04 eye_anim*, 0x08 interest_anim*
//   HSD_WObjAnim    (0x08): 0x00 aobjdesc*, 0x04 robjanim*
//   HSD_Spline      (0x18): 0x00 type:u8, 0x02 numcv:s16, 0x04 tension:f32,
//                           0x08 cv* (numcv + 2*(type != 0) Vec3), 0x0C totalLength:f32,
//                           0x10 segLength* (numcv f32), 0x14 segPoly* ((numcv-1)*5 f32)
//   HSD_CObjDesc perspective: 0x06 projection:u16, 0x18 eyepos:WObjDesc*,
//                           0x1C interest:WObjDesc*, 0x20 roll:f32, 0x28 near:f32,
//                           0x2C far:f32, 0x30 fov:f32, 0x34 aspect:f32
//   HSD_WObjDesc    (0x14): 0x00 class*, 0x04 position:Vec3, 0x10 robjdesc*
//
// Track state machine (HSD_FObjInterpretAnim at rate 1, so frame N means "after
// N calls" and frame 0 is the un-stepped state). Opcodes: CON(1) hold-then-jump,
// LIN(2) linear, SPL0(3)/SPL(4)/SLP(5) Hermite (splGetHelmite), KEY(6) step.
// Each pack is [opcode+count][value][wait] repeated `count` times; the first
// value of a pack is the value at the START of the segment that the following
// wait describes. EOF is a call-local dispatch result, not a stored stop state;
// it evaluates the current interpolation using this call's consumed-wait carry.
//
// Documented deviations from the Python reference, all deliberate:
//   * A malformed byte stream (a value or wait past the declared `length`, an
//     unsupported frac encoding, an impossible pack count) is REFUSED with a
//     message instead of reading past the buffer. The reference can read past
//     `length`. Unknown opcodes are refused: publishing a partial pose as if the
//     whole clip evaluated was misleading (a vsel00 tail exercises this gate).
//   * Joint paths use their declared splines; NODE/BRANCH visibility is carried
//     into the pose. Published scene root records bind animation trees directly.
//     Non-scene fixtures may use unique shape/content matches; ties refuse.
//   * FObjDesc chains whose `next` is not a relocated pointer are refused.
//   * Revisiting an animation node is refused instead of walking it twice.
//
// Material color/alpha animation is bound through MatAnimJoint root records.
// Not implemented: texture animation, skinning, and deciding which clip the game
// plays. `hsd_anim_end_frame` reports the archive's authored clip length (max
// AObjDesc end_frame); it is not a claim that the game runs that long.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "engine/assets/hsd.hpp"
#include "engine/core/types.hpp"

namespace gf {

// ---- pose ------------------------------------------------------------

// One joint's local SRT at an evaluated frame. Euler rotation is radians in the
// same X->Y->Z order mat4_from_srt pins.
struct HsdAnimJointPose {
    f32 rotation[3] = {0.0f, 0.0f, 0.0f};
    f32 scale[3] = {1.0f, 1.0f, 1.0f};
    f32 translation[3] = {0.0f, 0.0f, 0.0f};
    // JOBJ_HIDDEN at this frame: the file's flag, overridden by an
    // HSD_A_J_NODE (visibility) track when the clip has one. Carried because a
    // clip can hide a whole group (the menu room's animated plates are hidden
    // except during their ramp); a renderer that ignores it draws geometry the
    // game does not.
    bool hidden = false;
    // Optional explicit affine local matrix for portable code-driven instances
    // with a rotation order different from the archive SRT convention.
    bool matrix_override=false;
    f32 matrix[12]={1,0,0,0,0,1,0,0,0,0,1,0};
};

// An evaluated pose. `joints` is parallel to HsdScene::joints; joints that no
// animation tree reaches keep the pose stored in the file.
struct HsdAnimPose {
    std::vector<HsdAnimJointPose> joints;
    bool has_camera = false;
    f32 camera_eye[3] = {0.0f, 0.0f, 0.0f};
    f32 camera_interest[3] = {0.0f, 0.0f, 0.0f};
    f32 camera_fov_y_degrees = 0.0f;
    f32 camera_near = 0.0f;
    f32 camera_far = 0.0f;
    struct Material {
        bool present=false;f32 ambient[3]={};f32 diffuse[3]={};f32 alpha=1;
        bool has_specular=false;f32 specular[3]={};
        // Requested material/texture clips may supply a bounded UV transform.
        // Legacy whole-scene evaluation leaves these disabled.
        bool has_texture=false;f32 uv_scale[2]={1,1},uv_offset[2]={};f32 texture_weight=1;
        bool has_texture_colours=false;u32 texture_low_colour=0,texture_high_colour=0xffffffffu;
    };
    std::vector<Material> materials; // parallel to scene.meshes
};

// ---- decoded animation tables ---------------------------------------

// One HSD_FObjDesc track: a channel id, its sample range and the raw byte
// stream (kept as bytes, not as samples, so any frame can be evaluated).
struct HsdAnimTrack {
    u32 channel = 0;          // HSD_A_* id: 1..3 rotation, 4 path, 5..7 translation,
                              // 8..10 scale, 11 node visibility, 12 branch
    f32 startframe = 0.0f;
    u8 frac_value = 0;        // 0 = f32, else quantised (see parse_value)
    u8 frac_slope = 0;
    std::vector<u8> data;
};

// One HSD_AnimJoint node. An empty track list is a pass-through node (the
// descriptor's aobjdesc is NULL), which real archives do use.
struct HsdAnimNode {
    u32 depth = 0;            // preorder depth inside its tree
    std::vector<HsdAnimTrack> tracks;
    u32 path_spline = 0xFFFFFFFFu;
};

// One HSD_AnimJoint tree in preorder: root first, and a node's siblings follow
// its subtree. `channels` is the sorted set of channel ids the tree uses; it is
// part of the evidence that decides which joint subtree the tree belongs to.
struct HsdAnimTree {
    std::vector<HsdAnimNode> nodes;
    std::vector<u32> channels;
    u32 archive_offset = 0;
};

// A decoded HSD_Spline (camera eye/interest paths).
struct HsdAnimSpline {
    bool valid = false;
    u32 type = 0;                 // 0 linear, 1 bezier, 2 bspline, 3 cardinal
    u32 numcv = 0;
    f64 tension = 0.0;
    f64 total_length = 0.0;
    std::vector<f64> points;      // (numcv + 2*(type != 0)) * 3, xyz
    std::vector<f64> seg_length;  // numcv
    std::vector<f64> seg_poly;    // (numcv - 1) * 5 polynomial coefficients
};

// The archive's camera animation: a base AObjDesc (roll/fov/near/far) plus two
// HSD_WObjAnim records that move the eye and the interest point, usually along
// a spline.
struct HsdCameraAnim {
    bool present = false;
    std::vector<HsdAnimTrack> base;
    std::vector<HsdAnimTrack> eye;
    std::vector<HsdAnimTrack> interest;
    HsdAnimSpline eye_spline;
    HsdAnimSpline interest_spline;
    f32 static_eye[3] = {0.0f, 0.0f, 0.0f};        // the CObjDesc's own WObj positions
    f32 static_interest[3] = {0.0f, 0.0f, 0.0f};
    f32 fov = 0.0f;
    f32 near_plane = 0.0f;
    f32 far_plane = 0.0f;
};

// Everything hsd_anim_load recovered. Plain data: it owns its bytes, so the
// caller's archive buffer may go away.
struct HsdAnimation {
    std::vector<HsdAnimTree> trees;
    HsdCameraAnim camera;
    f32 end_frame = 0.0f;              // max AObjDesc end_frame, 0 when there is none
    u32 aobj_count = 0;                // diagnostics against the reference tool
    u32 anim_node_count = 0;
    std::vector<f32> aobj_end_frames;  // every authored clip length, sorted
    // The archive's JOBJ preorder offsets, used only to order shape-match ties
    // the way the reference does. Empty when the archive has no decodable JOBJ.
    std::vector<u32> joint_offsets;
    struct MaterialTracks {u32 mobj=0;std::vector<HsdAnimTrack> tracks;};
    std::vector<MaterialTracks> materials;
    std::vector<HsdAnimSpline> joint_splines;
};

namespace hsd_anim_detail {

inline constexpr std::size_t kMaxTrackBytes = 0x10000;
inline constexpr std::size_t kMaxAobjDesc = 4096;
inline constexpr std::size_t kMaxAnimNodes = 4096;
inline constexpr std::size_t kMaxRelocations = 1u << 20;
inline constexpr std::size_t kMaxSplinePoints = 4096;
inline constexpr u32 kMaxFrame = 100000;
inline constexpr u32 kMaxTrackSteps = 100000;
inline constexpr u32 kMaxPackCount = 65536;
inline constexpr unsigned kMaxDepth = 64;
inline constexpr u32 kHiddenFlag = 0x10u;   // JOBJ_HIDDEN

// ---- little-endian reads (track payloads are little-endian) ----------

inline f32 le_f32(const u8* p) {
    const u32 bits = u32(p[0]) | (u32(p[1]) << 8) | (u32(p[2]) << 16) | (u32(p[3]) << 24);
    return bits_to_f32(bits);
}

inline u16 le_u16(const u8* p) { return u16(u16(p[0]) | (u16(p[1]) << 8)); }

inline s16 bits_s16(u16 value) { return std::bit_cast<s16>(value); }

inline s8 bits_s8(u8 value) { return std::bit_cast<s8>(value); }

// ---- HSD_FObjInterpretAnim state machine -----------------------------

enum class TrackState : u32 {
    LoadData = 1,
    LoadWait = 2,
    CheckWait = 3,
    Interpolate = 4,
    NextSegment = 5,
    Exhausted = 6,
};

struct TrackMachine {
    const u8* data = nullptr;
    std::size_t length = 0;
    std::size_t pos = 0;
    u8 frac_value = 0;
    u8 frac_slope = 0;
    f64 time = 0.0;
    u32 op = 0;
    u32 op_intrp = 0;
    s32 nb_pack = 0;
    f64 p0 = 0.0;
    f64 p1 = 0.0;
    f64 d0 = 0.0;
    f64 d1 = 0.0;
    f64 fterm = 0.0;
    u32 flags = 0;      // 0x20 wait loaded, 0x40 a KEY value is pending, 0x80 jump
    u32 state = u32(TrackState::LoadData);
    f64 value = 0.0;
    u64 callbacks = 0; // successful scalar callbacks, not interpreter calls
    bool float_time = false; // explicit retained-player f32 clock boundaries
    bool stopped = false;   // an unknown opcode ended the track (see load_data)
};

// Verbatim transcription of HSD's splGetHelmite.
inline f64 spl_helmite(f64 fterm, f64 time, f64 p0, f64 p1, f64 d0, f64 d1) {
    const f64 t2 = time * time;
    const f64 n2 = fterm * fterm;
    const f64 t2n = t2 * fterm;
    const f64 t3n2 = n2 * (t2 * time);
    const f64 two = 2.0 * t3n2 * fterm;
    const f64 three = 3.0 * t2 * n2;
    return (d1 * (t3n2 - t2n)) +
           ((d0 * (time + ((t3n2 - t2n) - t2n))) +
            ((p0 * (1.0 + (two - three))) + (p1 * (-two + three))));
}

inline bool parse_value(TrackMachine& m, u8 frac, f64* out, std::string* error) {
    if (frac == 0) {
        if (m.pos + 4 > m.length) {
            if (error) *error = "hsd anim: track value runs past the declared length";
            return false;
        }
        *out = f64(le_f32(m.data + m.pos));
        m.pos += 4;
        return true;
    }
    const f64 denom = f64(u32(1) << (frac & 0x1Fu));
    switch (u8(frac & 0xE0u)) {
    case 0x60:   // s8
        if (m.pos + 1 > m.length) {
            if (error) *error = "hsd anim: track value runs past the declared length";
            return false;
        }
        *out = f64(bits_s8(m.data[m.pos])) / denom;
        m.pos += 1;
        return true;
    case 0x80:   // u8
        if (m.pos + 1 > m.length) {
            if (error) *error = "hsd anim: track value runs past the declared length";
            return false;
        }
        *out = f64(m.data[m.pos]) / denom;
        m.pos += 1;
        return true;
    case 0x20:   // s16
        if (m.pos + 2 > m.length) {
            if (error) *error = "hsd anim: track value runs past the declared length";
            return false;
        }
        *out = f64(bits_s16(le_u16(m.data + m.pos))) / denom;
        m.pos += 2;
        return true;
    case 0x40:   // u16
        if (m.pos + 2 > m.length) {
            if (error) *error = "hsd anim: track value runs past the declared length";
            return false;
        }
        *out = f64(le_u16(m.data + m.pos)) / denom;
        m.pos += 2;
        return true;
    default:
        if (error) {
            *error = "hsd anim: unsupported frac_value encoding 0x" +
                     std::to_string(unsigned(frac));
        }
        return false;
    }
}

inline bool parse_wait(TrackMachine& m, f64* out, std::string* error) {
    u64 wait = 0;
    unsigned shift = 0;
    for (;;) {
        if (m.pos >= m.length) {
            if (error) *error = "hsd anim: track wait runs past the declared length";
            return false;
        }
        const u8 byte = m.data[m.pos++];
        if (shift >= 64) {
            if (error) *error = "hsd anim: track wait is out of range";
            return false;
        }
        wait |= u64(byte & 0x7Fu) << shift;
        shift += 7;
        if ((byte & 0x80u) == 0) break;
    }
    *out = f64(wait);
    return true;
}

inline bool parse_pack(TrackMachine& m, s32* count, std::string* error) {
    const u8 byte = m.data[m.pos++];
    s64 value = s64((byte >> 4) & 7u) + 1;
    unsigned shift = 3;
    if ((byte & 0x80u) != 0) {
        for (;;) {
            if (m.pos >= m.length) {
                if (error) *error = "hsd anim: track pack count runs past the declared length";
                return false;
            }
            const u8 ext = m.data[m.pos++];
            if (shift >= 32) {
                if (error) *error = "hsd anim: track pack count is out of range";
                return false;
            }
            value += s64(ext & 0x7Fu) << shift;
            shift += 7;
            if ((ext & 0x80u) == 0) break;
        }
    }
    *count = s32(value);
    return true;
}

// One loader step. `next` is the state the caller moves to; false means the
// stream is malformed and the caller must stop.
inline bool load_data(TrackMachine& m, u32* next, std::string* error) {
    if (m.pos >= m.length) {
        *next = u32(TrackState::Exhausted);
        return true;
    }
    m.op_intrp = m.op;
    if (m.nb_pack == 0) {
        // One byte carries the opcode (low nibble) and the pack count (high
        // nibble); parse_pack consumes that same byte.
        m.op = u32(m.data[m.pos] & 0xFu);
        if (!parse_pack(m, &m.nb_pack, error)) return false;
        if (m.nb_pack < 1 || std::size_t(m.nb_pack) > kMaxPackCount) {
            if (error) *error = "hsd anim: track pack count is out of range";
            return false;
        }
    }
    m.nb_pack -= 1;
    switch (m.op) {
    case 1:   // CON
    case 2:   // LIN
        m.p0 = m.p1;
        if (!parse_value(m, m.frac_value, &m.p1, error)) return false;
        if (m.op_intrp != 5) {
            m.d0 = m.d1;
            m.d1 = 0.0;
        }
        break;
    case 3:   // SPL0
        m.p0 = m.p1;
        m.d0 = m.d1;
        if (!parse_value(m, m.frac_value, &m.p1, error)) return false;
        m.d1 = 0.0;
        break;
    case 4:   // SPL
        m.p0 = m.p1;
        if (!parse_value(m, m.frac_value, &m.p1, error)) return false;
        m.d0 = m.d1;
        if (!parse_value(m, m.frac_slope, &m.d1, error)) return false;
        break;
    case 5:   // SLP
        m.d0 = m.d1;
        if (!parse_value(m, m.frac_slope, &m.d1, error)) return false;
        // Original802489F4 returns the current loader state for SLP. It has
        // neither a key value nor a wait; immediately load the following op.
        // Advancing to CheckWait/Interpolate consumes the next opcode as time.
        *next=m.state;return true;
    case 6:   // KEY (a step, launched on the next key)
        if ((m.flags & 0x40u) != 0) {
            m.op_intrp = m.op;
            m.flags &= ~0x40u;
            m.flags |= 0x80u;
            m.p0 = m.p1;
        }
        if (!parse_value(m, m.frac_value, &m.p1, error)) return false;
        m.flags |= 0x40u;
        break;
    default:
        // HSD_FObjInterpretAnim has no case for this opcode and ends the track
        // there, holding the last value; vsel00_mdl.arc really does contain one
        // such tail, so refusing would make a real archive unevaluable.
        *next = 0;
        return true;
    }
    *next = m.state == u32(TrackState::LoadData) ? u32(TrackState::CheckWait)
                                                 : u32(TrackState::Interpolate);
    return true;
}

inline bool load_wait(TrackMachine& m, u32* next, std::string* error) {
    if (m.pos >= m.length) {
        *next = u32(TrackState::Exhausted);
        return true;
    }
    if (!parse_wait(m, &m.fterm, error)) return false;
    m.flags |= 0x20u;
    *next = u32(TrackState::LoadWait);
    return true;
}

// Returns whether the value was updated this step.
inline bool update(TrackMachine& m) {
    switch (m.op_intrp) {
    case 6:   // KEY
        if ((m.flags & 0x80u) != 0) {
            m.value = m.p0;
            ++m.callbacks;
            m.flags &= ~0x80u;
            return true;
        }
        return false;
    case 1:   // CON: hold, then jump when the segment elapses
        m.value = (m.time >= m.fterm) ? m.p1 : m.p0;
        ++m.callbacks;
        return true;
    case 2:   // LIN
        if ((m.flags & 0x20u) != 0) {
            m.flags &= ~0x20u;
            if (m.fterm != 0.0) {
                m.d0 = (m.p1 - m.p0) / m.fterm;
            } else {
                m.d0 = 0.0;
                m.p0 = m.p1;
            }
        }
        m.value = m.d0 * m.time + m.p0;
        ++m.callbacks;
        return true;
    case 3:   // SPL0
    case 4:   // SPL
    case 5:   // SLP
        if (m.fterm != 0.0) {
            m.value = spl_helmite(1.0 / m.fterm, m.time, m.p0, m.p1, m.d0, m.d1);
        } else {
            m.value = m.p1;
        }
        ++m.callbacks;
        return true;
    default:
        return false;
    }
}

// Interpret at the supplied rate (ordinary playback defaults to one frame;
// the first update following an explicit request uses zero).
inline bool track_step(TrackMachine& m, std::string* error, f64 rate=1.0) {
    m.time += rate;
    if(m.float_time)m.time=f32(m.time);
    if (m.time < 0.0) return true;
    if (m.stopped) return true;
    // Original80249AC4: f31 starts at zero on EVERY call, and holds only
    // the last wait subtracted in this call (80249F9C..FDc). Loader EOF
    // returns6 in r3 WITHOUT writing state6 to the FObj (802489F4+30).
    // Persisting6/re-adding fterm caused repeated EOF updates to skip time.
    f64 consumed_wait=0;
    u32 current=m.state;
    for (u32 guard = 0; guard < kMaxTrackSteps; ++guard) {
        switch (TrackState(current)) {
        case TrackState::Exhausted:
            m.time += consumed_wait;
            if(m.float_time)m.time=f32(m.time);
            if ((m.flags & 0x40u) != 0) {
                m.op_intrp = m.op;
                m.flags &= ~0x40u;
                m.flags |= 0x80u;
                m.p0 = m.p1;
            }
            (void)update(m);
            return true;
        case TrackState::LoadData:
        case TrackState::LoadWait: {
            u32 next = 0;
            if (!load_data(m, &next, error)) return false;
            if (next == 0) {
                m.stopped = true;
                return true;
            }
            current=next;
            if(next!=u32(TrackState::Exhausted))m.state=next;
            break;
        }
        case TrackState::CheckWait: {
            if ((m.flags & 0x80u) != 0) (void)update(m);
            u32 next = 0;
            if (!load_wait(m, &next, error)) return false;
            current=next;
            if(next!=u32(TrackState::Exhausted))m.state=next;
            break;
        }
        case TrackState::Interpolate:
            if (m.fterm <= m.time) {
                consumed_wait=m.fterm;
                m.time -= m.fterm;
                if(m.float_time)m.time=f32(m.time);
                current=m.state=u32(TrackState::CheckWait);
                break;
            }
            (void)update(m);
            m.state = u32(TrackState::NextSegment);
            return true;
        case TrackState::NextSegment:
            current=m.state=u32(TrackState::Interpolate);
            break;
        default:
            if (error) *error = "hsd anim: track state machine reached an unknown state";
            return false;
        }
    }
    if (error) *error = "hsd anim: track state machine did not settle";
    return false;
}

// ---- HSD_Spline ------------------------------------------------------

inline bool spline_point_at(const HsdAnimSpline& s, u32 index, f64 out[3]) {
    if (std::size_t(index) * 3 + 2 >= s.points.size()) return false;
    for (unsigned k = 0; k < 3; ++k) out[k] = s.points[std::size_t(index) * 3 + k];
    return true;
}

inline void spline_combine(const f64 cp[4][3], const f64 weights[4], f64 out[3]) {
    for (unsigned i = 0; i < 3; ++i) {
        out[i] = cp[0][i] * weights[0] + cp[1][i] * weights[1] + cp[2][i] * weights[2] +
                 cp[3][i] * weights[3];
    }
}

// splGetSplinePoint: the curve parameter u in [0,1] over the control points.
inline bool spline_point(const HsdAnimSpline& s, f64 u, f64 out[3]) {
    if (!s.valid || s.numcv == 0) return false;
    if (u < 0.0 || u > 1.0) return false;
    f64 cp[4][3];
    if (u < 1.0) {
        const f64 t = u * f64(s.numcv - 1);
        const u32 index = u32(t);
        const f64 frac = t - f64(index);
        switch (s.type) {
        case 0: {   // linear
            f64 a[3], b[3];
            if (!spline_point_at(s, index, a) || !spline_point_at(s, index + 1, b)) return false;
            for (unsigned i = 0; i < 3; ++i) out[i] = a[i] + frac * (b[i] - a[i]);
            return true;
        }
        case 1:   // bezier
            for (unsigned k = 0; k < 4; ++k) {
                if (!spline_point_at(s, index * 3 + k, cp[k])) return false;
            }
            {
                const f64 u1 = 1.0 - frac;
                const f64 w[4] = {u1 * u1 * u1, 3.0 * frac * u1 * u1,
                                  3.0 * frac * frac * u1, frac * frac * frac};
                spline_combine(cp, w, out);
                return true;
            }
        case 2:   // bspline
            for (unsigned k = 0; k < 4; ++k) {
                if (!spline_point_at(s, index + k, cp[k])) return false;
            }
            {
                const f64 u2 = frac * frac;
                const f64 u3 = u2 * frac;
                const f64 u1 = 1.0 - frac;
                const f64 k = 1.0 / 6.0;
                const f64 w[4] = {k * u1 * u1 * u1,
                                  k * (4.0 + (3.0 * u3 - 6.0 * u2)),
                                  k * (3.0 * (-u3 + u2 + frac) + 1.0),
                                  k * u3};
                spline_combine(cp, w, out);
                return true;
            }
        default:  // 3: cardinal
            for (unsigned k = 0; k < 4; ++k) {
                if (!spline_point_at(s, index + k, cp[k])) return false;
            }
            {
                const f64 u2 = frac * frac;
                const f64 u3 = u2 * frac;
                const f64 tension = s.tension;
                const f64 w[4] = {tension * (-u3 + 2.0 * u2 - frac),
                                  (2.0 - tension) * u3 + (tension - 3.0) * u2 + 1.0,
                                  (tension - 2.0) * u3 + (3.0 - 2.0 * tension) * u2 + tension * frac,
                                  tension * (u3 - u2)};
                spline_combine(cp, w, out);
                return true;
            }
        }
    }
    const u32 index = s.numcv - 1;
    switch (s.type) {
    case 0:
        return spline_point_at(s, index, out);
    case 1:
        return spline_point_at(s, index * 3, out);
    case 2: {
        if (index < 1) return false;
        for (unsigned k = 0; k < 4; ++k) {
            if (!spline_point_at(s, index - 1 + k, cp[k])) return false;
        }
        const f64 w[4] = {1.0 / 6.0, 4.0 / 6.0, 1.0 / 6.0, 0.0};
        spline_combine(cp, w, out);
        return true;
    }
    default:
        return spline_point_at(s, index + 1, out);
    }
}

inline f64 spline_poly(const HsdAnimSpline& s, u32 segment, f64 t) {
    const std::size_t base = std::size_t(segment) * 5;
    if (base + 4 >= s.seg_poly.size()) return 0.0;
    const f64 t2 = t * t;
    const f64 t3 = t2 * t;
    const f64 t4 = t3 * t;
    f64 value = s.seg_poly[base] * t4 + s.seg_poly[base + 1] * t3 + s.seg_poly[base + 2] * t2 +
                s.seg_poly[base + 3] * t + s.seg_poly[base + 4];
    if (value > -0.001 && value < 0.0) value = 0.0;
    return value > 0.0 ? std::sqrt(value) : 0.0;
}

// splArcLengthGetParameter: normalised arc length -> curve parameter.
inline f64 spline_arc_parameter(const HsdAnimSpline& s, f64 arg) {
    if (arg <= 0.0) return 0.0;
    if (arg >= 1.0) return 1.0;
    if (s.numcv < 2) return 0.0;
    u32 index = 0;
    while (index + 1 < s.numcv && s.seg_length[std::size_t(index) + 1] < arg) ++index;
    f64 result = 0.0;
    if (s.type == 0) {
        const f64 a = s.seg_length[index];
        const f64 b = s.seg_length[std::size_t(index) + 1];
        result = (b != a) ? (arg - a) / (b - a) : 0.0;
    } else {
        f64 remaining = s.total_length * (arg - s.seg_length[index]);
        f64 start = 0.0;
        f64 end = 1.0;
        for (unsigned iteration = 0; iteration < 200; ++iteration) {
            if (std::fabs(start - end) < 1e-5) break;
            result = (start + end) / 2.0;
            const f64 dx = (result - start) / 8.0;
            f64 middle = 0.0;
            f64 t = start + dx;
            for (unsigned step = 2; step <= 8; ++step) {
                middle += ((step % 2 == 0) ? 4.0 : 2.0) * spline_poly(s, index, t);
                t += dx;
            }
            const f64 simpson = dx * (middle + spline_poly(s, index, start) +
                                      spline_poly(s, index, result)) / 3.0;
            if (remaining < 1e-5 + simpson) {
                end = result;
            } else {
                start = result;
                remaining -= simpson;
            }
        }
    }
    return (result + f64(index)) / (f64(s.numcv) - 1.0);
}

inline bool spline_arc_point(const HsdAnimSpline& s, f64 arg, f64 out[3]) {
    return spline_point(s, spline_arc_parameter(s, arg), out);
}

// ---- discovery (ported from the reference tool) ----------------------

// The reference tool's own classifier set, ported field for field. It is
// deliberately NOT hsd.hpp's: hsd.hpp's classifiers are weaker in places (its
// as_mobj only requires valid pointer words, so any 0x18-byte animation node
// with null pointers "looks like" a MOBJ), and this rule decides what counts as
// an animation node, so it has to match the validated reference exactly.
inline bool ref_class_ok(const hsd_detail::Reader& r, std::size_t at) {
    if (!r.has(at, 4)) return false;
    const u32 value = hsd_detail::be32(r.block, at);
    return value == 0 || std::binary_search(r.targets.begin(), r.targets.end(), value);
}

inline bool ref_finite(const hsd_detail::Reader& r, std::size_t at, unsigned count, f32 limit) {
    for (unsigned i = 0; i < count; ++i) {
        if (!r.has(at + std::size_t(i) * 4, 4)) return false;
        const f32 value = hsd_detail::be_f32(r.block, at + std::size_t(i) * 4);
        if (!std::isfinite(value) || std::fabs(value) > limit) return false;
    }
    return true;
}

inline bool ref_as_material(const hsd_detail::Reader& r, std::size_t at) {
    if (!r.has(at, 0x14)) return false;
    for (unsigned i = 0; i < 3; ++i) {
        const u32 low = hsd_detail::be32(r.block, at + i * 4) & 0xFFu;
        if (low != 0x00u && low != 0x40u && low != 0x80u && low != 0xC0u && low != 0xFFu) {
            return false;
        }
    }
    const f32 alpha = hsd_detail::be_f32(r.block, at + 0x0C);
    const f32 shininess = hsd_detail::be_f32(r.block, at + 0x10);
    if (!std::isfinite(alpha) || !std::isfinite(shininess)) return false;
    if (std::fabs(alpha) > 1.0e5f || std::fabs(shininess) > 1.0e5f) return false;
    return alpha >= 0.0f && alpha <= 2.0f;
}

inline bool ref_as_image_desc(const hsd_detail::Reader& r, std::size_t at) {
    if (!r.has(at, 0x18)) return false;
    u32 data = 0;
    if (!r.ptr(at, &data) || data == 0) return false;
    const u32 width = hsd_detail::be16(r.block, at + 4);
    const u32 height = hsd_detail::be16(r.block, at + 6);
    const u32 format = hsd_detail::be32(r.block, at + 8);
    const u32 mipmaps = hsd_detail::be32(r.block, at + 0x0C);
    if (width == 0 || height == 0 || width > 2048 || height > 2048) return false;
    switch (format) {
    case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 8: case 9: case 10: case 14:
        break;
    default:
        return false;
    }
    if (mipmaps > 16) return false;
    return ref_finite(r, at + 0x10, 2, 64.0f);
}

inline bool ref_as_tlut_desc(const hsd_detail::Reader& r, std::size_t at) {
    if (!r.has(at, 0x10)) return false;
    u32 data = 0;
    if (!r.ptr(at, &data) || data == 0) return false;
    const u32 format = hsd_detail::be32(r.block, at + 4);
    const u32 entries = hsd_detail::be16(r.block, at + 0x0C);
    return format <= 2 && entries >= 1 && entries <= 256;
}

inline bool ref_vtx_desc_list(const hsd_detail::Reader& r, u32 list) {
    if (list == 0) return false;
    for (std::size_t i = 0; i < 32; ++i) {
        const std::size_t at = std::size_t(list) + i * 0x18;
        if (!r.has(at, 0x18)) return false;
        const u32 attr = hsd_detail::be32(r.block, at);
        if (attr == 0xFF) return i > 0;
        const u32 type = hsd_detail::be32(r.block, at + 4);
        const u32 comp_count = hsd_detail::be32(r.block, at + 8);
        const u32 comp_type = hsd_detail::be32(r.block, at + 0x0C);
        if (attr > 25 || type > 3 || comp_count > 3 || comp_type > 5) return false;
        if (type == 2 || type == 3) {
            const u32 array = hsd_detail::be32(r.block, at + 0x14);
            if (array != 0 && !std::binary_search(r.targets.begin(), r.targets.end(), array)) {
                return false;
            }
        }
    }
    return false;
}

inline bool ref_as_pobj(const hsd_detail::Reader& r, std::size_t at) {
    if (!r.has(at, 0x18)) return false;
    if (!ref_class_ok(r, at)) return false;
    u32 sibling = 0, vtx = 0, disp = 0;
    if (!r.ptr(at + 4, &sibling)) return false;
    if (!r.ptr(at + 8, &vtx) || vtx == 0) return false;
    const u16 flags = hsd_detail::be16(r.block, at + 0x0C);
    const u16 n_disp = hsd_detail::be16(r.block, at + 0x0E);
    if ((flags & ~u16(0xF001)) != 0) return false; // retained bit0 in actual envelope descriptors
    if (n_disp == 0) return false;
    if (!r.ptr(at + 0x10, &disp) || disp == 0) return false;
    return ref_vtx_desc_list(r, vtx);
}

inline bool ref_as_tobj(const hsd_detail::Reader& r, std::size_t at) {
    if (!r.has(at, 0x5C)) return false;
    if (!ref_class_ok(r, at)) return false;
    u32 sibling = 0, image = 0, tlut = 0, lod = 0, tev = 0;
    if (!r.ptr(at + 4, &sibling)) return false;
    if (!ref_finite(r, at + 0x10, 3, 1.0e6f) || !ref_finite(r, at + 0x1C, 3, 1.0e6f) ||
        !ref_finite(r, at + 0x28, 3, 1.0e6f)) {
        return false;
    }
    const u32 wrap_s = hsd_detail::be32(r.block, at + 0x34);
    const u32 wrap_t = hsd_detail::be32(r.block, at + 0x38);
    if (wrap_s > 2 || wrap_t > 2) return false;
    if (r.block.data[at + 0x3C] == 0 || r.block.data[at + 0x3D] == 0) return false;
    if ((hsd_detail::be32(r.block, at + 0x40) & ~0x01FF01FFu) != 0) return false;
    if (!ref_finite(r, at + 0x44, 1, 1.0e4f)) return false;
    if (hsd_detail::be32(r.block, at + 0x48) > 5) return false;
    if (!r.ptr(at + 0x4C, &image) || image == 0) return false;
    if (!r.ptr(at + 0x50, &tlut) || !r.ptr(at + 0x54, &lod) || !r.ptr(at + 0x58, &tev)) {
        return false;
    }
    return true;
}

inline bool ref_as_mobj(const hsd_detail::Reader& r, std::size_t at) {
    if (!r.has(at, 0x18)) return false;
    if (!ref_class_ok(r, at)) return false;
    u32 tex = 0, material = 0, render = 0, pe = 0;
    if (!r.ptr(at + 8, &tex) || !r.ptr(at + 0x0C, &material)) return false;
    if (!r.ptr(at + 0x10, &render) || !r.ptr(at + 0x14, &pe)) return false;
    if (tex == 0 && material == 0) return false;
    if (material != 0 && !ref_as_material(r, material)) return false;
    if (tex != 0 && !ref_as_tobj(r, tex)) return false;
    return true;
}

inline bool ref_as_dobj(const hsd_detail::Reader& r, std::size_t at) {
    if (!r.has(at, 0x10)) return false;
    if (!ref_class_ok(r, at)) return false;
    u32 sibling = 0, mobj = 0, pobj = 0;
    if (!r.ptr(at + 4, &sibling) || !r.ptr(at + 8, &mobj) || !r.ptr(at + 0x0C, &pobj)) {
        return false;
    }
    if (mobj != 0 && ref_as_mobj(r, mobj)) return true;
    if (pobj != 0 && ref_as_pobj(r, pobj)) return true;
    return false;
}

inline bool ref_as_jobj(const hsd_detail::Reader& r, std::size_t at) {
    if (!r.has(at, 0x40)) return false;
    if (!ref_class_ok(r, at)) return false;
    const u32 flags = hsd_detail::be32(r.block, at + 4);
    if ((flags & ~0x73FFFFFFu) != 0) return false;
    u32 child = 0, sibling = 0, contents = 0, inverse = 0, robj = 0;
    if (!r.ptr(at + 0x08, &child) || !r.ptr(at + 0x0C, &sibling) || !r.ptr(at + 0x10, &contents)) {
        return false;
    }
    if (contents != 0 && !ref_as_dobj(r, contents)) return false;
    if (!ref_finite(r, at + 0x14, 3, 1.0e6f) || !ref_finite(r, at + 0x20, 3, 1.0e6f) ||
        !ref_finite(r, at + 0x2C, 3, 1.0e6f)) {
        return false;
    }
    for (unsigned i = 0; i < 3; ++i) {
        if (std::fabs(hsd_detail::be_f32(r.block, at + 0x20 + i * 4)) <= 1.0e-9f) return false;
    }
    if (!r.ptr(at + 0x38, &inverse) || !r.ptr(at + 0x3C, &robj)) return false;
    return true;
}

// True when the node at `at` also validates as a piece of scene geometry. Such a
// node cannot be an HSD_AnimJoint: those layouts contain pointer-shaped words
// and would otherwise be false animation nodes.
inline bool anim_classifies_as_scene(const hsd_detail::Reader& r, std::size_t at) {
    if (ref_as_jobj(r, at)) return true;
    if (ref_as_tobj(r, at)) return true;
    if (ref_as_mobj(r, at)) return true;
    if (ref_as_dobj(r, at)) return true;
    if (ref_as_pobj(r, at)) return true;
    if (ref_as_image_desc(r, at)) return true;
    if (ref_as_tlut_desc(r, at)) return true;
    return ref_as_material(r, at);
}

// One track descriptor: both pointer fields relocated, `length` inside the
// block, a real channel id and a finite start frame.
inline bool is_fobjdesc(const hsd_detail::Reader& r, std::size_t at) {
    if (!r.has(at, 0x14)) return false;
    u32 next = 0, ad = 0;
    if (!r.ptr(at, &next)) return false;
    if (!r.ptr(at + 0x10, &ad)) return false;
    const u32 length = hsd_detail::be32(r.block, at + 4);
    const f32 start = hsd_detail::be_f32(r.block, at + 8);
    const u8 kind = r.block.data[at + 0x0C];
    if (length > kMaxTrackBytes || kind == 0) return false;
    if (!std::isfinite(start) || !(start >= 0.0f && start <= 1.0e5f)) return false;
    if (ad == 0) return false;
    return r.has(ad, std::min<std::size_t>(length, kMaxTrackBytes));
}

// Follow an FObjDesc sibling chain. Fills `out` when given. Every node must
// validate; a chain that loops is refused rather than followed.
inline bool read_fobj_chain(const hsd_detail::Reader& r, u32 start,
                            std::vector<HsdAnimTrack>* out) {
    std::vector<u32> seen;
    u32 node = start;
    while (node != 0 && std::find(seen.begin(), seen.end(), node) == seen.end() &&
           seen.size() < kMaxAnimNodes) {
        if (!is_fobjdesc(r, node)) return false;
        seen.push_back(node);
        if (out) {
            HsdAnimTrack track;
            track.channel = r.block.data[node + 0x0C];
            track.startframe = hsd_detail::be_f32(r.block, node + 8);
            track.frac_value = r.block.data[node + 0x0D];
            track.frac_slope = r.block.data[node + 0x0E];
            const u32 ad = hsd_detail::be32(r.block, node + 0x10);
            const u32 length = hsd_detail::be32(r.block, node + 4);
            track.data.assign(r.block.data + ad, r.block.data + ad + length);
            out->push_back(std::move(track));
        }
        u32 next = 0;
        if (!r.ptr(node, &next)) return false;
        node = next;
    }
    return node == 0;
}

inline bool is_aobjdesc(const hsd_detail::Reader& r, std::size_t at) {
    if (!r.has(at, 0x10)) return false;
    const u32 flags = hsd_detail::be32(r.block, at);
    const f32 end = hsd_detail::be_f32(r.block, at + 4);
    u32 tracks = 0, obj = 0;
    if (!r.ptr(at + 8, &tracks) || !r.ptr(at + 0x0C, &obj)) return false;
    if (flags > 0xFFFFFu) return false;
    if (!std::isfinite(end) || !(end > 0.0f && end <= 1.0e6f)) return false;
    if (tracks == 0) return false;
    return read_fobj_chain(r, tracks, nullptr);
}

inline bool is_animjoint(const hsd_detail::Reader& r, std::size_t at,
                         const std::vector<u32>& aobjs) {
    if (!r.has(at, 0x14)) return false;
    u32 child = 0, next = 0, aobj = 0, robj = 0;
    if (!r.ptr(at, &child) || !r.ptr(at + 4, &next) || !r.ptr(at + 8, &aobj) ||
        !r.ptr(at + 0x0C, &robj)) {
        return false;
    }
    const u32 flags = hsd_detail::be32(r.block, at + 0x10);
    if (flags > 0xFFFFu) return false;
    if (aobj != 0 && !std::binary_search(aobjs.begin(), aobjs.end(), aobj)) return false;
    return !anim_classifies_as_scene(r, at);
}

// The relocation table carries no struct sizes, so a node is a candidate only
// when the next relocation target sits exactly one node away: the same span
// heuristic hsd.hpp uses for its structures.
inline std::size_t target_span(const std::vector<u32>& ordered, std::size_t at,
                               std::size_t block_size) {
    const auto it = std::upper_bound(ordered.begin(), ordered.end(), u32(at));
    const std::size_t next = it == ordered.end() ? block_size : std::size_t(*it);
    return next > at ? next - at : 0;
}

inline bool read_spline(const hsd_detail::Reader& r, u32 at, HsdAnimSpline* out) {
    if (!r.has(at, 0x18)) return false;
    const u8 kind = r.block.data[at];
    const u16 numcv = hsd_detail::be16(r.block, at + 2);
    const f32 tension = hsd_detail::be_f32(r.block, at + 4);
    u32 cv = 0, seglen = 0, segpoly = 0;
    if (!r.ptr(at + 8, &cv) || !r.ptr(at + 0x10, &seglen) || !r.ptr(at + 0x14, &segpoly)) {
        return false;
    }
    const f32 total = hsd_detail::be_f32(r.block, at + 0x0C);
    if (kind > 3 || numcv == 0 || numcv > kMaxSplinePoints) return false;
    if (!std::isfinite(tension) || !std::isfinite(total)) return false;
    if (cv == 0 || seglen == 0 || segpoly == 0) return false;
    const u32 extra = kind != 0 ? 2u : 0u;
    if (!r.has(cv, std::size_t(numcv + extra) * 12)) return false;
    if (!r.has(seglen, std::size_t(numcv) * 4)) return false;
    if (numcv > 1 && !r.has(segpoly, std::size_t(numcv - 1) * 20)) return false;
    if (kind != 0 && numcv < 2) return false;   // 4-point templates need 2 cv
    out->valid = true;
    out->type = kind;
    out->numcv = numcv;
    out->tension = f64(tension);
    out->total_length = f64(total);
    out->points.resize(std::size_t(numcv + extra) * 3);
    for (std::size_t i = 0; i < std::size_t(numcv + extra); ++i) {
        for (unsigned k = 0; k < 3; ++k) {
            out->points[i * 3 + k] =
                f64(hsd_detail::be_f32(r.block, std::size_t(cv) + i * 12 + k * 4));
        }
    }
    out->seg_length.resize(numcv);
    for (std::size_t i = 0; i < numcv; ++i) {
        out->seg_length[i] = f64(hsd_detail::be_f32(r.block, std::size_t(seglen) + i * 4));
    }
    if (numcv > 1) {
        out->seg_poly.resize(std::size_t(numcv - 1) * 5);
        for (std::size_t i = 0; i < out->seg_poly.size(); ++i) {
            out->seg_poly[i] = f64(hsd_detail::be_f32(r.block, std::size_t(segpoly) + i * 4));
        }
    }
    return true;
}

// Opens the container through the same validation hsd.hpp applies, exposing the
// block and relocation targets the animation scan needs. This repeats the
// header checks (hsd_load publishes only the decoded scene, not its reader) but
// not the scene decode: no structure below is interpreted here.
inline bool open_container(ByteView archive, hsd_detail::Reader* out, std::string* error) {
    const auto fail = [error](std::string message) {
        if (error) *error = std::move(message);
        return false;
    };
    if (!archive.data || archive.size < kHsdHeaderBytes) {
        return fail("hsd anim: buffer shorter than the 32-byte archive header");
    }
    const u32 file_size = hsd_detail::be32(archive, 0);
    const u32 data_size = hsd_detail::be32(archive, 4);
    const u32 reloc_count = hsd_detail::be32(archive, 8);
    const u32 public_count = hsd_detail::be32(archive, 12);
    const u32 extern_count = hsd_detail::be32(archive, 16);
    if (file_size < kHsdHeaderBytes || std::size_t(file_size) > archive.size) {
        return fail("hsd anim: invalid archive size");
    }
    if (hsd_detail::be32(archive, 24) != 0 || hsd_detail::be32(archive, 28) != 0) {
        return fail("hsd anim: unsupported nonzero reserved header words");
    }
    for (std::size_t i = file_size; i < archive.size; ++i) {
        if (archive.data[i] != 0) return fail("hsd anim: nonzero bytes after the declared archive");
    }
    if (data_size == 0 || std::size_t(kHsdHeaderBytes) + data_size > file_size) {
        return fail("hsd anim: invalid data block size");
    }
    if (reloc_count > kMaxRelocations) return fail("hsd anim: relocation count exceeds the cap");

    hsd_detail::Reader reader;
    reader.block = ByteView{archive.data + kHsdHeaderBytes, data_size};
    const std::size_t reloc_at = kHsdHeaderBytes + data_size;
    if (reloc_at + std::size_t(reloc_count) * 4 > file_size) {
        return fail("hsd anim: relocation table runs past the archive");
    }
    const std::size_t symbol_at = reloc_at + std::size_t(reloc_count) * 4;
    if (symbol_at + (std::size_t(public_count) + extern_count) * 8 > file_size) {
        return fail("hsd anim: symbol table runs past the archive");
    }
    reader.targets.reserve(std::size_t(reloc_count) + public_count);
    for (u32 i = 0; i < reloc_count; ++i) {
        const u32 location = hsd_detail::be32(archive, reloc_at + std::size_t(i) * 4);
        if ((location & 3u) != 0 || u64(location) + 4 > data_size) {
            return fail("hsd anim: relocation field outside the data block");
        }
        const u32 target = hsd_detail::be32(reader.block, location);
        if (target >= data_size) return fail("hsd anim: relocation target outside the data block");
        reader.targets.push_back(target);
        reader.fields.push_back(location);
    }
    std::sort(reader.fields.begin(), reader.fields.end());
    if (std::adjacent_find(reader.fields.begin(), reader.fields.end()) != reader.fields.end())
        return fail("hsd anim: duplicate relocation field");
    // Public symbol offsets are legitimate structure starts in the reference too.
    for (u32 i = 0; i < public_count; ++i) {
        const u32 offset = hsd_detail::be32(archive, symbol_at + std::size_t(i) * 8);
        if (offset >= data_size) return fail("hsd anim: public symbol outside the data block");
        reader.targets.push_back(offset);
    }
    std::sort(reader.targets.begin(), reader.targets.end());
    reader.targets.erase(std::remove(reader.targets.begin(), reader.targets.end(), 0u),
                         reader.targets.end());
    reader.targets.erase(std::unique(reader.targets.begin(), reader.targets.end()),
                         reader.targets.end());
    *out = std::move(reader);
    return true;
}

// Preorder JOBJ offsets, recovered so animation-tree ties can be ordered the way
// the reference orders them (by block offset). False when the archive has no
// decodable JOBJ at all.
inline bool jobj_preorder(const hsd_detail::Reader& r, std::vector<u32>* out) {
    std::vector<u32> jobjs;
    for (u32 offset : r.targets) {
        if (hsd_detail::as_jobj(r, offset)) jobjs.push_back(offset);
    }
    if (jobjs.empty()) return false;
    std::vector<u32> referenced;
    for (u32 offset : jobjs) {
        u32 child = 0, sibling = 0;
        if (r.ptr(offset + 0x08, &child) && child) referenced.push_back(child);
        if (r.ptr(offset + 0x0C, &sibling) && sibling) referenced.push_back(sibling);
    }
    std::sort(referenced.begin(), referenced.end());
    std::vector<u32> roots;
    for (u32 offset : jobjs) {
        if (!std::binary_search(referenced.begin(), referenced.end(), offset)) roots.push_back(offset);
    }
    if (roots.empty()) roots = jobjs;

    std::vector<u32> stack;
    for (auto it = roots.rbegin(); it != roots.rend(); ++it) stack.push_back(*it);
    std::vector<u32> visited;
    while (!stack.empty()) {
        const u32 offset = stack.back();
        stack.pop_back();
        if (std::find(visited.begin(), visited.end(), offset) != visited.end()) return false;
        visited.push_back(offset);
        out->push_back(offset);
        std::vector<u32> children;
        u32 child = 0;
        if (r.ptr(offset + 0x08, &child)) {
            std::size_t guard = 0;
            while (child && guard++ < kHsdMaxNodes) {
                children.push_back(child);
                u32 next = 0;
                if (!r.ptr(child + 0x0C, &next)) break;
                child = next;
            }
        }
        for (auto it = children.rbegin(); it != children.rend(); ++it) stack.push_back(*it);
    }
    return !out->empty();
}

// ---- evaluation helpers ----------------------------------------------

inline bool sample_track(const HsdAnimTrack& track, u32 frame, f32* value, std::string* error);

// Last track of `channel` wins, matching the reference's per-channel table.
inline bool sample_channel(const std::vector<HsdAnimTrack>& tracks, u32 channel, u32 frame,
                           f32* value, bool* found, std::string* error) {
    *found = false;
    for (const HsdAnimTrack& track : tracks) {
        if (track.channel != channel) continue;
        if (!sample_track(track, frame, value, error)) return false;
        *found = true;
    }
    return true;
}

// Depth of every joint in the (preorder) scene joint list.
inline std::vector<u32> joint_depths(const HsdScene& scene) {
    std::vector<u32> depth(scene.joints.size(), 0);
    for (std::size_t i = 0; i < scene.joints.size(); ++i) {
        const u32 parent = scene.joints[i].parent;
        if (parent == kHsdNoJoint || std::size_t(parent) >= i) {
            depth[i] = 0;
        } else {
            depth[i] = depth[parent] + 1;
        }
    }
    return depth;
}

// End of the sibling-inclusive preorder chain starting at `start`: the subtree
// of `start` plus every following sibling's subtree.
inline std::size_t chain_end(const std::vector<u32>& depth, std::size_t start) {
    const u32 base = depth[start];
    std::size_t index = start + 1;
    while (index < depth.size()) {
        // A scene root has no siblings (its nextSibling field is null), so its
        // chain is its own subtree: it ends at the next root. A joint below a
        // root does have a sibling chain, so it ends only when the preorder
        // leaves the parent's child list.
        const bool stop = base == 0 ? depth[index] <= base : depth[index] < base;
        if (stop) break;
        ++index;
    }
    return index;
}

// Two preorder sequences with the same relative depth sequence describe the same
// ordered tree, so that is the whole shape test. The window must also be a
// complete chain, which is what the reference's subtree walk enumerates.
inline bool shape_matches(const std::vector<u32>& depth, std::size_t start,
                          const std::vector<HsdAnimNode>& nodes) {
    if (nodes.empty()) return false;
    if (start + nodes.size() > depth.size()) return false;
    if (chain_end(depth, start) != start + nodes.size()) return false;
    const u32 base = depth[start];
    for (std::size_t k = 0; k < nodes.size(); ++k) {
        if (depth[start + k] - base != nodes[k].depth) return false;
    }
    return true;
}

inline bool has_channel(const HsdAnimTree& tree, u32 channel) {
    return std::binary_search(tree.channels.begin(), tree.channels.end(), channel);
}

// Count of nodes whose first scale-X key equals the authored scale: the content
// tie-break that separates two roots with an identical tree shape.
inline u32 initial_scale_matches(const HsdAnimTree& tree, const HsdScene& scene,
                                 std::size_t start) {
    u32 score = 0;
    for (std::size_t k = 0; k < tree.nodes.size(); ++k) {
        const std::size_t joint = start + k;
        if (joint >= scene.joints.size()) break;
        for (const HsdAnimTrack& track : tree.nodes[k].tracks) {
            if (track.channel != 8) continue;
            f32 value = 0.0f;
            std::string ignored;
            if (!sample_track(track, 1, &value, &ignored)) continue;
            if (std::fabs(value - scene.joints[joint].scale[0]) < 1.0e-4f) ++score;
        }
    }
    return score;
}

inline bool camera_pose(const HsdCameraAnim& camera, u32 frame, HsdAnimPose* out,
                        std::string* error) {
    out->has_camera = true;
    for (unsigned k = 0; k < 3; ++k) {
        out->camera_eye[k] = camera.static_eye[k];
        out->camera_interest[k] = camera.static_interest[k];
    }
    out->camera_fov_y_degrees = camera.fov;
    out->camera_near = camera.near_plane;
    out->camera_far = camera.far_plane;

    const auto place = [&](const HsdAnimSpline& spline, const std::vector<HsdAnimTrack>& tracks,
                           f32* target) -> bool {
        f32 path = 0.0f;
        bool found = false;
        if (!sample_channel(tracks, 4, frame, &path, &found, error)) return false;
        if (found && spline.valid) {
            f64 point[3] = {0.0, 0.0, 0.0};
            if (!spline_arc_point(spline, f64(path), point)) {
                if (error) *error = "hsd anim: camera spline evaluation failed";
                return false;
            }
            for (unsigned k = 0; k < 3; ++k) target[k] = f32(point[k]);
            return true;
        }
        // No spline: a window-object animation can still move the point with its
        // own translation channels (HSD_A_W_TX/TY/TZ = 5/6/7).
        for (unsigned axis = 0; axis < 3; ++axis) {
            f32 value = 0.0f;
            bool axis_found = false;
            if (!sample_channel(tracks, 5 + axis, frame, &value, &axis_found, error)) return false;
            if (axis_found) target[axis] = value;
        }
        return true;
    };
    if (!place(camera.eye_spline, camera.eye, out->camera_eye)) return false;
    if (!place(camera.interest_spline, camera.interest, out->camera_interest)) return false;

    f32 value = 0.0f;
    bool found = false;
    if (!sample_channel(camera.base, 10, frame, &value, &found, error)) return false;
    if (found) out->camera_fov_y_degrees = value;
    if (!sample_channel(camera.base, 11, frame, &value, &found, error)) return false;
    if (found) out->camera_near = value;
    if (!sample_channel(camera.base, 12, frame, &value, &found, error)) return false;
    if (found) out->camera_far = value;
    return true;
}

} // namespace hsd_anim_detail

// ---- public API -------------------------------------------------------

// A decoded track's byte stream, for direct decoding (tests and diagnostics).
struct HsdAnimTrackView {
    ByteView data{};
    u8 frac_value = 0;
    u8 frac_slope = 0;
    f32 startframe = 0.0f;
};

// Value of one track after `frame` interpret calls (frame 0 = no update). False
// with a message when the byte stream is malformed.
inline bool hsd_anim_sample_track(const HsdAnimTrackView& view, u32 frame, f32* value,
                                  std::string* error) {
    if (!value) {
        if (error) *error = "hsd anim: no sample target";
        return false;
    }
    if (!view.data.data || view.data.size == 0) {
        if (error) *error = "hsd anim: empty track stream";
        return false;
    }
    if (frame > hsd_anim_detail::kMaxFrame) {
        if (error) *error = "hsd anim: frame exceeds the evaluation cap";
        return false;
    }
    hsd_anim_detail::TrackMachine machine;
    machine.data = view.data.data;
    machine.length = view.data.size;
    machine.frac_value = view.frac_value;
    machine.frac_slope = view.frac_slope;
    machine.time = f64(view.startframe);
    for (u32 step = 0; step < frame; ++step) {
        if (!hsd_anim_detail::track_step(machine, error)) return false;
    }
    if(machine.stopped){if(error)*error="hsd anim: unknown packed-track opcode";return false;}
    *value = f32(machine.value);
    return true;
}

// Evaluate a requested integer time, including the initial key at time zero.
// This is NOT the legacy API's count of interpreter calls. HSD_AObjReqAnim sets
// FIRST_PLAY; the first interpretation then uses rate zero at startframe+time.
// Keep the old no-update frame-zero contract above.
namespace hsd_anim_detail {
inline bool sample_requested_track(const HsdAnimTrackView& view,f64 time,bool float_clock,
                                   f32* value,std::string* error){
    auto fail=[&](const char* why){if(error)*error=std::string("hsd requested track: ")+why;return false;};
    if(!value||!view.data.data||!view.data.size)return fail("missing output/stream");
    if(view.data.size>hsd_anim_detail::kMaxTrackBytes)return fail("stream exceeds byte budget");
    if(!std::isfinite(time)||time<0||time>hsd_anim_detail::kMaxFrame||!std::isfinite(view.startframe)||
       std::fabs(view.startframe)>hsd_anim_detail::kMaxFrame)return fail("time/startframe outside budget");
    hsd_anim_detail::TrackMachine machine;
    machine.data=view.data.data;machine.length=view.data.size;
    machine.frac_value=view.frac_value;machine.frac_slope=view.frac_slope;
    machine.float_time=float_clock;
    machine.time=float_clock?f64(f32(view.startframe+f32(time))):f64(view.startframe)+time;
    if(!hsd_anim_detail::track_step(machine,error,0.0))return false;
    if(machine.stopped)return fail("unknown packed-track opcode");
    const f32 sampled=f32(machine.value);
    if(!std::isfinite(sampled))return fail("nonfinite sample");
    *value=sampled;return true;
}
} // namespace hsd_anim_detail
inline bool hsd_anim_sample_requested_track(const HsdAnimTrackView& view,u32 time,
                                          f32* value,std::string* error){
    return hsd_anim_detail::sample_requested_track(view,time,false,value,error);
}
// Explicit fractional FIRST_PLAY request, matching retained-player f32 clock
// boundaries. Integer callers keep the original entry's clock semantics.
inline bool hsd_anim_sample_requested_track_fractional(const HsdAnimTrackView& view,f32 time,
                                                     f32* value,std::string* error){
    return hsd_anim_detail::sample_requested_track(view,time,true,value,error);
}

namespace hsd_anim_detail {

inline bool sample_track(const HsdAnimTrack& track, u32 frame, f32* value, std::string* error) {
    HsdAnimTrackView view;
    view.data = ByteView{track.data.data(), track.data.size()};
    view.frac_value = track.frac_value;
    view.frac_slope = track.frac_slope;
    view.startframe = track.startframe;
    return hsd_anim_sample_track(view, frame, value, error);
}

} // namespace hsd_anim_detail

// Highest frame over all tracks: the archive's authored clip length, 0 when the
// archive carries no animation.
inline f32 hsd_anim_end_frame(const HsdAnimation& anim) { return anim.end_frame; }

// Decodes every joint animation tree and the camera animation of an HSD
// archive. Succeeds with zero trees when the archive has no animation; fails
// with a message when the container or a descriptor is malformed. Nothing is
// published on failure.
inline bool hsd_anim_load(ByteView archive, HsdAnimation* out, std::string* error) {
    using namespace hsd_anim_detail;
    if (!out) {
        if (error) *error = "hsd anim: no animation target";
        return false;
    }
    *out = HsdAnimation{};
    const auto fail = [out, error](std::string message) {
        *out = HsdAnimation{};
        if (error) *error = std::move(message);
        return false;
    };

    hsd_detail::Reader reader;
    if (!open_container(archive, &reader, error)) return false;

    // 1. Every HSD_AObjDesc the relocation graph reaches. The archive's clip
    //    length is the maximum end_frame over them (all 69 agree in tl00).
    std::vector<u32> aobjs;
    for (u32 offset : reader.targets) {
        if (!is_aobjdesc(reader, offset)) continue;
        aobjs.push_back(offset);
        out->aobj_end_frames.push_back(hsd_detail::be_f32(reader.block, offset + 4));
    }
    if (aobjs.size() > kMaxAobjDesc) return fail("hsd anim: AObjDesc count exceeds the cap");
    out->aobj_count = static_cast<u32>(aobjs.size());
    std::sort(aobjs.begin(), aobjs.end());
    std::sort(out->aobj_end_frames.begin(), out->aobj_end_frames.end());
    if (!out->aobj_end_frames.empty()) out->end_frame = out->aobj_end_frames.back();

    // 2. HSD_AnimJoint nodes: 0x14 bytes each, accepted only when the next
    //    relocation target is exactly one node away and every field validates.
    struct AnimNode {
        u32 child = 0;
        u32 next = 0;
        u32 aobj = 0;
    };
    std::vector<u32> node_offsets;
    std::vector<AnimNode> nodes;
    std::vector<u32> declared_roots;
    HsdScene declared_scene;
    std::string scene_error;
    if (hsd_load(archive,&declared_scene,&scene_error))
        for(const auto& binding:declared_scene.animation_roots) declared_roots.push_back(binding.first);
    std::vector<u32> declared_nodes;
    std::vector<u32> discover=declared_roots;
    while(!discover.empty()) {
        const u32 node=discover.back();discover.pop_back();
        if(std::find(declared_nodes.begin(),declared_nodes.end(),node)!=declared_nodes.end()) continue;
        if(declared_nodes.size()>=kMaxAnimNodes || !reader.has(node,0x14)) return fail("hsd anim: invalid declared animation node");
        u32 child=0,next=0,aobj=0,robj=0;
        if(!reader.ptr(node,&child)||!reader.ptr(node+4,&next)||!reader.ptr(node+8,&aobj)||!reader.ptr(node+12,&robj))
            return fail("hsd anim: invalid declared animation pointer");
        if(aobj && !is_aobjdesc(reader,aobj)) return fail("hsd anim: invalid declared AObj");
        declared_nodes.push_back(node);
        if(child)discover.push_back(child);
        if(next)discover.push_back(next);
    }
    for (u32 offset : reader.targets) {
        if(!declared_roots.empty()) {
            if(std::find(declared_nodes.begin(),declared_nodes.end(),offset)==declared_nodes.end())continue;
        } else {
            if (target_span(reader.targets, offset, reader.block.size) != 0x14) continue;
            if (!is_animjoint(reader, offset, aobjs)) continue;
        }
        AnimNode node;
        (void)reader.ptr(offset, &node.child);
        (void)reader.ptr(offset + 4, &node.next);
        (void)reader.ptr(offset + 8, &node.aobj);
        node_offsets.push_back(offset);
        nodes.push_back(node);
    }
    if (nodes.size() > kMaxAnimNodes) return fail("hsd anim: AnimJoint count exceeds the cap");
    const auto node_index = [&](u32 offset) -> std::size_t {
        const auto it = std::lower_bound(node_offsets.begin(), node_offsets.end(), offset);
        if (it == node_offsets.end() || *it != offset) return node_offsets.size();
        return std::size_t(it - node_offsets.begin());
    };

    // 3. Tree roots are the nodes nothing else points at; each tree is walked in
    //    preorder (a node, then its children, then its next sibling).
    std::vector<u32> referenced;
    for (const AnimNode& node : nodes) {
        if (node.child) referenced.push_back(node.child);
        if (node.next) referenced.push_back(node.next);
    }
    std::sort(referenced.begin(), referenced.end());
    for (u32 root : node_offsets) {
        if (std::binary_search(referenced.begin(), referenced.end(), root)) continue;
        HsdAnimTree tree;
        tree.archive_offset = root;
        std::vector<u32> visited;
        std::vector<std::pair<u32, u32>> stack;   // (node offset, depth)
        stack.push_back({root, 0});
        while (!stack.empty()) {
            const auto entry = stack.back();
            stack.pop_back();
            const std::size_t index = node_index(entry.first);
            if (index >= nodes.size() || entry.second>=kMaxDepth) return fail("hsd anim: invalid node/depth in declared tree");
            if (std::find(visited.begin(), visited.end(), entry.first) != visited.end()) {
                return fail("hsd anim: an animation tree visits one node twice");
            }
            visited.push_back(entry.first);
            HsdAnimNode decoded;
            decoded.depth = entry.second;
            if (nodes[index].aobj != 0) {
                u32 tracks = 0;
                if (!reader.ptr(std::size_t(nodes[index].aobj) + 8, &tracks)) {
                    return fail("hsd anim: AObjDesc track pointer is not valid");
                }
                if (!read_fobj_chain(reader, tracks, &decoded.tracks)) {
                    return fail("hsd anim: a track chain no longer validates");
                }
                for (const HsdAnimTrack& track : decoded.tracks) {
                    tree.channels.push_back(track.channel);
                }
                if(std::find(tree.channels.begin(),tree.channels.end(),4u)!=tree.channels.end()) {
                    u32 object=0,spline=0;
                    if(reader.ptr(nodes[index].aobj+12,&object) && object && reader.has(object,0x14) &&
                       (hsd_detail::be32(reader.block,object+4)&0x4000u) && reader.ptr(object+16,&spline) && spline){
                        HsdAnimSpline path;
                        if(!read_spline(reader,spline,&path))return fail("hsd anim: invalid joint path spline");
                        decoded.path_spline=static_cast<u32>(out->joint_splines.size());out->joint_splines.push_back(std::move(path));
                    }
                }
            }
            tree.nodes.push_back(std::move(decoded));
            // The stack pops the last entry first, so push the sibling before the
            // child to keep preorder.
            if (nodes[index].next != 0 && entry.second < kMaxDepth) {
                stack.push_back({nodes[index].next, entry.second});
            }
            if (nodes[index].child != 0 && entry.second + 1 < kMaxDepth) {
                stack.push_back({nodes[index].child, entry.second + 1});
            } else if(nodes[index].child!=0) {
                return fail("hsd anim: animation tree exceeds depth cap");
            }
        }
        std::sort(tree.channels.begin(), tree.channels.end());
        tree.channels.erase(std::unique(tree.channels.begin(), tree.channels.end()),
                            tree.channels.end());
        const bool any_tracks = !tree.channels.empty();
        if (!any_tracks) continue;   // pass-through fragments are not a tree
        out->trees.push_back(std::move(tree));
    }
    out->anim_node_count = static_cast<u32>(nodes.size());

    // Material animation is attached by the published root record, then follows
    // the same child/sibling and DOBJ order as the model. No shape-scoring here.
    for(const auto& binding:declared_scene.material_animation_roots) {
        std::vector<std::pair<u32,u32>> work{{binding.first,binding.second}};
        std::vector<u32> seen;
        while(!work.empty()) {
            const auto pair=work.back();work.pop_back();
            const u32 node=pair.first,jobj=pair.second;
            if(!reader.has(node,12)||!reader.has(jobj,0x40)||seen.size()>=kMaxAnimNodes ||
                std::find(seen.begin(),seen.end(),node)!=seen.end()) return fail("hsd anim: invalid material joint tree");
            seen.push_back(node);
            u32 child=0,next=0,mat=0,jchild=0,jnext=0,dobj=0;
            if(!reader.ptr(node,&child)||!reader.ptr(node+4,&next)||!reader.ptr(node+8,&mat)||
               !reader.ptr(jobj+8,&jchild)||!reader.ptr(jobj+12,&jnext)||!reader.ptr(jobj+16,&dobj))
                return fail("hsd anim: invalid material joint pointers");
            if(child){if(!jchild)return fail("hsd anim: material child shape differs");work.push_back({child,jchild});}
            if(next){if(!jnext)return fail("hsd anim: material sibling shape differs");work.push_back({next,jnext});}
            unsigned guard=0;
            while(mat) {
                if(++guard>256 || !reader.has(mat,16)||!reader.has(dobj,16))return fail("hsd anim: material chain invalid");
                u32 aobj=0,mobj=0,following=0,dnext=0;
                if(!reader.ptr(mat+4,&aobj)||!reader.ptr(mat,&following)||!reader.ptr(dobj+8,&mobj)||!reader.ptr(dobj+4,&dnext))
                    return fail("hsd anim: material pointer invalid");
                if(aobj) {
                    u32 fobj=0;
                    if(!reader.ptr(aobj+8,&fobj))return fail("hsd anim: material AObj invalid");
                    HsdAnimation::MaterialTracks item;item.mobj=mobj;
                    if(!read_fobj_chain(reader,fobj,&item.tracks))return fail("hsd anim: invalid material tracks");
                    out->materials.push_back(std::move(item));
                }
                mat=following;dobj=dnext;
            }
        }
    }

    // 4. The camera animation. Two layouts occur: the scene record points
    //    straight at the HSD_CameraAnim, or at a small {camera_anim, null}
    //    wrapper. The descriptor itself is validated as a real camera first.
    const auto is_camera_anim = [&](u32 offset) -> bool {
        if (offset == 0 || !reader.has(offset, 0x0C)) return false;
        u32 base = 0;
        if (!reader.ptr(offset, &base)) return false;
        if (!std::binary_search(aobjs.begin(), aobjs.end(), base)) return false;
        for (unsigned field = 4; field <= 8; field += 4) {
            u32 wobj = 0;
            if (!reader.ptr(offset + field, &wobj)) return false;
            if (wobj == 0) return false;
            u32 aobj = 0, robj = 0;
            if (!reader.ptr(wobj, &aobj)) return false;
            if (!std::binary_search(aobjs.begin(), aobjs.end(), aobj)) return false;
            if (!reader.ptr(std::size_t(wobj) + 4, &robj)) return false;
        }
        return true;
    };
    struct CameraDesc {
        u32 desc = 0;
        u32 eyepos = 0;
        u32 interest = 0;
        f32 near_plane = 0.0f;
        f32 far_plane = 0.0f;
        f32 fov = 0.0f;
    };
    const auto valid_desc = [&](u32 offset, CameraDesc* desc) -> bool {
        if (offset == 0 || !reader.has(offset, 0x38)) return false;
        const u16 projection = hsd_detail::be16(reader.block, std::size_t(offset) + 6);
        if (projection < 1 || projection > 3) return false;
        u32 eyepos = 0, interest = 0;
        if (!reader.ptr(std::size_t(offset) + 0x18, &eyepos)) return false;
        if (!reader.ptr(std::size_t(offset) + 0x1C, &interest)) return false;
        if (eyepos == 0 || interest == 0) return false;
        if (!reader.has(eyepos, 0x14) || !reader.has(interest, 0x14)) return false;
        const f32 near_plane = hsd_detail::be_f32(reader.block, std::size_t(offset) + 0x28);
        const f32 far_plane = hsd_detail::be_f32(reader.block, std::size_t(offset) + 0x2C);
        const f32 fov = hsd_detail::be_f32(reader.block, std::size_t(offset) + 0x30);
        const f32 aspect = hsd_detail::be_f32(reader.block, std::size_t(offset) + 0x34);
        const f32 roll = hsd_detail::be_f32(reader.block, std::size_t(offset) + 0x20);
        if (!std::isfinite(near_plane) || !std::isfinite(far_plane) || !std::isfinite(fov) ||
            !std::isfinite(aspect) || !std::isfinite(roll)) {
            return false;
        }
        if (!(near_plane > 0.0f && near_plane < far_plane)) return false;
        if (!(aspect > 0.0f && aspect <= 16.0f)) return false;
        if (!(fov > 0.0f && fov < 180.0f)) return false;
        desc->desc = offset;
        desc->eyepos = eyepos;
        desc->interest = interest;
        desc->near_plane = near_plane;
        desc->far_plane = far_plane;
        desc->fov = fov;
        return true;
    };

    for (u32 entry : reader.targets) {
        u32 first = 0, second = 0;
        const bool have_first = reader.ptr(entry, &first);
        const bool have_second = reader.ptr(std::size_t(entry) + 4, &second);
        CameraDesc desc;
        for (unsigned order = 0; order < 2; ++order) {
            const u32 candidate_desc = order == 0 ? first : second;
            const u32 candidate_anim = order == 0 ? second : first;
            if (order == 0 ? !have_first : !have_second) continue;
            if (!valid_desc(candidate_desc, &desc)) continue;
            u32 canim = 0;
            if (is_camera_anim(candidate_anim)) {
                canim = candidate_anim;
            } else if (reader.ptr(candidate_anim, &canim) && is_camera_anim(canim)) {
                // wrapper: the inner HSD_CameraAnim
            } else {
                continue;
            }
            HsdCameraAnim camera;
            camera.present = true;
            u32 base = 0, eye_anim = 0, interest_anim = 0;
            (void)reader.ptr(canim, &base);
            (void)reader.ptr(std::size_t(canim) + 4, &eye_anim);
            (void)reader.ptr(std::size_t(canim) + 8, &interest_anim);
            u32 tracks = 0;
            if (reader.ptr(std::size_t(base) + 8, &tracks)) {
                if (!read_fobj_chain(reader, tracks, &camera.base)) {
                    return fail("hsd anim: camera track chain is malformed");
                }
            }
            // Each WObj animation points at its own AObjDesc, which owns the
            // chain of track descriptors (a second hop from the CameraAnim).
            for (unsigned side = 0; side < 2; ++side) {
                const u32 wobj_anim = side == 0 ? eye_anim : interest_anim;
                std::vector<HsdAnimTrack>* tracks_out = side == 0 ? &camera.eye : &camera.interest;
                u32 aobjdesc = 0;
                if (!reader.ptr(std::size_t(wobj_anim), &aobjdesc) || aobjdesc == 0) continue;
                if (!reader.has(std::size_t(aobjdesc), 0x10)) {
                    return fail("hsd anim: camera window-object AObjDesc is out of range");
                }
                if (!reader.ptr(std::size_t(aobjdesc) + 8, &tracks) || tracks == 0) {
                    return fail("hsd anim: camera window-object has no track chain");
                }
                if (!read_fobj_chain(reader, tracks, tracks_out)) {
                    return fail("hsd anim: camera window-object track chain is malformed");
                }
            }
            // The spline lives on the descriptor named by each WObj animation's
            // AObjDesc.obj_id (HSD loads it as a spline joint); the JOBJ_SPLINE
            // flag at +4 selects the spline field at +0x10.
            const auto spline_of = [&](u32 wobj_anim, HsdAnimSpline* spline) {
                u32 aobjdesc = 0, object = 0;
                if (!reader.ptr(wobj_anim, &aobjdesc) || aobjdesc == 0) return;
                if (!reader.ptr(std::size_t(aobjdesc) + 0x0C, &object) || object == 0) return;
                if ((hsd_detail::be32(reader.block, std::size_t(object) + 4) & 0x4000u) == 0) return;
                u32 spline_ptr = 0;
                if (!reader.ptr(std::size_t(object) + 0x10, &spline_ptr) || spline_ptr == 0) return;
                (void)read_spline(reader, spline_ptr, spline);
            };
            spline_of(eye_anim, &camera.eye_spline);
            spline_of(interest_anim, &camera.interest_spline);
            for (unsigned k = 0; k < 3; ++k) {
                camera.static_eye[k] = hsd_detail::be_f32(reader.block, std::size_t(desc.eyepos) + 4 + k * 4);
                camera.static_interest[k] =
                    hsd_detail::be_f32(reader.block, std::size_t(desc.interest) + 4 + k * 4);
            }
            camera.fov = desc.fov;
            camera.near_plane = desc.near_plane;
            camera.far_plane = desc.far_plane;
            out->camera = std::move(camera);
            break;
        }
        if (out->camera.present) break;
    }

    // 5. JOBJ preorder offsets: evidence for the shape-match tie-break only.
    out->joint_offsets.clear();
    (void)jobj_preorder(reader, &out->joint_offsets);
    return true;
}

// Evaluates every mapped joint animation and the camera animation at `frame`.
// The returned pose is parallel to `scene.joints`; joints no tree maps to keep
// the file's SRT. Fails with a message on a malformed track, publishing nothing.
inline bool hsd_anim_evaluate(const HsdAnimation& anim, const HsdScene& scene, f32 frame,
                              HsdAnimPose* out, std::string* error) {
    using namespace hsd_anim_detail;
    if (!out) {
        if (error) *error = "hsd anim: no pose target";
        return false;
    }
    if (!(frame >= 0.0f) || frame > f32(kMaxFrame)) {
        if (error) *error = "hsd anim: frame outside the evaluatable range";
        return false;
    }
    const u32 frame_index = u32(frame);

    HsdAnimPose pose;
    pose.joints.resize(scene.joints.size());
    for (std::size_t i = 0; i < scene.joints.size(); ++i) {
        for (unsigned k = 0; k < 3; ++k) {
            pose.joints[i].rotation[k] = scene.joints[i].rotation[k];
            pose.joints[i].scale[k] = scene.joints[i].scale[k];
            pose.joints[i].translation[k] = scene.joints[i].translation[k];
        }
        pose.joints[i].hidden = (scene.joints[i].flags & kHiddenFlag) != 0;
    }

    const std::vector<u32> depth = joint_depths(scene);
    const bool have_offsets = anim.joint_offsets.size() == scene.joints.size();

    for (const HsdAnimTree& tree : anim.trees) {
        u32 bound_root=0;
        if(tree.archive_offset && !scene.animation_roots.empty()) {
            for(const auto& binding:scene.animation_roots) if(binding.first==tree.archive_offset) {
                if(bound_root && bound_root!=binding.second) {
                    if(error)*error="hsd anim: animation shared by multiple roots requires explicit instances";
                    return false;
                }
                bound_root=binding.second;
            }
            if(!bound_root) continue;
        }
        struct Candidate {
            std::size_t joint = 0;
            s32 score = 0;
        };
        std::vector<Candidate> candidates;
        for (std::size_t start = 0; start < scene.joints.size(); ++start) {
            if(bound_root && scene.joints[start].archive_offset!=bound_root) continue;
            if (!shape_matches(depth, start, tree.nodes)) continue;
            s32 score = 0;
            // A NODE (visibility) track implies the children were authored
            // hidden; a ROTY track implies they were authored rotated in Y.
            if (has_channel(tree, 11)) {
                for (std::size_t k = 1; k < tree.nodes.size(); ++k) {
                    if ((scene.joints[start + k].flags & kHiddenFlag) != 0) ++score;
                }
            }
            if (has_channel(tree, 2)) {
                for (std::size_t k = 1; k < tree.nodes.size(); ++k) {
                    if (std::fabs(scene.joints[start + k].rotation[1]) > 1.0e-6f) ++score;
                }
            }
            candidates.push_back(Candidate{start, score});
        }
        if (candidates.empty()) continue;
        if (candidates.size() > 1) {
            s32 best = candidates.front().score;
            for (const Candidate& candidate : candidates) best = std::max(best, candidate.score);
            std::vector<std::size_t> tied;
            for (std::size_t i = 0; i < candidates.size(); ++i) {
                if (candidates[i].score == best) tied.push_back(i);
            }
            if (tied.size() > 1) {
                // Content evidence: the first scale-X keys must equal the
                // authored scales.
                for (std::size_t slot : tied) {
                    candidates[slot].score +=
                        s32(initial_scale_matches(tree, scene, candidates[slot].joint));
                }
            }
        }
        std::size_t chosen = 0;
        for (std::size_t i = 1; i < candidates.size(); ++i) {
            const Candidate& best = candidates[chosen];
            const Candidate& option = candidates[i];
            if (option.score != best.score) {
                if (option.score > best.score) chosen = i;
                continue;
            }
            if (have_offsets) {
                if (anim.joint_offsets[option.joint] < anim.joint_offsets[best.joint]) chosen = i;
            } else if (option.joint < best.joint) {
                chosen = i;
            }
        }
        if(!bound_root)for(std::size_t i=0;i<candidates.size();++i)
            if(i!=chosen && candidates[i].score==candidates[chosen].score){if(error)*error="hsd anim: ambiguous joint tree binding";return false;}
        std::vector<std::size_t> starts;
        if(bound_root){for(const auto& candidate:candidates)starts.push_back(candidate.joint);}
        else starts.push_back(candidates[chosen].joint);
        for(const std::size_t start:starts){
        for (std::size_t k = 0; k < tree.nodes.size(); ++k) {
            HsdAnimJointPose& joint = pose.joints[start + k];
            // HSD attaches one AObj per joint, so a second tree mapped onto the
            // same joints replaces the first one's pose rather than adding to
            // it: restart from the file's SRT (vsel00_mdl.arc has two trees on
            // one joint subtree and the reference resolves it this way).
            for (unsigned axis = 0; axis < 3; ++axis) {
                joint.rotation[axis] = scene.joints[start + k].rotation[axis];
                joint.scale[axis] = scene.joints[start + k].scale[axis];
                joint.translation[axis] = scene.joints[start + k].translation[axis];
            }
            joint.hidden = (scene.joints[start + k].flags & kHiddenFlag) != 0;
            for (const HsdAnimTrack& track : tree.nodes[k].tracks) {
                f32 value = 0.0f;
                if (!sample_track(track, frame_index, &value, error)) return false;
                switch (track.channel) {
                case 1:
                case 2:
                case 3:
                    joint.rotation[track.channel - 1] = value;
                    break;
                case 5:
                case 6:
                case 7:
                    joint.translation[track.channel - 5] = value;
                    break;
                case 8:
                case 9:
                case 10:
                    joint.scale[track.channel - 8] = value;
                    break;
                case 11:   // HSD_A_J_NODE: 1 visible, 0 hidden
                    joint.hidden = value <= 0.5f;
                    break;
                case 12: { // branch operation writes each descendant's HIDDEN flag
                    const std::size_t base=start+k;
                    pose.joints[base].hidden=value<=0.5f;
                    for(std::size_t child=base+1;child<pose.joints.size() && depth[child]>depth[base];++child)
                        pose.joints[child].hidden=value<=0.5f;
                    break;
                }
                case 4:
                    if(tree.nodes[k].path_spline<anim.joint_splines.size()) {
                        f64 point[3];
                        if(!spline_arc_point(anim.joint_splines[tree.nodes[k].path_spline],value,point))return false;
                        for(unsigned axis=0;axis<3;++axis)joint.translation[axis]=static_cast<f32>(point[axis]);
                    } else {if(error)*error="hsd anim: joint path animation requires spline binding";return false;}
                    break;
                default:
                    if(error)*error="hsd anim: unsupported joint animation channel";
                    return false;
                }
            }
        }
        }
    }

    pose.materials.resize(scene.meshes.size());
    for(std::size_t m=0;m<scene.meshes.size();++m) {
        const auto& mesh=scene.meshes[m];auto& material=pose.materials[m];
        for(unsigned k=0;k<3;++k){material.ambient[k]=mesh.material_ambient[k]/255.0f;material.diffuse[k]=mesh.material_colour[k]/255.0f;}
        material.alpha=mesh.material_alpha;
        for(const auto& item:anim.materials) if(item.mobj==mesh.material_offset){
            material.present=true;
            for(const auto& track:item.tracks){
                f32 value=0;
                if(!sample_track(track,frame_index,&value,error))return false;
                if(track.channel>=1 && track.channel<=3)material.ambient[track.channel-1]=std::clamp(value,0.0f,1.0f);
                else if(track.channel>=4 && track.channel<=6)material.diffuse[track.channel-4]=std::clamp(value,0.0f,1.0f);
                else if(track.channel==10)material.alpha=std::clamp(1.0f-value,0.0f,1.0f);
            }
        }
    }
    if (anim.camera.present && !camera_pose(anim.camera, frame_index, &pose, error)) return false;
    *out = std::move(pose);
    return true;
}

} // namespace gf
