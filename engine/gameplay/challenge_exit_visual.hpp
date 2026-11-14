#pragma once
#include "engine/assets/challenge_exit_dialog.hpp"
#include "engine/gameplay/challenge_setup_scene.hpp"
namespace gf {
//Source type15. Panel Y scale uses the SAME native HSD sampler as the model,
//not a fitted easing curve. Zero-Y endpoints are explicit degenerate no-draw
//poses, never a relaxation of AnimatedModelScene's singular-root guard.
class ChallengeExitVisual {
public:
    bool open(const HsdRecordAnimationBank& bank,const ChallengeExitDialogArt& art,f32 alpha_step,u16 tick,std::string* error){
        *this={};if(!art.valid()||bank.hash()!=art.bank_hash||!std::isfinite(alpha_step)||alpha_step<=0||alpha_step>1)return false;
        art_=art;step_=alpha_step;HsdRecordAnimationBinding binding;
        if(!bank.select_compact_textures(art.parts[0].record,1,0,0,&binding,error)||!panel_.bind(binding,error))return false;
        ready_=true;return update(1,1,tick,error);
    }
    bool update(u8 state,u8 selection,u16 tick,std::string* error){
        if(!ready_||state>2||selection>1||updates_>=8192)return fail();
        if(retiring_){if(!hidden_){hidden_=true;draws_.clear();++updates_;return true;}ready_=false;draws_.clear();return true;}
        if(!closing_){
            if(!state){closing_=true;time_=art_.close_start;}
            else if(time_<art_.panel_end)time_=std::min(art_.panel_end,f32(time_+1));
            if(time_>=art_.close_start&&reveal_<1)reveal_=std::min(1.f,f32(reveal_+step_));
        }else{
            if(reveal_>0)reveal_=std::max(0.f,f32(reveal_-step_));
            time_=f32(time_-1);if(time_<=0){time_=0;retiring_=true;}
        }
        if(panel_.pose()){if(!panel_.request_again_selectors(time_,0,error))return fail();}
        else if(!panel_.request_selectors(time_,0,error))return fail();
        const auto* pose=panel_.pose();if(!pose||pose->joints.empty())return fail();
        const auto& scale=pose->joints[0].scale;
        if(reveal_<1)angle_=0;else angle_=std::bit_cast<s16>(u16(u16(angle_)-256u));
        draws_.clear();const f32 unit=bits_to_f32(0x38c90fdb);
        for(unsigned i=0;i<6;++i){if(i&&reveal_<=0)continue;auto point=art_.parts[i].position;if(i==1||i==2)point[0]=std::fma(art_.choice_step,f32(selection),point[0]);point[2]=f32(point[2]+art_.depth);
            const f32 sizes[3]={i?1:scale[0],i?reveal_:scale[1],i?1:scale[2]};
            if(!sizes[0]||!sizes[1]||!sizes[2])continue;
            ChallengeSetupDraw draw;draw.record=art_.parts[i].record;draw.mask=0x44;draw.authored_alpha=true;draw.joint_time=i?0:time_;
            if(i>=4)draw.material_time=i-4==selection?((state==2&&(tick&4))?1:0):2;
            auto world=mat4_translation(point.data());if(i==1)world=mat4_multiply(world,mat4_rotation_z(f32(angle_)*unit));draw.world=mat4_multiply(world,mat4_scale(sizes));draws_.push_back(draw);
        }
        ++updates_;return true;
    }
    const std::vector<ChallengeSetupDraw>& draws()const{return draws_;}bool live()const{return ready_;}f32 time()const{return time_;}f32 reveal()const{return reveal_;}
private:
    bool fail(){ready_=false;draws_.clear();return false;}
    ChallengeExitDialogArt art_;HsdRecordAnimationPlayer panel_;std::vector<ChallengeSetupDraw> draws_;f32 step_=0,time_=0,reveal_=0;s16 angle_=0;u32 updates_=0;bool ready_=false,closing_=false,retiring_=false,hidden_=false;
};
} // namespace gf
