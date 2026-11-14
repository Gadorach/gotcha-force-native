#pragma once
#include "engine/assets/force_wheel_motion.hpp"
#include "engine/gameplay/force_budget_band.hpp"
namespace gf {
struct ForceWheelPose {f32 offset=0;s16 angle=0;};
struct ForceWheelBandPose {f32 frame=0;s16 angle=0;u8 slot=0;};
class ForceWheelMotion {
public:
    bool configure(const ForceWheelMotionProfile& profile,unsigned turn=0,bool folded=false){
        *this={};if(!profile.valid()||turn>=30)return false;profile_=profile;previous_=target_=turn;folded_=folded;wheel_=folded?profile.wheel_fold:0;cost_=folded?profile.cost_fold:0;tab_=folded?profile.tab_fold:0;ready_=true;return true;
    }
    bool request_turn(int direction){
        if(!ready_||remaining_||folded_||(direction!=-1&&direction!=1))return false;
        previous_=target_;target_=unsigned((int(target_)+direction+30)%30);remaining_=profile_.turn_ticks;return true;
    }
    bool toggle_fold(){if(!ready_)return false;folded_=!folded_;return true;}
    //BF144 accepts a turn and stores8; BABBC decrements BEFORE actor callbacks.
    //Therefore the first visible update is1/8, with eight callbacks to settle.
    bool step(){
        if(!ready_)return false;
        if(remaining_){--remaining_;if(!remaining_)previous_=target_;}
        wheel_=approach(wheel_,folded_?profile_.wheel_fold:0,profile_.wheel_step);cost_=approach(cost_,folded_?profile_.cost_fold:0,profile_.cost_step);tab_=approach(tab_,folded_?profile_.tab_fold:0,profile_.tab_step);return true;
    }
    unsigned turn()const{return target_;}unsigned previous()const{return previous_;}unsigned remaining()const{return remaining_;}
    bool turning()const{return remaining_!=0;}bool folded()const{return folded_;}
    f32 wheel_shift()const{return wheel_;}f32 cost_shift()const{return cost_;}f32 tab_shift()const{return tab_;}
    bool member(const ForceEditorArt& art,unsigned slot,ForceWheelPose* out)const{
        if(!ready_||!out||slot>=30)return false;
        const auto old=(slot+30-previous_)%30,next=(slot+30-target_)%30;return interpolate(art,old,next,progress(),out);
    }
    bool marker(const ForceEditorArt& art,unsigned slot,ForceWheelPose* out)const{
        if(!ready_||!out||slot>=30)return false;
        const unsigned old=(slot+30-previous_)%30,next=(slot+30-target_)%30,difference=(next+30-old)%30;const auto t=progress();
        f32 at=f32(difference>15?next:old)+(difference>15?1.f-t:t)-profile_.half;while(at<0)at+=30;while(at>=30)at-=30;const auto first=unsigned(at);return interpolate(art,first,(first+1)%30,f32(at-first),out);
    }
    bool band(const ForceBudgetBandArt& art,s32 prefix,std::vector<ForceWheelBandPose>* out)const{
        if(!out)return false;
        out->clear();if(!ready_||prefix<-1||prefix>30||art.parts.size()!=11||!art.has_following)return false;if(prefix<0)return true;
        for(unsigned i=0;i<11;++i){unsigned slot=(target_+i+27)%30;const auto old=(previous_+i+27)%30;const auto& a=art.parts[i];const auto& b=i+1<11?art.parts[i+1]:art.following;f32 weight=0;
            if(slot!=old){weight=f32(remaining_)*profile_.reciprocal;if((slot+30-old)%30>15){weight=1.f-weight;slot=old;}}
            if(slot<unsigned(prefix))out->push_back({std::fma(weight,f32(s32(b.frame)-s32(a.frame)),f32(a.frame)),angle(a.angle,b.angle,weight),u8(slot)});
        }return true;
    }
private:
    static f32 approach(f32 value,f32 target,f32 step){return value+std::clamp(target-value,-step,step);}
    f32 progress()const{return remaining_?std::fma(-f32(remaining_),profile_.reciprocal,1.f):0.f;}
    static s16 angle(s16 a,s16 b,f32 t){const s32 delta=s32(f32(f32(s32(b)-s32(a))*t));return std::bit_cast<s16>(u16(s32(a)+std::bit_cast<s16>(u16(delta))));}
    static bool interpolate(const ForceEditorArt& art,unsigned a,unsigned b,f32 t,ForceWheelPose* out){
        if(!std::isfinite(art.offsets[a])||!std::isfinite(art.offsets[b]))return false;
        const auto offset=std::fma(t,f32(art.offsets[b]-art.offsets[a]),art.offsets[a]);if(!std::isfinite(offset))return false;*out={offset,angle(art.angles[a],art.angles[b],t)};return true;
    }
    bool ready_=false,folded_=false;ForceWheelMotionProfile profile_;unsigned previous_=0,target_=0,remaining_=0;f32 wheel_=0,cost_=0,tab_=0;
};
} // namespace gf
