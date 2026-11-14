#pragma once
// Explicit leaf-JOBJ material clips. No tree-shape matching, relocation-target
// scan, image/palette switching or partial publication. The caller supplies the
// leaf's archive identity from the decoded scene's published root bindings.
#include "engine/assets/hsd_anim.hpp"
#include "engine/animation/hsd_texture.hpp"
#include <limits>
namespace gf {
struct HsdLeafMaterialClip {
    u32 joint=0,material=0,texture=0;
    f32 end_frame=0;
    HsdAnimPose::Material baseline;
    f32 translate[2]={},scale[2]={1,1};
    u8 repeat[2]={1,1};
    std::vector<HsdAnimTrack> material_tracks,texture_tracks;
};
namespace hsd_leaf_detail {
// Reconfirm the binding against the archive, not only the caller's decoded
// scene. This subset accepts exactly one published material clip for a leaf.
inline bool published_clip(ByteView archive,const hsd_detail::Reader& r,u32 joint,u32* clip){
    const u32 publics=archive.be32(12),externs=archive.be32(16);
    if(publics>4096||externs>4096)return false;
    const std::size_t symbols=32+r.block.size+std::size_t(archive.be32(8))*4;
    const std::size_t strings=symbols+(std::size_t(publics)+externs)*8;
    const std::size_t size=archive.be32(0);u32 scene=0;bool found=false;
    for(u32 i=0;i<publics;++i){
        const u32 name=archive.be32(symbols+i*8+4);
        if(name>=size-strings)return false;
        std::size_t end=strings+name;
        while(end<size&&archive.data[end]){if(end-strings-name>=256)return false;++end;}
        if(end==size)return false;
        if(std::string(reinterpret_cast<const char*>(archive.data+strings+name),end-strings-name)=="scene_data"){
            if(found)return false;
            scene=archive.be32(symbols+i*8);found=true;
        }
    }
    u32 models=0;if(!found||!r.ptr(scene,&models)||!models)return false;
    bool matched=false,terminated=false;
    for(unsigned i=0;i<=256;++i){
        u32 record=0,root=0;if(!r.ptr(std::size_t(models)+i*4,&record))return false;
        if(!record){terminated=true;break;}
        if(i==256||!r.has(record,12)||!r.ptr(record,&root))return false;
        if(root==joint){
            if(matched)return false;
            matched=true;u32 clips=0,second=0;
            if(!r.ptr(record+8,&clips)||!clips||!r.ptr(clips,clip)||!*clip||!r.ptr(clips+4,&second)||second)return false;
        }
    }
    return terminated&&matched;
}
inline bool aobj(const hsd_detail::Reader& r,u32 at,std::vector<HsdAnimTrack>& tracks,
                 f32& end){
    if(!at)return true;
    u32 first=0,object=0;
    if(!r.has(at,16)||r.block.be32(at)!=0||!r.ptr(at+8,&first)||!first||
       !r.ptr(at+12,&object)||object)return false;
    const f32 last=r.block.bef32(at+4);
    // Widget clips are short; refusing long timelines also bounds sampling work.
    if(!std::isfinite(last)||last<=0||last>4096||(end!=0&&last!=end))return false;
    std::vector<u32> seen;std::size_t payload=0;
    for(u32 node=first;node;){
        if(seen.size()>=32||std::find(seen.begin(),seen.end(),node)!=seen.end()||
           !hsd_anim_detail::is_fobjdesc(r,node))return false;
        seen.push_back(node);
        const u32 size=r.block.be32(node+4);u32 data=0;
        if(!size||size>(1u<<16)-payload||!r.ptr(node+16,&data)||!data)return false;
        payload+=size;
        HsdAnimTrack track;track.channel=r.block.data[node+12];track.frac_value=r.block.data[node+13];
        track.frac_slope=r.block.data[node+14];track.startframe=r.block.bef32(node+8);
        if(r.block.data[node+15]!=0||!std::isfinite(track.startframe)||std::fabs(track.startframe)>4096)return false;
        for(const auto& prior:tracks)if(prior.channel==track.channel)return false;
        track.data.assign(r.block.data+data,r.block.data+data+size);tracks.push_back(std::move(track));
        if(!r.ptr(node,&node))return false;
    }
    end=std::max(end,last);return true;
}
}
inline bool hsd_leaf_material_clip_load(ByteView archive,const HsdScene& scene,u32 joint,
                                       HsdLeafMaterialClip* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("hsd leaf material: ")+why;return false;};
    if(!out||scene.mesh_joint.size()!=scene.meshes.size())return fail("missing output/invalid scene bindings");
    hsd_detail::Reader r;if(!hsd_anim_detail::open_container(archive,&r,error))return false;
    const HsdMesh* mesh=nullptr;const HsdJoint* leaf=nullptr;
    for(std::size_t m=0;m<scene.meshes.size();++m){
        if(scene.mesh_joint[m]>=scene.joints.size())return fail("invalid mesh joint");
        const auto& j=scene.joints[scene.mesh_joint[m]];
        if(j.archive_offset==joint){if(mesh)return fail("leaf must have exactly one mesh");mesh=&scene.meshes[m];leaf=&j;}
    }
    if(!mesh||leaf->parent!=kHsdNoJoint||!mesh->material_supported||mesh->deformation_required)return fail("unsupported leaf mesh");
    u32 child=0,sibling=0,dobj=0,next=0,mobj=0,matjoint=0,klass=0;
    if(!r.has(joint,0x40)||!r.ptr(joint,&klass)||klass||!r.ptr(joint+8,&child)||child||!r.ptr(joint+12,&sibling)||sibling||
       !r.ptr(joint+16,&dobj)||!dobj||dobj!=mesh->dobj_offset||!r.has(dobj,16)||
       !r.ptr(dobj,&klass)||klass||!r.ptr(dobj+4,&next)||next||!r.ptr(dobj+8,&mobj)||!mobj||mobj!=mesh->material_offset)
        return fail("not a single published leaf/material");
    for(const auto& binding:scene.material_animation_roots)if(binding.second==joint){
        if(matjoint)return fail("ambiguous leaf clip");
        matjoint=binding.first;
    }
    u32 published=0;
    if(!hsd_leaf_detail::published_clip(archive,r,joint,&published)||published!=matjoint)return fail("published material clip mismatch");
    if(!matjoint||!r.has(matjoint,12)||!r.ptr(matjoint,&child)||child||
       !r.ptr(matjoint+4,&sibling)||sibling)return fail("missing/non-leaf material binding");
    u32 mat=0,aobj=0,texanim=0,renderanim=0;
    if(!r.ptr(matjoint+8,&mat)||!mat||!r.has(mat,16)||!r.ptr(mat,&next)||next||
       !r.ptr(mat+4,&aobj)||!r.ptr(mat+8,&texanim)||!r.ptr(mat+12,&renderanim)||renderanim)
        return fail("unsupported material animation chain");
    HsdLeafMaterialClip candidate;candidate.joint=joint;candidate.material=mobj;
    auto& base=candidate.baseline;base.present=true;base.has_specular=true;base.alpha=mesh->material_alpha;
    for(unsigned k=0;k<3;++k){base.ambient[k]=mesh->material_ambient[k]/255.0f;
        base.diffuse[k]=mesh->material_colour[k]/255.0f;base.specular[k]=mesh->material_specular[k]/255.0f;}
    if(!hsd_leaf_detail::aobj(r,aobj,candidate.material_tracks,candidate.end_frame))return fail("invalid material AObj/tracks");
    for(const auto& t:candidate.material_tracks)if(t.channel<1||t.channel>10)return fail("unsupported material channel");
    u32 tobj=0;
    if(!r.has(mobj,24)||!r.ptr(mobj,&klass)||klass||!r.ptr(mobj+8,&tobj))return fail("invalid material class/texture pointer");
    if(tobj){
        if(!r.has(tobj,0x5c)||!r.ptr(tobj,&klass)||klass||!r.ptr(tobj+4,&next)||next||!r.finite(r.block,tobj+0x10,9))return fail("unsupported texture class/chain/transform");
        for(unsigned k=0;k<3;++k)if(r.block.bef32(tobj+0x10+k*4)!=0)return fail("texture rotation unsupported");
        // This subset deliberately rejects mirror wrapping; its T offset differs.
        if(r.block.be32(tobj+0x34)>1||r.block.be32(tobj+0x38)>1)return fail("texture mirror/unknown wrap unsupported");
        candidate.texture=tobj;base.has_texture=true;base.texture_weight=r.block.bef32(tobj+0x44);
        for(unsigned k=0;k<2;++k){candidate.scale[k]=r.block.bef32(tobj+0x1c+k*4);
            candidate.translate[k]=r.block.bef32(tobj+0x28+k*4);candidate.repeat[k]=r.block.data[tobj+0x3c+k];
            if(!candidate.repeat[k])return fail("zero texture repeat");}
        if(!std::isfinite(base.texture_weight)||base.texture_weight<0||base.texture_weight>1)return fail("invalid texture blend");
    }
    if(texanim){
        u32 images=0,palettes=0,taobj=0;
        if(!tobj||!r.has(texanim,24)||!r.ptr(texanim,&next)||next||
           r.block.be32(texanim+4)!=r.block.be32(tobj+8)||!r.ptr(texanim+8,&taobj)||!taobj||
           !r.ptr(texanim+12,&images)||images||!r.ptr(texanim+16,&palettes)||palettes||
           r.block.be16(texanim+20)||r.block.be16(texanim+22))return fail("unsupported texture id/image/palette/chain");
        if(!hsd_leaf_detail::aobj(r,taobj,candidate.texture_tracks,candidate.end_frame))return fail("invalid texture AObj/tracks");
        for(const auto& t:candidate.texture_tracks)if(t.channel<2||t.channel>9)return fail("unsupported texture channel");
    }
    if(candidate.material_tracks.empty()&&candidate.texture_tracks.empty())return fail("empty leaf clip");
    *out=std::move(candidate);return true;
}
inline bool hsd_leaf_material_sample(const HsdLeafMaterialClip& clip,u32 time,
                                     HsdAnimPose::Material* out,std::string* error){
    auto fail=[&](const char* why){if(error)*error=std::string("hsd leaf material: ")+why;return false;};
    if(!out||!clip.baseline.present||!std::isfinite(clip.end_frame)||time>clip.end_frame||time>4096)
        return fail("invalid clip/output/time");
    if(clip.material_tracks.size()>32||clip.texture_tracks.size()>32)return fail("track budget exceeded");
    u64 bytes=0;for(const auto* tracks:{&clip.material_tracks,&clip.texture_tracks})for(const auto& t:*tracks){
        if(t.data.size()>(1u<<16)||bytes+t.data.size()>(1u<<17))return fail("payload budget exceeded");
        bytes+=t.data.size();
    }
    if((u64(time)+1)*(bytes+1)>(1u<<23))return fail("sample work budget exceeded");
    auto result=clip.baseline;
    auto sample=[&](const HsdAnimTrack& t,f32* value){return hsd_anim_sample_requested_track(
        {{t.data.data(),t.data.size()},t.frac_value,t.frac_slope,t.startframe},time,value,error);};
    for(const auto& t:clip.material_tracks){
        f32 value=0;if(!sample(t,&value))return false;
        if(value<0||value>1)return fail("material channel outside supported unit range");
        // The HSD material callback stores RGB as u8(255.0 * value), i.e.
        // truncation, not round-to-nearest. Preserve that before rendering.
        const f32 colour=static_cast<u8>(255.0*f64(value))/255.0f;
        if(t.channel>=1&&t.channel<=3)result.ambient[t.channel-1]=colour;
        else if(t.channel>=4&&t.channel<=6)result.diffuse[t.channel-4]=colour;
        else if(t.channel>=7&&t.channel<=9)result.specular[t.channel-7]=colour;
        else if(t.channel==10)result.alpha=1-value;
        else return fail("unsupported material channel");
    }
    f32 translate[2]={clip.translate[0],clip.translate[1]},scale[2]={clip.scale[0],clip.scale[1]};
    for(const auto& t:clip.texture_tracks){
        f32 value=0;if(!sample(t,&value))return false;
        if(t.channel==2||t.channel==3)translate[t.channel-2]=value;
        else if(t.channel==4||t.channel==5)scale[t.channel-4]=value;
        else if(t.channel>=6&&t.channel<=8){if(value!=0)return fail("animated texture rotation unsupported");}
        else if(t.channel==9){if(value<0||value>1)return fail("texture blend outside supported unit range");result.texture_weight=value;}
        else return fail("unsupported texture channel");
    }
    if(result.has_texture){
        std::array<f32,2> uv{},offset{};
        if(!hsd_texture_uv({translate[0],translate[1]},{scale[0],scale[1]},{clip.repeat[0],clip.repeat[1]},&uv,&offset))return fail("texture transform outside supported range");
        for(unsigned k=0;k<2;++k){result.uv_scale[k]=uv[k];result.uv_offset[k]=offset[k];}
    }
    *out=result;return true;
}
} // namespace gf
