// Asset decode tests: meshes and images are the first untrusted binary data the
// portable tree reads, so the decoders must round-trip valid input and fail
// closed on every documented corruption mode.
// No SDL, GPU, emulator or game data involved; fixtures are built in memory.
#include "engine/assets/mesh.hpp"
#include "engine/assets/texture.hpp"

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

void check(bool ok, const char* what) {
    if (!ok) {
        std::fprintf(stderr, "asset decode failure: %s\n", what);
        std::abort();
    }
}

gf::ByteView view(const std::vector<gf::u8>& bytes) {
    return gf::ByteView{bytes.data(), bytes.size()};
}

struct Counts {
    int mesh = 0;
    int texture = 0;
    int negative = 0;
};

bool same_vertex(const gf::MeshVertex& a, const gf::MeshVertex& b) {
    return gf::f32_to_bits(a.x) == gf::f32_to_bits(b.x) && gf::f32_to_bits(a.y) == gf::f32_to_bits(b.y) &&
           gf::f32_to_bits(a.z) == gf::f32_to_bits(b.z);
}

// Build an uncompressed true-colour TGA header plus raw BGR(A) pixel bytes.
std::vector<gf::u8> make_tga(gf::u32 width, gf::u32 height, gf::u8 bpp, bool top_left,
                             const std::vector<gf::u8>& bgra) {
    std::vector<gf::u8> tga(gf::kTgaHeaderBytes, 0);
    tga[2] = gf::kTgaTypeTrueColour;
    gf::store_be16(tga.data() + 12, gf::u16(width));
    gf::store_be16(tga.data() + 14, gf::u16(height));
    tga[16] = bpp;
    tga[17] = top_left ? gf::kTgaTopLeftOrigin : gf::u8(0);
    tga.insert(tga.end(), bgra.begin(), bgra.end());
    return tga;
}

Counts test_mesh() {
    Counts counts;
    std::string error = "sentinel";

    // 1. Encode -> decode round-trip, narrow index form chosen automatically.
    gf::Mesh mesh;
    mesh.vertices = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.25f}, {0.0f, 2.0f, -0.5f}, {3.5f, 1.0f, 1.0e6f}};
    mesh.indices = {0, 1, 2, 2, 1, 3};

    const std::vector<gf::u8> bytes = gf::encode_mesh(mesh);
    check(bytes.size() == gf::kMeshHeaderBytes + 4 * gf::kMeshVertexStride + 6 * 2,
          "small mesh encodes with 16-bit indices");
    check(gf::load_be32(bytes.data()) == gf::kMeshMagic, "mesh magic is big-endian 'GFM1'");
    check(gf::load_be32(bytes.data() + 4) == gf::kMeshVersion, "mesh version is written");
    check(gf::load_be32(bytes.data() + 16) == gf::kMeshFlag16BitIndices, "16-bit index flag set");

    gf::Mesh decoded;
    check(gf::decode_mesh(view(bytes), decoded, &error), "mesh round-trip decodes");
    check(error.empty(), "successful mesh decode clears the error");
    check(decoded.vertices.size() == mesh.vertices.size(), "vertex count round-trips");
    check(decoded.indices == mesh.indices, "index list round-trips");
    for (std::size_t i = 0; i < mesh.vertices.size(); ++i) {
        check(same_vertex(decoded.vertices[i], mesh.vertices[i]), "vertex values round-trip exactly");
    }
    ++counts.mesh;

    // 2. Wide (32-bit) index form, selected when an index does not fit in 16 bits.
    gf::Mesh wide;
    wide.vertices.resize(65537);
    wide.vertices[0] = {1.0f, 2.0f, 3.0f};
    wide.vertices[65536] = {4.0f, 5.0f, 6.0f};
    wide.indices = {0, 1, 65536};
    const std::vector<gf::u8> wide_bytes = gf::encode_mesh(wide);
    check(gf::load_be32(wide_bytes.data() + 16) == 0, "large indices force the 32-bit form");
    check(wide_bytes.size() == gf::kMeshHeaderBytes + 65537 * gf::kMeshVertexStride + 3 * 4,
          "32-bit index stride is used");
    gf::Mesh wide_back;
    check(gf::decode_mesh(view(wide_bytes), wide_back, &error), "32-bit-index mesh decodes");
    check(wide_back.indices == wide.indices, "32-bit indices round-trip");
    check(same_vertex(wide_back.vertices[65536], wide.vertices[65536]), "last wide vertex round-trips");
    ++counts.mesh;

    // 3. Every documented rejection rule, one case each.
    auto expect_reject = [&](std::vector<gf::u8> bad, const char* what) {
        gf::Mesh rejected;
        std::string why = "sentinel";
        check(!gf::decode_mesh(view(bad), rejected, &why), what);
        check(!why.empty() && why != "sentinel", "mesh rejection carries an error message");
        check(rejected.vertices.empty() && rejected.indices.empty(), "mesh rejection leaves no partial state");
        ++counts.negative;
        ++counts.mesh;
    };

    std::vector<gf::u8> bad = bytes;
    bad[0] ^= 0xFF;
    expect_reject(bad, "mesh: bad magic rejected");

    bad = bytes;
    bad[7] = 2;  // version 2, big-endian low byte
    expect_reject(bad, "mesh: unknown version rejected");

    bad = bytes;
    bad[8] = bad[9] = bad[10] = bad[11] = bad[12] = bad[13] = bad[14] = bad[15] = 0;
    expect_reject(bad, "mesh: zero vertex and index counts rejected");

    bad = bytes;
    gf::store_be32(bad.data() + 8, gf::u32(gf::kMaxMeshVertices) + 1u);
    expect_reject(bad, "mesh: vertex count above cap rejected");

    bad = bytes;
    gf::store_be32(bad.data() + 12, gf::u32(gf::kMaxMeshIndices) + 3u);
    expect_reject(bad, "mesh: index count above cap rejected");

    bad = bytes;
    bad.resize(gf::kMeshHeaderBytes + 4 * gf::kMeshVertexStride + 4);
    expect_reject(bad, "mesh: payload shorter than declared rejected");

    bad = bytes;
    bad.resize(gf::kMeshHeaderBytes - 1);
    expect_reject(bad, "mesh: buffer shorter than the header rejected");

    bad = bytes;
    gf::store_be32(bad.data() + 12, 4);  // 4 is not a multiple of 3
    expect_reject(bad, "mesh: non-triangle index count rejected");

    bad = bytes;
    gf::store_be16(bad.data() + gf::kMeshHeaderBytes + 4 * gf::kMeshVertexStride, 4);
    expect_reject(bad, "mesh: index outside the vertex range rejected");

    return counts;
}

Counts test_texture() {
    Counts counts;
    std::string error = "sentinel";

    // 1. 32-bit, top-left origin: BGR(A) source becomes RGBA output.
    const std::vector<gf::u8> bgra32 = {
        10, 20, 30, 40,   50,  60,  70,  80,
        90, 100, 110, 120, 130, 140, 150, 160,
    };
    const std::vector<gf::u8> tga32 = make_tga(2, 2, 32, true, bgra32);
    gf::Image image;
    check(gf::decode_tga(view(tga32), image, &error), "32-bit TGA decodes");
    check(error.empty(), "successful texture decode clears the error");
    check(image.width == 2 && image.height == 2, "texture dimensions decode");
    check(image.rgba.size() == 16, "texture RGBA size is width*height*4");
    const gf::u8 expected32[16] = {30, 20, 10, 40, 70, 60, 50, 80, 110, 100, 90, 120, 150, 140, 130, 160};
    check(image.rgba == std::vector<gf::u8>(expected32, expected32 + 16),
          "32-bit BGRA pixels become RGBA in order");
    ++counts.texture;

    // 2. 24-bit, top-left origin: channels reorder, alpha defaults to opaque.
    const std::vector<gf::u8> bgra24 = {
        10, 20, 30,   40,  50,  60,
        70, 80, 90,  100, 110, 120,
    };
    const std::vector<gf::u8> tga24 = make_tga(2, 2, 24, true, bgra24);
    gf::Image image24;
    check(gf::decode_tga(view(tga24), image24, &error), "24-bit TGA decodes");
    const gf::u8 expected24[16] = {30, 20, 10, 255, 60, 50, 40, 255, 90, 80, 70, 255, 120, 110, 100, 255};
    check(image24.rgba == std::vector<gf::u8>(expected24, expected24 + 16),
          "24-bit BGR pixels become opaque RGBA");
    ++counts.texture;

    // 3. Bottom-left origin flips rows so row 0 of the output is the image top.
    const std::vector<gf::u8> flipped_pixels = {
        // file row 0 == bottom row of the image
        1, 2, 3, 4,   5, 6, 7, 8,
        // file row 1 == top row of the image
        9, 10, 11, 12, 13, 14, 15, 16,
    };
    gf::Image flipped;
    check(gf::decode_tga(view(make_tga(2, 2, 32, false, flipped_pixels)), flipped, &error),
          "bottom-left 32-bit TGA decodes");
    check(flipped.rgba[0] == 11 && flipped.rgba[1] == 10 && flipped.rgba[2] == 9 && flipped.rgba[3] == 12,
          "bottom-left flip puts the source top row first");
    check(flipped.rgba[8] == 3 && flipped.rgba[9] == 2 && flipped.rgba[10] == 1 && flipped.rgba[11] == 4,
          "bottom-left flip puts the source bottom row last");
    ++counts.texture;

    const std::vector<gf::u8> flipped24_pixels = {
        1, 2, 3,   4, 5, 6,
        7, 8, 9,  10, 11, 12,
    };
    gf::Image flipped24;
    check(gf::decode_tga(view(make_tga(2, 2, 24, false, flipped24_pixels)), flipped24, &error),
          "bottom-left 24-bit TGA decodes");
    check(flipped24.rgba[0] == 9 && flipped24.rgba[1] == 8 && flipped24.rgba[2] == 7 &&
              flipped24.rgba[3] == 255 && flipped24.rgba[8] == 3 && flipped24.rgba[11] == 255,
          "24-bit flip keeps channel order and opaque alpha");
    ++counts.texture;

    // 4. encode_tga -> decode_tga round-trip (canonical 32-bit, top-left).
    const std::vector<gf::u8> encoded = gf::encode_tga(image);
    check(encoded.size() == gf::kTgaHeaderBytes + 16, "encoded TGA has a header plus RGBA pixels");
    check(encoded[16] == 32 && encoded[17] == gf::u8(gf::kTgaTopLeftOrigin | 0x08),
          "encoded TGA advertises 32-bit top-left pixels");
    gf::Image back;
    check(gf::decode_tga(view(encoded), back, &error), "encoded TGA decodes");
    check(back.width == image.width && back.height == image.height, "round-trip keeps dimensions");
    check(back.rgba == image.rgba, "round-trip preserves RGBA bytes");
    ++counts.texture;

    // 5. A non-zero ID field shifts pixel data and must be honoured, not skipped.
    std::vector<gf::u8> with_id = make_tga(1, 1, 32, true, {1, 2, 3, 4});
    with_id.insert(with_id.begin() + std::ptrdiff_t(gf::kTgaHeaderBytes), {'T', 'E', 'S', 'T'});
    with_id[0] = 4;
    gf::Image id_image;
    check(gf::decode_tga(view(with_id), id_image, &error), "TGA with an ID field decodes");
    check(id_image.rgba.size() == 4 && id_image.rgba[0] == 3 && id_image.rgba[1] == 2 &&
              id_image.rgba[2] == 1 && id_image.rgba[3] == 4,
          "pixel data starts after the ID field");
    ++counts.texture;

    // 6. Every documented rejection rule.
    auto expect_reject = [&](std::vector<gf::u8> bad, const char* what) {
        gf::Image rejected;
        std::string why = "sentinel";
        check(!gf::decode_tga(view(bad), rejected, &why), what);
        check(!why.empty() && why != "sentinel", "texture rejection carries an error message");
        check(rejected.width == 0 && rejected.height == 0 && rejected.rgba.empty(),
              "texture rejection leaves no partial state");
        ++counts.negative;
        ++counts.texture;
    };

    std::vector<gf::u8> bad = tga32;
    bad[2] = 10;  // RLE true-colour
    expect_reject(bad, "texture: RLE TGA rejected");

    bad = tga32;
    bad[1] = 1;  // colour map present
    bad[2] = 1;  // colour-mapped image type
    bad[6] = 1;  // colour map length 1
    bad[7] = 24;
    expect_reject(bad, "texture: colour-mapped TGA rejected");

    bad = tga32;
    bad[16] = 8;
    expect_reject(bad, "texture: 8-bit depth rejected");

    bad = tga32;
    bad[12] = 0;
    bad[13] = 0;
    expect_reject(bad, "texture: zero width rejected");

    bad = make_tga(gf::kMaxImageDimension + 1u, 1, 32, true, {});
    expect_reject(bad, "texture: oversize dimension rejected");

    bad = tga32;
    bad.pop_back();
    expect_reject(bad, "texture: truncated pixel data rejected");

    bad = tga32;
    bad.resize(gf::kTgaHeaderBytes - 1);
    expect_reject(bad, "texture: short header rejected");

    return counts;
}

} // namespace

int main() {
    const Counts mesh = test_mesh();
    const Counts texture = test_texture();
    std::printf("{\"mesh_cases\":%d,\"texture_cases\":%d,\"negative_cases\":%d,\"all_passed\":true}\n",
                mesh.mesh, texture.texture, mesh.negative + texture.negative);
    return 0;
}
