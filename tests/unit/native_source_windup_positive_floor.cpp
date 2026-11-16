#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/native_battle_mvp.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"positive wind-up floor: %s\n",why);std::abort();}}
struct Flat:gf::IBattleMvpTerrain{
    bool metadata=true,blocked=false;float level=0;
    bool height(float,float,float,float* out)const override{*out=blocked?100:level;return true;}
    bool static_surface(float,float,float,gf::NativeBorgStaticSurface* out)const override{
        if(!metadata)return false;
        *out={level,{0,1,0},7};return true;
    }
};
}
int main(){
    using namespace gf;Flat terrain;NativeBattleMvp world;NativeBorgMotionObservation observation;
    NativeBorgMotionOffer offer;NativeBorgMotionReceipt receipt;std::shared_ptr<const NativeBorgMotionProducer> producer;u64 serial=0;
    auto step=[&](NativeBorgMotionKind kind,f32 y,f32 speed){
        offer={observation,observation.position,observation.facing,BattleMvpAction::Jump,++serial,producer};
        offer.kind=kind;offer.proposed_position[1]=y;offer.vertical_speed=speed;
        check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation),"ordered source proxy candidate");
    };
    auto setup=[&](NativeBorgMotionPolicy policy,bool positive=true){
        check(world.start({},&terrain),"fresh world");serial=0;
        producer=world.new_motion_producer(policy);check(producer&&world.observe_motion(&observation),"same-world producer");
        step(NativeBorgMotionKind::JumpTakeoff,0,0);step(NativeBorgMotionKind::JumpAir,10,10);
        step(NativeBorgMotionKind::JumpAir,20,positive?-19.875f:-10.f);
        step(NativeBorgMotionKind::JumpAir,positive?.125f:10.f,positive?-15.125f:-30.f);
        NativeBorgRelaunchFloor e;e.acceleration=bits_to_f32(0x3f4ccccdu);e.integrated_position=observation.position;
        const auto speed=f32(observation.vertical_speed+15);
        e.integrated_position[1]=f32(observation.position[1]+speed);e.vertical_speed=std::clamp(f32(speed+e.acceleration),-30.f,18.f);
        e.response_factor=e.vertical_speed<0?e.acceleration:1.f;check(terrain.static_surface(0,0,0,&e.surface),"prior selected floor");
        offer={observation,{observation.position[0],0,observation.position[2]},observation.facing,BattleMvpAction::Jump,++serial,producer};
        offer.kind=NativeBorgMotionKind::RelaunchEntryFloor;offer.entry_vertical_delta=15;offer.relaunch_floor=e;offer.vertical_speed=f32(e.vertical_speed*e.response_factor);
        check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation)&&!world.actors()[0].grounded,
            "prior null-callback relaunch snap preserves airborne history");
    };
    const auto acceleration=bits_to_f32(0xbd8f5c29u);
    auto entry=[&](){
        NativeBorgAirDashFloor e;e.integrated_position=observation.position;e.vertical_speed=f32(observation.vertical_speed*.5f);e.acceleration=acceleration;
        check(terrain.static_surface(0,0,0,&e.surface),"independent entry floor");
        offer={observation,observation.position,observation.facing,BattleMvpAction::AirDash,serial+1,producer};
        offer.kind=NativeBorgMotionKind::AirDashWindupPositiveEntryFloor;offer.air_dash_floor=e;offer.vertical_speed=0;
    };
    for(auto policy:{NativeBorgMotionPolicy::RelaunchFloor,NativeBorgMotionPolicy::AirDashFloor,NativeBorgMotionPolicy::WindupEntryFloor}){
        setup(policy);entry();const auto tick=world.tick();
        check(observation.vertical_speed>0&&!world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.tick()==tick,
            "all older floor policies refuse positive entry even with exact matching evidence");
    }
    setup(NativeBorgMotionPolicy::WindupPositiveFloor);entry();const auto valid=offer;const auto tick=world.tick();
    auto refuses=[&](NativeBorgMotionOffer candidate,const char* why){
        check(!world.step({},false,&terrain,nullptr,nullptr,&candidate)&&world.tick()==tick&&world.actors()[0].position==observation.position,why);
    };
    auto invalid=valid;invalid.kind=NativeBorgMotionKind::AirDashWindupEntryFloor;refuses(invalid,"nonascending entry contract cannot borrow positive authority");
    invalid=valid;invalid.kind=NativeBorgMotionKind::AirDashWindupEntry;invalid.air_dash_floor.reset();refuses(invalid,"ordinary entry still requires height above floor");
    invalid=valid;invalid.air_dash_floor.reset();refuses(invalid,"positive entry requires its exact floor payload");
    invalid=valid;invalid.air_dash_floor->vertical_speed+=acceleration;refuses(invalid,"future50 is not applied during entry");
    invalid=valid;invalid.air_dash_floor->vertical_speed=0;refuses(invalid,"entry pre-response half-speed is positive, not already zero");
    invalid=valid;invalid.air_dash_floor->vertical_speed=observation.vertical_speed;refuses(invalid,"entry must halve old positive speed");
    invalid=valid;invalid.vertical_speed=valid.air_dash_floor->vertical_speed;refuses(invalid,"ret1 zeros positive48 too, without lifting off");
    invalid=valid;invalid.air_dash_floor->integrated_position[1]+=valid.air_dash_floor->vertical_speed;
    refuses(invalid,"positive half-speed is not upward displacement");
    invalid=valid;invalid.proposed_position[0]+=1;invalid.air_dash_floor->integrated_position[0]+=1;
    refuses(invalid,"entry cannot translate horizontally even with matching payload coordinates");
    invalid=valid;invalid.air_dash_floor->acceleration=0;refuses(invalid,"entry retains50 rather than fabricating65E64");
    invalid=valid;invalid.air_dash_floor->surface.triangle++;refuses(invalid,"selected surface is independently verified");
    invalid=valid;invalid.relaunch_floor=NativeBorgRelaunchFloor{};refuses(invalid,"relaunch evidence cannot accompany positive entry");
    invalid=valid;invalid.entry_vertical_delta=15;refuses(invalid,"positive entry has no additive relaunch step");
    terrain.metadata=false;refuses(valid,"height-only provider refuses");terrain.metadata=true;
    terrain.blocked=true;refuses(valid,"native clipping cannot be certified by source floor evidence");terrain.blocked=false;
    terrain.level=.001f;invalid=valid;invalid.proposed_position[1]=terrain.level;invalid.air_dash_floor->surface.height=terrain.level;
    refuses(invalid,"positive entry cannot borrow even a tiny raised-surface height adjustment");terrain.level=0;
    offer=valid;
    check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.motion_receipt(offer,&receipt)&&world.observe_motion(&observation),
        "exact positive-entry receipt");
    serial=offer.serial;
    check(!world.actors()[0].grounded&&observation.vertical_speed==0&&offer.proposed_position==offer.observed.position,
        "positive half-speed then zero48 remains airborne at unchanged floor position");
    invalid=offer;invalid.kind=NativeBorgMotionKind::AirDashWindupEntryFloor;
    check(!world.motion_receipt(invalid,&receipt),"old entry kind cannot borrow positive receipt");
    entry();const auto zero_tick=world.tick();
    check(!world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.tick()==zero_tick,"zero-speed entry cannot masquerade as positive");
    NativeBorgAirDashFloor e;e.integrated_position=observation.position;e.vertical_speed=acceleration;e.acceleration=acceleration;
    check(terrain.static_surface(0,0,0,&e.surface),"continued wind-up surface");
    offer={observation,observation.position,observation.facing,BattleMvpAction::AirDash,serial+1,producer};
    offer.kind=NativeBorgMotionKind::AirDashWindupFloor;offer.air_dash_floor=e;
    check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&!world.actors()[0].grounded,
        "ordinary continued wind-up follows positive entry without replaying it");
    setup(NativeBorgMotionPolicy::WindupPositiveFloor,false);entry();const auto negative_tick=world.tick();
    check(observation.vertical_speed<0&&!world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.tick()==negative_tick,
        "negative speed cannot borrow positive entry contract");
    offer.kind=NativeBorgMotionKind::AirDashWindupEntryFloor;
    check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&!world.actors()[0].grounded,
        "new policy inherits the unchanged nonascending entry contract separately");
    std::printf("native positive wind-up entry floor/half-speed/no-motion: %u checks PASS; native proxies only\n",checks);
}
