// HSD decoder tests: the GX texture units it shares with TPL, a synthetic
// archive that exercises the JOBJ/DOBJ/MOBJ/POBJ walk and display-list decode,
// the refusal paths, and a read-only sweep of the real menu room when the game
// archive is present.
//
// The synthetic archive is built byte by byte here (big-endian container with a
// relocation table), so the test does not depend on any game data.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "engine/assets/afs.hpp"
#include "engine/assets/gx_texture.hpp"
#include "engine/assets/hsd.hpp"

namespace {

void check(bool ok, const char* what) {
    if (!ok) {
        std::fprintf(stderr, "hsd failure: %s\n", what);
        std::abort();
    }
}

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
};

// Minimal valid HSD archive: one joint with one DOBJ/MOBJ/POBJ that draws a
// three-vertex triangle strip, no texture.
std::vector<gf::u8> make_scene() {
    constexpr std::size_t kJobj = 0x100, kDobj = 0x140, kMobj = 0x150, kPobj = 0x168;
    constexpr std::size_t kVtxDesc = 0x180, kDisplay = 0x1D0, kPositions = 0x200,
                         kColours = 0x230;
    Writer block;
    block.pad(0x280);

    // JOBJ: child/sibling null, contents -> DOBJ, identity SRT.
    block.poke_u32(kJobj + 0x00, 0);
    block.poke_u32(kJobj + 0x04, 0x20000000u);
    block.poke_u32(kJobj + 0x08, 0);
    block.poke_u32(kJobj + 0x0C, 0);
    block.poke_u32(kJobj + 0x10, gf::u32(kDobj));
    for (unsigned i = 0; i < 3; ++i) {
        block.poke_u32(kJobj + 0x14 + i * 4, 0);
        block.poke_u32(kJobj + 0x20 + i * 4, 0x3F800000u);   // scale 1.0
        block.poke_u32(kJobj + 0x2C + i * 4, 0);
    }
    block.poke_u32(kJobj + 0x38, 0);
    block.poke_u32(kJobj + 0x3C, 0);

    // DOBJ: sibling null, MOBJ, POBJ.
    block.poke_u32(kDobj + 0x00, 0);
    block.poke_u32(kDobj + 0x04, 0);
    block.poke_u32(kDobj + 0x08, gf::u32(kMobj));
    block.poke_u32(kDobj + 0x0C, gf::u32(kPobj));

    // MOBJ: no texture, material -> 0x240.
    block.poke_u32(kMobj + 0x00, 0);
    block.poke_u32(kMobj + 0x04, 0);
    block.poke_u32(kMobj + 0x08, 0);
    block.poke_u32(kMobj + 0x0C, 0x240);
    block.poke_u32(kMobj + 0x10, 0);
    block.poke_u32(kMobj + 0x14, 0);

    // Material: diffuse white, alpha in the low byte.
    block.poke_u32(0x240, 0xFFFFFFFFu);
    block.poke_u32(0x244, 0xFFFFFFFFu);
    block.poke_u32(0x248, 0xFFFFFFFFu);
    block.poke_u32(0x24C, 0x3F800000u);
    block.poke_u32(0x250, 0);

    // POBJ: sibling null, vtx desc -> list, flags CULLBACK, nDisp (unused), disp.
    block.poke_u32(kPobj + 0x00, 0);
    block.poke_u32(kPobj + 0x04, 0);
    block.poke_u32(kPobj + 0x08, gf::u32(kVtxDesc));
    block.poke_u32(kPobj + 0x0C, 0x80000001u); // flags CULLBACK, one 32-byte DL block
    block.poke_u32(kPobj + 0x10, gf::u32(kDisplay));
    block.poke_u32(kPobj + 0x14, 0);

    // Vertex descriptors: position (index16, F32, XYZ, stride 12) and
    // colour (index8, RGBA8, stride 4).
    // Entry layout: attr, attrType, compCnt, compType, compShift:u8, pad, stride:u16,
    // arrayOffs:u32 (0x18 bytes each, terminated by attribute 0xFF).
    block.poke_u32(kVtxDesc + 0x00, 9);
    block.poke_u32(kVtxDesc + 0x04, 3);
    block.poke_u32(kVtxDesc + 0x08, 1);
    block.poke_u32(kVtxDesc + 0x0C, 4);
    block.bytes[kVtxDesc + 0x10] = 0;
    block.bytes[kVtxDesc + 0x11] = 0;
    block.bytes[kVtxDesc + 0x12] = 0;
    block.bytes[kVtxDesc + 0x13] = 12;
    block.poke_u32(kVtxDesc + 0x14, gf::u32(kPositions));
    block.poke_u32(kVtxDesc + 0x18 + 0x00, 11);
    block.poke_u32(kVtxDesc + 0x18 + 0x04, 2);
    block.poke_u32(kVtxDesc + 0x18 + 0x08, 1);
    block.poke_u32(kVtxDesc + 0x18 + 0x0C, 5);
    block.bytes[kVtxDesc + 0x18 + 0x10] = 0;
    block.bytes[kVtxDesc + 0x18 + 0x11] = 0;
    block.bytes[kVtxDesc + 0x18 + 0x12] = 0;
    block.bytes[kVtxDesc + 0x18 + 0x13] = 4;
    block.poke_u32(kVtxDesc + 0x18 + 0x14, gf::u32(kColours));
    block.poke_u32(kVtxDesc + 0x30, 0xFF);

    // Display list: triangle strip of three vertices, then the terminator.
    Writer display;
    display.u8(0x98);          // draw triangle strip
    display.u16(3);            // vertex count
    display.u16(0); display.u8(0);   // v0: position index 0, colour index 0
    display.u16(1); display.u8(1);   // v1
    display.u16(2); display.u8(2);   // v2
    display.u8(0x00);          // end of list
    for (std::size_t i = 0; i < display.bytes.size(); ++i) {
        block.bytes[kDisplay + i] = display.bytes[i];
    }

    // Positions.
    const gf::f32 positions[3][3] = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    for (unsigned v = 0; v < 3; ++v) {
        for (unsigned k = 0; k < 3; ++k) {
            Writer temp;
            temp.f32(positions[v][k]);
            for (unsigned j = 0; j < 4; ++j) block.bytes[kPositions + v * 12 + k * 4 + j] = temp.bytes[j];
        }
    }
    // Colours: red, green, blue with full alpha.
    const gf::u8 colours[3][4] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    for (unsigned v = 0; v < 3; ++v) {
        for (unsigned k = 0; k < 4; ++k) block.bytes[kColours + v * 4 + k] = colours[v][k];
    }

    // Relocations: every field that holds a data-relative pointer.
    const std::vector<std::size_t> relocated{
        kJobj + 0x10, kDobj + 0x08, kDobj + 0x0C, kMobj + 0x0C, kPobj + 0x08, kPobj + 0x10,
        kVtxDesc + 0x14, kVtxDesc + 0x18 + 0x14};
    // The archive has to be self-consistent: the public symbol points at the
    // joint, and every relocation field must actually hold its target.
    const std::size_t data_size = block.bytes.size();
    Writer archive;
    const std::size_t header_and_tables = 0x20 + data_size + relocated.size() * 4 + 8;
    const std::size_t string_at = header_and_tables;
    archive.pad(0x20);
    for (gf::u8 byte : block.bytes) archive.u8(byte);
    for (std::size_t field : relocated) archive.u32(gf::u32(field));
    // One public symbol: offset = kJobj, name offset = 0.
    archive.u32(gf::u32(kJobj));
    archive.u32(0);
    archive.u8('s'); archive.u8('c'); archive.u8('e'); archive.u8('n'); archive.u8('e');
    archive.u8(0);
    // Header.
    Writer header;
    header.u32(gf::u32(archive.bytes.size()));
    header.u32(gf::u32(data_size));
    header.u32(gf::u32(relocated.size()));
    header.u32(1);   // public symbols
    header.u32(0);   // externs
    header.u32(0);
    header.u32(0);
    header.u32(0);
    for (std::size_t i = 0; i < 0x20; ++i) archive.bytes[i] = header.bytes[i];
    (void)string_at;
    return archive.bytes;
}

int synthetic_cases() {
    int cases = 0;
    const std::vector<gf::u8> archive = make_scene();
    gf::HsdScene scene;
    std::string error;
    check(gf::hsd_load({archive.data(), archive.size()}, &scene, &error), "synthetic scene loads");
    check(scene.joints.size() == 1, "one joint");
    check(scene.meshes.size() == 1, "one mesh");
    check(scene.mesh_joint.size() == 1 && scene.mesh_joint[0] == 0, "mesh bound to the joint");
    check(scene.meshes[0].vertices.size() == 3, "three vertices");
    check(scene.meshes[0].indices.size() == 3, "one triangle from the strip");
    check(scene.textures.empty(), "no textures in this archive");
    check(scene.meshes[0].material_colour[0] == 255 && scene.meshes[0].material_colour[3] == 255,
          "material colour is white");
    cases += 7;

    const gf::HsdMesh& mesh = scene.meshes[0];
    check(mesh.vertices[1].position[0] == 1.0f && mesh.vertices[2].position[1] == 1.0f,
          "positions decode in order");
    check(mesh.vertices[0].colour[0] == 255 && mesh.vertices[1].colour[1] == 255 &&
              mesh.vertices[2].colour[2] == 255,
          "vertex colours decode in order");
    cases += 2;

    // Determinism: the same bytes produce the same scene twice.
    gf::HsdScene again;
    check(gf::hsd_load({archive.data(), archive.size()}, &again, &error), "reload");
    check(again.meshes.size() == scene.meshes.size() &&
              again.meshes[0].indices == scene.meshes[0].indices,
          "decode is deterministic");
    ++cases;

    // Refusals. Each must fail with a message and leave the scene empty.
    auto rejects = [&](std::vector<gf::u8> data, const char* what) {
        gf::HsdScene bad;
        std::string message;
        check(!gf::hsd_load({data.data(), data.size()}, &bad, &message), what);
        check(!message.empty(), "refusal carries a message");
        check(bad.meshes.empty() && bad.joints.empty(), "refusal publishes nothing");
        check(!bad.error.empty(), "refusal is recorded on the scene");
        ++cases;
    };
    {
        auto data = archive;
        data[0] = 0xFF;   // absurd declared file size
        rejects(data, "bad declared size is refused");
    }
    {
        auto data = archive;
        data[24] = 1;     // reserved header word
        rejects(data, "nonzero reserved word is refused");
    }
    {
        // Point every relocation field at an offset outside the data block.
        auto data = archive;
        const std::size_t reloc_at = 0x20 + gf::u32(0);
        (void)reloc_at;
        gf::HsdScene bad;
        std::string message;
        std::vector<gf::u8> broken = data;
        // Corrupt the first relocation target to 0xFFFFFF00.
        const std::size_t data_size = (std::size_t(data[4]) << 24) | (std::size_t(data[5]) << 16) |
                                      (std::size_t(data[6]) << 8) | data[7];
        const std::size_t first_reloc = 0x20 + data_size;
        broken[first_reloc + 0] = 0xFF;
        broken[first_reloc + 1] = 0xFF;
        broken[first_reloc + 2] = 0xFF;
        broken[first_reloc + 3] = 0x00;
        check(!gf::hsd_load({broken.data(), broken.size()}, &bad, &message),
              "relocation outside the block is refused");
        check(bad.meshes.empty(), "no meshes on refusal");
        ++cases;
    }
    {
        auto data = archive;
        gf::HsdScene bad;
        std::string message;
        check(!gf::hsd_load({nullptr, 0}, &bad, &message), "empty buffer is refused");
        check(!message.empty(), "empty buffer has a message");
        ++cases;
    }
    return cases;
}

// GX texture units used by HSD (and shared with TPL): CMPR, I8 and IA8 are the
// ones TPL never ships, so they are pinned here with hand-computed pixels.
int texture_cases() {
    int cases = 0;
    // CMPR: one 8x8 block = four sub-blocks. Use c0 > c1 in the first sub-block
    // (white / black) and c0 <= c1 in the second (two mid greys).
    std::vector<gf::u8> cmpr(32, 0);
    const auto set16 = [&cmpr](std::size_t at, gf::u32 value) {
        cmpr[at] = gf::u8(value >> 8);
        cmpr[at + 1] = gf::u8(value);
    };
    set16(0, 0xFFFF);   // c0 white
    set16(2, 0x0000);   // c1 black
    // Index bits are two per pixel, most significant pair first: pixel 0 -> 0
    // (c0), pixel 1 -> 1 (c1), pixel 2 -> 2 (blend), pixel 3 -> 3 (blend).
    cmpr[4] = 0x1B;     // 00 01 10 11
    cmpr[5] = 0x00;
    cmpr[6] = 0x00;
    cmpr[7] = 0x00;
    set16(8, 0x0000);   // second sub-block: c0 black
    set16(10, 0xFFFF);  // c1 white  (c0 <= c1 branch)
    std::vector<gf::u8> rgba;
    std::string error;
    check(gf::gx_decode({cmpr.data(), cmpr.size()}, 8, 8, 14, {}, 0, 0, &rgba, &error),
          "CMPR decodes");
    check(rgba.size() == 8 * 8 * 4, "CMPR output size");
    check(rgba[3] == 255, "CMPR endpoint 0 is opaque");
    cases += 3;
    // Sub-block 0 pixel 0 uses index 0 -> c0 (white).
    check(rgba[0] == 255 && rgba[1] == 255 && rgba[2] == 255, "CMPR index 0 is c0");
    // Sub-block 0 pixel 1 uses index 1 -> c1 (black).
    check(rgba[4] == 0 && rgba[5] == 0 && rgba[6] == 0, "CMPR index 1 is c1");
    // Pixels 2 and 3 are the two blends between white and black; they must be
    // strictly between the endpoints and not equal to either.
    check(rgba[8] > 0 && rgba[8] < 255, "CMPR blend colour 2 is interpolated");
    check(rgba[12] > 0 && rgba[12] < 255, "CMPR blend colour 3 is interpolated");
    cases += 3;

    // I8: one 8x4 tile, intensity expands to all four channels.
    std::vector<gf::u8> i8(32, 0);
    i8[0] = 0x80;
    i8[1] = 0x00;
    check(gf::gx_decode({i8.data(), i8.size()}, 8, 4, 1, {}, 0, 0, &rgba, &error), "I8 decodes");
    check(rgba[0] == 0x80 && rgba[1] == 0x80 && rgba[2] == 0x80 && rgba[3] == 0x80,
          "I8 expands intensity into alpha too");
    check(rgba[4] == 0 && rgba[7] == 0, "I8 zero pixel is fully transparent");
    cases += 3;

    // IA8: intensity low byte, alpha high byte, 4x4 tiles.
    std::vector<gf::u8> ia8(32, 0);
    ia8[0] = 0xC0;   // high byte: alpha
    ia8[1] = 0x40;   // low byte: intensity
    check(gf::gx_decode({ia8.data(), ia8.size()}, 4, 4, 3, {}, 0, 0, &rgba, &error), "IA8 decodes");
    check(rgba[0] == 0x40 && rgba[1] == 0x40 && rgba[2] == 0x40, "IA8 intensity");
    check(rgba[3] == 0xC0, "IA8 alpha is the high byte");
    cases += 2;

    // Indexed texture without a palette is refused; C14X2 (format 10) too.
    std::vector<gf::u8> c8(32, 0);
    check(!gf::gx_decode({c8.data(), c8.size()}, 8, 4, 9, {}, 0, 0, &rgba, &error),
          "C8 without a palette is refused");
    ++cases;
    check(!gf::gx_decode({c8.data(), c8.size()}, 8, 8, 10, {}, 0, 0, &rgba, &error),
          "C14X2 (format 10) is refused, not decoded as CMPR");
    ++cases;
    // Truncated payload is refused rather than read past.
    check(!gf::gx_decode({c8.data(), 16}, 8, 4, 9, {c8.data(), 32}, 0, 16, &rgba, &error),
          "short payload is refused");
    ++cases;
    return cases;
}

// Real menu room: read-only, skipped when the private extraction is absent.
struct RealResult {
    bool present = false;
    bool ok = false;
    unsigned meshes = 0;
    unsigned joints = 0;
    unsigned textures = 0;
    unsigned vertices = 0;
    unsigned billboards = 0;
    unsigned translucent = 0;
};

RealResult real_room() {
    RealResult result;
    std::ifstream file("local/analysis/arc/tl00_mdl.arc", std::ios::binary);
    if (!file) return result;
    result.present = true;
    const std::vector<gf::u8> bytes((std::istreambuf_iterator<char>(file)), {});
    gf::HsdScene scene;
    std::string error;
    if (!gf::hsd_load({bytes.data(), bytes.size()}, &scene, &error)) return result;
    result.meshes = static_cast<unsigned>(scene.meshes.size());
    result.joints = static_cast<unsigned>(scene.joints.size());
    result.textures = static_cast<unsigned>(scene.textures.size());
    for (const gf::HsdMesh& mesh : scene.meshes) {
        result.vertices += static_cast<unsigned>(mesh.vertices.size());
        check(mesh.indices.size() % 3 == 0, "real indices are whole triangles");
        for (gf::u32 index : mesh.indices) {
            check(index < mesh.vertices.size(), "real index points at a real vertex");
        }
        for (const gf::HsdVertex& vertex : mesh.vertices) {
            check(std::isfinite(vertex.position[0]) && std::isfinite(vertex.position[1]) &&
                      std::isfinite(vertex.position[2]),
                  "real positions are finite");
        }
    }
    // Actual material alpha is separate from TOBJ combiner weights and PE state.
    unsigned translucent = 0;
    for (const gf::HsdMesh& mesh : scene.meshes) {
        check(mesh.material_alpha >= 0.0f && mesh.material_alpha <= 1.0f, "material alpha is bounded");
        if (mesh.material_alpha < 0.999f) ++translucent;
    }
    check(translucent > 0, "the real room has translucent meshes");
    result.translucent = translucent;

    // Billboard joints are exposed so the renderer can orient them per frame.
    unsigned billboards = 0;
    for (const gf::HsdJoint& joint : scene.joints) {
        if ((joint.flags & 0xE00u) != 0) ++billboards;
    }
    check(billboards == 10, "ten full billboard joints, not TEXGEN");
    result.billboards = billboards;
    for (const gf::HsdTexture& texture : scene.textures) {
        check(texture.width && texture.height, "real texture has dimensions");
        check(texture.rgba.size() == std::size_t(texture.width) * texture.height * 4,
              "real texture has a complete RGBA image");
    }
    result.ok = result.meshes > 60 && result.joints > 70 && result.textures > 10 &&
                result.vertices > 9000;
    return result;
}

// Sweep every member of the real archive whose header looks like an HSD archive.
// The point is robustness: a decoder that survives its own file but reads past a
// buffer on a different member is not safe to run on game data.
struct SweepResult {
    bool present = false;
    unsigned tried = 0;
    unsigned decoded = 0;
    unsigned refused = 0;
};

SweepResult sweep_archive() {
    SweepResult result;
    std::ifstream file("original/extracted/files/afs_data.afs", std::ios::binary);
    if (!file) return result;
    result.present = true;
    std::vector<gf::u8> bytes((std::istreambuf_iterator<char>(file)), {});
    gf::AfsArchive archive;
    std::string error;
    if (!archive.open({bytes.data(), bytes.size()}, &error)) {
        result.present = false;
        return result;
    }
    for (std::size_t i = 0; i < archive.size(); ++i) {
        const gf::ByteView member = archive.file(i);
        if (member.size < 0x20) continue;
        // HSD archives start with their own declared size, big-endian.
        const gf::u32 declared = (gf::u32(member.data[0]) << 24) | (gf::u32(member.data[1]) << 16) |
                                 (gf::u32(member.data[2]) << 8) | gf::u32(member.data[3]);
        if (declared < 0x20 || declared > member.size) continue;
        ++result.tried;
        gf::HsdScene scene;
        std::string member_error;
        if (gf::hsd_load(member, &scene, &member_error)) {
            ++result.decoded;
            // Whatever decoded must be self-consistent.
            for (const gf::HsdMesh& mesh : scene.meshes) {
                check(mesh.indices.size() % 3 == 0, "sweep indices are whole triangles");
                for (gf::u32 index : mesh.indices) {
                    check(index < mesh.vertices.size(), "sweep index points at a vertex");
                }
            }
        } else {
            ++result.refused;
            check(!member_error.empty(), "a refused member explains itself");
        }
    }
    return result;
}

} // namespace

int main() {
    const int cases = synthetic_cases() + texture_cases();
    const RealResult real = real_room();
    if (real.present) check(real.ok, "the real menu room decodes with the expected shape");
    const SweepResult sweep = sweep_archive();
    if (sweep.present) {
        check(sweep.tried > 100, "the archive has many HSD-shaped members");
        check(sweep.decoded > 100, "most of them decode");
        check(sweep.decoded + sweep.refused == sweep.tried, "every member is accounted for");
    }
    std::printf("{\"hsd_cases\":%d,\"real_present\":%s,\"real_meshes\":%u,\"real_joints\":%u,"
                "\"real_textures\":%u,\"real_vertices\":%u,\"real_billboards\":%u,"
                "\"real_alpha_below_one\":%u,\"sweep_tried\":%u,\"sweep_decoded\":%u,"
                "\"sweep_refused\":%u,\"all_passed\":true}\n",
                cases, real.present ? "true" : "false", real.meshes, real.joints, real.textures,
                real.vertices, real.billboards, real.translucent, sweep.tried, sweep.decoded,
                sweep.refused);
    return 0;
}
