#pragma once
#include "engine/assets/hsd_shape_kernel.hpp"
namespace gf {
// Explicit complete single-joint DObj chain, one position-only average shape
// per DObj plus default MObj animation. The legacy shape-only entry stays strict.
class HsdShapeMaterialRecord {
public:
    bool load(const HsdRecordAnimationBank& bank,u16 record,std::string* error){
        *this={};auto fail=[&](const char* why){if(error)*error=std::string("HSD shape/material: ")+why;return false;};
        if(!bank.data_||record>=bank.data_->records.size())return fail("published record");
        const auto& data=*bank.data_;const auto& r=data.reader;const auto& entry=data.records[record];
        if(entry.shape.size()!=1||entry.material.size()!=1||!entry.joint.empty())return fail("one shape and material clip, no joint clip");
        const HsdJoint* joint=nullptr;unsigned expected=0;
        for(const auto& j:data.scene.joints)if(j.model_root==record){if(joint)return fail("single root required");joint=&j;}
        for(unsigned i=0;i<data.scene.meshes.size();++i)expected+=data.scene.joints[data.scene.mesh_joint[i]].model_root==record;
        if(!joint||joint->archive_offset!=entry.root||joint->parent!=kHsdNoJoint||(joint->flags&0xe00u)||!expected||expected>16)return fail("root/mesh domain");
        auto ptr=[&](u32 at,u32* value){return hsd_record_detail::optional(r,at,value);};u32 klass=0,child=0,next=0,robj=0,dobj=0;
        if(!ptr(entry.root,&klass)||klass||!ptr(entry.root+8,&child)||child||!ptr(entry.root+12,&next)||next||!ptr(entry.root+16,&dobj)||!dobj||!ptr(entry.root+60,&robj)||robj)return fail("default single joint");
        u32 shape=0,material=0;for(unsigned which=0;which<2;++which){const auto root=which?entry.material[0]:entry.shape[0];u32 leaf=0;if(!r.has(root,12)||!ptr(root,&child)||child||!ptr(root+4,&next)||next||!ptr(root+8,&leaf)||!leaf)return fail("parallel leaf animation trees");if(which)material=leaf;else shape=leaf;}
        auto instance=std::make_shared<hsd_record_detail::Instance>();instance->bank=bank.data_;instance->record=record;instance->scene.lighting=data.scene.lighting;instance->scene.model_roots={entry.root};instance->scene.joints={*joint};auto& j=instance->scene.joints[0];j.model_root=0;for(unsigned k=0;k<3;++k){j.rotation[k]=j.translation[k]=0;j.scale[k]=1;}
        std::vector<hsd_shape_detail::Mesh> meshes;std::vector<u32> seen;std::size_t payload=0;
        while(dobj){
            if(seen.size()>=16||std::find(seen.begin(),seen.end(),dobj)!=seen.end()||!shape||!material)return fail("DObj/animation chain extent");
            seen.push_back(dobj);
            u32 following=0,pobj=0,mobj=0,set=0,shape_next=0,shape_anim=0,shape_aobj=0,mat_next=0,mat_aobj=0,texture=0,render=0;
            if(!r.has(dobj,16)||!ptr(dobj,&klass)||klass||!ptr(dobj+4,&following)||!ptr(dobj+8,&mobj)||!mobj||!ptr(dobj+12,&pobj)||!pobj||!r.has(pobj,24)||!ptr(pobj,&klass)||klass||!ptr(pobj+4,&next)||next||!ptr(pobj+20,&set)||!set)return fail("DObj/single PObj binding");
            const HsdMesh* mesh=nullptr;for(unsigned i=0;i<data.scene.meshes.size();++i)if(data.scene.joints[data.scene.mesh_joint[i]].model_root==record&&data.scene.meshes[i].dobj_offset==dobj){if(mesh)return fail("ambiguous DObj mesh");mesh=&data.scene.meshes[i];}
            if(!mesh||mesh->pobj_offset!=pobj||mesh->material_offset!=mobj||!mesh->material_supported||mesh->texture!=kHsdNoTexture||mesh->generated_uv_required||mesh->texture_matrix_required||(mesh->pobj_flags&0x3000u)!=0x1000u)return fail("untextured average mesh required");
            if(!r.has(shape,8)||!ptr(shape,&shape_next)||!ptr(shape+4,&shape_anim)||!shape_anim||!r.has(shape_anim,8)||!ptr(shape_anim,&next)||next||!ptr(shape_anim+4,&shape_aobj)||!shape_aobj)return fail("shape animation pairing");
            if(!r.has(material,16)||!ptr(material,&mat_next)||!ptr(material+4,&mat_aobj)||!mat_aobj||!ptr(material+8,&texture)||texture||!ptr(material+12,&render)||render||!r.has(mobj,24)||!ptr(mobj,&klass)||klass||!ptr(mobj+8,&texture)||texture)return fail("default MObj animation without texture/render channels");
            hsd_shape_detail::Mesh evaluated;if(!hsd_shape_detail::load(r,*mesh,set,shape_aobj,&evaluated,error))return false;
            hsd_record_detail::Object animation;animation.kind=hsd_record_detail::Object::Material;animation.target=mobj;animation.meshes={u32(meshes.size())};if(!hsd_record_detail::object(r,mat_aobj,true,&animation,&payload))return fail("MObj AObj subset");
            meshes.push_back(std::move(evaluated));instance->objects.push_back(std::move(animation));instance->scene.meshes.push_back(*mesh);instance->scene.meshes.back().deformation_required=false;instance->scene.mesh_joint.push_back(0);dobj=following;shape=shape_next;material=mat_next;
        }
        if(shape||material||meshes.size()!=expected)return fail("all source meshes and animations must be consumed");
        meshes_=std::move(meshes);binding_.data_=std::move(instance);ready_=true;if(error)error->clear();return true;
    }
    bool sample_shapes(u32 frame,std::vector<std::vector<HsdVertex>>* out,std::vector<f32>* blends,std::string* error)const{
        return sample_impl(frame,false,out,blends,error);
    }
    bool sample_shapes_fractional(f32 frame,std::vector<std::vector<HsdVertex>>* out,std::vector<f32>* blends,std::string* error)const{
        return sample_impl(frame,true,out,blends,error);
    }
    unsigned meshes()const{return unsigned(meshes_.size());}
    const HsdMesh* original_mesh(unsigned i)const{return ready_&&i<meshes_.size()?&meshes_[i].original:nullptr;}
private:
    bool sample_impl(f64 frame,bool fractional,std::vector<std::vector<HsdVertex>>* out,std::vector<f32>* blends,std::string* error)const{
        if(!ready_||!out)return false;
        std::vector<std::vector<HsdVertex>> vertices(meshes_.size());std::vector<f32> values(meshes_.size());
        for(unsigned i=0;i<meshes_.size();++i)if(!hsd_shape_detail::sample_impl(meshes_[i],frame,fractional,&vertices[i],&values[i],error))return false;
        *out=std::move(vertices);if(blends)*blends=std::move(values);return true;
    }
    friend class AnimatedShapeModelScene;
    bool ready_=false;std::vector<hsd_shape_detail::Mesh> meshes_;HsdRecordAnimationBinding binding_;
};
} // namespace gf
