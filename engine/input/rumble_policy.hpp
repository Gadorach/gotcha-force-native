#pragma once
#include <array>
#include "engine/core/backends.hpp"
namespace gf {
// Native adaptation of the bounded 8010D450 on/stop/terminator preview sequence.
// Durations are native updates, not measured original wall-clock units.
class RumblePolicy {
public:
    explicit RumblePolicy(IHaptics& backend):backend_(backend){}
    ~RumblePolicy(){stop_all();}
    RumblePolicy(const RumblePolicy&)=delete;
    RumblePolicy& operator=(const RumblePolicy&)=delete;
    bool configure(u16 on_updates){if(!on_updates||on_updates>118)return false;on_updates_=on_updates;return true;}
    bool set_policy(u8 mask,const std::array<unsigned,4>& devices){
        if(mask>15)return false;
        for(unsigned device:devices)if(device>=4)return false;
        if(devices!=devices_){stop_all();devices_=devices;}
        mask_=mask;
        for(unsigned port=0;port<4;++port)if(!(mask&(1u<<port)))remaining_[port]=0;
        emit();return true;
    }
    bool preview(unsigned port){
        if(port>=4||!(mask_&(1u<<port))||!on_updates_)return false;
        remaining_[port]=on_updates_;return true;
    }
    void update(){
        emit();
        for(auto& remaining:remaining_)if(remaining)--remaining;
    }
    void stop_all(){
        remaining_={};
        for(unsigned device=0;device<4;++device)if(driven_[device]){backend_.stop(device);driven_[device]=false;}
    }
private:
    void emit(){
        std::array<bool,4> next{};
        for(unsigned port=0;port<4;++port)if((mask_&(1u<<port))&&remaining_[port])next[devices_[port]]=true;
        for(unsigned device=0;device<4;++device){
            if(next[device]&&backend_.supported(device)){backend_.rumble(device,1,1);driven_[device]=true;}
            else if(driven_[device]){backend_.stop(device);driven_[device]=false;}
        }
    }
    IHaptics& backend_;u8 mask_=0;u16 on_updates_=0;
    std::array<unsigned,4> devices_={0,1,2,3};std::array<u16,4> remaining_{};
    std::array<bool,4> driven_{};
};
} // namespace gf
