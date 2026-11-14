#pragma once
#include <map>
#include <span>
#include "engine/gameplay/challenge_setup_chrome.hpp"
#include "engine/gameplay/animated_model_scene.hpp"
namespace gf {
//One prepared bank/model cache for the Challenge setup pages. The caller owns
//widget clocks and packet order; repeated draw/pass calls never advance them.
//Uses the existing record/material/UV/world renderer, not a renderer fork.
class ChallengeSetupScene {
public:
    bool load_prepared(const HsdRecordAnimationBank& bank,const ChallengeDifficultyArt& art,const AttachmentAlternateProfile& style,std::string* error){
        *this={};if(!art.valid()||!attachment_alternate_profile_valid(style)||bank.hash()!=art.bank_hash||!bank.records())return fail(error,"art/bank/style binding");
        bank_=bank;camera_=art.camera;style_=style;ready_=true;return true;
    }
    bool draw(RenderEngine& render,std::span<const ChallengeSetupDraw> poses,std::string* error){
        if(!ready_)return false;
        if(poses.size()>80)return fail(error,"packet budget");
        for(const auto& pose:poses){
            if(pose.authored_alpha&&pose.alpha!=1)return fail(error,"authored alpha cannot also be replaced");
            if(pose.authored_alpha&&pose.after_callbacks_alpha)return fail(error,"conflicting alpha owners");
            if(pose.record>=bank_.records()||(pose.mask&~0x7fu)||!std::isfinite(pose.alpha)||pose.alpha<0||pose.alpha>1||!std::isfinite(pose.joint_time)||pose.joint_time<0||pose.joint_time>8192||!std::isfinite(pose.material_time)||pose.material_time<0||pose.material_time>8192)return fail(error,"packet domain");
            for(f32 value:pose.world.m)if(!std::isfinite(value)||std::abs(value)>1e7f)return fail(error,"packet world");
        }
        for(u8 pass:{u8(1),u8(4),u8(2)}){
            if(!render.begin_model_pass(pass))return fail(error,"model pass");
            bool accepted=true;
            //097B4 prepends each submitted instance to its source render queue.
            for(auto it=poses.rbegin();it!=poses.rend();++it){
                const auto& pose=*it;const bool queued=pass==1?(pose.mask&1):pass==4?(pose.mask&2):(pose.mask&0x7c);
                if(queued&&!draw_pose(render,pose,error)){accepted=false;break;}
            }
            if(!accepted)render.reject_model_pass();
            const bool ended=render.end_model_pass();if(!accepted||!ended)return fail(error,"shared submission refused");
        }
        return true;
    }
    bool ready()const{return ready_;}std::size_t models()const{return models_.size();}
    //Append one source widget inside the caller's already-open global pass.
    //This reuses the retained prepared bank/cache; it never advances a clock.
    bool draw_in_pass(RenderEngine& render,const ChallengeSetupDraw& pose,std::string* error){
        const auto pass=render.model_pass();
        if(!ready_||(pass!=1&&pass!=4&&pass!=2)||pose.record>=bank_.records()||(pose.mask&~0x7fu)||!std::isfinite(pose.alpha)||pose.alpha<0||pose.alpha>1||!std::isfinite(pose.joint_time)||pose.joint_time<0||pose.joint_time>8192||!std::isfinite(pose.material_time)||pose.material_time<0||pose.material_time>8192||(pose.authored_alpha&&(pose.alpha!=1||pose.after_callbacks_alpha)))return fail(error,"queued pose/pass domain");
        for(f32 value:pose.world.m)if(!std::isfinite(value)||std::abs(value)>1e7f)return fail(error,"queued world");
        const bool queued=pass==1?(pose.mask&1):pass==4?(pose.mask&2):(pose.mask&0x7c);
        return !queued||draw_pose(render,pose,error);
    }
    u32 bank_hash()const{return ready_?bank_.hash():0;}
private:
    struct Model {AnimatedModelScene scene;f32 joint_time=0,material_time=0;};
    bool draw_pose(RenderEngine& render,const ChallengeSetupDraw& pose,std::string* error){
        const u32 key=u32(pose.record)*2+unsigned(pose.translucent)+(pose.authored_alpha?8192u:0u)+(pose.after_callbacks_alpha?16384u:0u);auto found=models_.find(key);
        if(found==models_.end()){
            if(models_.size()>=80)return fail(error,"model budget");
            Model next;const bool loaded=pose.translucent?next.scene.load_compact_translucent_textures(bank_,pose.record,3,0,0,style_,error):next.scene.load_compact_textures(bank_,pose.record,3,0,0,error);
            if(!loaded||!next.scene.request_selectors(pose.joint_time,pose.material_time,error))return false;
            next.scene.set_enabled(true);next.scene.set_untextured_billboards(true);next.joint_time=pose.joint_time;next.material_time=pose.material_time;
            next.scene.upload_textures_shared(render,40000);if(render.shared_texture_upload_failed())return false;
            found=models_.emplace(key,std::move(next)).first;
        }
        auto& model=found->second;
        if(model.joint_time!=pose.joint_time||model.material_time!=pose.material_time){
            if(!model.scene.request_again_selectors(pose.joint_time,pose.material_time,error))return false;
            model.joint_time=pose.joint_time;model.material_time=pose.material_time;
        }
        if(!pose.authored_world){
            if(!model.scene.set_root_world(pose.world,error))return false;
            if(pose.after_callbacks_alpha){if(!model.scene.replace_after_material_callbacks_alpha(pose.alpha,error))return false;}
            else if(!pose.authored_alpha&&!model.scene.replace_unanimated_alpha(pose.alpha,error))return false;
        }
        if(pose.always_depth){if(!model.scene.set_depth_compare(7))return false;}else model.scene.clear_depth_compare();
        model.scene.draw(render,camera_);return true;
    }
    bool fail(std::string* error,const char* why){ready_=false;if(error&&error->empty())*error=std::string("Challenge setup scene: ")+why;return false;}
    HsdRecordAnimationBank bank_;Camera camera_;AttachmentAlternateProfile style_;std::map<u32,Model> models_;bool ready_=false;
};
} // namespace gf
