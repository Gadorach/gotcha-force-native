#pragma once
// HSD scene decoder: the model format the game uses for rooms, characters and
// effects.
//
// An HSD archive is a big-endian blob with an internal relocation table. The
// container is:
//
//   0x00  8 x u32 BE: file_size, data_size, reloc_count, public_count,
//                     extern_count, version, reserved0, reserved1
//   0x20  data block (data_size bytes)
//         relocation table: reloc_count x u32 BE, each the data-relative offset
//         of a field that holds a data-relative pointer
//         symbols: (public + extern) x { u32 offset, u32 name_offset }
//         string table for the symbol names
//
// Every field offset in this file is relative to the data block. Pointer fields
// are only followed when the relocation table says they are pointers; a random
// word that happens to look like an offset is never dereferenced.
//
// Structures decoded (offsets from the pinned noclip SYSDOLPHIN.ts reference,
// validated against real archives by tools/analyze/hsd_scene.py):
//   JOBJ 0x40, DOBJ 0x10, MOBJ 0x18, POBJ 0x18, TOBJ 0x5C,
//   ImageDesc 0x18, TlutDesc 0x10, Material 0x14, vertex descriptor 0x18.
//
// Geometry comes from POBJ display lists: a byte stream where `opcode & 0xF8`
// selects the primitive, the low three bits select the vertex format, a u16 BE
// vertex count follows, and then `count * stride` bytes of inline vertex data
// (indices or direct values). `nDisp * 32` bounds the stream; zero is NOP/padding.
// Stateful or unknown commands are refused rather than silently skipped.
//
// Animation lives in hsd_anim.hpp. Deforming meshes and unsupported material
// stages are tagged for inspection; MenuScene refuses to render them as static
// equivalents. The default combiner/PE subset used by the menu is decoded.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <cstddef>
#include <set>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "engine/assets/gx_texture.hpp"
#include "engine/core/types.hpp"
#include "engine/graphics/lighting.hpp"
#include "engine/assets/hsd_lighting.hpp"

namespace gf {

inline constexpr u32 kHsdNoTexture = 0xFFFFFFFFu;
inline constexpr u32 kHsdNoJoint = 0xFFFFFFFFu;
inline constexpr std::size_t kHsdHeaderBytes = 0x20;
inline constexpr std::size_t kHsdMaxNodes = 20000;
inline constexpr std::size_t kHsdMaxRelocations = 1u << 20;
inline constexpr std::size_t kHsdMaxVerticesPerMesh = 200000;
inline constexpr std::size_t kHsdMaxTextureDimension = 2048;
inline constexpr std::size_t kHsdMaxDecodedBytes = 256u * 1024u * 1024u;

struct HsdTexture {
    u32 width = 0;
    u32 height = 0;
    u32 format = 0;
    bool has_palette = false;
    u32 palette_format = 0;
    std::vector<u8> rgba;   // width * height * 4, RGBA8, top row first
    u8 wrap_s = 0, wrap_t = 0;
    bool linear = true;
    // Explicit immutable-bank mode. Legacy decoded textures retain rgba.
    std::shared_ptr<const std::vector<u8>> shared_rgba;
    const std::vector<u8>& pixels()const{return shared_rgba?*shared_rgba:rgba;}
};

struct HsdVertex {
    f32 position[3] = {0.0f, 0.0f, 0.0f};
    f32 normal[3] = {0.0f, 0.0f, 0.0f};
    f32 uv[2] = {0.0f, 0.0f};
    u8 colour[4] = {255, 255, 255, 255};
    f32 uv1[2] = {0.0f, 0.0f};
    u8 matrix_index = 0; // raw GX PNMTXIDX row selector, not a host joint index
    std::array<u8,8> texture_matrix_indices={60,60,60,60,60,60,60,60};
    u32 source_position_index=0xffffffffu; // Indexed shape lookup; direct vertices remain absent.
};

// Small value state can be evaluated per draw without copying geometry buffers.
// This is host metadata, never a serialized/source-memory struct layout.
struct HsdMeshProperties {
    u32 texture = kHsdNoTexture;                       // index into HsdScene::textures
    u8 material_colour[4] = {255, 255, 255, 255};   // diffuse
    u8 material_ambient[4] = {255, 255, 255, 255};   // flat term, from the material
    bool cull_backface = true;
    bool cull_frontface = false;
    u32 render_mode = 1;
    f32 material_alpha = 1.0f;
    bool blend = false, depth_test = true, depth_write = true;
    bool colour_write = true,alpha_write = false; // HSD default: RGB on, alpha off
    u8 blend_src = 5, blend_dst = 6, depth_compare = 6;
    u8 alpha_compare0 = 7, alpha_compare1 = 7, alpha_op = 0, alpha_ref0 = 0, alpha_ref1 = 0;
    u8 texture_colour_mode = 4, texture_alpha_mode = 3;
    f32 texture_weight = 1.0f;
    // Geometry may be inspected/animated without claiming unsupported materials
    // can render correctly. MenuScene refuses these explicitly.
    bool material_supported = true;
    // A retained single TObj with flags0 is not included in any MObj lighting
    // expression (DIFFUSE/AMBIENT/SPECULAR/EXT). Keep its descriptor/texture for
    // metadata, but explicitly consume the inactive stage at draw time.
    bool texture_stage_inactive = false;
    bool texture_colour_lerp = false;
    u32 texture_low_colour = 0, texture_high_colour = 0xFFFFFFFFu;
    bool deformation_required = false;
    u8 texture_matrix_mask=0,texture_coordinate_mode=0;
    bool texture_matrix_required=false,generated_uv_required=false;
    bool reflection_uv_supported=false;
    u32 material_offset = 0;
    u32 dobj_offset = 0;
    u32 pobj_offset = 0,skin_binding = 0;
    u16 pobj_flags = 0;
    bool hidden = false;
    bool texture_uv1 = false;
    u8 material_specular[4] = {};
    f32 shininess = 50.0f;
    bool texture_after_lighting = false; // HSD LIGHTMAP_EXT (vs diffuse/ambient)
};

struct HsdMesh : HsdMeshProperties {
    std::vector<HsdVertex> vertices;
    std::vector<u32> indices;                          // triangles
};

struct HsdJoint {
    // Raw JOBJ flags. Bits 8..11 are the billboard modes, which the renderer has
    // to apply per frame (the file pose is not the on-screen orientation).
    u32 flags = 0;
    f32 rotation[3] = {0.0f, 0.0f, 0.0f};
    f32 scale[3] = {1.0f, 1.0f, 1.0f};
    f32 translation[3] = {0.0f, 0.0f, 0.0f};
    u32 parent = kHsdNoJoint;
    u32 archive_offset = 0;
    u32 model_root = kHsdNoJoint; // published model-record index, not discovery order
};

struct HsdScene {
    std::vector<HsdMesh> meshes;
    std::vector<HsdTexture> textures;
    std::vector<HsdJoint> joints;
    std::vector<u32> mesh_joint;   // parallel to meshes
    std::vector<u32> model_roots; // published JOBJ root offsets, in original record order
    std::string error;
    // Explicit scene root-record bindings (animation archive offset -> JOBJ root).
    std::vector<std::pair<u32,u32>> animation_roots;
    std::vector<std::pair<u32,u32>> material_animation_roots;
    struct MenuSeal {u32 item=0,caption_dobj=0,gear_dobj=0;f32 centre[3]={};f32 radius=1;};
    std::vector<MenuSeal> menu_seals;
    unsigned menu_selection = 5;
    SceneLighting lighting;
    struct MenuMotion {
        struct Binding {u32 item=0,gear=0,identifier=0,caption=0;s32 step=0;};
        bool present=false;
        f32 angle_unit=0;
        s32 selector_step=0,identifier_step=0;
        u32 selector_joint=0;
        std::vector<Binding> bindings;
    } menu_motion;
    // Optional original-bank indices for compact immutable texture selections.
    // Mesh indices stay local; shared upload handles preserve the bank namespace.
    std::vector<u32> source_texture_slots;
};

inline bool hsd_compact_scene_textures(const HsdScene& full,HsdScene* selected){
    if(!selected||selected==&full||(!full.source_texture_slots.empty()&&full.source_texture_slots.size()!=full.textures.size()))return false;
    std::vector<u32> sources,slots,indices;std::vector<HsdTexture> textures;
    for(const auto& mesh:selected->meshes){if(mesh.texture==kHsdNoTexture){indices.push_back(kHsdNoTexture);continue;}if(mesh.texture>=full.textures.size())return false;
        const auto source=mesh.texture;const auto found=std::find(sources.begin(),sources.end(),source);
        if(found==sources.end()){indices.push_back(u32(textures.size()));sources.push_back(source);slots.push_back(full.source_texture_slots.empty()?source:full.source_texture_slots[source]);textures.push_back(full.textures[source]);}
        else indices.push_back(u32(found-sources.begin()));
    }
    selected->textures=std::move(textures);selected->source_texture_slots=std::move(slots);for(unsigned i=0;i<indices.size();++i)selected->meshes[i].texture=indices[i];return true;
}

// Metadata/sampler/index identities remain distinct. Only equal immutable pixel
// payloads share ownership; there is no global cache or borrowed archive pointer.
inline bool hsd_share_texture_images(HsdScene& scene){
    for(const auto& t:scene.textures)if((t.shared_rgba&&!t.rgba.empty())||u64(t.width)*t.height*4!=t.pixels().size()||t.pixels().empty())return false;
    std::map<u64,std::vector<std::shared_ptr<const std::vector<u8>>>> pool;
    for(auto& t:scene.textures){u64 key=1469598103934665603ull;for(auto byte:t.pixels())key=(key^byte)*1099511628211ull;std::shared_ptr<const std::vector<u8>> found;
        for(const auto& candidate:pool[key])if(*candidate==t.pixels()){found=candidate;break;}
        if(!found){found=t.shared_rgba?t.shared_rgba:std::make_shared<const std::vector<u8>>(std::move(t.rgba));pool[key].push_back(found);}
        t.shared_rgba=std::move(found);std::vector<u8>().swap(t.rgba);
    }return true;
}

namespace hsd_detail {

inline u32 be32(const ByteView& data, std::size_t at) {
    return (u32(data.data[at]) << 24) | (u32(data.data[at + 1]) << 16) |
           (u32(data.data[at + 2]) << 8) | u32(data.data[at + 3]);
}

inline u16 be16(const ByteView& data, std::size_t at) {
    return static_cast<u16>((u32(data.data[at]) << 8) | u32(data.data[at + 1]));
}

inline f32 be_f32(const ByteView& data, std::size_t at) {
    const u32 bits = be32(data, at);
    f32 value = 0.0f;
    static_assert(sizeof(f32) == sizeof(u32), "f32 must be 32 bits");
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

struct Reader {
    ByteView block{};
    std::vector<u32> targets{};   // sorted data-relative offsets a relocation points at
    std::vector<u32> fields{};    // sorted relocated FIELD offsets

    bool has(std::size_t at, std::size_t bytes) const {
        return block.data != nullptr && at <= block.size && bytes <= block.size - at;
    }

    // A field is a usable pointer when it is 0/0xFFFFFFFF (null) or a validated
    // relocation target.
    bool ptr(std::size_t at, u32* out) const {
        if (!has(at, 4)) return false;
        const u32 value = be32(block, at);
        if (value == 0 || value == 0xFFFFFFFFu) {
            *out = 0;
            return true;
        }
        if (!std::binary_search(fields.begin(), fields.end(), static_cast<u32>(at))) return false;
        if (!std::binary_search(targets.begin(), targets.end(), value)) return false;
        *out = value;
        return true;
    }

    bool finite(const ByteView& data, std::size_t at, unsigned count) const {
        for (unsigned i = 0; i < count; ++i) {
            if (!has(at + i * 4, 4)) return false;
            const f32 value = be_f32(data, at + i * 4);
            if (!std::isfinite(value) || std::fabs(value) > 1.0e7f) return false;
        }
        return true;
    }
};

// Structural classifiers: positive evidence only. The class-name pointer is NULL
// in every archive observed in this game, so a structure is identified by its
// field layout, and the callers cross-check the graph (JOBJ->DOBJ->MOBJ->TOBJ,
// DOBJ->POBJ->vtx-desc) so a false positive cannot survive on its own.
inline bool class_ok(const Reader& reader, std::size_t at) {
    u32 value = 0;
    if (!reader.ptr(at, &value)) return false;
    // 0 (null) and validated targets are the only accepted class pointers.
    return true;
}

inline bool as_dobj(const Reader& r, std::size_t at);

// A JOBJ must look right *and* hold something: a validated DOBJ, or a child that
// does. That second half is what separates real joints from unrelated relocation
// targets whose floats happen to be finite.
inline bool as_jobj(const Reader& r, std::size_t at, unsigned depth = 0) {
    if (depth > 64) return false;
    if (!r.has(at, 0x40)) return false;
    if (!class_ok(r, at)) return false;
    // Flags outside the reviewed JOBJ mask are a strong negative signal.
    const u32 flags = be32(r.block, at + 4);
    if ((flags & ~0x73FFFFFFu) != 0) return false;
    u32 child = 0, sibling = 0, contents = 0, inverse = 0, robj = 0;
    if (!r.ptr(at + 0x08, &child) || !r.ptr(at + 0x0C, &sibling) || !r.ptr(at + 0x10, &contents) ||
        !r.ptr(at + 0x38, &inverse) || !r.ptr(at + 0x3C, &robj)) {
        return false;
    }
    if (!r.finite(r.block, at + 0x14, 3) || !r.finite(r.block, at + 0x20, 3) ||
        !r.finite(r.block, at + 0x2C, 3)) {
        return false;
    }
    // Degenerate scale is a classification error signal in the reference too.
    for (unsigned i = 0; i < 3; ++i) {
        const f32 value = be_f32(r.block, at + 0x20 + i * 4);
        if (std::fabs(value) <= 1.0e-6f) return false;
    }
    if (contents != 0 && as_dobj(r, contents)) return true;
    if (child != 0 && as_jobj(r, child, depth + 1)) return true;
    return false;
}

inline bool as_pobj(const Reader& r, std::size_t at);   // mutually validating
inline bool as_mobj(const Reader& r, std::size_t at);

inline bool as_dobj(const Reader& r, std::size_t at) {
    if (!r.has(at, 0x10)) return false;
    u32 sibling = 0, mobj = 0, pobj = 0;
    if (!class_ok(r, at) || !r.ptr(at + 0x04, &sibling)) return false;
    if (!r.ptr(at + 0x08, &mobj) || !r.ptr(at + 0x0C, &pobj)) return false;
    if (mobj == 0 && pobj == 0) return false;
    if (pobj != 0 && !as_pobj(r, pobj)) return false;
    if (mobj != 0 && !as_mobj(r, mobj)) return false;
    return true;
}

inline bool as_mobj(const Reader& r, std::size_t at) {
    if (!r.has(at, 0x18)) return false;
    u32 tex = 0, material = 0, render = 0, pe = 0;
    return class_ok(r, at) && r.ptr(at + 0x08, &tex) && r.ptr(at + 0x0C, &material) &&
           r.ptr(at + 0x10, &render) && r.ptr(at + 0x14, &pe);
}

// A descriptor list is a strong cross-check a POBJ has: matrix-selector bytes
// may precede its position attribute; entries stay inside the reviewed
// field ranges, and the list is terminated by GX_VA_NULL (0xFF) within 32
// entries. A structure that fails this is not a POBJ, whatever its fields say.
inline bool valid_vtx_list(const Reader& r, u32 list) {
    if (list == 0) return false;
    bool saw_position = false;
    for (std::size_t i = 0; i < 32; ++i) {
        const std::size_t at = std::size_t(list) + i * 0x18;
        if (!r.has(at, 0x18)) return false;
        const u32 attr = be32(r.block, at);
        if (attr == 0xFF) return i > 0 && saw_position;
        const u32 type = be32(r.block, at + 4);
        const u32 comp_count = be32(r.block, at + 8);
        const u32 comp_type = be32(r.block, at + 0x0C);
        if (attr > 25 || type > 3 || comp_count > 2 || comp_type > 5) return false;
        if (attr == 9) saw_position = true;
    }
    return false;
}

inline bool as_pobj(const Reader& r, std::size_t at) {
    if (!r.has(at, 0x18)) return false;
    u32 sibling = 0, vtx = 0, disp = 0;
    if (!class_ok(r, at) || !r.ptr(at + 0x04, &sibling) || !r.ptr(at + 0x08, &vtx) ||
        !r.ptr(at + 0x10, &disp)) {
        return false;
    }
    const u16 flags = be16(r.block, at + 0x0C);
    // Original8024A708 copies all descriptor flags, and8024C844/8024CAD8
    // dispatch type/cull independently of bit0. Real Sasuke envelope POBJs
    // carry A001. Rejecting this retained bit silently lost its whole body.
    if ((flags & ~u16(0xF001)) != 0 || vtx == 0 || disp == 0) return false;
    if (disp >= r.block.size) return false;
    // A real display list starts with a draw primitive; anything else means this
    // structure is not a POBJ even if every field validated in isolation.
    const u8 first = r.block.data[disp];
    if (first != 0x80 && first != 0x90 && first != 0x98 && first != 0xA0) return false;
    return valid_vtx_list(r, vtx);
}

inline bool as_image_desc(const Reader& r, std::size_t at) {
    if (!r.has(at, 0x18)) return false;
    u32 data = 0;
    if (!r.ptr(at, &data) || data == 0) return false;
    const u32 width = be16(r.block, at + 4);
    const u32 height = be16(r.block, at + 6);
    const u32 format = be32(r.block, at + 8);
    const u32 mipmaps = be32(r.block, at + 0x0C);
    if (width == 0 || height == 0 || width > kHsdMaxTextureDimension ||
        height > kHsdMaxTextureDimension) {
        return false;
    }
    switch (format) {
    case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 8: case 9: case 14:
        break;
    default:
        return false;
    }
    if (mipmaps > 16) return false;
    return r.finite(r.block, at + 0x10, 2);
}

inline bool as_tlut_desc(const Reader& r, std::size_t at) {
    if (!r.has(at, 0x10)) return false;
    u32 data = 0;
    if (!r.ptr(at, &data) || data == 0) return false;
    const u32 format = be32(r.block, at + 4);
    const u32 entries = be16(r.block, at + 0x0C);
    return format <= 2 && entries >= 1 && entries <= 256;
}

inline bool as_tobj(const Reader& r, std::size_t at) {
    if (!r.has(at, 0x5C)) return false;
    u32 sibling = 0, image = 0, tlut = 0, lod = 0, tev = 0;
    if (!class_ok(r, at) || !r.ptr(at + 0x04, &sibling) || !r.ptr(at + 0x4C, &image) ||
        !r.ptr(at + 0x50, &tlut) || !r.ptr(at + 0x54, &lod) || !r.ptr(at + 0x58, &tev)) {
        return false;
    }
    if (image == 0) return false;
    const u32 wrap_s = be32(r.block, at + 0x34);
    const u32 wrap_t = be32(r.block, at + 0x38);
    if (wrap_s > 2 || wrap_t > 2) return false;
    if (r.block.data[at + 0x3C] == 0 || r.block.data[at + 0x3D] == 0) return false;
    return r.finite(r.block, at + 0x10, 9);
}

} // namespace hsd_detail

// ---- geometry ---------------------------------------------------------

namespace hsd_detail {

struct VtxDesc {
    u32 attr = 0;
    u32 type = 0;        // 0 none, 1 direct, 2 index8, 3 index16
    u32 comp_count = 0;  // GXCompCnt
    u32 comp_type = 0;
    u32 shift = 0;
    u32 stride = 0;
    u32 array = 0;       // data-relative
};

// Inline bytes this descriptor consumes per vertex in the display list.
inline bool inline_size(const VtxDesc& d, std::size_t* out, std::string* error) {
    switch (d.type) {
    case 0: *out = 0; return true;
    case 2: *out = 1; return true;
    case 3: *out = 2; return true;
    case 1:
        if(d.attr<=8){*out=1;return true;} // GX position/texture matrix selectors are bytes
        if((d.attr==11 || d.attr==12) && d.comp_type==5){*out=4;return true;}
        if(d.comp_type==4){
            if(d.attr==9)*out=(d.comp_count==0?2:3)*4;
            else if(d.attr==10 && d.comp_count==0)*out=12;
            else if(d.attr==13 || d.attr==14)*out=(d.comp_count==0?1:2)*4;
            else {if(error)*error="hsd: unsupported direct format";return false;}
            return true;
        }
        if (error) *error = "hsd: unsupported direct vertex format";
        return false;
    default:
        if (error) *error = "hsd: unknown vertex attribute type";
        return false;
    }
}

inline bool read_descriptors(const Reader& r, u32 list, std::vector<VtxDesc>* out,
                             std::string* error) {
    out->clear();
    for (std::size_t i = 0; i < 32; ++i) {
        const std::size_t at = std::size_t(list) + i * 0x18;
        if (!r.has(at, 0x18)) {
            if (error) *error = "hsd: vertex descriptor list runs past the data block";
            return false;
        }
        const u32 attr = be32(r.block, at);
        if (attr == 0xFF) {
            if (out->empty()) {
                if (error) *error = "hsd: vertex descriptor list is empty";
                return false;
            }
            return true;
        }
        VtxDesc d;
        d.attr = attr;
        d.type = be32(r.block, at + 4);
        d.comp_count = be32(r.block, at + 8);
        d.comp_type = be32(r.block, at + 0x0C);
        d.shift = r.block.data[at + 0x10];
        d.stride = be16(r.block, at + 0x12);
        d.array = be32(r.block, at + 0x14);
        if(d.type==2 || d.type==3){
            if(!std::binary_search(r.fields.begin(),r.fields.end(),static_cast<u32>(at+0x14)) || !d.stride){
                if(error)*error="hsd: indexed array field is not relocated or has zero stride";
                return false;
            }
        }
        if (d.attr > 25 || d.type > 3 || d.comp_count > 2 || d.comp_type > 5) {
            if (error) {
                *error = "hsd: vertex descriptor field outside the supported range (attr=" +
                         std::to_string(d.attr) + " type=" + std::to_string(d.type) +
                         " compcnt=" + std::to_string(d.comp_count) +
                         " comptype=" + std::to_string(d.comp_type) + " at " +
                         std::to_string(at) + ")";
            }
            return false;
        }
        if(d.attr<=8&&d.type!=0&&d.type!=1){if(error)*error="hsd: matrix selectors must be direct bytes";return false;}
        if (d.attr > 8 && d.attr != 9 && d.attr != 10 && d.attr != 11 && d.attr != 12 &&
            d.attr != 13 && d.attr != 14) {
            // Only the attributes this game actually ships are decoded.
            if (error) *error = "hsd: unsupported vertex attribute " + std::to_string(d.attr);
            return false;
        }
        out->push_back(d);
    }
    if (error) *error = "hsd: too many vertex descriptors";
    return false;
}

// Reads one indexed element and returns it in `v`. Indexed arrays use
// `array + index * stride`, both data-relative.
inline bool read_element(const Reader& r, const VtxDesc& d, u32 index, HsdVertex* v,
                         bool* have_position, bool* have_normal, bool* have_uv, bool* have_colour,
                         std::string* error) {
    if(d.attr<=8){
        u32 raw=index;
        if(d.type==1){if(!r.has(d.array,1)){if(error)*error="hsd: matrix index outside display list";return false;}raw=r.block.data[d.array];}
        if(raw>255){if(error)*error="hsd: matrix index exceeds byte domain";return false;}
        if(d.attr==0)v->matrix_index=u8(raw);
        else v->texture_matrix_indices[d.attr-1]=u8(raw);
        return true;
    }
    if(d.attr==9&&(d.type==2||d.type==3))v->source_position_index=index;
    const u64 at = u64(d.array) + u64(index) * d.stride;
    if (d.attr == 11 || d.attr == 12) {
        if (d.comp_type != 5) {
            if (error) *error = "hsd: colour attribute is not RGBA8";
            return false;
        }
        if (!r.has(std::size_t(at), 4)) {
            if (error) *error = "hsd: colour element outside the data block";
            return false;
        }
        v->colour[0] = r.block.data[at + 0];
        v->colour[1] = r.block.data[at + 1];
        v->colour[2] = r.block.data[at + 2];
        v->colour[3] = r.block.data[at + 3];
        *have_colour = true;
        return true;
    }
    if (d.comp_type != 4) {
        if (error) *error = "hsd: non-float position/normal/texcoord is not supported";
        return false;
    }
    unsigned components = 0;
    if (d.attr == 9) components = d.comp_count == 0 ? 2u : 3u;
    else if (d.attr == 10) {
        if (d.comp_count != 0) {
            if (error) *error = "hsd: NBT normals are not supported";
            return false;
        }
        components = 3;
    } else {
        components = d.comp_count == 0 ? 1u : 2u;   // TEX0/TEX1
    }
    if (!r.has(std::size_t(at), components * 4)) {
        if (error) *error = "hsd: vertex element outside the data block";
        return false;
    }
    if(d.stride && d.stride<components*4){if(error)*error="hsd: array stride shorter than element";return false;}
    f32 values[3] = {0.0f, 0.0f, 0.0f};
    for (unsigned i = 0; i < components; ++i) values[i] = be_f32(r.block, std::size_t(at) + i * 4);
    for (unsigned i=0;i<components;++i) if(!std::isfinite(values[i])){if(error)*error="hsd: nonfinite vertex attribute";return false;}
    if (d.attr == 9) {
        for (unsigned i = 0; i < 3; ++i) v->position[i] = values[i];
        *have_position = true;
    } else if (d.attr == 10) {
        for (unsigned i = 0; i < 3; ++i) v->normal[i] = values[i];
        *have_normal = true;
    } else {
        f32* uv=d.attr==14?v->uv1:v->uv;
        uv[0] = values[0];
        uv[1] = components > 1 ? values[1] : 0.0f;
        *have_uv = true;
    }
    return true;
}

inline bool decode_pobj(const Reader& r, u32 pobj, HsdMesh* mesh, std::string* error) {
    std::vector<VtxDesc> descs;
    if (!read_descriptors(r, be32(r.block, pobj + 0x08), &descs, error)) return false;

    std::size_t stride = 0;
    for (const VtxDesc& d : descs) {
        std::size_t bytes = 0;
        if (!inline_size(d, &bytes, error)) return false;
        stride += bytes;
    }
    if (stride == 0) {
        if (error) *error = "hsd: display list has no inline vertex data";
        return false;
    }

    const u16 flags = be16(r.block, pobj + 0x0C);
    mesh->pobj_offset=pobj;mesh->pobj_flags=flags;mesh->skin_binding=be32(r.block,pobj+0x14);
    if(mesh->skin_binding==0xffffffffu)mesh->skin_binding=0;
    mesh->cull_backface = (flags & 0x8000u) != 0;
    mesh->cull_frontface = (flags & 0x4000u) != 0;
    mesh->deformation_required = (flags & 0x3000u) != 0||mesh->skin_binding!=0;
    for(const auto& d:descs)if(d.attr>=1&&d.attr<=8&&d.type)mesh->texture_matrix_mask|=u8(1u<<(d.attr-1));

    const std::size_t start = be32(r.block, pobj + 0x10);
    std::size_t at = start;
    const u64 extent = u64(be16(r.block, pobj + 0x0E)) * 32;
    if (!extent || !r.has(start, static_cast<std::size_t>(extent))) {
        if (error) *error = "hsd: invalid display list extent (nDisp * 32)";
        return false;
    }
    const std::size_t end = start + static_cast<std::size_t>(extent);
    while (at < end) {
        const u8 cmd = r.block.data[at];
        if (cmd == 0x00) { ++at; continue; }        // GX NOP/padding
        const u32 primitive = cmd & 0xF8u;
        if (primitive == 0x80 || primitive == 0x90 || primitive == 0x98 || primitive == 0xA0) {
            if (end - at < 3) {
                if (error) *error = "hsd: display list command runs past the data block";
                return false;
            }
            const u32 count = be16(r.block, at + 1);
            at += 3;
            const std::size_t data_bytes = stride * count;
            if (data_bytes > end - at) {
                if (error) *error = "hsd: display list vertex data runs past the data block";
                return false;
            }
            if (mesh->vertices.size() + count > kHsdMaxVerticesPerMesh) {
                if (error) *error = "hsd: mesh vertex count exceeds the cap";
                return false;
            }
            const u32 base = static_cast<u32>(mesh->vertices.size());
            for (u32 v = 0; v < count; ++v) {
                HsdVertex vertex;
                bool have_position = false, have_normal = false, have_uv = false, have_colour = false;
                std::size_t cursor = at + std::size_t(v) * stride;
                for (const VtxDesc& d : descs) {
                    if (d.type == 0) continue;
                    u32 index = 0;
                    if (d.type == 3) {
                        index = be16(r.block, cursor);
                        cursor += 2;
                    } else if (d.type == 2) {
                        index = r.block.data[cursor];
                        cursor += 1;
                    }
                    VtxDesc element=d;
                    if(d.type==1){
                        std::size_t size=0;if(!inline_size(d,&size,error))return false;
                        element.array=static_cast<u32>(cursor);element.stride=0;cursor+=size;
                    }
                    if (!read_element(r, element, index, &vertex, &have_position, &have_normal, &have_uv,
                                      &have_colour, error)) {
                        return false;
                    }
                }
                if (!have_position) {
                    if (error) *error = "hsd: a vertex has no position";
                    return false;
                }
                if(vertex.matrix_index)mesh->deformation_required=true;
                for(auto index:vertex.texture_matrix_indices)if(index!=60)mesh->texture_matrix_required=true;
                mesh->vertices.push_back(vertex);
            }
            at += data_bytes;

            switch (primitive) {
            case 0x90:   // triangles
                for (u32 i = 0; i + 2 < count; i += 3) {
                    mesh->indices.push_back(base + i);
                    mesh->indices.push_back(base + i + 1);
                    mesh->indices.push_back(base + i + 2);
                }
                break;
            case 0x98:   // triangle strip, winding flips every other triangle
                for (u32 i = 0; i + 2 < count; ++i) {
                    if (i % 2 == 0) {
                        mesh->indices.push_back(base + i);
                        mesh->indices.push_back(base + i + 1);
                        mesh->indices.push_back(base + i + 2);
                    } else {
                        mesh->indices.push_back(base + i + 1);
                        mesh->indices.push_back(base + i);
                        mesh->indices.push_back(base + i + 2);
                    }
                }
                break;
            case 0x80:   // quads
                for (u32 i = 0; i + 3 < count; i += 4) {
                    mesh->indices.push_back(base + i);
                    mesh->indices.push_back(base + i + 1);
                    mesh->indices.push_back(base + i + 2);
                    mesh->indices.push_back(base + i);
                    mesh->indices.push_back(base + i + 2);
                    mesh->indices.push_back(base + i + 3);
                }
                break;
            default:     // 0xA0 fan
                for (u32 i = 1; i + 1 < count; ++i) {
                    mesh->indices.push_back(base);
                    mesh->indices.push_back(base + i);
                    mesh->indices.push_back(base + i + 1);
                }
                break;
            }
            continue;
        }

        // Passthrough commands: skipped with their documented length. CALL_DL is
        // not bounded here, so it is refused instead of guessed.
        switch (cmd) {
        case 0x01: at += 1; break;
        case 0x08:
        case 0x10:
        case 0x20: case 0x28: case 0x30: case 0x38:
        case 0x61:
            if (error) *error = "hsd: display-list state changes are unsupported";
            return false;
        case 0x40:
            if (error) *error = "hsd: display list CALL_DL is not supported";
            return false;
        default:
            if (error) {
                *error = "hsd: unknown display list opcode " +
                         std::to_string(cmd) + " at offset " + std::to_string(at);
            }
            return false;
        }
    }
    return true;
}

// Resolves the first texture in an MOBJ's TOBJ chain, decoding it once per
// image descriptor.
inline bool resolve_texture(const Reader& r, u32 tobj, HsdScene* scene, u32* index,
                            f32* weight, std::string* error) {
    std::size_t guard = 0;
    while (tobj && guard++ < 256) {
        if (!as_tobj(r, tobj)) { if (error) *error = "hsd: invalid TOBJ"; return false; }
        const u32 image = be32(r.block, tobj + 0x4C);
        if (image && as_image_desc(r, image)) {
            HsdTexture texture;
            texture.width = be16(r.block, image + 4);
            texture.height = be16(r.block, image + 6);
            texture.format = be32(r.block, image + 8);
            texture.wrap_s = static_cast<u8>(be32(r.block, tobj + 0x34));
            texture.wrap_t = static_cast<u8>(be32(r.block, tobj + 0x38));
            texture.linear = be32(r.block, tobj + 0x48) != 0;
            u64 bytes = 0;
            if (!gx_texture_size(texture.width, texture.height, texture.format, &bytes, error)) {
                return false;
            }
            const u32 data = be32(r.block, image);
            ByteView pixels{};
            if (data != 0 && r.has(data, std::size_t(bytes))) {
                pixels = ByteView{r.block.data + data, std::size_t(bytes)};
            } else {
                if (error) *error = "hsd: texture payload outside the data block";
                return false;
            }
            ByteView palette{};
            u32 palette_format = 0;
            u32 palette_entries = 0;
            const u32 tlut = be32(r.block, tobj + 0x50);
            if (texture.format == 8 || texture.format == 9) {
                if (!tlut || !as_tlut_desc(r, tlut)) {
                    if (error) *error = "hsd: indexed texture without a palette";
                    return false;
                }
                palette_format = be32(r.block, tlut + 4);
                palette_entries = be16(r.block, tlut + 0x0C);
                const u32 palette_data = be32(r.block, tlut);
                if (!palette_data || !r.has(palette_data, std::size_t(palette_entries) * 2)) {
                    if (error) *error = "hsd: palette payload outside the data block";
                    return false;
                }
                palette = ByteView{r.block.data + palette_data, std::size_t(palette_entries) * 2};
                texture.has_palette = true;
                texture.palette_format = palette_format;
            }
            if (!gx_decode(pixels, texture.width, texture.height, texture.format, palette,
                           palette_format, palette_entries, &texture.rgba, error)) {
                return false;
            }
            std::size_t total=texture.rgba.size();
            for(const auto& old:scene->textures){
                if(old.rgba.size()>kHsdMaxDecodedBytes-total){if(error)*error="hsd: decoded texture budget exceeded";return false;}
                total+=old.rgba.size();
            }
            scene->textures.push_back(std::move(texture));
            *index = static_cast<u32>(scene->textures.size() - 1);
            // TOBJ blending is a combiner interpolation weight, not opacity.
            const f32 value = be_f32(r.block, tobj + 0x44);
            if(!std::isfinite(value)||value<0||value>1){if(error)*error="hsd: invalid texture weight";return false;}
            *weight = value;
            return true;
        }
        u32 next = 0;
        if (!r.ptr(tobj + 0x04, &next)) break;
        tobj = next;
    }
    *index = kHsdNoTexture;
    *weight = 1.0f;
    return true;
}

} // namespace hsd_detail

// Decodes one HSD archive. On failure `out` is left empty and `error` explains
// why; nothing is partially published.
inline bool hsd_load(ByteView archive, HsdScene* out, std::string* error) {
    if (!out) {
        if (error) *error = "hsd: no scene target";
        return false;
    }
    *out = HsdScene{};
    const auto fail = [out, error](std::string message) {
        *out = HsdScene{};
        out->error = message;
        if (error) *error = std::move(message);
        return false;
    };

    using namespace hsd_detail;
    if (!archive.data || archive.size < kHsdHeaderBytes) {
        return fail("hsd: buffer shorter than the 32-byte archive header");
    }
    const u32 file_size = be32(archive, 0);
    const u32 data_size = be32(archive, 4);
    const u32 reloc_count = be32(archive, 8);
    const u32 public_count = be32(archive, 12);
    const u32 extern_count = be32(archive, 16);
    const u32 reserved0 = be32(archive, 24);
    const u32 reserved1 = be32(archive, 28);
    if (file_size < kHsdHeaderBytes || std::size_t(file_size) > archive.size) {
        return fail("hsd: invalid archive size");
    }
    if (reserved0 != 0 || reserved1 != 0) {
        return fail("hsd: unsupported nonzero reserved header words");
    }
    for (std::size_t i = file_size; i < archive.size; ++i) {
        if (archive.data[i] != 0) return fail("hsd: nonzero bytes after the declared archive");
    }
    if (data_size == 0 || std::size_t(kHsdHeaderBytes) + data_size > file_size) {
        return fail("hsd: invalid data block size");
    }
    if (reloc_count > kHsdMaxRelocations) return fail("hsd: relocation count exceeds the cap");

    Reader reader;
    reader.block = ByteView{archive.data + kHsdHeaderBytes, data_size};

    const std::size_t reloc_at = kHsdHeaderBytes + data_size;
    if (reloc_at + std::size_t(reloc_count) * 4 > file_size) {
        return fail("hsd: relocation table runs past the archive");
    }
    const std::size_t symbol_at = reloc_at + std::size_t(reloc_count) * 4;
    const std::size_t symbol_bytes = (std::size_t(public_count) + extern_count) * 8;
    if (symbol_at + symbol_bytes > file_size) {
        return fail("hsd: symbol table runs past the archive");
    }

    reader.targets.reserve(reloc_count);
    for (u32 i = 0; i < reloc_count; ++i) {
        const u32 location = be32(archive, reloc_at + std::size_t(i) * 4);
        if ((location & 3u) != 0 || u64(location) + 4 > data_size) {
            return fail("hsd: relocation field outside the data block");
        }
        const u32 target = be32(reader.block, location);
        if (target >= data_size) return fail("hsd: relocation target outside the data block");
        reader.targets.push_back(target);
        reader.fields.push_back(location);
    }
    std::sort(reader.fields.begin(), reader.fields.end());
    if (std::adjacent_find(reader.fields.begin(), reader.fields.end()) != reader.fields.end())
        return fail("hsd: duplicate relocation field");
    std::sort(reader.targets.begin(), reader.targets.end());
    reader.targets.erase(std::unique(reader.targets.begin(), reader.targets.end()),
                         reader.targets.end());

    std::vector<u32> public_offsets;
    u32 scene_table = 0;
    public_offsets.reserve(public_count);
    for (u32 i = 0; i < public_count; ++i) {
        const u32 offset = be32(archive, symbol_at + std::size_t(i) * 8);
        if (offset >= data_size) return fail("hsd: public symbol outside the data block");
        public_offsets.push_back(offset);
        const u32 name_offset = be32(archive, symbol_at + std::size_t(i) * 8 + 4);
        const std::size_t strings = symbol_at + symbol_bytes;
        if (name_offset >= file_size - strings) return fail("hsd: symbol name outside string table");
        std::size_t end_name = strings + name_offset;
        while (end_name < file_size && archive.data[end_name]) ++end_name;
        if (end_name == file_size) return fail("hsd: unterminated symbol name");
        if (std::string(reinterpret_cast<const char*>(archive.data + strings + name_offset),
                        end_name - strings - name_offset) == "scene_data") scene_table = offset;
    }
    if (public_offsets.empty()) return fail("hsd: archive exports no symbols");

    std::vector<u32> candidates = reader.targets;
    for (u32 offset : public_offsets) candidates.push_back(offset);
    std::sort(candidates.begin(), candidates.end());
    candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());

    std::vector<u32> jobj_offsets;
    for (u32 offset : candidates) {
        if (as_jobj(reader, offset)) jobj_offsets.push_back(offset);
    }
    if (jobj_offsets.empty()) return fail("hsd: archive contains no JOBJ nodes");

    std::vector<u32> referenced;
    for (u32 offset : jobj_offsets) {
        u32 child = 0, sibling = 0;
        if (reader.ptr(offset + 0x08, &child) && child) referenced.push_back(child);
        if (reader.ptr(offset + 0x0C, &sibling) && sibling) referenced.push_back(sibling);
    }
    std::sort(referenced.begin(), referenced.end());

    std::vector<u32> roots;
    for (u32 offset : jobj_offsets) {
        if (!std::binary_search(referenced.begin(), referenced.end(), offset)) roots.push_back(offset);
    }
    if (roots.empty()) roots = jobj_offsets;
    // Prefer the published scene table over classification/discovery order.
    // Non-scene public JOBJ fixtures remain supported for small portable assets.
    if (scene_table) {
        if(!read_scene_lighting(reader,scene_table,&out->lighting,error))return fail(error?*error:"hsd: invalid lights");
        u32 root_table = 0;
        if (!reader.ptr(scene_table, &root_table) || !root_table)
            return fail("hsd: invalid scene root table");
        roots.clear();
        for (std::size_t i = 0; i <= kHsdMaxNodes; ++i) {
            u32 record = 0, root = 0;
            if (!reader.ptr(std::size_t(root_table) + i * 4, &record))
                return fail("hsd: invalid root record pointer");
            if (!record) break;
            if (i == kHsdMaxNodes || !reader.ptr(record, &root) || !root || !reader.has(root,0x40) ||
                !reader.finite(reader.block,root+0x14,9))
                return fail("hsd: invalid published joint root");
            roots.push_back(root);
            u32 animations=0;
            if (!reader.ptr(record+4,&animations)) return fail("hsd: invalid animation table");
            if (animations) {
                for (std::size_t clip=0;clip<=256;++clip) {
                    u32 animation=0;
                    if (!reader.ptr(std::size_t(animations)+clip*4,&animation)) return fail("hsd: invalid clip pointer");
                    if (!animation) break;
                    if (clip==256) return fail("hsd: too many clips");
                    out->animation_roots.push_back({animation,root});
                }
            }
            u32 material_animations=0;
            if(!reader.ptr(record+8,&material_animations))return fail("hsd: invalid material animation table");
            if(material_animations) {
                u32 first=0;
                if(!reader.ptr(material_animations,&first))return fail("hsd: invalid material animation pointer");
                if(first)out->material_animation_roots.push_back({first,root});
            }
        }
        if (roots.empty()) return fail("hsd: empty published scene");
    }

    // Geometry is collected during the joint walk and decoded afterwards so the
    // joint indices are already stable.
    std::vector<std::pair<u32, u32>> pending;   // (dobj offset, joint index)
    struct WalkEntry{u32 first,second,model_root;};
    std::vector<WalkEntry> stack; // (jobj offset, parent joint, model record)
    out->model_roots=roots;
    for(std::size_t i=roots.size();i-->0;)stack.push_back({roots[i],kHsdNoJoint,u32(i)});
    std::vector<std::pair<u32, u32>> visited;   // (jobj offset, joint index)

    while (!stack.empty()) {
        const auto entry = stack.back();
        stack.pop_back();
        const u32 offset = entry.first;
        if(!reader.has(offset,0x40) || !reader.finite(reader.block,offset+0x14,9))
            return fail("hsd: invalid joint record extent/transform");
        if (out->joints.size() >= kHsdMaxNodes) return fail("hsd: joint count exceeds the cap");
        const auto seen = std::find_if(visited.begin(), visited.end(),
                                       [offset](const std::pair<u32, u32>& item) {
                                           return item.first == offset;
                                       });
        if (seen != visited.end()) {
            // A joint reachable from two roots is a shared node, not a cycle. Only
            // an ancestor of the current parent is a real loop.
            u32 parent = entry.second;
            std::size_t guard = 0;
            while (parent != kHsdNoJoint && guard++ <= out->joints.size()) {
                if (out->joints[parent].archive_offset == offset) {
                    return fail("hsd: joint graph contains a cycle (joint " +
                                std::to_string(seen->second) + " reaches itself)");
                }
                parent = out->joints[parent].parent;
            }
            // A shared descriptor is a new transformed instance. Do not drop
            // its geometry merely because another root already used it.
        }
        visited.push_back({offset, static_cast<u32>(out->joints.size())});

        HsdJoint joint;
        joint.parent = entry.second;
        joint.archive_offset = offset;
        joint.model_root=entry.model_root;
        joint.flags = be32(reader.block, offset + 0x04);
        joint.rotation[0] = be_f32(reader.block, offset + 0x14);
        joint.rotation[1] = be_f32(reader.block, offset + 0x18);
        joint.rotation[2] = be_f32(reader.block, offset + 0x1C);
        joint.scale[0] = be_f32(reader.block, offset + 0x20);
        joint.scale[1] = be_f32(reader.block, offset + 0x24);
        joint.scale[2] = be_f32(reader.block, offset + 0x28);
        joint.translation[0] = be_f32(reader.block, offset + 0x2C);
        joint.translation[1] = be_f32(reader.block, offset + 0x30);
        joint.translation[2] = be_f32(reader.block, offset + 0x34);
        const u32 joint_index = static_cast<u32>(out->joints.size());
        out->joints.push_back(joint);

        u32 contents = 0;
        if(!reader.ptr(offset+0x10,&contents))return fail("hsd: invalid joint contents pointer");
        if(contents && !(joint.flags&0x4020u)) { // PTCL/SPLINE use a different contents union
            u32 dobj = contents;
            std::set<u32> seen_dobjs;
            while(dobj){
                if(pending.size()>=kHsdMaxNodes || !seen_dobjs.insert(dobj).second)
                    return fail("hsd: DOBJ cycle/metadata budget");
                // A published JOBJ's DOBJ role is explicit. Unsupported/bad
                // POBJ data must refuse the asset, not erase an entire chain.
                if(!as_dobj(reader,dobj))return fail("hsd: invalid/unsupported published DOBJ chain");
                pending.push_back({dobj, joint_index});
                u32 next = 0;
                if (!reader.ptr(dobj + 0x04, &next)) return fail("hsd: invalid DOBJ next pointer");
                dobj = next;
            }
        }

        std::vector<u32> children;
        u32 child = 0;
        if (reader.ptr(offset + 0x08, &child)) {
            std::size_t guard = 0;
            while (child && guard++ < kHsdMaxNodes) {
                children.push_back(child);
                u32 next = 0;
                if (!reader.ptr(child + 0x0C, &next)) break;
                child = next;
            }
        }
        for (auto it = children.rbegin(); it != children.rend(); ++it) {
            stack.push_back({*it, joint_index,entry.model_root});
        }
    }

    // Mesh pass: one HsdMesh per POBJ, textured and tinted from its MOBJ.
    std::size_t mesh_bytes=0;
    for (const auto& item : pending) {
        const u32 dobj = item.first;
        const u32 joint_index = item.second;
        const u32 mobj = be32(reader.block, dobj + 0x08);
        u32 pobj = 0;
        if (!reader.ptr(dobj + 0x0C, &pobj)) return fail("hsd: DOBJ object pointer is not valid");
        u8 colour[4] = {255, 255, 255, 255};
        u8 ambient[4] = {255, 255, 255, 255};
        u32 texture = kHsdNoTexture;
        f32 texture_weight = 1.0f;
        if (mobj && as_mobj(reader, mobj)) {
            const u32 material = be32(reader.block, mobj + 0x0C);
            if (material && reader.has(material, 0x14)) {
                // Colour words are RGBA8 with alpha in the LOW byte.
                const u32 diffuse = be32(reader.block, material + 4);
                colour[0] = u8(diffuse >> 24);
                colour[1] = u8(diffuse >> 16);
                colour[2] = u8(diffuse >> 8);
                colour[3] = u8(diffuse & 0xFFu);
                const u32 flat = be32(reader.block, material + 0);
                ambient[0] = u8(flat >> 24);
                ambient[1] = u8(flat >> 16);
                ambient[2] = u8(flat >> 8);
                ambient[3] = u8(flat & 0xFFu);
            }
            const u32 tobj = be32(reader.block, mobj + 0x08);
            if (tobj) {
                std::string texture_error;
                if (!resolve_texture(reader, tobj, out, &texture, &texture_weight, &texture_error)) {
                    return fail(texture_error);
                }
            }
        }
        std::set<u32> seen_pobjs;
        while(pobj){
            if(seen_pobjs.size()>=kHsdMaxNodes||!seen_pobjs.insert(pobj).second)
                return fail("hsd: POBJ cycle/metadata budget");
            HsdMesh mesh;
            mesh.material_colour[0] = colour[0];
            mesh.material_colour[1] = colour[1];
            mesh.material_colour[2] = colour[2];
            mesh.material_colour[3] = colour[3];
            mesh.material_ambient[0] = ambient[0];
            mesh.material_ambient[1] = ambient[1];
            mesh.material_ambient[2] = ambient[2];
            mesh.material_ambient[3] = ambient[3];
            mesh.texture = texture;
            mesh.material_offset = mobj;
            mesh.dobj_offset = dobj;
            mesh.texture_weight = texture_weight;
            if (mobj) {
                mesh.render_mode = be32(reader.block, mobj + 4);
                const u32 material = be32(reader.block, mobj + 12);
                mesh.material_alpha = material ? be_f32(reader.block, material + 12) : 1.0f;
                if(material){
                    for(unsigned k=0;k<4;++k)mesh.material_specular[k]=reader.block.data[material+8+k];
                    mesh.shininess=be_f32(reader.block,material+16);
                    if(!std::isfinite(mesh.shininess)||mesh.shininess<0)return fail("hsd: invalid material shininess");
                }
                if (!std::isfinite(mesh.material_alpha) || mesh.material_alpha < 0 || mesh.material_alpha > 1)
                    return fail("hsd: invalid material alpha");
                const u32 tex = be32(reader.block, mobj + 8);
                if (tex) {
                    const u32 src=be32(reader.block,tex+12);
                    mesh.texture_uv1=src==5;
                    if(src!=4 && src!=5)mesh.material_supported=false;
                    const u32 flags_tex = be32(reader.block, tex + 0x40);
                    mesh.texture_coordinate_mode=u8(flags_tex&15u);
                    mesh.generated_uv_required=mesh.texture_coordinate_mode!=0;
                    if(mesh.texture_coordinate_mode>1)mesh.material_supported=false;
                    if(mesh.texture_coordinate_mode==1){
                        mesh.texture_uv1=false; // reflection overrides src with GX_TG_NRM
                        // Supported reflection subset: the source TObj matrix
                        // is identity; camera-normal generation is a separate
                        // evaluated path, NEVER a default UV=(0,0) fallback.
                        bool identity=reader.has(tex,0x40)&&reader.block.data[tex+0x3c]==1&&reader.block.data[tex+0x3d]==1&&be32(reader.block,tex+0x38)!=2;
                        for(unsigned axis=0;axis<3&&identity;++axis)
                            identity=be_f32(reader.block,tex+0x10+axis*4)==0&&be_f32(reader.block,tex+0x1c+axis*4)==1&&be_f32(reader.block,tex+0x28+axis*4)==0;
                        mesh.reflection_uv_supported=identity;
                    }
                    mesh.texture_after_lighting=(flags_tex&0x80u)!=0;
                    mesh.texture_stage_inactive=flags_tex==0&&!be32(reader.block,tex+4)&&!be32(reader.block,tex+0x58);
                    if(!mesh.texture_stage_inactive&&(flags_tex&0x1f0u)!=0x10u && (flags_tex&0x1f0u)!=0x80u)
                        mesh.material_supported=false; // unsupported lightmap stage placement
                    mesh.texture_colour_mode = static_cast<u8>((flags_tex >> 16) & 15);
                    mesh.texture_alpha_mode = static_cast<u8>((flags_tex >> 20) & 15);
                    // This menu uses only one texture and identity texture matrices.
                    // Report unsupported stages rather than silently omitting them.
                    if (be32(reader.block, tex + 4))
                        mesh.material_supported = false;
                    const u32 tev=be32(reader.block,tex+0x58);
                    if(tev) {
                        // Reviewed subset: ADD(mix(TEV0, KONST, TEXC), ZERO),
                        // unity scale, zero bias, clamped color; alpha unchanged.
                        // Other custom expressions remain explicitly unsupported.
                        static constexpr u8 expression[16]={0,0,0,0,0,0,1,1,0x85,0x80,8,15,7,7,7,7};
                        if(reader.has(tev,32) && (be32(reader.block,tev+28)&0xC0000000u)==0){
                            //8023FB48 skips custom colour generation when BOTH
                            // activation bits are clear. Retain ordinary maps.
                        } else if(reader.has(tev,32) && std::memcmp(reader.block.data+tev,expression,16)==0 &&
                           (be32(reader.block,tev+28)&0xC0000000u)==0x40000000u){
                            mesh.texture_colour_lerp=true;
                            mesh.texture_low_colour=be32(reader.block,tev+20);
                            mesh.texture_high_colour=be32(reader.block,tev+16);
                        } else mesh.material_supported=false;
                    }
                    if (mesh.texture_colour_mode > 8 || mesh.texture_alpha_mode > 7)
                        return fail("hsd: unsupported texture combiner mode");
                }
                mesh.blend = (mesh.render_mode & 0x40000000u) != 0;
                mesh.depth_write = (mesh.render_mode & 0x20000000u) == 0;
                mesh.depth_compare = (mesh.render_mode & 0x08000000u) ? 7 : 6;
                if (mesh.blend && mesh.depth_write) mesh.alpha_compare0 = 4;
                const u32 pe = be32(reader.block, mobj + 20);
                if (pe) {
                    if (!reader.has(pe, 12)) return fail("hsd: truncated PE descriptor");
                    const u8* state = reader.block.data + pe;
                    if (state[4] > 1 || state[5] > 7 || state[6] > 7)
                        return fail("hsd: unsupported PE blend operation");
                    mesh.blend = state[4] == 1;
                    mesh.blend_src = state[5] + 1;
                    mesh.blend_dst = state[6] + 1;
                    mesh.depth_test = (state[0] & 0x10) != 0;
                    mesh.depth_write = (state[0] & 0x20) != 0;
                    mesh.colour_write=(state[0]&1)!=0;mesh.alpha_write=(state[0]&2)!=0;
                    static constexpr u8 reverse_compare[] = {0,4,2,6,1,5,3,7};
                    if (state[8] > 7 || state[9] > 7 || state[10] > 3 || state[11] > 7)
                        return fail("hsd: invalid PE compare state");
                    mesh.depth_compare = reverse_compare[state[8]];
                    mesh.alpha_compare0 = state[9]; mesh.alpha_compare1 = state[11];
                    mesh.alpha_op = state[10]; mesh.alpha_ref0 = state[1]; mesh.alpha_ref1 = state[2];
                }
            }
            std::string mesh_error;
            if (!as_pobj(reader, pobj)) {
                return fail("hsd: DOBJ points at a POBJ that does not validate");
            }
            if (!decode_pobj(reader, pobj, &mesh, &mesh_error)) return fail(mesh_error);
            const std::size_t bytes=mesh.vertices.size()*sizeof(HsdVertex)+mesh.indices.size()*sizeof(u32);
            if(out->meshes.size()>=kHsdMaxNodes || bytes>kHsdMaxDecodedBytes-mesh_bytes)return fail("hsd: decoded mesh budget exceeded");
            mesh_bytes+=bytes;
            if (!mesh.indices.empty()) {
                out->meshes.push_back(std::move(mesh));
                out->mesh_joint.push_back(joint_index);
            }
            u32 next = 0;
            if (!reader.ptr(pobj + 0x04, &next)) return fail("hsd: invalid POBJ next pointer");
            pobj = next;
        }
    }

    if (out->meshes.empty()) return fail("hsd: archive has no drawable meshes");
    return true;
}

} // namespace gf
