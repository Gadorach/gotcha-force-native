#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/native_battle_mvp.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"native relaunch floor: %s\n",why);std::abort();}}
struct Flat:gf::IBattleMvpTerrain{
    bool metadata=true;
    bool height(float,float,float,float* out)const override{*out=0;return true;}
    bool static_surface(float,float,float,gf::NativeBorgStaticSurface* out)const override{
        if(!metadata)return false;
        *out={0,{0,1,0},7};return true;
    }
};
}
int main(){
    using namespace gf;Flat terrain;NativeBattleMvp world;NativeBorgMotionObservation observation;NativeBorgMotionOffer offer;NativeBorgMotionReceipt receipt;
    std::shared_ptr<const NativeBorgMotionProducer> producer;u64 serial=0;
    auto step=[&](NativeBorgMotionKind kind,f32 y,f32 speed,f32 delta=0){
        offer={observation,observation.position,observation.facing,BattleMvpAction::Jump,++serial,producer};
        offer.kind=kind;offer.proposed_position[1]=y;offer.vertical_speed=speed;offer.entry_vertical_delta=delta;
        check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation),"accepted native phase candidate");
    };
    auto setup=[&](NativeBorgMotionPolicy policy,unsigned stage=0){
        check(world.start({},&terrain),"fresh native flat floor");serial=0;
        producer=world.new_motion_producer(policy);check(producer&&world.observe_motion(&observation),"fresh same-world producer");
        step(NativeBorgMotionKind::JumpTakeoff,0,0);step(NativeBorgMotionKind::JumpAir,10,10);
        step(NativeBorgMotionKind::JumpAir,20,stage?-20.f:-10.f);
        if(!stage)step(NativeBorgMotionKind::JumpAir,10,-30);
        else{
            step(NativeBorgMotionKind::RelaunchEntry,15,stage==1?-20.f:-5.f,15);
            if(stage==2){step(NativeBorgMotionKind::RelaunchWindup,10,-5);step(NativeBorgMotionKind::RelaunchActive,5,-20);}
        }
    };
    const f32 acceleration=bits_to_f32(0x3f4ccccdu);
    auto contact=[&](NativeBorgMotionKind kind){
        const bool entry=kind==NativeBorgMotionKind::RelaunchEntryFloor;
        const f32 speed=entry?f32(observation.vertical_speed+15):observation.vertical_speed;
        NativeBorgRelaunchFloor e;e.integrated_position=observation.position;e.integrated_position[1]=f32(observation.position[1]+speed);
        e.acceleration=acceleration;e.vertical_speed=std::clamp(f32(speed+acceleration),-30.f,18.f);
        e.response_factor=e.vertical_speed<0?acceleration:1.f;
        check(terrain.static_surface(0,0,0,&e.surface),"independent selected floor");
        offer={observation,{observation.position[0],0,observation.position[2]},observation.facing,BattleMvpAction::Jump,serial+1,producer};
        offer.kind=kind;offer.entry_vertical_delta=entry?15.f:0;offer.relaunch_floor=e;offer.vertical_speed=f32(e.vertical_speed*e.response_factor);
    };
    for(auto policy:{NativeBorgMotionPolicy::Relaunch,NativeBorgMotionPolicy::StaticPush,NativeBorgMotionPolicy::FallHistoryPush,
                     NativeBorgMotionPolicy::RecoveryPush,NativeBorgMotionPolicy::WindupPush,NativeBorgMotionPolicy::ActivePush,
                     NativeBorgMotionPolicy::RelaunchPush,NativeBorgMotionPolicy::RelaunchEntryPush}){
        setup(policy);contact(NativeBorgMotionKind::RelaunchEntryFloor);const auto tick=world.tick();
        check(!world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.tick()==tick,"all older policies refuse null-callback floor contract");
    }
    setup(NativeBorgMotionPolicy::RelaunchFloor);contact(NativeBorgMotionKind::RelaunchEntryFloor);const auto valid=offer;const auto tick=world.tick();
    auto refuses=[&](const NativeBorgMotionOffer& invalid,const char* why){
        check(!world.step({},false,&terrain,nullptr,nullptr,&invalid)&&world.tick()==tick&&world.actors()[0].position==observation.position,why);
    };
    auto invalid=valid;invalid.relaunch_floor.reset();refuses(invalid,"floor kind needs separate evidence");
    invalid=valid;invalid.kind=NativeBorgMotionKind::RelaunchEntry;refuses(invalid,"ordinary entry cannot borrow floor evidence");
    invalid.relaunch_floor.reset();refuses(invalid,"ordinary entry retains strict above-floor check");
    invalid=valid;invalid.kind=NativeBorgMotionKind::JumpTouchdown;invalid.entry_vertical_delta=0;invalid.vertical_speed=0;
    refuses(invalid,"floor evidence cannot fabricate a landing receipt");
    invalid=valid;invalid.entry_vertical_delta=14;refuses(invalid,"entry snap requires exact15");
    invalid=valid;invalid.relaunch_floor->acceleration=-.07f;refuses(invalid,"dash acceleration cannot replace relaunch");
    invalid=valid;invalid.relaunch_floor->response_factor=.9f;refuses(invalid,"steep factor cannot replace negative-speed floor response");
    invalid=valid;invalid.vertical_speed=0;refuses(invalid,"null callback cannot zero vertical speed like65E64");
    invalid=valid;invalid.relaunch_floor->surface.triangle++;refuses(invalid,"spoofed selected floor refuses");
    invalid=valid;invalid.relaunch_floor->integrated_position[1]+=.001f;refuses(invalid,"pre-snap integration is exact");
    invalid=valid;invalid.proposed_position[0]+=1;refuses(invalid,"snap cannot alter planar integration");
    invalid=valid;invalid.proposed_position[1]+=.001f;refuses(invalid,"snap height is exact selected height");
    terrain.metadata=false;refuses(valid,"height-only provider cannot certify floor response");terrain.metadata=true;
    check(world.step({},false,&terrain,nullptr,nullptr,&valid)&&world.motion_receipt(valid,&receipt)&&
        !world.actors()[0].grounded&&world.actors()[0].position[1]==0&&world.actors()[0].vertical_speed<0&&
        world.actors()[0].action==BattleMvpAction::Jump,"null-callback snap stays ungrounded with damped negative speed, not landing");
    invalid=valid;invalid.relaunch_floor->response_factor=1;
    check(!world.motion_receipt(invalid,&receipt),"edited response cannot borrow exact floor receipt");
    check(world.observe_motion(&observation),"current floor observation");serial=valid.serial;
    for(unsigned n=0;n<32&&world.actors()[0].vertical_speed<=0;++n){
        contact(NativeBorgMotionKind::RelaunchWindupFloor);
        check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation)&&!world.actors()[0].grounded,
            "repeated null-callback snaps keep airborne source history as positive acceleration recovers speed");
        serial=offer.serial;
    }
    check(observation.vertical_speed>0,"nonnegative pre-speed is not damped to zero; relaunch can lift off again");
    step(NativeBorgMotionKind::RelaunchWindup,f32(observation.position[1]+observation.vertical_speed),f32(observation.vertical_speed+acceleration));
    for(unsigned stage:{1u,2u}){
        setup(NativeBorgMotionPolicy::RelaunchFloor,stage);contact(stage==1?NativeBorgMotionKind::RelaunchWindupFloor:NativeBorgMotionKind::RelaunchActiveFloor);
        check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&!world.actors()[0].grounded,"wind-up/active floor contracts preserve accepted relaunch stages");
    }
    setup(NativeBorgMotionPolicy::RelaunchFloor);contact(NativeBorgMotionKind::RelaunchEntryFloor);
    check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation),"floor contact before actual falling touchdown");
    serial=offer.serial;step(NativeBorgMotionKind::JumpTouchdown,0,0);
    check(world.actors()[0].grounded,"only separate touchdown phase makes the native actor grounded");
    std::printf("native null-callback relaunch floor/phase/receipt: %u checks PASS; proxy candidates, no original whole-frame equivalence\n",checks);
}
