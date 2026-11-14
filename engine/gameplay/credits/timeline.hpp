#pragma once
#include <algorithm>
#include "engine/gameplay/credits/config.hpp"
namespace gf {
struct CreditsRowState {bool alive=false,holding=false;u32 hold_left=0;f32 y=0;};
// Portable operations from 801F1874/1954/1D24. Native callback ordering is
// explicit: producer, completion check, then actor updates in creation order.
class CreditsTimeline {
public:
    bool begin(const CreditsConfig& c){
        cancel();have_config_=false;if(!credits_config_valid(c))return false;
        config_=c;have_config_=true;states_.assign(c.rows.size(),{});next_=0;elapsed_=delay_=0;updates_=0;active_=1;
        running_=true;ended_=failed_=false;peak_=1;return true;
    }
    bool step(){
        if(!running_||!have_config_)return false;
        const auto& c=config_;
        if(++updates_>c.max_updates){fail();return false;}
        elapsed_+=c.speed;
        if(!ended_&&elapsed_>=delay_){
            if(next_>=c.rows.size()){fail();return false;}
            const auto& row=c.rows[next_];
            if(row.kind==9)ended_=true;
            else {states_[next_].alive=row.count!=0;states_[next_].y=row.spawn_y;active_+=row.count;delay_=f32(row.delay);}
            ++next_;elapsed_=0;
        }
        peak_=std::max(peak_,active_);
        if(active_>c.max_active){fail();return false;}
        if(active_==1){if(!ended_){fail();return false;}running_=false;active_=0;return true;}
        for(std::size_t i=0;i<states_.size();++i){auto& state=states_[i];if(!state.alive)continue;
            const auto& row=c.rows[i];bool remove=false;
            if(row.kind==5){
                if(state.holding){if(state.hold_left&&--state.hold_left==0)remove=true;}
                else{
                    if(state.y<c.hold_y)state.y+=c.speed; // source permits overshoot; no clamp
                    if(active_<=2){state.holding=true;state.hold_left=c.final_hold;}
                }
            }else{state.y+=c.speed;remove=state.y>=c.delete_y;}
            if(remove){state.alive=false;active_-=row.count;}
        }
        return true;
    }
    void cancel(){running_=false;active_=0;for(auto& state:states_)state.alive=false;}
    bool running()const{return running_;}bool ended()const{return ended_;}bool failed()const{return failed_;}
    u32 updates()const{return updates_;}u32 active()const{return active_;}u32 peak()const{return peak_;}
    std::size_t emitted()const{return next_;}const std::vector<CreditsRowState>& rows()const{return states_;}
private:
    void fail(){failed_=true;cancel();}
    CreditsConfig config_;bool have_config_=false;std::vector<CreditsRowState> states_;
    f32 elapsed_=0,delay_=0;u32 updates_=0,active_=0,peak_=0;std::size_t next_=0;
    bool running_=false,ended_=false,failed_=false;
};
inline bool credits_preflight(const CreditsConfig& c,u32* peak=nullptr,u32* updates=nullptr){
    CreditsTimeline timeline;if(!timeline.begin(c))return false;
    while(timeline.running())if(!timeline.step())return false;
    if(timeline.failed()||!timeline.ended()||timeline.emitted()!=c.rows.size())return false;
    if(peak)*peak=timeline.peak();
    if(updates)*updates=timeline.updates();
    return true;
}
} // namespace gf
