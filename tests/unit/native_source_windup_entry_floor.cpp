#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/native_battle_mvp.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"wind-up entry floor: %s\n",why);std::abort();}}
struct Flat:gf::IBattleMvpTerrain{
    bool metadata=true,blocked=false;
    bool height(float,float,float,float* out)const override{*out=blocked?100:0;return true;}
    bool static_surface(float,float,float,gf::NativeBorgStaticSurface* out)const override{
        if(!metadata)return false;
        *out={0,{0,1,0},7};return true;
    }
};
}
int main(){
    using namespace gf;Flat terrain;NativeBattleMvp world;NativeBorgMotionObservation observation;
    NativeBorgMotionOffer offer;NativeBorgMotionReceipt receipt;std::shared_ptr<const NativeBorgMotionProducer> producer;u64 serial=0;
    auto step=[&](NativeBorgMotionKind kind,f32 y,f32 speed){
        offer={observation,observation.position,observation.facing,native_borg_motion_air_dash(kind)?BattleMvpAction::AirDash:BattleMvpAction::Jump,++serial,producer};
        offer.kind=kind;offer.proposed_position[1]=y;offer.vertical_speed=speed;
        check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation),"ordered native candidate");
    };
    auto setup=[&](NativeBorgMotionPolicy policy,bool positive=false){
        check(world.start({},&terrain),"fresh world");serial=0;
        producer=world.new_motion_producer(policy);check(producer&&world.observe_motion(&observation),"fresh same-world producer");
        step(NativeBorgMotionKind::JumpTakeoff,0,0);step(NativeBorgMotionKind::JumpAir,10,10);
        step(NativeBorgMotionKind::JumpAir,20,positive?-19.875f:-10.f);
        step(NativeBorgMotionKind::JumpAir,positive?.125f:10.f,positive?-15.125f:-30.f);
        NativeBorgRelaunchFloor e;e.acceleration=bits_to_f32(0x3f4ccccdu);e.integrated_position=observation.position;
        const auto speed=f32(observation.vertical_speed+15);
        e.integrated_position[1]=f32(observation.position[1]+speed);e.vertical_speed=std::clamp(f32(speed+e.acceleration),-30.f,18.f);
        e.response_factor=e.vertical_speed<0?e.acceleration:1.f;check(terrain.static_surface(0,0,0,&e.surface),"paired floor observation");
        offer={observation,{observation.position[0],0,observation.position[2]},observation.facing,BattleMvpAction::Jump,++serial,producer};
        offer.kind=NativeBorgMotionKind::RelaunchEntryFloor;offer.entry_vertical_delta=15;offer.relaunch_floor=e;offer.vertical_speed=f32(e.vertical_speed*e.response_factor);
        check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation)&&!world.actors()[0].grounded,
            "prior relaunch floor snap, not a grounded landing");
    };
    const auto acceleration=bits_to_f32(0xbd8f5c29u);
    auto entry=[&](){
        NativeBorgAirDashFloor e;e.integrated_position=observation.position;e.vertical_speed=f32(observation.vertical_speed*.5f);e.acceleration=acceleration;
        check(terrain.static_surface(0,0,0,&e.surface),"independent entry surface");
        offer={observation,observation.position,observation.facing,BattleMvpAction::AirDash,serial+1,producer};
        offer.kind=NativeBorgMotionKind::AirDashWindupEntryFloor;offer.air_dash_floor=e;offer.vertical_speed=0;
    };
    auto accept=[&](){
        check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.motion_receipt(offer,&receipt)&&world.observe_motion(&observation),
            "exact wind-up floor receipt");serial=offer.serial;
    };
    for(auto policy:{NativeBorgMotionPolicy::RelaunchFloor,NativeBorgMotionPolicy::AirDashFloor}){
        setup(policy);entry();const auto tick=world.tick();
        check(!world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.tick()==tick,
            "both older floor policies refuse entry-floor authority even with valid matching evidence");
    }
    setup(NativeBorgMotionPolicy::WindupEntryFloor);entry();const auto valid=offer;const auto tick=world.tick();
    auto refuses=[&](NativeBorgMotionOffer candidate,const char* why){
        check(!world.step({},false,&terrain,nullptr,nullptr,&candidate)&&world.tick()==tick&&world.actors()[0].position==observation.position,why);
    };
    auto invalid=valid;invalid.air_dash_floor.reset();refuses(invalid,"entry-floor kind requires its exact floor payload");
    invalid=valid;invalid.kind=NativeBorgMotionKind::AirDashWindupEntry;refuses(invalid,"ordinary entry cannot borrow a floor payload");
    invalid.air_dash_floor.reset();refuses(invalid,"ordinary entry still requires height above floor");
    invalid=valid;invalid.kind=NativeBorgMotionKind::AirDashWindupFloor;refuses(invalid,"entry cannot masquerade as continued wind-up");
    invalid=valid;invalid.air_dash_floor->vertical_speed+=acceleration;refuses(invalid,"entry does not apply future50 acceleration before contact");
    invalid=valid;invalid.air_dash_floor->vertical_speed=observation.vertical_speed;refuses(invalid,"entry must halve old vertical speed");
    invalid=valid;invalid.air_dash_floor->vertical_speed=f32(observation.vertical_speed*.8f);refuses(invalid,"relaunch factor cannot replace entry half-speed");
    invalid=valid;invalid.vertical_speed=valid.air_dash_floor->vertical_speed;refuses(invalid,"wind-up ret1 must zero48 after the entry half-speed");
    invalid=valid;invalid.air_dash_floor->integrated_position[1]+=valid.air_dash_floor->vertical_speed;
    refuses(invalid,"entry does not translate by half-speed");
    invalid=valid;invalid.proposed_position[0]+=1;invalid.air_dash_floor->integrated_position[0]+=1;
    refuses(invalid,"entry cannot translate horizontally even with internally paired coordinates");
    invalid=valid;invalid.air_dash_floor->acceleration=0;refuses(invalid,"entry preserves data70 rather than claiming65E64 reset50");
    invalid=valid;invalid.air_dash_floor->surface.triangle++;refuses(invalid,"entry surface independently matches terrain");
    invalid=valid;invalid.relaunch_floor=NativeBorgRelaunchFloor{};refuses(invalid,"entry cannot borrow relaunch floor evidence");
    invalid=valid;invalid.entry_vertical_delta=15;refuses(invalid,"entry cannot claim relaunch's additive speed");
    terrain.metadata=false;refuses(valid,"height-only providers cannot authorize entry snaps");terrain.metadata=true;
    terrain.blocked=true;refuses(valid,"native clipping cannot be acknowledged by an entry snap");terrain.blocked=false;
    offer=valid;accept();
    check(!world.actors()[0].grounded&&world.actors()[0].vertical_speed==0&&offer.proposed_position==offer.observed.position,
        "zero-speed entry at unchanged floor position stays airborne");
    invalid=offer;invalid.air_dash_floor->vertical_speed+=.001f;
    check(!world.motion_receipt(invalid,&receipt),"edited entry half-speed cannot borrow receipt");
    for(unsigned i=0;i<2;++i){
        NativeBorgAirDashFloor e;e.integrated_position=observation.position;e.vertical_speed=acceleration;e.acceleration=acceleration;
        check(terrain.static_surface(0,0,0,&e.surface),"continued surface");
        offer={observation,observation.position,observation.facing,BattleMvpAction::AirDash,serial+1,producer};
        offer.kind=NativeBorgMotionKind::AirDashWindupFloor;offer.air_dash_floor=e;offer.vertical_speed=0;accept();
    }
    check(!world.actors()[0].grounded&&observation.vertical_speed==0,
        "subsequent wind-up floor updates apply -.07 and zero48, without re-entering half-speed path");
    setup(NativeBorgMotionPolicy::WindupEntryFloor,true);entry();const auto positive_tick=world.tick();
    check(observation.vertical_speed>0&&!world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.tick()==positive_tick,
        "positive-speed source entry remains outside the reviewed native admission subset");
    std::printf("native wind-up entry floor/half-speed/no-motion/receipt: %u checks PASS; native proxies only\n",checks);
}
