#pragma once
#include <limits>
#include <memory>
#include "engine/assets/source_main_digital_input.hpp"
#include "engine/input/input_router.hpp"
#include "engine/gameplay/source_setup_input.hpp"
namespace gf {
struct SourceMainDigitalPort {
    u32 held=0,previous=0,pressed=0,released=0;
    bool operator==(const SourceMainDigitalPort&)const=default;
};
struct SourceMainDigitalInputState {
    std::array<SourceMainDigitalPort,4> ports{};
    u64 sample=0;u8 known_ports=0; //edge history known in BOTH snapshots
    bool operator==(const SourceMainDigitalInputState&)const=default;
};
namespace source_main_digital_detail {
struct Scope {
    InputRouter router;SourceMainDigitalInputState state;
    u8 held_known_ports=0;bool live=true,published=false;
};
inline bool snapshot_valid(const InputSnapshot& input){return input.port_count<=InputSnapshot::kMaxPorts&&input.touch_count<=InputSnapshot::kMaxTouches;}
inline u8 route(const InputRouter& router,const InputSnapshot& input,std::array<u32,4>* held){
    u8 known=0;
    for(unsigned port=0;port<4;++port){
        //Never turn InputSnapshot::pad's unreported-device fallback into a
        //known released controller. Reuse the shared router and digital policy.
        if(router.port(port).device_pad>=input.port_count)continue;
        known|=u8(1u<<port);
        (*held)[port]=source_setup_digital_pad(router.route(input,port),true).source_buttons&0x1f70u;
    }
    return known;
}
}
class SourceMainDigitalInputOwner;
class SourceMainDigitalInputReceipt {
public:
    bool current()const{const auto s=scope_.lock();return s&&s->live&&s->published&&s->state.sample==sample_;}
    u64 sample()const{return current()?sample_:0;}
    bool covers(u8 ports,u32 mask)const{
        const auto s=scope_.lock();
        return current()&&ports&&!(ports&0xf0u)&&mask&&!(mask&~0x1f70u)&&(s->state.known_ports&ports)==ports;
    }
    bool masked_pressed(u8 ports,u32 mask,u32* out)const{
        if(out)*out=0;
        if(!out||!covers(ports,mask))return false;
        const auto s=scope_.lock();u32 value=0;
        for(unsigned port=0;port<4;++port)if(ports&(1u<<port))value|=s->state.ports[port].pressed;
        *out=value&mask;return true;
    }
    bool same_sample(const SourceMainDigitalInputReceipt& other)const{return current()&&other.current()&&sample_==other.sample_&&scope_.lock()==other.scope_.lock();}
private:
    friend class SourceMainDigitalInputOwner;
    std::weak_ptr<const source_main_digital_detail::Scope> scope_;u64 sample_=0;
};
//Native digital subset of8B650, fed by the EXISTING input router. Explicit
//previous/current snapshots are mandatory; no default zero history, device
//polling, analogue calibration, repeat counter or source-frame ACK is invented.
class SourceMainDigitalInputOwner {
public:
    SourceMainDigitalInputOwner()=default;
    SourceMainDigitalInputOwner(const SourceMainDigitalInputOwner&)=delete;
    SourceMainDigitalInputOwner& operator=(const SourceMainDigitalInputOwner&)=delete;
    SourceMainDigitalInputOwner(SourceMainDigitalInputOwner&&)=default;
    SourceMainDigitalInputOwner& operator=(SourceMainDigitalInputOwner&&)=default;
    ~SourceMainDigitalInputOwner(){retire();}
    bool begin_explicit_history(const SourceMainDigitalInputProfile& profile,const InputRouter& router,const InputSnapshot& previous,u64 serial,std::string* error){
        auto reject=[&](){if(error)*error="MAIN digital input: fresh supported profile/router/explicit history required";return false;};
        if(attempted_||scope_||!profile.ready()||!source_main_digital_detail::snapshot_valid(previous)||router.touch_layout().button_count>TouchLayout::kMaxButtons)return reject();
        for(unsigned port=0;port<4;++port)if(router.port(port).device_pad>=InputSnapshot::kMaxPorts)return reject();
        attempted_=true;auto next=std::make_shared<source_main_digital_detail::Scope>();next->router=router;next->state.sample=serial;
        std::array<u32,4> held{};next->held_known_ports=source_main_digital_detail::route(router,previous,&held);
        for(unsigned port=0;port<4;++port)next->state.ports[port].held=held[port];
        scope_=std::move(next);if(error){error->clear();}return true;
    }
    bool publish(const InputSnapshot& current,u64 serial,std::string* error){
        if(!scope_||!scope_->live||!source_main_digital_detail::snapshot_valid(current)||scope_->state.sample==std::numeric_limits<u64>::max()||serial!=scope_->state.sample+1){if(error)*error="MAIN digital input: current owner/next explicit sample required";return false;}
        std::array<u32,4> held{};const u8 known=source_main_digital_detail::route(scope_->router,current,&held);
        SourceMainDigitalInputState next;next.sample=serial;next.known_ports=known&scope_->held_known_ports;
        for(unsigned port=0;port<4;++port){
            auto& p=next.ports[port];p.held=held[port];
            if(next.known_ports&(1u<<port)){p.previous=scope_->state.ports[port].held;p.pressed=p.held&~p.previous;p.released=p.previous&~p.held;}
        }
        scope_->state=next;scope_->held_known_ports=known;scope_->published=true;
        if(error){error->clear();}return true;
    }
    const SourceMainDigitalInputState* state()const{return scope_&&scope_->live?&scope_->state:nullptr;}
    SourceMainDigitalInputReceipt receipt()const{SourceMainDigitalInputReceipt r;if(scope_&&scope_->live&&scope_->published){r.scope_=scope_;r.sample_=scope_->state.sample;}return r;}
    void retire(){if(scope_)scope_->live=false;scope_.reset();attempted_=true;}
private:
    std::shared_ptr<source_main_digital_detail::Scope> scope_;bool attempted_=false;
};
}
