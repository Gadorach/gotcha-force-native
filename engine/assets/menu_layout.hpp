#pragma once
// Portable, versioned camera, visibility/material and seal-binding content.
// Authored transforms are retained. The engine reads only this small
// format and HSD assets, not an emulator snapshot or guest RAM. Layout files
// derived from the reference capture stay private. The source archive hash binds
// descriptor keys to the exact asset; unknown/duplicate keys fail closed.
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include "engine/assets/hsd.hpp"
#include "engine/graphics/camera.hpp"

namespace gf {
inline u32 menu_asset_hash(ByteView bytes){u32 h=2166136261u;for(std::size_t i=0;i<bytes.size;++i){h^=bytes.data[i];h*=16777619u;}return h;}
inline bool menu_layout_load(ByteView file,ByteView source_archive,const HsdScene& source,
                             HsdScene* out,Camera* camera,std::string* error){
    if(out==&source){if(error)*error="menu layout: source/output must be distinct";return false;}
    if(out)*out={};
    auto fail=[&](const char* reason){if(error)*error=std::string("menu layout: ")+reason;return false;};
    if(!out||!camera||!file.data||file.size<64||!source_archive.data)return fail("short header or missing output/source");
    std::size_t at=0;
    auto word=[&](){u32 value=hsd_detail::be32(file,at);at+=4;return value;};
    auto real=[&](){f32 value=bits_to_f32(word());return value;};
    if(word()!=0x47464D4Cu)return fail("bad magic");
    const u32 version=word();if(version!=2&&version!=3)return fail("bad version");
    if(word()!=menu_asset_hash(source_archive))return fail("source archive identity mismatch");
    const u32 count=word();if(!count||count>4096)return fail("instance count out of bounds");
    Camera candidate=*camera;
    for(unsigned k=0;k<3;++k)candidate.eye[k]=real();
    for(unsigned k=0;k<3;++k)candidate.target[k]=real();
    for(unsigned k=0;k<3;++k)candidate.up[k]=real();
    candidate.fov_y_degrees=real();candidate.near_plane=real();candidate.far_plane=real();
    Camera::Basis basis;if(!candidate.basis(&basis))return fail("invalid camera");
    HsdScene scene=source;
    std::vector<u32> updated;
    std::size_t mesh_bytes=0;
    for(u32 i=0;i<count;++i){
        if(at>file.size || file.size-at<12)return fail("truncated node state");
        const u32 descriptor=word(),flags=word();
        if(flags&~0x73FFFFFFu)return fail("unsupported joint flags");
        const auto found=std::find_if(source.joints.begin(),source.joints.end(),[&](const HsdJoint& j){return j.archive_offset==descriptor;});
        if(found==source.joints.end())return fail("unknown joint descriptor");
        const std::size_t source_joint=std::size_t(found-source.joints.begin());
        if(std::find(updated.begin(),updated.end(),descriptor)!=updated.end())return fail("duplicate joint state");
        updated.push_back(descriptor);scene.joints[source_joint].flags=flags;
        const u32 materials=word();if(materials>256 || u64(materials)*20>file.size-at)return fail("invalid material records");
        struct Material{u32 descriptor,hidden,ambient,diffuse;f32 alpha;};
        std::vector<Material> records;
        for(u32 m=0;m<materials;++m){Material record;record.descriptor=word();record.hidden=word();record.ambient=word();record.diffuse=word();record.alpha=real();
            if(!std::isfinite(record.alpha)||record.alpha<0||record.alpha>1||record.hidden>1)return fail("invalid material value");
            for(const auto& old:records)if(old.descriptor==record.descriptor)return fail("duplicate material descriptor");
            records.push_back(record);
        }
        for(const auto& record:records){
            bool bound=false;
            for(std::size_t m=0;m<source.meshes.size();++m)
                if(source.mesh_joint[m]==source_joint && source.meshes[m].dobj_offset==record.descriptor)bound=true;
            if(!bound)return fail("material descriptor is not attached to joint");
        }
        for(std::size_t m=0;m<source.meshes.size();++m)if(source.mesh_joint[m]==source_joint){
            HsdMesh mesh=source.meshes[m];
            const std::size_t bytes=mesh.vertices.size()*sizeof(HsdVertex)+mesh.indices.size()*sizeof(u32);
            if(bytes>kHsdMaxDecodedBytes-mesh_bytes)return fail("decoded instance budget exceeded");
            mesh_bytes+=bytes;
            auto material=std::find_if(records.begin(),records.end(),[&](const Material& r){return r.descriptor==mesh.dobj_offset;});
            if(material==records.end())return fail("missing mesh material binding");
            mesh.hidden=material->hidden!=0;
            for(unsigned k=0;k<4;++k){mesh.material_ambient[k]=u8(material->ambient>>(24-k*8));mesh.material_colour[k]=u8(material->diffuse>>(24-k*8));}
            mesh.material_alpha=material->alpha;
            scene.meshes[m]=std::move(mesh);
        }
    }
    if(version==3){
        if(file.size-at<8)return fail("truncated seal header");
        const u32 seals=word();scene.menu_selection=word();
        if(seals!=7||scene.menu_selection>=seals||u64(seals)*28>file.size-at)return fail("invalid seal table");
        for(u32 i=0;i<seals;++i){
            HsdScene::MenuSeal seal;seal.item=word();seal.caption_dobj=word();seal.gear_dobj=word();
            for(float& v:seal.centre)v=real();
            seal.radius=real();
            if(seal.item>=7 || !std::isfinite(seal.radius)||seal.radius<=0)return fail("invalid seal geometry");
            for(float v:seal.centre)if(!std::isfinite(v))return fail("nonfinite seal anchor");
            for(const auto& old:scene.menu_seals)if(old.item==seal.item)return fail("duplicate seal identity");
            const auto exists=[&](u32 descriptor){return std::any_of(source.meshes.begin(),source.meshes.end(),[&](const HsdMesh& m){return m.dobj_offset==descriptor;});};
            if(!exists(seal.caption_dobj)||!exists(seal.gear_dobj))return fail("unknown seal mesh binding");
            scene.menu_seals.push_back(seal);
        }
    }
    if(at!=file.size)return fail("trailing bytes");
    *camera=candidate;*out=std::move(scene);return true;
}
} // namespace gf
