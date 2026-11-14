// HSD animation tests: the packed keyframe decoder (HSD_FObjData), the
// animation-tree -> joint-tree shape match, camera animation, and the values the
// validated Python reference tool (tools/analyze/hsd_anim.py) reports for the
// real menu room.
//
// The synthetic archive is built byte by byte here (big-endian container with a
// relocation table, little-endian track payloads), so most cases do not depend
// on any game data. The real-archive cases are skipped when the private
// extraction is absent.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "engine/assets/hsd_anim.hpp"
#include "engine/graphics/transform.hpp"

namespace {

void check(bool ok, const char* what) {
    if (!ok) {
        std::fprintf(stderr, "hsd_anim failure: %s\n", what);
        std::abort();
    }
}

bool near(double a, double b, double tolerance) { return std::fabs(a - b) <= tolerance; }

struct Writer {
    std::vector<gf::u8> bytes;
    void u8(gf::u32 value) { bytes.push_back(gf::u8(value)); }
    void u16(gf::u32 value) { u8(value >> 8); u8(value); }
    void u32(gf::u32 value) { u16(value >> 16); u16(value); }
    void f32(gf::f32 value) {
        gf::u32 bits = 0;
        static_assert(sizeof(bits) == sizeof(value), "f32 size");
        std::memcpy(&bits, &value, sizeof(bits));
        u32(bits);
    }
    void pad(std::size_t to) { while (bytes.size() < to) bytes.push_back(0); }
    void poke_u32(std::size_t at, gf::u32 value) {
        bytes[at + 0] = gf::u8(value >> 24);
        bytes[at + 1] = gf::u8(value >> 16);
        bytes[at + 2] = gf::u8(value >> 8);
        bytes[at + 3] = gf::u8(value);
    }
    void poke_f32(std::size_t at, gf::f32 value) {
        gf::u32 bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        poke_u32(at, bits);
    }
};

// Track payloads are little-endian, unlike the container.
void le_f32(std::vector<gf::u8>& out, gf::f32 value) {
    gf::u32 bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    for (unsigned i = 0; i < 4; ++i) out.push_back(gf::u8(bits >> (i * 8)));
}

// Two tracks on one joint animation node:
//   rot X (channel 1): CON, 0 for four frames then 100 (hold-then-jump).
//   rot Y (channel 2): LIN, 0 -> 10 over ten frames (so frame N = N, N <= 10).
// Both are f32-valued packs, one value per pack, so each pack is
// [opcode+count=1][f32 value little-endian][wait].
std::vector<gf::u8> con_track() {
    std::vector<gf::u8> out;
    const gf::f32 values[3] = {0.0f, 100.0f, 100.0f};
    const gf::u32 waits[3] = {5, 5, 1};
    for (unsigned i = 0; i < 3; ++i) {
        out.push_back(0x01);   // opcode CON in the low nibble, pack count 1 above it
        le_f32(out, values[i]);
        out.push_back(gf::u8(waits[i]));
    }
    return out;
}

std::vector<gf::u8> lin_track() {
    std::vector<gf::u8> out;
    const gf::f32 values[3] = {0.0f, 10.0f, 10.0f};
    const gf::u32 waits[3] = {10, 10, 10};
    for (unsigned i = 0; i < 3; ++i) {
        out.push_back(0x02);   // opcode LIN
        le_f32(out, values[i]);
        out.push_back(gf::u8(waits[i]));
    }
    return out;
}

// 0x104, not 0x100: a colour-word check treats a low byte of 0x00 as a
// plausible HSD_Material, and the animation node must not look like one.
constexpr std::size_t kAobj = 0x104;
constexpr std::size_t kTrack1 = 0x120;   // CON, channel 1
constexpr std::size_t kTrack2 = 0x134;   // LIN, channel 2
constexpr std::size_t kAnimJoint = 0x160;
constexpr std::size_t kData1 = 0x174;    // exactly one anim node past the node
constexpr std::size_t kData2 = 0x186;
constexpr std::size_t kBlockBytes = 0x198;

// HSD_AObjDesc at kAobj -> FObjDesc chain (CON, LIN) at kTrack1/kTrack2, one
// HSD_AnimJoint at kAnimJoint whose descriptor is that chain.
//
// `relocate_track2_data=false` leaves the LIN track's `ad` field out of the
// relocation table: the field still holds the offset, but a decoder that follows
// it without the relocation entry would be dereferencing an unvalidated word.
std::vector<gf::u8> make_animation_archive(bool relocate_track2_data) {
    Writer block;
    block.pad(kBlockBytes);

    // HSD_AObjDesc: flags, end_frame, track chain, obj id (null).
    block.poke_u32(kAobj + 0x00, 0);
    block.poke_f32(kAobj + 0x04, 30.0f);
    block.poke_u32(kAobj + 0x08, gf::u32(kTrack1));
    block.poke_u32(kAobj + 0x0C, 0);

    // HSD_FObjDesc x2: next*, length, startframe, type, frac_value, frac_slope.
    block.poke_u32(kTrack1 + 0x00, gf::u32(kTrack2));
    block.poke_u32(kTrack1 + 0x04, gf::u32(con_track().size()));
    block.poke_f32(kTrack1 + 0x08, 0.0f);
    block.bytes[kTrack1 + 0x0C] = 1;   // channel 1: rotate X
    block.bytes[kTrack1 + 0x0D] = 0;   // frac_value 0: f32
    block.bytes[kTrack1 + 0x0E] = 0;
    block.bytes[kTrack1 + 0x0F] = 0;
    block.poke_u32(kTrack1 + 0x10, gf::u32(kData1));

    block.poke_u32(kTrack2 + 0x00, 0);
    block.poke_u32(kTrack2 + 0x04, gf::u32(lin_track().size()));
    block.poke_f32(kTrack2 + 0x08, 0.0f);
    block.bytes[kTrack2 + 0x0C] = 2;   // channel 2: rotate Y
    block.bytes[kTrack2 + 0x0D] = 0;
    block.bytes[kTrack2 + 0x0E] = 0;
    block.bytes[kTrack2 + 0x0F] = 0;
    block.poke_u32(kTrack2 + 0x10, gf::u32(kData2));

    // HSD_AnimJoint: no children, no siblings, one descriptor, null robj/flags.
    block.poke_u32(kAnimJoint + 0x00, 0);
    block.poke_u32(kAnimJoint + 0x04, 0);
    block.poke_u32(kAnimJoint + 0x08, gf::u32(kAobj));
    block.poke_u32(kAnimJoint + 0x0C, 0);
    block.poke_u32(kAnimJoint + 0x10, 0);

    const std::vector<gf::u8> first = con_track();
    const std::vector<gf::u8> second = lin_track();
    for (std::size_t i = 0; i < first.size(); ++i) block.bytes[kData1 + i] = first[i];
    for (std::size_t i = 0; i < second.size(); ++i) block.bytes[kData2 + i] = second[i];

    std::vector<std::size_t> relocated{kAobj + 0x08, kTrack1 + 0x00, kTrack1 + 0x10,
                                       kAnimJoint + 0x08};
    if (relocate_track2_data) relocated.push_back(kTrack2 + 0x10);

    const std::size_t data_size = block.bytes.size();
    Writer archive;
    archive.pad(0x20);
    for (gf::u8 byte : block.bytes) archive.u8(byte);
    for (std::size_t field : relocated) archive.u32(gf::u32(field));
    // Two public symbols, as a real archive has: the descriptor, and the
    // animation tree root the scene record points at (nothing else points at a
    // tree root, so this symbol is what makes it a structure start).
    archive.u32(gf::u32(kAobj));
    archive.u32(0);
    archive.u32(gf::u32(kAnimJoint));
    archive.u32(0);
    archive.u8('a'); archive.u8('n'); archive.u8('i'); archive.u8('m'); archive.u8(0);

    Writer header;
    header.u32(gf::u32(archive.bytes.size()));
    header.u32(gf::u32(data_size));
    header.u32(gf::u32(relocated.size()));
    header.u32(2);   // public symbols
    header.u32(0);   // externs
    header.u32(0);
    header.u32(0);
    header.u32(0);
    for (std::size_t i = 0; i < 0x20; ++i) archive.bytes[i] = header.bytes[i];
    return archive.bytes;
}

// A scene with hand-written joints: the decoder's job is the animation, so the
// joint pose is supplied directly (the HSD geometry path is covered by hsd.cpp).
gf::HsdScene make_scene(std::size_t joints) {
    gf::HsdScene scene;
    for (std::size_t i = 0; i < joints; ++i) {
        gf::HsdJoint joint;
        joint.parent = i == 0 ? gf::kHsdNoJoint : 0;
        joint.rotation[0] = 0.5f;
        joint.rotation[1] = 0.25f;
        joint.rotation[2] = 0.125f;
        joint.scale[0] = 2.0f;
        joint.scale[1] = 3.0f;
        joint.scale[2] = 4.0f;
        joint.translation[0] = 7.0f;
        joint.translation[1] = 8.0f;
        joint.translation[2] = 9.0f;
        scene.joints.push_back(joint);
    }
    return scene;
}

int synthetic_cases() {
    int cases = 0;
    const std::vector<gf::u8> archive = make_animation_archive(true);
    gf::HsdAnimation anim;
    std::string error;
    const gf::ByteView view{archive.data(), archive.size()};
    check(gf::hsd_anim_load(view, &anim, &error), "synthetic animation archive loads");
    check(anim.anim_node_count == 1, "one animation node");
    check(anim.trees.size() == 1, "one animation tree");
    check(anim.trees[0].nodes.size() == 1, "one node in the tree");
    check(anim.trees[0].nodes[0].tracks.size() == 2, "two tracks on the node");
    check(anim.aobj_count == 1, "one AObjDesc");
    cases += 5;
    check(gf::hsd_anim_end_frame(anim) == 30.0f, "end frame is the authored clip length");
    check(!anim.camera.present, "no camera animation in this archive");
    ++cases;

    // One joint: the single-node tree can only match it.
    const gf::HsdScene scene = make_scene(1);
    struct Expect {
        gf::f32 frame;
        gf::f32 rotation_x;   // CON: hold 0, then jump to 100 at frame 5
        gf::f32 rotation_y;   // LIN: 0 -> 10 across ten frames
    };
    const Expect expectations[] = {
        {0.0f, 0.0f, 0.0f},   // frame 0 is the un-stepped state
        {1.0f, 0.0f, 1.0f},   {2.0f, 0.0f, 2.0f},   {3.0f, 0.0f, 3.0f},
        {4.0f, 0.0f, 4.0f},   {5.0f, 100.0f, 5.0f}, {6.0f, 100.0f, 6.0f},
        {9.0f, 100.0f, 9.0f}, {10.0f, 100.0f, 10.0f}, {20.0f, 100.0f, 10.0f},
    };
    for (const Expect& expect : expectations) {
        gf::HsdAnimPose pose;
        check(gf::hsd_anim_evaluate(anim, scene, expect.frame, &pose, &error),
              "synthetic evaluation succeeds");
        check(pose.joints.size() == 1, "pose is parallel to the scene joints");
        check(near(pose.joints[0].rotation[0], expect.rotation_x, 1e-5),
              "stepped (CON) track holds then jumps");
        check(near(pose.joints[0].rotation[1], expect.rotation_y, 1e-5),
              "interpolated (LIN) track ramps linearly");
        ++cases;
    }
    // Channels the animation does not touch keep the file pose, and the camera
    // fields stay empty.
    gf::HsdAnimPose pose;
    check(gf::hsd_anim_evaluate(anim, scene, 5.0f, &pose, &error), "evaluate at frame 5");
    check(near(pose.joints[0].rotation[2], 0.125, 1e-6), "unanimated rotation channel is kept");
    check(near(pose.joints[0].scale[0], 2.0, 1e-6) && near(pose.joints[0].scale[1], 3.0, 1e-6) &&
              near(pose.joints[0].scale[2], 4.0, 1e-6),
          "unanimated scale is kept");
    check(near(pose.joints[0].translation[0], 7.0, 1e-6) &&
              near(pose.joints[0].translation[2], 9.0, 1e-6),
          "unanimated translation is kept");
    check(!pose.has_camera && pose.camera_fov_y_degrees == 0.0f, "no camera pose without a camera");
    cases += 4;

    // Two joints: the one-node tree matches only the child's chain (a root's
    // chain includes its subtree), so the root must keep the file pose.
    const gf::HsdScene pair = make_scene(2);
    gf::HsdAnimPose pair_pose;
    check(gf::hsd_anim_evaluate(anim, pair, 5.0f, &pair_pose, &error), "evaluate against two joints");
    check(near(pair_pose.joints[0].rotation[0], 0.5, 1e-6) &&
              near(pair_pose.joints[0].rotation[1], 0.25, 1e-6),
          "an unmapped root keeps the file pose");
    check(near(pair_pose.joints[1].rotation[0], 100.0, 1e-5) &&
              near(pair_pose.joints[1].rotation[1], 5.0, 1e-5),
          "the shaped-matched joint is animated");
    cases += 2;

    // Determinism.
    gf::HsdAnimation again;
    check(gf::hsd_anim_load(view, &again, &error), "reload");
    gf::HsdAnimPose again_pose;
    check(gf::hsd_anim_evaluate(again, scene, 7.0f, &again_pose, &error), "re-evaluate");
    check(again_pose.joints[0].rotation[1] == 5.0f + 2.0f, "evaluation is deterministic");
    ++cases;

    // A malformed byte stream is refused rather than read past the buffer.
    const auto track_view = [](const std::vector<gf::u8>& bytes, gf::u8 frac) {
        gf::HsdAnimTrackView view;
        view.data = gf::ByteView{bytes.data(), bytes.size()};
        view.frac_value = frac;
        return view;
    };
    {
        std::vector<gf::u8> bytes{4};le_f32(bytes,0);le_f32(bytes,0);bytes.push_back(10);
        bytes.push_back(5);le_f32(bytes,2); // NO wait after slope-only key
        bytes.push_back(4);le_f32(bytes,10);le_f32(bytes,0);bytes.push_back(10);
        bytes.push_back(1);le_f32(bytes,10);bytes.push_back(10);
        auto v=track_view(bytes,0);gf::f32 value=0;
        check(gf::hsd_anim_sample_requested_track(v,0,&value,&error)&&near(value,0,1e-6),"SPL/SLP initial requested key");
        check(gf::hsd_anim_sample_requested_track(v,5,&value,&error)&&near(value,7.5,1e-6),"slope-only key changes tangent without consuming next opcode as wait");
        check(gf::hsd_anim_sample_requested_track(v,10,&value,&error)&&near(value,10,1e-6),"SPL/SLP boundary consumes only real waits");
        for(gf::u32 state:{1u,2u}){
            std::vector<gf::u8> slope{5};le_f32(slope,2);gf::hsd_anim_detail::TrackMachine m;
            m.data=slope.data();m.length=slope.size();m.state=state;gf::u32 next=0;
            check(gf::hsd_anim_detail::load_data(m,&next,&error)&&next==state&&m.pos==slope.size()&&m.d1==2,"SLP preserves both initial/subsequent loader states");
        }
        cases+=5;
    }
    {
        // No duplicate terminal hold key: original FObj extrapolates at EOF;
        // AObj/actor stopping is separate. Capture stored states as well as RGB/
        // pose values so a sticky6 cannot accidentally pass a constant tail.
        for(bool final_wait:{false,true}){
            std::vector<gf::u8> bytes{2};le_f32(bytes,0);bytes.push_back(10);
            bytes.push_back(2);le_f32(bytes,10);if(final_wait)bytes.push_back(10);
            gf::hsd_anim_detail::TrackMachine m;m.data=bytes.data();m.length=bytes.size();
            check(gf::hsd_anim_detail::track_step(m,&error,0)&&m.value==0,"EOF fixture explicit time zero");
            for(unsigned time=1;time<=13;++time){
                check(gf::hsd_anim_detail::track_step(m,&error)&&m.value==time&&m.time==time,"EOF carry resets each call, no repeated last-wait addition");
                if(time>=10)check(m.state==(final_wait?2u:3u),"loader/wait EOF never stores dispatch6");
            }
            gf::f32 value=0;
            check(gf::hsd_anim_sample_requested_track(track_view(bytes,0),30,&value,&error)&&value==30,"direct beyond-tail request is not clamped to endpoint");
            check(gf::hsd_anim_sample_track(track_view(bytes,0),0,&value,&error)&&value==0,"legacy frame0 remains unstepped");
            cases+=20;
        }
        std::vector<gf::u8> key{6};le_f32(key,7);gf::f32 value=0;
        check(gf::hsd_anim_sample_requested_track(track_view(key,0),0,&value,&error)&&value==7,"EOF flushes pending KEY even if previous interpolation opcode was zero");
        gf::hsd_anim_detail::TrackMachine m;m.data=key.data();m.length=key.size();
        check(gf::hsd_anim_detail::track_step(m,&error,0)&&m.state==3&&m.value==7,"singleton KEY EOF keeps stored wait-loader state");
        check(gf::hsd_anim_detail::track_step(m,&error)&&m.state==3&&m.value==7,"pending KEY is flushed once, not replayed as a new key");
        cases+=3;
    }
    {
        // A CON pack whose f32 value is cut off by the declared length.
        const std::vector<gf::u8> truncated{0x01, 0x00, 0x00, 0x00};
        gf::f32 value = 0.0f;
        check(!gf::hsd_anim_sample_track(track_view(truncated, 0), 1, &value, &error),
              "a truncated track value is refused");
        check(!error.empty(), "the refusal carries a message");
        ++cases;
    }
    {
        // A wait whose continuation bit says another byte follows, at the end.
        std::vector<gf::u8> bytes{0x01};
        le_f32(bytes, 1.0f);
        bytes.push_back(0x80);
        gf::f32 value = 0.0f;
        check(!gf::hsd_anim_sample_track(track_view(bytes, 0), 1, &value, &error),
              "a wait past the buffer is refused");
        ++cases;
    }
    {
        // An unsupported quantisation encoding is refused.
        std::vector<gf::u8> bytes{0x01, 0x00, 0x02};
        gf::f32 value = 0.0f;
        check(!gf::hsd_anim_sample_track(track_view(bytes, 0x10), 1, &value, &error),
              "an unsupported frac_value encoding is refused");
        ++cases;
    }
    {
        // Unsupported opcodes must not silently publish a partial pose.
        const std::vector<gf::u8> bytes{0x07, 0x00, 0x00, 0x00, 0x00, 0x00};
        gf::f32 value = 123.0f;
        check(!gf::hsd_anim_sample_track(track_view(bytes, 0), 4, &value, &error) && !error.empty(),
               "an unknown opcode is explicitly refused");
        ++cases;
    }
    {
        // The track data pointer is not in the relocation table, so the
        // descriptor does not validate and no animation is discovered. The load
        // still succeeds (an archive without animation is legal).
        const std::vector<gf::u8> unrelocated = make_animation_archive(false);
        gf::HsdAnimation partial;
        check(gf::hsd_anim_load({unrelocated.data(), unrelocated.size()}, &partial, &error),
              "an archive with an unrelocated track pointer still loads");
        check(partial.trees.empty() && partial.anim_node_count == 0,
              "the unvalidated track pointer is never followed");
        check(gf::hsd_anim_end_frame(partial) == 0.0f, "no animation means end frame 0");
        gf::HsdAnimPose file_pose;
        check(gf::hsd_anim_evaluate(partial, scene, 5.0f, &file_pose, &error),
              "an empty animation evaluates to the file pose");
        check(near(file_pose.joints[0].rotation[0], 0.5, 1e-6) &&
                  near(file_pose.joints[0].scale[1], 3.0, 1e-6),
              "the file pose is preserved when nothing is animated");
        cases += 4;
    }

    // Refusals: a non-HSD container, an empty buffer and a truncated one.
    gf::HsdAnimation bad;
    check(!gf::hsd_anim_load({nullptr, 0}, &bad, &error), "an empty buffer is refused");
    check(!error.empty(), "the empty buffer refusal explains itself");
    ++cases;
    std::vector<gf::u8> junk(128, 0xAB);
    check(!gf::hsd_anim_load({junk.data(), junk.size()}, &bad, &error), "junk is refused");
    ++cases;
    {
        std::vector<gf::u8> short_archive = archive;
        short_archive.resize(0x40);
        check(!gf::hsd_anim_load({short_archive.data(), short_archive.size()}, &bad, &error),
              "an archive truncated below its tables is refused");
        ++cases;
    }
    // A frame outside the evaluatable range is refused (not silently clamped).
    check(!gf::hsd_anim_evaluate(anim, scene, -1.0f, &pose, &error), "a negative frame is refused");
    ++cases;
    return cases;
}

// ---- real archives ----------------------------------------------------

struct RealResult {
    bool present = false;
    bool loaded = false;
    unsigned trees = 0;
    unsigned nodes = 0;
    unsigned aobjs = 0;
    double end_frame = 0.0;
};

RealResult load_real(const char* path, gf::HsdScene* scene, gf::HsdAnimation* anim) {
    RealResult result;
    std::ifstream file(path, std::ios::binary);
    if (!file) return result;
    result.present = true;
    const std::vector<gf::u8> bytes((std::istreambuf_iterator<char>(file)), {});
    std::string error;
    check(gf::hsd_load({bytes.data(), bytes.size()}, scene, &error), "real scene loads");
    check(gf::hsd_anim_load({bytes.data(), bytes.size()}, anim, &error), "real animation loads");
    result.loaded = true;
    result.trees = static_cast<unsigned>(anim->trees.size());
    result.nodes = anim->anim_node_count;
    result.aobjs = anim->aobj_count;
    result.end_frame = double(gf::hsd_anim_end_frame(*anim));
    return result;
}

// The pose the archive itself stores, with nothing animated.
gf::HsdAnimPose file_pose(const gf::HsdScene& scene) {
    gf::HsdAnimPose pose;
    pose.joints.resize(scene.joints.size());
    for (std::size_t i = 0; i < scene.joints.size(); ++i) {
        for (unsigned k = 0; k < 3; ++k) {
            pose.joints[i].rotation[k] = scene.joints[i].rotation[k];
            pose.joints[i].scale[k] = scene.joints[i].scale[k];
            pose.joints[i].translation[k] = scene.joints[i].translation[k];
        }
    }
    return pose;
}

// World position of one joint at the evaluated pose: the same composition the
// scene renderer uses (T * R * S per joint, parent matrices multiplied).
void world_position(const gf::HsdScene& scene, const gf::HsdAnimPose& pose, unsigned joint,
                    double out[3]) {
    std::vector<gf::Mat4> world(scene.joints.size());
    for (std::size_t i = 0; i < scene.joints.size(); ++i) {
        const gf::HsdAnimJointPose& j = pose.joints[i];
        const gf::Mat4 local = gf::mat4_from_srt(j.rotation, j.scale, j.translation);
        const gf::u32 parent = scene.joints[i].parent;
        world[i] = parent == gf::kHsdNoJoint ? local : gf::mat4_multiply(world[parent], local);
    }
    const gf::Mat4& matrix = world[joint];
    for (unsigned k = 0; k < 3; ++k) out[k] = double(matrix.at(k, 3));
}

// The ten VBILLBOARD disc joints of tl00_mdl.arc, in preorder.
const unsigned kBillboards[10] = {31, 32, 34, 35, 36, 39, 40, 42, 43, 44};
// The seven joints whose authored Y scale (5.7 .. 10) is a sample of the
// animation's 1 -> 10 ramp.
const unsigned kStretched[7] = {19, 23, 24, 25, 26, 27, 28};
const double kStretchedFileScaleY[7] = {5.73684, 10.0, 9.52632, 9.05263, 9.05263, 9.52631, 8.10526};

int real_tl00_cases(const gf::HsdScene& scene, const gf::HsdAnimation& anim,
                    const RealResult& info) {
    int cases = 0;
    check(info.loaded, "tl00 animation loads");
    check(info.trees == 4, "tl00 has four animation trees");
    check(info.nodes == 40, "tl00 has forty animation nodes");
    check(info.aobjs == 69, "tl00 has 69 AObjDescs");
    check(scene.joints.size() == 78, "tl00 has 78 joints");
    check(near(info.end_frame, 1999.0, 1e-3), "tl00 clip length is 1999 frames");
    ++cases;

    // Frame 1 is the flat-panel pose: the seven stretched joints are all 1.0,
    // and their file values are mid-ramp samples of the same 1 -> 10 ramp.
    gf::HsdAnimPose pose;
    std::string error;
    check(gf::hsd_anim_evaluate(anim, scene, 1.0f, &pose, &error), "tl00 frame 1 evaluates");
    for (unsigned i = 0; i < 7; ++i) {
        const unsigned joint = kStretched[i];
        check(near(double(scene.joints[joint].scale[1]), kStretchedFileScaleY[i], 1e-4),
              "tl00 file scale Y matches the reference table");
        check(near(double(pose.joints[joint].scale[1]), 1.0, 1e-5),
              "tl00 frame 1 flattens every stretched joint to scale Y 1.0");
        ++cases;
    }

    // The staggered wave: joint 19 ramps first, joint 28 last.
    struct Step {
        gf::f32 frame;
        double scale_y[7];
    };
    const Step steps[] = {
        {10.0f, {4.31579, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0}},
        {30.0f, {10.0, 8.10526, 6.68421, 5.26316, 3.84211, 2.42105, 1.0}},
        {60.0f, {10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0}},
    };
    for (const Step& step : steps) {
        check(gf::hsd_anim_evaluate(anim, scene, step.frame, &pose, &error),
              "tl00 stretched frames evaluate");
        for (unsigned i = 0; i < 7; ++i) {
            check(near(double(pose.joints[kStretched[i]].scale[1]), step.scale_y[i], 1e-3),
                  "tl00 staggered ramp reproduces the reference values");
        }
        cases += 7;
    }

    // The visibility channel: the plates are hidden except while their own ramp
    // plays, which matches the reference tool's table.
    struct Visibility {
        gf::f32 frame;
        bool hidden[7];
    };
    const Visibility visibility[] = {
        {1.0f, {true, true, true, true, true, true, true}},
        {10.0f, {false, true, true, true, true, true, true}},
        {30.0f, {true, false, false, false, false, false, false}},
        {60.0f, {true, true, true, true, true, true, true}},
    };
    for (const Visibility& step : visibility) {
        check(gf::hsd_anim_evaluate(anim, scene, step.frame, &pose, &error),
              "tl00 visibility frames evaluate");
        for (unsigned i = 0; i < 7; ++i) {
            check(pose.joints[kStretched[i]].hidden == step.hidden[i],
                  "tl00 node-visibility track matches the reference");
        }
        cases += 7;
    }

    // No frame moves any joint: every translation track in tl00 is a zero
    // constant, so the ten billboard discs keep their file world position.
    const gf::f32 frames[] = {1.0f, 10.0f, 30.0f, 60.0f, 900.0f, 1999.0f};
    const gf::HsdAnimPose stored = file_pose(scene);
    double max_delta = 0.0;
    for (gf::f32 frame : frames) {
        check(gf::hsd_anim_evaluate(anim, scene, frame, &pose, &error), "tl00 frame evaluates");
        for (std::size_t i = 0; i < scene.joints.size(); ++i) {
            for (unsigned k = 0; k < 3; ++k) {
                max_delta = std::max(max_delta,
                                     std::fabs(double(pose.joints[i].translation[k]) -
                                               double(scene.joints[i].translation[k])));
            }
        }
        for (unsigned i = 0; i < 10; ++i) {
            double before[3], after[3];
            world_position(scene, stored, kBillboards[i], before);
            world_position(scene, pose, kBillboards[i], after);
            for (unsigned k = 0; k < 3; ++k) {
                check(near(after[k], before[k], 1e-4),
                      "tl00 animation never moves a billboard disc");
            }
            // Two authored stacks: five at (318.264, 0, 0), five at the origin.
            const double stack_x = i < 5 ? 318.264 : 0.0;
            check(near(after[0], stack_x, 1e-2) && near(after[1], 0.0, 1e-2) &&
                      near(after[2], 0.0, 1e-2),
                  "tl00 billboard world position is the authored stack");
        }
        cases += 10 + 10;
    }
    check(max_delta <= 1e-5, "tl00 translation tracks are constant zero");

    // The camera flythrough: settled framing and the animation start.
    check(pose.has_camera, "tl00 has a camera animation");
    struct CameraSample {
        gf::f32 frame;
        double eye[3];
        double interest[3];
    };
    const CameraSample samples[] = {
        {1.0f, {5.0, 3557.15, -3145.1}, {5.0, 3557.15, -12625.0}},
        {30.0f, {5.0083, 3549.0765, -3130.9341}, {5.0093, 3555.9665, -12611.6307}},
        {60.0f, {8.0682, 295.66, 2725.3996}, {8.0694, 2983.5236, -6539.8811}},
        {900.0f, {85.0, 1092.0, 1811.095}, {85.0, 480.0, 1497.573}},
        {1999.0f, {85.0, 1092.0, 1811.095}, {85.0, 480.0, 1497.573}},
    };
    for (const CameraSample& sample : samples) {
        check(gf::hsd_anim_evaluate(anim, scene, sample.frame, &pose, &error),
              "tl00 camera frame evaluates");
        for (unsigned k = 0; k < 3; ++k) {
            check(near(double(pose.camera_eye[k]), sample.eye[k], 1e-2),
                  "tl00 camera eye matches the reference");
            check(near(double(pose.camera_interest[k]), sample.interest[k], 1e-2),
                  "tl00 camera interest matches the reference");
        }
        check(near(double(pose.camera_fov_y_degrees), 41.539, 1e-3), "tl00 camera fov is 41.539");
        check(near(double(pose.camera_near), 0.1, 1e-3), "tl00 camera near is 0.1");
        check(near(double(pose.camera_far), 32768.0, 1e-3), "tl00 camera far is 32768");
        cases += 8;
    }
    return cases;
}

int real_vsel00_cases(const gf::HsdScene& scene, const gf::HsdAnimation& anim,
                      const RealResult& info, double* max_translation_delta, bool* complete) {
    int cases = 0;
    check(info.loaded, "vsel00 animation loads");
    check(info.trees == 10, "vsel00 published root records attach ten animation trees");
    check(info.nodes == 29, "explicit traversal finds nodes missed by span heuristics");
    check(info.aobjs == 47, "vsel00 has 47 AObjDescs");
    check(near(info.end_frame, 133.0, 1e-3), "vsel00 clip length is 133 frames");
    ++cases;

    gf::HsdAnimPose pose;
    std::string error;
    if(!gf::hsd_anim_evaluate(anim, scene, 60.0f, &pose, &error)){
        check(error.find("unknown packed-track opcode")!=std::string::npos,"vsel00 partial clip is explicitly refused");
        // Positive control is independent of the malformed tail: inspect known
        // translation tracks, never call the refused whole pose a success.
        double delta=0;
        for(const auto& tree:anim.trees)for(const auto& node:tree.nodes)for(const auto& track:node.tracks)
            if(track.channel>=5 && track.channel<=7){float a=0,b=0;std::string e;
                if(gf::hsd_anim_detail::sample_track(track,1,&a,&e)&&gf::hsd_anim_detail::sample_track(track,60,&b,&e))delta=std::max(delta,double(std::fabs(a-b)));}
        check(delta>100,"valid vsel00 translation tracks move");
        *max_translation_delta=delta;*complete=false;return cases+2;
    }
    *complete=true;
    double delta = 0.0;
    for (std::size_t i = 0; i < scene.joints.size(); ++i) {
        for (unsigned k = 0; k < 3; ++k) {
            delta = std::max(delta, std::fabs(double(pose.joints[i].translation[k]) -
                                              double(scene.joints[i].translation[k])));
        }
    }
    if (max_translation_delta) *max_translation_delta = delta;
    // The positive control: this archive really animates joint translation.
    check(delta > 1000.0, "vsel00 translation animation moves a joint by >1000 units");
    ++cases;
    // The old shape-scored binding put an unrelated track on joint 75. The
    // published root tables do not attach translation to that joint.
    for(unsigned k=0;k<3;++k) check(near(pose.joints[75].translation[k],scene.joints[75].translation[k],1e-5),
                                    "unbound joint 75 remains at the file pose");
    check(near(double(scene.joints[75].translation[1]), 80.766, 1e-2),
          "vsel00 joint 75 file translation is the unanimated pose");
    cases += 2;
    return cases;
}

} // namespace

int main() {
    const int synthetic = synthetic_cases();

    gf::HsdScene tl00_scene;
    gf::HsdAnimation tl00_anim;
    const RealResult tl00 = load_real("local/analysis/arc/tl00_mdl.arc", &tl00_scene, &tl00_anim);

    gf::HsdScene vsel_scene;
    gf::HsdAnimation vsel_anim;
    const RealResult vsel =
        load_real("local/analysis/arc/vsel00_mdl.arc", &vsel_scene, &vsel_anim);

    int cases = synthetic;
    if (tl00.present) cases += real_tl00_cases(tl00_scene, tl00_anim, tl00);
    double vsel_delta = 0.0;
    bool vsel_complete=false;
    if (vsel.present) cases += real_vsel00_cases(vsel_scene, vsel_anim, vsel, &vsel_delta,&vsel_complete);
    // The cross-archive claim only means something when both archives are here.
    if (tl00.present && vsel.present) {
        std::printf("{\"hsd_anim_cases\":%d,\"real_present\":true,"
                    "\"tl00_trees\":%u,\"tl00_nodes\":%u,\"tl00_aobjs\":%u,"
                    "\"tl00_end_frame\":%.9g,\"tl00_translation_moves_anything\":false,"
                    "\"vsel00_trees\":%u,\"vsel00_nodes\":%u,\"vsel00_aobjs\":%u,"
                    "\"vsel00_end_frame\":%.9g,\"vsel00_translation_moves_anything\":true,"
                    "\"vsel00_max_translation_delta\":%.6f,\"vsel00_complete_pose\":%s,"
                    "\"all_passed\":true}\n",
                    cases, tl00.trees, tl00.nodes, tl00.aobjs, tl00.end_frame, vsel.trees,
                    vsel.nodes, vsel.aobjs, vsel.end_frame, vsel_delta,vsel_complete?"true":"false");
    } else {
        std::printf("{\"hsd_anim_cases\":%d,\"real_present\":false,\"all_passed\":true}\n",
                    cases);
    }
    return 0;
}
