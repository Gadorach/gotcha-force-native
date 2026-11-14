#pragma once
// Portable mesh container and a self-describing binary decode.
//
// The wire format is explicit and big-endian. Nothing about host struct layout,
// padding, alignment or byte order reaches the stream:
//
//   u32 magic 'GFM1' (0x47464D31) | u32 version (1)
//   u32 vertex_count | u32 index_count
//   u32 flags   (bit0 set: indices are u16, otherwise u32; other bits ignored)
//   f32 x,y,z  per vertex (vertex_count * 12 bytes)
//   u16 or u32 indices (index_count entries)
//
// Decoding takes a plain byte window: no file I/O, no GPU types, no globals.
// Every read is bounds-checked and every validation rule fails closed, so a
// caller either gets a fully decoded mesh or an untouched-empty one plus a
// human-readable reason.
#include <cstddef>
#include <string>
#include <vector>

#include "engine/core/types.hpp"

namespace gf {

inline constexpr u32 kMeshMagic = 0x47464D31u;  // "GFM1"
inline constexpr u32 kMeshVersion = 1;
inline constexpr std::size_t kMeshHeaderBytes = 20;
inline constexpr u32 kMeshFlag16BitIndices = 1u << 0;
inline constexpr std::size_t kMeshVertexStride = 12;  // f32 x, y, z
inline constexpr std::size_t kMaxMeshVertices = 1u << 20;  // 1,048,576
inline constexpr std::size_t kMaxMeshIndices = 1u << 21;   // 2,097,152

struct MeshVertex {
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 z = 0.0f;
};

// Triangle-list mesh. `indices` is always 32-bit in memory; the file may store
// it narrower, which is a transport detail only.
struct Mesh {
    std::vector<MeshVertex> vertices;
    std::vector<u32> indices;
};

namespace detail {

inline bool mesh_error(std::string* error, std::string message) {
    if (error) *error = message;
    return false;
}

} // namespace detail

// Decode a checked-in mesh blob. Trailing bytes past the declared payload are
// ignored (they may carry future chunks); everything inside the declared extent
// must be valid.
inline bool decode_mesh(const ByteView& buffer, Mesh& out, std::string* error) {
    out = Mesh{};
    if (error) error->clear();

    if (!buffer.contains(0, kMeshHeaderBytes)) {
        return detail::mesh_error(error, "mesh: buffer shorter than the 20-byte header");
    }
    if (buffer.be32(0) != kMeshMagic) {
        return detail::mesh_error(error, "mesh: bad magic, expected 'GFM1' (0x47464D31)");
    }
    const u32 version = buffer.be32(4);
    if (version != kMeshVersion) {
        return detail::mesh_error(error, "mesh: unsupported version " + std::to_string(version));
    }

    const u32 vertex_count = buffer.be32(8);
    const u32 index_count = buffer.be32(12);
    const u32 flags = buffer.be32(16);

    if (vertex_count == 0 && index_count == 0) {
        return detail::mesh_error(error, "mesh: zero vertices and zero indices");
    }
    if (vertex_count > kMaxMeshVertices) {
        return detail::mesh_error(error,
                                  "mesh: vertex count " + std::to_string(vertex_count) +
                                      " exceeds cap " + std::to_string(kMaxMeshVertices));
    }
    if (index_count > kMaxMeshIndices) {
        return detail::mesh_error(error,
                                  "mesh: index count " + std::to_string(index_count) +
                                      " exceeds cap " + std::to_string(kMaxMeshIndices));
    }
    if (index_count % 3 != 0) {
        return detail::mesh_error(error,
                                  "mesh: index count " + std::to_string(index_count) +
                                      " is not a multiple of 3 (triangle list only)");
    }

    const bool indices_16 = (flags & kMeshFlag16BitIndices) != 0;
    const std::size_t index_stride = indices_16 ? 2 : 4;
    const std::size_t vertex_bytes = std::size_t(vertex_count) * kMeshVertexStride;
    const std::size_t index_bytes = std::size_t(index_count) * index_stride;
    if (!buffer.contains(kMeshHeaderBytes, vertex_bytes + index_bytes)) {
        return detail::mesh_error(
            error, "mesh: truncated payload, need " + std::to_string(vertex_bytes + index_bytes) +
                       " bytes after the header, have " +
                       std::to_string(buffer.size - kMeshHeaderBytes));
    }

    out.vertices.resize(vertex_count);
    for (u32 i = 0; i < vertex_count; ++i) {
        const std::size_t base = kMeshHeaderBytes + std::size_t(i) * kMeshVertexStride;
        out.vertices[i] = MeshVertex{buffer.bef32(base), buffer.bef32(base + 4), buffer.bef32(base + 8)};
    }

    const std::size_t index_base = kMeshHeaderBytes + vertex_bytes;
    out.indices.resize(index_count);
    for (u32 i = 0; i < index_count; ++i) {
        const std::size_t at = index_base + std::size_t(i) * index_stride;
        const u32 index = indices_16 ? u32(buffer.be16(at)) : buffer.be32(at);
        if (index >= vertex_count) {
            // Never hand a caller an index it cannot safely use.
            out = Mesh{};
            return detail::mesh_error(error,
                                      "mesh: index " + std::to_string(index) + " at slot " +
                                          std::to_string(i) + " is outside vertex count " +
                                          std::to_string(vertex_count));
        }
        out.indices[i] = index;
    }
    return true;
}

// Serialise a mesh for tests and round-trips. Uses the narrow 16-bit index form
// whenever every index fits, so the compact path is exercised by fixtures.
// Returns an empty vector when the mesh exceeds the documented decode caps
// (encode never produces a stream its own decoder would reject).
inline std::vector<u8> encode_mesh(const Mesh& mesh) {
    if (mesh.vertices.size() > kMaxMeshVertices || mesh.indices.size() > kMaxMeshIndices) {
        return {};
    }
    bool indices_16 = true;
    for (const u32 index : mesh.indices) {
        if (index > 0xFFFFu) {
            indices_16 = false;
            break;
        }
    }
    const std::size_t index_stride = indices_16 ? 2 : 4;
    std::vector<u8> bytes(kMeshHeaderBytes + mesh.vertices.size() * kMeshVertexStride +
                          mesh.indices.size() * index_stride);

    store_be32(bytes.data() + 0, kMeshMagic);
    store_be32(bytes.data() + 4, kMeshVersion);
    store_be32(bytes.data() + 8, u32(mesh.vertices.size()));
    store_be32(bytes.data() + 12, u32(mesh.indices.size()));
    store_be32(bytes.data() + 16, indices_16 ? kMeshFlag16BitIndices : 0u);

    for (std::size_t i = 0; i < mesh.vertices.size(); ++i) {
        const std::size_t base = kMeshHeaderBytes + i * kMeshVertexStride;
        store_be_f32(bytes.data() + base + 0, mesh.vertices[i].x);
        store_be_f32(bytes.data() + base + 4, mesh.vertices[i].y);
        store_be_f32(bytes.data() + base + 8, mesh.vertices[i].z);
    }

    const std::size_t index_base = kMeshHeaderBytes + mesh.vertices.size() * kMeshVertexStride;
    for (std::size_t i = 0; i < mesh.indices.size(); ++i) {
        const std::size_t at = index_base + i * index_stride;
        if (indices_16) {
            store_be16(bytes.data() + at, u16(mesh.indices[i]));
        } else {
            store_be32(bytes.data() + at, mesh.indices[i]);
        }
    }
    return bytes;
}

} // namespace gf
