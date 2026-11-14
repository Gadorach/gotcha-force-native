#pragma once
// Explicit published static-model records, not relocation-candidate/shape matching.
// Decode a bank once; selected instances share immutable source/animation bytes.
#include <memory>
#include "engine/assets/hsd_anim.hpp"
#include "engine/animation/hsd_texture.hpp"
#include "engine/animation/hsd_alternate_material.hpp"
#include "engine/assets/hsd_fog.hpp"
namespace gf {
namespace hsd_record_detail {
inline bool optional(const hsd_detail::Reader& r,u32 at,u32* value){
    if(!r.has(at,4)||((r.block.be32(at)==0||r.block.be32(at)==0xffffffffu)&&std::binary_search(r.fields.begin(),r.fields.end(),at)))return false;
    return r.ptr(at,value); // descriptor-at-zero unsupported; payload handled separately
}
inline bool pointers(const hsd_detail::Reader& r,u32 table,std::vector<u32>* out){
    if(!table)return true;
    for(unsigned i=0;i<=256;++i){u32 value=0;if(!optional(r,table+i*4,&value))return false;if(!value)return true;if(i==256)return false;out->push_back(value);}return false;
}
struct Record {u32 root=0;std::vector<u32> joint,material,shape;};
struct Bank {std::vector<u8> bytes;hsd_detail::Reader reader;HsdScene scene;std::vector<Record> records;u32 hash=0,scene_export=0;};
struct Object {enum Kind{Joint,Material,Texture} kind=Joint;u32 target=0;std::vector<u32> meshes;f32 end=0;std::vector<HsdAnimTrack> tracks;u32 path_reference=0;HsdAnimSpline path;HsdTextureState texture;bool retain_tev1=false;};
struct Instance {std::shared_ptr<const Bank> bank;HsdScene scene;std::vector<Object> objects;u16 record=0;};
inline bool object(const hsd_detail::Reader& r,u32 at,bool material,Object* out,std::size_t* bytes,bool paths=false,bool texture=false,bool retain_tev1=false){
    if(!r.has(at,16)||r.block.be32(at)!=0)return false; // nonloop/default source AObj only
    u32 track=0,reference=0;if(!optional(r,at+8,&track)||!track||!optional(r,at+12,&reference)||(reference&&(!paths||material)))return false;
    out->end=r.block.bef32(at+4);if(!std::isfinite(out->end)||out->end<=0||out->end>hsd_anim_detail::kMaxFrame)return false;
    std::vector<u32> seen;
    while(track){
        if(seen.size()>=(retain_tev1?32u:16u)||std::find(seen.begin(),seen.end(),track)!=seen.end()||!r.has(track,20))return false;
        seen.push_back(track);
        HsdAnimTrack t;t.channel=r.block.data[track+12];t.startframe=r.block.bef32(track+8);t.frac_value=r.block.data[track+13];t.frac_slope=r.block.data[track+14];
        if(r.block.data[track+15]||!std::isfinite(t.startframe)||t.startframe<0||t.startframe>hsd_anim_detail::kMaxFrame)return false;
        if(texture?!((t.channel>=2&&t.channel<=9)||(t.channel>=12&&t.channel<=(retain_tev1?23:19))||t.channel==24):material?(t.channel<1||t.channel>10):!((t.channel>=1&&t.channel<=3)||(paths&&t.channel==4)||(t.channel>=5&&t.channel<=12)))return false;
        for(const auto& prior:out->tracks)if(prior.channel==t.channel)return false;
        const u32 size=r.block.be32(track+4),data=r.block.be32(track+16);
        if(!size||size>hsd_anim_detail::kMaxTrackBytes||size>(4u<<20)-*bytes||!std::binary_search(r.fields.begin(),r.fields.end(),track+16)||!r.has(data,size))return false;
        *bytes+=size;t.data.assign(r.block.data+data,r.block.data+data+size);out->tracks.push_back(std::move(t));
        if(!optional(r,track,&track))return false;
    }
    const bool has_path=!material&&!texture&&std::any_of(out->tracks.begin(),out->tracks.end(),[](const HsdAnimTrack& t){return t.channel==4;});
    if(has_path){
        u32 klass=0,child=0,robj=0,spline=0;
        if(!paths||!reference||!r.has(reference,64)||!optional(r,reference,&klass)||klass||r.block.be32(reference+4)!=0x4000u||!optional(r,reference+8,&child)||child||!optional(r,reference+60,&robj)||robj||!optional(r,reference+16,&spline)||!spline||!hsd_anim_detail::read_spline(r,spline,&out->path))return false;
        const auto& p=out->path;if(p.numcv<2||!std::isfinite(p.total_length)||p.total_length<=0||p.total_length>1e7||std::abs(p.tension)>1||p.seg_length.front()!=0||p.seg_length.back()!=1)return false;
        for(f64 v:p.points)if(!std::isfinite(v)||std::abs(v)>1e7)return false;
        for(f64 v:p.seg_poly)if(!std::isfinite(v)||std::abs(v)>1e14)return false;
        for(unsigned i=1;i<p.seg_length.size();++i)if(!std::isfinite(p.seg_length[i])||p.seg_length[i]<=p.seg_length[i-1]||p.seg_length[i]>1)return false;
        const std::size_t memory=(p.points.size()+p.seg_length.size()+p.seg_poly.size())*sizeof(f64);if(memory>(4u<<20)-*bytes)return false;*bytes+=memory;out->path_reference=reference;
    }else if(reference)return false;
    return true;
}
} // namespace hsd_record_detail
class HsdRecordAnimationBinding {
public:
    const HsdScene* scene()const{return data_?&data_->scene:nullptr;}
    const std::vector<hsd_record_detail::Object>* objects()const{return data_?&data_->objects:nullptr;}
private:
    std::shared_ptr<const hsd_record_detail::Instance> data_;
    friend class HsdRecordAnimationBank;friend class HsdRecordAnimationPlayer;friend class HsdShapeMaterialRecord;
};
struct HsdArchiveSceneMetadata {u32 scene_key=0,models=0,cameras=0;bool fog_present=false;};
class HsdRecordAnimationBank {
public:
    bool load(ByteView archive,std::string* error,bool share_images=false){
        data_.reset();auto fail=[&](const char* why){if(error)*error=std::string("hsd record bank: ")+why;return false;};
        if(!archive.data||archive.size<32||archive.size>(64u<<20))return fail("input budget");
        auto data=std::make_shared<hsd_record_detail::Bank>();data->bytes.assign(archive.data,archive.data+archive.size);ByteView owned{data->bytes.data(),data->bytes.size()};
        if(!hsd_load(owned,&data->scene,error)||!hsd_anim_detail::open_container(owned,&data->reader,error))return false;
        if(share_images&&!hsd_share_texture_images(data->scene))return fail("immutable image payloads");
        auto& r=data->reader;const u32 publics=owned.be32(12),externs=owned.be32(16);if(!publics||publics>4096||externs>4096)return fail("symbol budget");
        const std::size_t symbols=32+r.block.size+std::size_t(owned.be32(8))*4,strings=symbols+std::size_t(publics+externs)*8;bool found=false;u32 scene=0;
        for(u32 i=0;i<publics;++i){const u32 name=owned.be32(symbols+i*8+4);if(strings>=owned.size||name>=owned.size-strings)return fail("symbol range");const auto first=strings+name;auto end=first;while(end<owned.size&&owned.data[end]){if(end-first>=256)return fail("symbol length");++end;}if(end==owned.size)return fail("unterminated symbol");
            if(std::string(reinterpret_cast<const char*>(owned.data+first),end-first)=="scene_data"){if(found)return fail("ambiguous published scene");found=true;scene=owned.be32(symbols+i*8);}}
        u32 table=0;if(!found||!scene||!hsd_record_detail::optional(r,scene,&table)||!table)return fail("nonzero published scene/model table required");
        data->scene_export=scene;
        bool ended=false;
        for(unsigned i=0;i<=4096;++i){u32 descriptor=0;if(!hsd_record_detail::optional(r,table+i*4,&descriptor))return fail("record table range");if(!descriptor){ended=true;break;}if(i==4096||!r.has(descriptor,16))return fail("record budget");
            hsd_record_detail::Record record;u32 joint=0,material=0,shape=0;
            if(!hsd_record_detail::optional(r,descriptor,&record.root)||!record.root||!hsd_record_detail::optional(r,descriptor+4,&joint)||!hsd_record_detail::optional(r,descriptor+8,&material)||!hsd_record_detail::optional(r,descriptor+12,&shape)||
               !hsd_record_detail::pointers(r,joint,&record.joint)||!hsd_record_detail::pointers(r,material,&record.material)||!hsd_record_detail::pointers(r,shape,&record.shape))return fail("record/animation lists");
            if(i>=data->scene.model_roots.size()||record.root!=data->scene.model_roots[i])return fail("decoded/published root mismatch");
            data->records.push_back(std::move(record));
        }
        if(!ended||data->records.size()!=data->scene.model_roots.size())return fail("record table termination");
        data->hash=2166136261u;for(u8 byte:data->bytes){data->hash^=byte;data->hash*=16777619u;}data_=std::move(data);if(error)error->clear();return true;
    }
    bool select(u16 record,u8 channels,u16 joint_clip,u16 material_clip,HsdRecordAnimationBinding* out,std::string* error)const{
        return select_impl(record,channels,joint_clip,material_clip,out,error,false);
    }
    // Explicit default-JObj PATH callback/SPLINE-node entry; legacy select still
    // refuses both. Metadata descriptor nodes are consumed, never mesh-skipped.
    bool select_paths(u16 record,u8 channels,u16 joint_clip,u16 material_clip,HsdRecordAnimationBinding* out,std::string* error)const{
        return select_impl(record,channels,joint_clip,material_clip,out,error,true);
    }
    bool select_textures(u16 record,u8 channels,u16 joint_clip,u16 material_clip,HsdRecordAnimationBinding* out,std::string* error)const{
        return select_impl(record,channels,joint_clip,material_clip,out,error,false,true);
    }
    bool select_compact_textures(u16 record,u8 channels,u16 joint_clip,u16 material_clip,HsdRecordAnimationBinding* out,std::string* error)const{return select_impl(record,channels,joint_clip,material_clip,out,error,false,true,true);}
    //Explicit complete default TObj constants for the existing reviewed colour
    //expression. TEV1 is retained but not an input to that expression. Legacy
    //selectors still refuse20..23 and retain their16-track work bound.
    bool select_compact_retained_tev1_textures(u16 record,u8 channels,u16 joint_clip,u16 material_clip,HsdRecordAnimationBinding* out,std::string* error)const{return select_impl(record,channels,joint_clip,material_clip,out,error,false,true,true,true);}
    bool select_alternate_textures(u16 record,u8 channels,u16 joint_clip,u16 material_clip,const AttachmentAlternateProfile& profile,HsdRecordAnimationBinding* out,std::string* error,bool compact=false)const{
        return select_styled_textures(record,channels,joint_clip,material_clip,profile,out,error,compact,false);
    }
    //80007C54's shared translucent flags without80007CAC's separate alpha
    //replacement. The old alternate-copy entry still replaces alpha with.4.
    bool select_translucent_textures(u16 record,u8 channels,u16 joint_clip,u16 material_clip,const AttachmentAlternateProfile& profile,HsdRecordAnimationBinding* out,std::string* error,bool compact=false)const{
        return select_styled_textures(record,channels,joint_clip,material_clip,profile,out,error,compact,true);
    }
private:
    bool select_styled_textures(u16 record,u8 channels,u16 joint_clip,u16 material_clip,const AttachmentAlternateProfile& profile,HsdRecordAnimationBinding* out,std::string* error,bool compact,bool preserve_alpha)const{
        if(!out)return false;
        *out={};if(!attachment_alternate_profile_valid(profile)){if(error)*error="record alternate: invalid style";return false;}
        HsdRecordAnimationBinding normal;if(!(compact?select_compact_textures(record,channels,joint_clip,material_clip,&normal,error):select_textures(record,channels,joint_clip,material_clip,&normal,error)))return false;
        auto result=std::make_shared<hsd_record_detail::Instance>(*normal.data_);const auto& reader=data_->reader;
        for(unsigned i=0;i<result->scene.meshes.size();++i){const auto& mesh=normal.data_->scene.meshes[i];u32 klass=0,render_desc=0,pe=0;
            if(!hsd_record_detail::optional(reader,mesh.material_offset,&klass)||klass||!hsd_record_detail::optional(reader,mesh.material_offset+16,&render_desc)||render_desc||!hsd_record_detail::optional(reader,mesh.material_offset+20,&pe)||(mesh.render_mode&(0x04000000u|0x1000u)))return false;
            const u32 category=mesh.render_mode&0x60000000u;if(category==0x20000000u)return false;AttachmentAlternateState input,styled;input.joint_flags=normal.data_->scene.joints[result->scene.mesh_joint[i]].flags;input.dobj_flags=category==0?2:category==0x40000000u?8:4;input.render_mode=mesh.render_mode;input.alpha=mesh.material_alpha;input.has_pe=pe!=0;
            if(pe){if(!reader.has(pe,12))return false;std::copy_n(reader.block.data+pe,12,input.pe.begin());}
            if(!attachment_alternate_apply(profile,input,&styled,error))return false;
            if(preserve_alpha)styled.alpha=input.alpha;
            if(!attachment_alternate_mesh(mesh,styled,&result->scene.meshes[i],error))return false;
        }
        for(auto& joint:result->scene.joints){if(joint.flags&0x1000u)return false;joint.flags=(joint.flags|profile.words[0])&~profile.words[1];}
        out->data_=std::move(result);return true;
    }
public:
    // Explicit metadata query for authored no-geometry placeholders. This does
    // not make an empty drawable binding valid or swallow a selection failure.
    bool is_empty_static_record(u16 record)const{
        if(!data_||record>=data_->records.size())return false;
        const auto& entry=data_->records[record];const auto& r=data_->reader;
        if(!entry.joint.empty()||!entry.material.empty()||!entry.shape.empty()||!r.has(entry.root,64)||r.block.be32(entry.root+4))return false;
        for(unsigned offset:{0u,8u,12u,16u,56u,60u}){u32 target=0;if(!hsd_record_detail::optional(r,entry.root+offset,&target)||target)return false;}
        return true;
    }
private:
    bool select_impl(u16 record,u8 channels,u16 joint_clip,u16 material_clip,HsdRecordAnimationBinding* out,std::string* error,bool paths,bool textures=false,bool compact=false,bool retain_tev1=false)const{
        if(out)out->data_.reset();
        auto fail=[&](const char* why){if(error)*error=std::string("hsd record selection: ")+why;return false;};
        if(!data_||!out||record>=data_->records.size()||(channels&~3u)||(!(channels&1)&&joint_clip)||(!(channels&2)&&material_clip))return fail("bank/record/channel domain");
        const auto& entry=data_->records[record];if(!entry.shape.empty())return fail("shape animation requires explicit evaluator");
        auto clip=[&](const std::vector<u32>& clips,u16 index,u32* at){if(clips.empty()){*at=0;return index==0;}if(index>=clips.size())return false;*at=clips[index];return true;};
        u32 joint_root=0,material_root=0;if(((channels&1)&&!clip(entry.joint,joint_clip,&joint_root))||((channels&2)&&!clip(entry.material,material_clip,&material_root)))return fail("clip index");
        auto result=std::make_shared<hsd_record_detail::Instance>();result->bank=data_;result->record=record;auto& selected=result->scene;const auto& full=data_->scene;const auto& r=data_->reader;
        selected.model_roots={entry.root};if(!compact)selected.textures=full.textures;selected.lighting=full.lighting;
        std::vector<u32> remap(full.joints.size(),kHsdNoJoint),path_nodes,used_paths;
        for(u32 i=0;i<full.joints.size();++i)if(full.joints[i].model_root==record){
            auto j=full.joints[i];
            if(paths&&j.flags==0x4000u){
                u32 klass=0,child=0,robj=0,spline=0;if(path_nodes.size()>=256||!hsd_record_detail::optional(r,j.archive_offset,&klass)||klass||!hsd_record_detail::optional(r,j.archive_offset+8,&child)||child||!hsd_record_detail::optional(r,j.archive_offset+60,&robj)||robj||!hsd_record_detail::optional(r,j.archive_offset+16,&spline)||!spline||std::find(full.mesh_joint.begin(),full.mesh_joint.end(),i)!=full.mesh_joint.end())return fail("pure spline descriptor node required");
                path_nodes.push_back(j.archive_offset);continue;
            }
            if(selected.joints.size()>=256||(j.flags&(0x3805020u)))return fail("joint budget/instance/constraint/user matrix branch");
            for(const auto& prior:selected.joints)if(prior.archive_offset==j.archive_offset)return fail("shared joint instance requires explicit binding");
            if(j.parent!=kHsdNoJoint){if(j.parent>=remap.size()||remap[j.parent]==kHsdNoJoint)return fail("parent outside selected record");j.parent=remap[j.parent];}
            else if(!selected.joints.empty())return fail("multiple top-level roots require explicit instances");
            remap[i]=u32(selected.joints.size());j.model_root=0;selected.joints.push_back(j);
        }
        if(selected.joints.empty()||selected.joints[0].archive_offset!=entry.root)return fail("missing selected root");
        for(u32 i=0;i<full.meshes.size();++i){const auto owner=full.mesh_joint[i];if(owner>=full.joints.size())return fail("mesh owner");if(full.joints[owner].model_root!=record)continue;
            if(selected.meshes.size()>=512||remap[owner]==kHsdNoJoint||full.meshes[i].deformation_required||!full.meshes[i].material_supported)return fail("mesh budget/skin/material evaluator required");
            u32 klass=0,material=0;const u32 mobj=full.meshes[i].material_offset;
            if(!r.has(mobj,24)||!hsd_record_detail::optional(r,mobj,&klass)||klass||!hsd_record_detail::optional(r,mobj+12,&material)||!material)return fail("default nonnull material required");
            selected.meshes.push_back(full.meshes[i]);selected.mesh_joint.push_back(remap[owner]);}
        if(selected.meshes.empty())return fail("empty selected geometry");
        if(compact&&!hsd_compact_scene_textures(full,&selected))return fail("compact texture reference");
        struct Work{u32 joint,anim,material,parent,depth;};std::vector<Work> work{{entry.root,joint_root,material_root,kHsdNoJoint,0}};std::vector<u32> visited,anim_seen,material_seen;std::size_t payload=0;
        while(!work.empty()){
            const auto w=work.back();work.pop_back();if(w.depth>=64||visited.size()>=256||std::find(visited.begin(),visited.end(),w.joint)!=visited.end())return fail("joint topology/depth");visited.push_back(w.joint);
            u32 slot=kHsdNoJoint;for(u32 i=0;i<selected.joints.size();++i)if(selected.joints[i].archive_offset==w.joint)slot=i;
            if(slot==kHsdNoJoint){
                if(!paths||std::find(path_nodes.begin(),path_nodes.end(),w.joint)==path_nodes.end())return fail("source/decoded joint topology mismatch");
                u32 next=0,anext=0,mnext=0,empty=0;if(!hsd_record_detail::optional(r,w.joint+12,&next))return fail("spline descriptor sibling");
                if(w.anim){if(std::find(anim_seen.begin(),anim_seen.end(),w.anim)!=anim_seen.end()||!r.has(w.anim,20)||!hsd_record_detail::optional(r,w.anim,&empty)||empty||!hsd_record_detail::optional(r,w.anim+4,&anext)||!hsd_record_detail::optional(r,w.anim+8,&empty)||empty||!hsd_record_detail::optional(r,w.anim+12,&empty)||empty||r.block.be32(w.anim+16))return fail("spline metadata animation must be pass-through");anim_seen.push_back(w.anim);}
                if(w.material){if(std::find(material_seen.begin(),material_seen.end(),w.material)!=material_seen.end()||!r.has(w.material,12)||!hsd_record_detail::optional(r,w.material,&empty)||empty||!hsd_record_detail::optional(r,w.material+4,&mnext)||!hsd_record_detail::optional(r,w.material+8,&empty)||empty)return fail("spline metadata material must be pass-through");material_seen.push_back(w.material);}
                if(!next&&(anext||mnext))return fail("spline metadata tree shape");
                if(next)work.push_back({next,anext,mnext,w.parent,w.depth});
                continue;
            }
            if(selected.joints[slot].parent!=w.parent)return fail("source/decoded joint topology mismatch");
            u32 child=0,next=0,dobj=0,klass=0,robj=0;
            if(!r.has(w.joint,64)||!hsd_record_detail::optional(r,w.joint,&klass)||klass||!hsd_record_detail::optional(r,w.joint+8,&child)||!hsd_record_detail::optional(r,w.joint+12,&next)||!hsd_record_detail::optional(r,w.joint+16,&dobj)||!hsd_record_detail::optional(r,w.joint+60,&robj)||robj)return fail("joint class/pointers/constraint");
            u32 achild=0,anext=0,aobj=0,mchild=0,mnext=0,mat=0;
            if(w.anim){if(std::find(anim_seen.begin(),anim_seen.end(),w.anim)!=anim_seen.end())return fail("animation cycle/shared node");anim_seen.push_back(w.anim);
                if(!r.has(w.anim,20)||!hsd_record_detail::optional(r,w.anim,&achild)||!hsd_record_detail::optional(r,w.anim+4,&anext)||!hsd_record_detail::optional(r,w.anim+8,&aobj)||!hsd_record_detail::optional(r,w.anim+12,&robj)||robj||r.block.be32(w.anim+16))return fail("joint animation pointers/flags");
                if(aobj){hsd_record_detail::Object object;object.target=slot;if(!hsd_record_detail::object(r,aobj,false,&object,&payload,paths))return fail("joint AObj/track subset");
                    if(object.path_reference){if(std::find(path_nodes.begin(),path_nodes.end(),object.path_reference)==path_nodes.end())return fail("path reference outside selected record");if(std::find(used_paths.begin(),used_paths.end(),object.path_reference)==used_paths.end())used_paths.push_back(object.path_reference);}
                    result->objects.push_back(std::move(object));}}
            if(w.material){if(std::find(material_seen.begin(),material_seen.end(),w.material)!=material_seen.end())return fail("material tree cycle/shared node");material_seen.push_back(w.material);
                if(!r.has(w.material,12)||!hsd_record_detail::optional(r,w.material,&mchild)||!hsd_record_detail::optional(r,w.material+4,&mnext)||!hsd_record_detail::optional(r,w.material+8,&mat))return fail("material joint pointers");
                std::vector<u32> seen;
                while(mat){if(!dobj||seen.size()>=256||std::find(seen.begin(),seen.end(),mat)!=seen.end())return fail("material/DObj list topology");seen.push_back(mat);u32 following=0,dnext=0,mobj=0,texture=0,render=0,material=0;
                    if(!r.has(mat,16)||!r.has(dobj,16)||!hsd_record_detail::optional(r,mat,&following)||!hsd_record_detail::optional(r,mat+4,&aobj)||!hsd_record_detail::optional(r,mat+8,&texture)||(texture&&!textures)||!hsd_record_detail::optional(r,mat+12,&render)||render||!hsd_record_detail::optional(r,dobj,&klass)||klass||!hsd_record_detail::optional(r,dobj+4,&dnext)||!hsd_record_detail::optional(r,dobj+8,&mobj)||!mobj)return fail("material binding/texture/render animation subset");
                    if(aobj){if(!r.has(mobj,24)||!hsd_record_detail::optional(r,mobj,&klass)||klass||!hsd_record_detail::optional(r,mobj+12,&material)||!material)return fail("default material object required");
                        hsd_record_detail::Object object;object.kind=hsd_record_detail::Object::Material;object.target=mobj;
                        for(u32 i=0;i<selected.meshes.size();++i)if(selected.mesh_joint[i]==slot&&selected.meshes[i].dobj_offset==dobj){if(selected.meshes[i].material_offset!=mobj)return fail("material descriptor identity");object.meshes.push_back(i);}
                        if(object.meshes.empty()||!hsd_record_detail::object(r,aobj,true,&object,&payload))return fail("material AObj/track subset");
                         result->objects.push_back(std::move(object));}
                    if(texture){
                        u32 tobj=0,tnext=0,images=0,palettes=0,taobj=0,tev=0;
                        if(!r.has(mobj,24)||!hsd_record_detail::optional(r,mobj,&klass)||klass||!hsd_record_detail::optional(r,mobj+8,&tobj)||!tobj||!r.has(tobj,0x5c)||!hsd_record_detail::optional(r,tobj,&klass)||klass||!hsd_record_detail::optional(r,tobj+4,&tnext)||tnext||!hsd_record_detail::optional(r,tobj+0x58,&tev)||!r.finite(r.block,tobj+0x10,9)||(r.block.be32(tobj+0x40)&15u)||r.block.be32(tobj+0x34)>1||r.block.be32(tobj+0x38)>1)return fail("single default nonmirror UV texture required");
                        for(unsigned k=0;k<3;++k)if(r.block.bef32(tobj+0x10+k*4)!=0)return fail("texture rotation requires separate evaluator");
                        if(!r.has(texture,24)||!hsd_record_detail::optional(r,texture,&tnext)||tnext||r.block.be32(texture+4)!=r.block.be32(tobj+8)||!hsd_record_detail::optional(r,texture+8,&taobj)||!taobj||!hsd_record_detail::optional(r,texture+12,&images)||images||!hsd_record_detail::optional(r,texture+16,&palettes)||palettes||r.block.be16(texture+20)||r.block.be16(texture+22))return fail("texture animation id/images/palettes/chain");
                        hsd_record_detail::Object object;object.kind=hsd_record_detail::Object::Texture;object.target=tobj;
                        for(u32 i=0;i<selected.meshes.size();++i)if(selected.mesh_joint[i]==slot&&selected.meshes[i].dobj_offset==dobj){const auto& mesh=selected.meshes[i];if(mesh.material_offset!=mobj||(tev&&!mesh.texture_colour_lerp))return fail("texture material/expression identity");object.meshes.push_back(i);object.texture.colour_lerp=mesh.texture_colour_lerp;object.texture.low=mesh.texture_low_colour;object.texture.high=mesh.texture_high_colour;}
                         object.texture.blend=r.block.bef32(tobj+0x44);
                         object.retain_tev1=retain_tev1;
                         if(retain_tev1){if(!tev||!r.has(tev,32)||!object.texture.colour_lerp)return fail("retained TEV1 requires reviewed independent colour expression");object.texture.tev1=r.block.be32(tev+24);}
                        for(unsigned k=0;k<2;++k){object.texture.translate[k]=r.block.bef32(tobj+0x28+k*4);object.texture.scale[k]=r.block.bef32(tobj+0x1c+k*4);object.texture.repeat[k]=r.block.data[tobj+0x3c+k];}
                         std::array<f32,2> uv{},offset{};if(object.meshes.empty()||!hsd_texture_uv(object.texture.translate,object.texture.scale,object.texture.repeat,&uv,&offset)||!std::isfinite(object.texture.blend)||object.texture.blend<0||object.texture.blend>1||!hsd_record_detail::object(r,taobj,false,&object,&payload,false,true,retain_tev1))return fail("texture AObj/transform subset");
                        for(const auto& track:object.tracks)if(track.channel>=12&&track.channel<=19&&!object.texture.colour_lerp)return fail("animated colour requires reviewed texture expression");
                        result->objects.push_back(std::move(object));
                    }
                    mat=following;dobj=dnext;
                }
            }
            if((!child&&(achild||mchild))||(!next&&(anext||mnext))||(!w.depth&&next)||result->objects.size()>1024)return fail("animation/model shape or object budget");
            if(next)work.push_back({next,anext,mnext,w.parent,w.depth});
            if(child)work.push_back({child,achild,mchild,slot,w.depth+1});
        }
        if(visited.size()!=selected.joints.size()+path_nodes.size()||used_paths.size()!=path_nodes.size())return fail("unvisited joint or unconsumed spline descriptor");
        out->data_=std::move(result);if(error)error->clear();return true;
    }
public:
    bool select_compact_static(u16 record,HsdRecordAnimationBinding* out,std::string* error)const{return select_impl(record,0,0,0,out,error,false,false,true);}
    u32 hash()const{return data_?data_->hash:0;}
    bool same_source(const HsdRecordAnimationBank& other)const{return data_&&data_==other.data_;}
    //06C88's public0→name lookup→scene data contract. Null camera TABLE is not
    //an empty list: source dereferences the table even when its first entry is0.
    bool archive_scene_metadata(HsdArchiveSceneMetadata* out,std::string* error)const{
        if(out)*out={};
        auto fail=[&](){if(error)*error="hsd archive scene: first scene export/nonempty model or camera-table domain";return false;};
        if(!out||!data_){return fail();}const ByteView b{data_->bytes.data(),data_->bytes.size()};const auto& r=data_->reader;const auto symbols=32+r.block.size+std::size_t(b.be32(8))*4;
        if(!b.be32(12)||!b.contains(symbols,8)||b.be32(symbols)!=data_->scene_export)return fail();
        const auto strings=symbols+std::size_t(b.be32(12)+b.be32(16))*8;const auto name=b.be32(symbols+4);if(!b.contains(strings+name,11)||!std::equal(b.data+strings+name,b.data+strings+name+11,reinterpret_cast<const u8*>("scene_data")))return fail();
        u32 cameras=0,fog=0;if(!hsd_record_detail::optional(r,data_->scene_export+8,&cameras)||!cameras||!hsd_record_detail::optional(r,data_->scene_export+12,&fog)||data_->records.empty())return fail();
        std::vector<u32> entries;if(!hsd_record_detail::pointers(r,cameras,&entries))return fail();*out={data_->scene_export,u32(data_->records.size()),u32(entries.size()),fog!=0};if(error)error->clear();return true;
    }
    std::size_t records()const{return data_?data_->records.size():0;}
    const SceneLighting* lighting()const{return data_?&data_->scene.lighting:nullptr;}
    bool read_linear_fog(LinearFog* out,std::string* error)const{if(out)*out={};return data_?hsd_detail::read_scene_linear_fog(data_->reader,data_->scene_export,out,error):false;}
private:
    std::shared_ptr<const hsd_record_detail::Bank> data_;
    friend class HsdShapeRecord;
    friend class HsdShapeMaterialRecord;
    friend class HsdJointInstanceBank;
    friend class HsdJointInstanceOwner;
    friend class CharacterModelScene;
};
} // namespace gf
