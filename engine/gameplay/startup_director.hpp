#pragma once
//801C7DE4 command prefix BEFORE actor updates;801C7BA0 camera/config AFTER them.
// Resource/audio/camera/actor effects are ordered outputs, not host admission.
#include <span>
#include "engine/assets/startup_director.hpp"
namespace gf {
struct StartupDirectorObservations {
    bool fade_busy=false,resources_busy=false;
    std::array<bool,33> camera_end_valid{}; //0..31 scene source,32 main/FF
    std::array<f32,33> camera_end{};
};
enum class StartupEffectKind {Command,ActorFrame,CameraSample,ConfigSample,WaitBegin,Terminal,FrameEnd};
struct StartupEffect {
    StartupEffectKind kind=StartupEffectKind::Command;u8 opcode=0,actor=255,render_group=0;
    u32 command_offset=0;std::array<u8,5> operands{};f32 value=0;
};
struct StartupDirectorState {
    u32 cursor=0,mark=0xffffffffu,updates=0;bool latch=false,terminal=false,actors_enabled=false,camera_link=false,config_enabled=false,phase12=false;
    s16 counter=0;u8 phase=0,camera_source=255;s8 camera_active=0;
    f32 camera_frame=0,camera_goal=0,camera_step=0,config_value=0,config_goal=0,config_step=0;
    std::array<u32,8> config_words{};
};
class StartupDirector {
public:
    bool load(const StartupDirectorProfile& profile,std::span<const u8> actor_render_indices,std::string* error){
        ready_=false;profile_={};state_={};render_indices_={};
        if(!startup_director_detail::valid(profile)||actor_render_indices.size()!=profile.actor_ids.size())return fail(error,"profile/actor render-index binding");
        profile_=profile;for(unsigned i=0;i<actor_render_indices.size();++i)render_indices_[i]=actor_render_indices[i];ready_=true;return true;
    }
    bool step(const StartupDirectorObservations& input,std::vector<StartupEffect>* out,std::string* error){
        if(out)out->clear();
        if(!ready_||!out)return refuse(error,"not ready/destination");
        auto next=state_;std::vector<StartupEffect> effects;if(next.updates>=8192)return refuse(error,"update budget");
        bool yielded=false;
        for(unsigned budget=0;budget<256;++budget){
            if(next.cursor>=profile_.script.size())return refuse(error,"cursor extent");
            const u32 at=next.cursor;const auto* c=profile_.script.data()+at;const u8 op=c[0];bool advance=true,emit=true;
            StartupEffect event;event.opcode=op;event.command_offset=at;std::copy_n(c+1,kStartupOperandBytes[op],event.operands.begin());
            switch(op){
            case 0:next.phase=c[1];if(next.phase==12)next.phase12=true;break;
            case 2:advance=!input.fade_busy;emit=false;break;
            case 4:
                advance=false;emit=!next.latch;next.terminal=true;next.latch=true;event.kind=StartupEffectKind::Terminal;break;
            case 5:
                emit=false;if(!next.latch){next.latch=true;next.counter=startup_director_detail::signed_word(load_be16(c+1));advance=false;event.kind=StartupEffectKind::WaitBegin;event.value=f32(next.counter);emit=true;}
                else{const auto bits=u16(std::bit_cast<u16>(next.counter)-u16(1));next.counter=std::bit_cast<s16>(bits);advance=next.counter<=0;}
                break;
            case 6:next.camera_source=c[1];break;
            case 7:{
                next.camera_frame=profile_.camera_values[c[1]];next.camera_step=profile_.camera_values[c[3]];next.camera_active=-1;
                if(c[2]==255){const unsigned index=next.camera_source==255?32:next.camera_source;if(!input.camera_end_valid[index]||!startup_director_detail::finite(input.camera_end[index])||input.camera_end[index]<0||input.camera_end[index]>100000)return refuse(error,"unresolved camera end observation");next.camera_goal=input.camera_end[index];}
                else next.camera_goal=profile_.camera_values[c[2]];
                event.value=next.camera_frame;break;}
            case 8:advance=c[1]==255?next.camera_active==0:profile_.camera_values[c[1]]<=next.camera_frame;emit=false;break;
            case 9:next.actors_enabled=c[1]!=0;break;
            case 10:advance=!input.resources_busy;emit=false;break;
            case 12:next.camera_link=c[1]!=0;break;
            case 13:
                event.actor=c[1]==2?c[3]:c[2];if(c[1]==2)event.render_group=c[2]==1?u8(unsigned(render_indices_[event.actor])+0x41u):0;break;
            case 14:if(c[1]==0)next.mark=next.cursor;else{if(next.mark==0xffffffffu)return refuse(error,"unbound loop mark");next.cursor=next.mark;}break;
            case 15:next.config_enabled=c[1]!=0;if(next.config_enabled)next.config_goal=next.config_step=0;break;
            case 16:next.config_words=profile_.config_words;next.config_value=bits_to_f32(next.config_words[2]);break;
            case 17:next.config_goal=0;next.config_step=f32((next.config_goal-next.config_value)/f32(startup_director_detail::signed_word(load_be16(c+1))));break;
            default:break; //1/3/11/18/19/20/21 are explicit external commands
            }
            if(emit)effects.push_back(event);
            if(!advance){yielded=true;break;}
            next.cursor+=1+kStartupOperandBytes[op];next.latch=false;
        }
        if(!yielded)return refuse(error,"per-update command/loop budget");
        // No actor-done observation exists: source director never waits on it.
        if(next.actors_enabled){StartupEffect event;event.kind=StartupEffectKind::ActorFrame;event.command_offset=next.cursor;effects.push_back(event);}
        if(next.camera_active){
            if(next.camera_active<0)next.camera_active=1;
            else{next.camera_frame=f32(next.camera_frame+next.camera_step);if(next.camera_goal<=next.camera_frame){next.camera_frame=next.camera_goal;next.camera_active=0;}
                StartupEffect event;event.kind=StartupEffectKind::CameraSample;event.value=next.camera_frame;event.command_offset=next.cursor;effects.push_back(event);}
        }
        if(next.config_enabled&&next.config_step!=0){next.config_value=f32(next.config_value+next.config_step);if(next.config_value<=next.config_goal){next.config_value=next.config_goal;next.config_step=0;}
            StartupEffect event;event.kind=StartupEffectKind::ConfigSample;event.value=next.config_value;event.command_offset=next.cursor;effects.push_back(event);}
        if(!startup_director_detail::finite(next.camera_frame)||!startup_director_detail::finite(next.config_value)||!startup_director_detail::finite(next.config_step))return refuse(error,"nonfinite counter output");
        ++next.updates;StartupEffect end;end.kind=StartupEffectKind::FrameEnd;end.command_offset=next.cursor;effects.push_back(end);state_=next;*out=std::move(effects);return true;
    }
    bool ready()const{return ready_;}
    const StartupDirectorState& state()const{return state_;}
private:
    bool fail(std::string* error,const char* why){if(error)*error=std::string("startup director: ")+why;return false;}
    bool refuse(std::string* error,const char* why){ready_=false;return fail(error,why);}
    StartupDirectorProfile profile_;StartupDirectorState state_;std::array<u8,8> render_indices_{};bool ready_=false;
};
} // namespace gf
