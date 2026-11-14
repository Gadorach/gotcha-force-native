#pragma once
#include "engine/assets/hsd_shape_kernel.hpp"
namespace gf {
// Published single-root/single-mesh average shapes. Original8024A660/8024BAF0:
// scalar frame selects adjacent shapes, clamps the fraction, then fsubs+fmadds.
// Additive blends, animated normals, NBT, skin and arbitrary trees stay refused.
class HsdShapeRecord {
public:
    bool load(const HsdRecordAnimationBank& bank,u16 record,std::string* error,bool compact=false){
        *this={};auto fail=[&](const char* why){if(error)*error=std::string("HSD shape: ")+why;return false;};
        if(!bank.data_||record>=bank.data_->records.size())return fail("published record required");
        const auto& entry=bank.data_->records[record];const auto& full=bank.data_->scene;const auto& r=bank.data_->reader;
        if(entry.shape.size()!=1||!entry.joint.empty()||!entry.material.empty())return fail("single shape-only clip required");
        const HsdJoint* joint=nullptr;const HsdMesh* mesh=nullptr;
        for(const auto& j:full.joints)if(j.model_root==record){if(joint)return fail("single root subset");joint=&j;}
        for(unsigned i=0;i<full.meshes.size();++i)if(full.joints[full.mesh_joint[i]].model_root==record){if(mesh)return fail("single shape mesh subset");mesh=&full.meshes[i];}
        if(!joint||!mesh||joint->archive_offset!=entry.root||joint->parent!=kHsdNoJoint||(joint->flags&0xe00u)||!mesh->material_supported||mesh->generated_uv_required||mesh->texture_matrix_required||(mesh->pobj_flags&0x3000u)!=0x1000u)return fail("root/material/shape geometry domain");
        u32 klass=0,child=0,next=0,dobj=0,robj=0,pobj=0,set=0;
        if(!hsd_record_detail::optional(r,entry.root,&klass)||klass||!hsd_record_detail::optional(r,entry.root+8,&child)||child||!hsd_record_detail::optional(r,entry.root+12,&next)||next||!hsd_record_detail::optional(r,entry.root+16,&dobj)||dobj!=mesh->dobj_offset||!hsd_record_detail::optional(r,entry.root+60,&robj)||robj||!hsd_record_detail::optional(r,dobj,&klass)||klass||!hsd_record_detail::optional(r,dobj+4,&next)||next||!hsd_record_detail::optional(r,dobj+12,&pobj)||pobj!=mesh->pobj_offset||!hsd_record_detail::optional(r,pobj+4,&next)||next||!hsd_record_detail::optional(r,pobj+20,&set)||!set||!r.has(set,28))return fail("published shape pointer chain");
        const u32 sh=entry.shape[0];u32 shd=0,sha=0,aobj=0;
        if(!r.has(sh,12)||!hsd_record_detail::optional(r,sh,&child)||child||!hsd_record_detail::optional(r,sh+4,&next)||next||!hsd_record_detail::optional(r,sh+8,&shd)||!shd||!r.has(shd,8)||!hsd_record_detail::optional(r,shd,&next)||next||!hsd_record_detail::optional(r,shd+4,&sha)||!sha||!r.has(sha,8)||!hsd_record_detail::optional(r,sha,&next)||next||!hsd_record_detail::optional(r,sha+4,&aobj)||!aobj)return fail("shape animation binding");
        if(!hsd_shape_detail::load(r,*mesh,set,aobj,&mesh_,error))return false;
        source_=bank.data_;
        if(!compact)scene_.textures=full.textures;
        scene_.lighting=full.lighting;scene_.model_roots={entry.root};scene_.joints={*joint};scene_.mesh_joint={0};scene_.meshes={*mesh};
        if(compact&&!hsd_compact_scene_textures(full,&scene_))return fail("compact shape texture reference");
        auto& j=scene_.joints[0];j.model_root=0;for(unsigned k=0;k<3;++k){j.rotation[k]=j.translation[k]=0;j.scale[k]=1;}
        // Explicit evaluated draw copy only; original raw shape remains flagged.
        scene_.meshes[0].deformation_required=false;ready_=true;if(error)error->clear();return true;
    }
    bool sample(u32 frame,std::vector<HsdVertex>* out,f32* blend,std::string* error)const{
        return ready_&&hsd_shape_detail::sample(mesh_,frame,out,blend,error);
    }
    //Explicit requested-time shape entry; integer callers retain their old path.
    bool sample_fractional(f32 frame,std::vector<HsdVertex>* out,f32* blend,std::string* error)const{
        return ready_&&hsd_shape_detail::sample_fractional(mesh_,frame,out,blend,error);
    }
    const HsdMesh* original_mesh()const{return ready_?&mesh_.original:nullptr;}
    unsigned shapes()const{return unsigned(mesh_.shapes.size());}f32 end()const{return mesh_.end;}
private:
    friend class ShapeModelScene;
    std::shared_ptr<const hsd_record_detail::Bank> source_;bool ready_=false;HsdScene scene_;hsd_shape_detail::Mesh mesh_;
};
} // namespace gf
