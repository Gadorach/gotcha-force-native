#pragma once
#include "engine/assets/character_model_parts.hpp"
#include "engine/gameplay/character_model_scene.hpp"
#include "engine/gameplay/attachment_model_scene.hpp"
namespace gf {
// Normal source4FCFC tail, using the existing rigid-record/material/renderer
// path. No alternate AE, optional4D4 callback or arbitrary source scene owner.
class CharacterModelParts {
public:
    bool bind(const CharacterModelScene& parent,ByteView bank,ByteView profile,u16 model_id,std::string* error){
        if(error)error->clear();
        ready_=loaded_=false;binding_.reset();parts_.clear();frame_=0;target_=nullptr;const auto binding=parent.instance_binding();
        if(!binding||!character_model_parts_load(profile,binding->model_hash,&profile_,error)||profile_.model_id!=model_id||parent.rig().binding.bones.size()<=11)return fail(error,"native parent/model/profile/rig");
        AttachmentModelBank decoded;if(!decoded.load(bank,error)||decoded.hash()!=binding->model_hash)return fail(error,"source bank binding");
        std::vector<AttachmentModelScene> parts(profile_.groups[1].base+profile_.groups[1].count);
        for(unsigned i=1;i<parts.size();++i)if(!parts[i].load_shared(decoded,u16(i),error))return false;
        parts_=std::move(parts);binding_=binding;target_=&parent;loaded_=true;return true;
    }
    bool prepare(const CharacterModelScene& parent,const CharacterWorldPose& world,u64 frame,std::array<u8,2> selectors,
                 u32 visible,u32 suppressed,const ActorFadeMaterial& fade,std::string* error){
        if(error)error->clear();
        discard_pose();if(!bound(parent)||!parent.ready()||!frame||frame_==~u64(0)||frame!=frame_+1||!fade.source_class)return fail(error,"parent/frame/material");
        const auto& rig=parent.rig();
        if(world.matrices.size()!=rig.binding.bones.size()||world.joint_offsets.size()!=world.matrices.size())return fail(error,"world shape");
        for(unsigned i=0;i<world.matrices.size();++i)if(world.joint_offsets[i]!=rig.binding.bones[i].joint_offset||!character_pose_detail::matrix(world.matrices[i]))return fail(error,"world/rig identity");
        CharacterModelPartSelection selected;if(!character_model_parts_select(profile_,selectors,visible,suppressed,&selected))return fail(error,"selector domain");
        for(unsigned i=0;i<2;++i)if(selected.visible[i]){AttachmentBoneTransform transform;transform.valid=true;transform.world=world.matrices[profile_.groups[i].bone];if(!parts_[selected.record[i]].set_world_fade(transform,fade,false,error)){discard_pose();return false;}}
        selected_=selected;frame_=frame;ready_=true;return true;
    }
    unsigned upload_textures_shared(RenderEngine& render,u32 base){if(!loaded_)return 0;unsigned count=0;for(unsigned i=1;i<parts_.size();++i)count+=parts_[i].upload_textures_shared(render,base);return count;}
    unsigned draw(const CharacterModelScene& parent,RenderEngine& render,const Camera& camera){if(!ready_||!bound(parent))return 0;unsigned n=0;for(unsigned i=0;i<2;++i)if(selected_.visible[i])n+=parts_[selected_.record[i]].draw(render,camera);return n;}
    void discard_pose(){ready_=false;selected_={};for(auto& part:parts_)part.discard_pose();}
    bool bound(const CharacterModelScene& parent)const{return loaded_&&target_==&parent&&binding_.lock()&&binding_.lock()==parent.instance_binding();}
    const CharacterModelPartSelection& selection()const{return selected_;}u64 frame()const{return frame_;}
    const AttachmentModelScene* part(unsigned record)const{return ready_&&record&&record<parts_.size()?&parts_[record]:nullptr;}
private:
    static bool fail(std::string* error,const char* why){if(error&&error->empty())*error=std::string("character model parts: ")+why;return false;}
    CharacterModelPartsProfile profile_;std::vector<AttachmentModelScene> parts_;std::weak_ptr<const CharacterModelBinding> binding_;const CharacterModelScene* target_=nullptr;
    CharacterModelPartSelection selected_;u64 frame_=0;bool loaded_=false,ready_=false;
};
} //namespace gf
