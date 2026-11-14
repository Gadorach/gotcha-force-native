#pragma once
#include <bit>
#include <cmath>
#include <vector>
#include "engine/assets/challenge_difficulty_art.hpp"
#include "engine/graphics/transform.hpp"
namespace gf {
struct ChallengeSetupDraw {
    u16 record=0,mask=0x24;Mat4 world;f32 joint_time=0,material_time=0,alpha=1;
    bool authored_world=false,translucent=false;
    //Explicit source-owned material animation: do not replace its alpha.
    bool authored_alpha=false;
    bool after_callbacks_alpha=false;
    bool always_depth=false;
};
//Shared 191BE4/C78 background, 191DE8/EB0 frames and 192258 footer.
//The ordinary entry preserves the qualified difficulty-only domain. Explicit
//all-pages entry adds the other frame masks and source background palette; it
//does not construct player/box widgets, acknowledge a controller or load a bank.
class ChallengeSetupChrome {
public:
    bool open(const ChallengeDifficultyArt& art,bool all_pages=false){
        *this={};if(!art.valid())return false;art_=art;all_pages_=all_pages;
        for(auto& frame:frames_)frame.alpha=1;
        for(auto& y:footer_y_)y=op(12);
        ready_=true;update_background(8);update_frames(8);if(!publish())return fail();return true;
    }
    bool step(u8 visible,u8 interactive){
        if(!ready_||updates_>=8192||((visible|interactive)&~(all_pages_?15u:8u)))return fail();
        auto next=*this;next.update_background(visible);next.update_frames(visible);
        for(unsigned i=0;i<10;++i){const f32 target=(interactive&art_.footers[i].mask)?op(10):op(12);next.footer_y_[i]=toward(footer_y_[i],target,op(8));}
        ++next.footer_tick_;++next.updates_;if(!next.publish())return fail();*this=std::move(next);return true;
    }
    const std::vector<ChallengeSetupDraw>& draws()const{return draws_;}
    bool ready()const{return ready_;}u32 updates()const{return updates_;}
    f32 background_time()const{return background_motion_;}f32 background_palette()const{return background_palette_;}
private:
    struct Frame {u16 tick=0;f32 alpha=1;};
    f32 op(unsigned index)const{return art_.operands[index];}
    static f32 toward(f32 current,f32 target,f32 factor){const f32 delta=f32(target-current);return f32(current+f32(delta*factor));}
    void update_background(u8 visible){
        if(all_pages_){
            const f32 target=(visible&8)?op(0):(visible&2)?op(2):(visible&1)?op(3):(visible&4)?op(1):op(4);
            if(target>=op(0)){
                if(background_palette_<target)background_palette_=std::min(target,f32(background_palette_+op(5)));
                if(background_palette_>target)background_palette_=std::max(target,f32(background_palette_-op(5)));
            }
        }
        background_motion_=f32(background_motion_+op(2));if(background_motion_>=op(6))background_motion_=f32(background_motion_-op(6));
    }
    void update_frames(u8 visible){
        for(unsigned i=all_pages_?0:3;i<4;++i){auto& frame=frames_[i];
            if(visible&(all_pages_?art_.frames[i].mask:8u)){++frame.tick;frame.alpha=std::min(op(2),f32(frame.alpha+op(5)));}
            else if(frame.tick>0){++frame.tick;frame.alpha=f32(frame.alpha-op(5));if(frame.alpha<=0){frame.tick=0;frame.alpha=1;}}
            while(frame.tick>=128)frame.tick-=64;
        }
    }
    bool publish(){
        draws_.clear();ChallengeSetupDraw background;background.record=98;background.mask=0x25;background.joint_time=background_motion_;background.material_time=background_palette_;background.authored_world=true;draws_.push_back(background);
        for(unsigned i=all_pages_?0:3;i<4;++i){const auto& state=frames_[i];if(!state.tick)continue;
            const auto& frame=art_.frames[i];const s16 angle=std::bit_cast<s16>(u16(u32(state.tick)<<10));const f32 sine=std::sin(f32(angle)*bits_to_f32(0x38c90fdb));
            const f32 scale[3]={f32(1.f+sine*(state.tick<16?op(7):op(8))),f32(1.f+sine*(state.tick<16?op(4):op(9))),1};
            ChallengeSetupDraw pose;pose.record=frame.record;pose.alpha=state.alpha;pose.world=mat4_multiply(mat4_translation(frame.position.data()),mat4_scale(scale));draws_.push_back(pose);
        }
        for(unsigned group=0;group<10;++group)for(unsigned component=0;component<4;++component){const auto& footer=art_.footers[group];s16 record=footer.records[component];if(record<0)continue;
            if(component==2&&(footer_tick_&16))++record;
            std::array<f32,3> point{footer.x,footer_y_[group],op(11)};f32 size=op(13);
            if(component==1){point[0]=f32(point[0]+op(14));size=op(15);}
            const f32 scale[3]={size,size,1};ChallengeSetupDraw pose;pose.record=u16(record);pose.world=mat4_multiply(mat4_translation(point.data()),mat4_scale(scale));draws_.push_back(pose);
        }
        for(const auto& pose:draws_){if(pose.record>=4096||!std::isfinite(pose.alpha)||pose.alpha<0||pose.alpha>1)return false;for(f32 v:pose.world.m)if(!std::isfinite(v)||std::abs(v)>1e7f)return false;}
        return draws_.size()<=80;
    }
    bool fail(){ready_=false;draws_.clear();return false;}
    ChallengeDifficultyArt art_;std::array<Frame,4> frames_;std::array<f32,10> footer_y_{};std::vector<ChallengeSetupDraw> draws_;
    f32 background_motion_=0,background_palette_=0;u16 footer_tick_=0;u32 updates_=0;bool ready_=false,all_pages_=false;
};
} // namespace gf
