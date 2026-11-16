#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/native_battle_mvp.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"native walk ledge: %s\n",why);std::abort();}}
struct Drop:gf::IBattleMvpTerrain{
    bool metadata=true,blocked=false;float top=80;
    bool height(float,float z,float,float* out)const override{*out=blocked?2000:z<0?top:0;return true;}
    bool static_surface(float,float z,float,gf::NativeBorgStaticSurface* out)const override{
        if(!metadata)return false;
        *out={z<0?top:0,{0,1,0},z<0?7u:8u};return true;
    }
};
}
int main(){
    using namespace gf;Drop terrain;NativeBattleMvp world;NativeBorgMotionObservation observation;
    std::shared_ptr<const NativeBorgMotionProducer> producer;NativeBorgMotionOffer offer;NativeBorgMotionReceipt receipt;u64 serial=0;
    auto setup=[&](NativeBorgMotionPolicy policy){
        BattleMvpConfig config;config.spawn={BattlePoint{0,-10},BattlePoint{-900,900}};
        check(world.start(config,&terrain),"fresh owned flat ledge");
        producer=world.new_motion_producer(policy);check(producer&&world.observe_motion(&observation),"fresh same-world producer");serial=0;
        offer={observation,observation.position,observation.facing,BattleMvpAction::Idle,++serial,producer};
        if(policy==NativeBorgMotionPolicy::WalkLedge){NativeBorgWalkLedge e;e.frame_start=observation.position;
            check(terrain.static_surface(0,-10,0,&e.surface),"initial selected surface");offer.walk_ledge=e;}
        check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation),"standing source-owned frame");
    };
    auto departure=[&](){
        NativeBorgWalkLedge e;e.frame_start=observation.position;e.speed44=12;
        check(terrain.static_surface(0,2,observation.position[1],&e.surface),"independent lower flat surface");
        offer={observation,{0,observation.position[1],2},observation.facing,BattleMvpAction::Jump,serial+1,producer};
        offer.kind=NativeBorgMotionKind::GroundDeparture;offer.walk_ledge=e;
    };
    auto accept=[&](){
        check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.motion_receipt(offer,&receipt)&&world.observe_motion(&observation),
            "matching owned ledge receipt");serial=offer.serial;
    };
    for(auto policy:{NativeBorgMotionPolicy::GroundOnly,NativeBorgMotionPolicy::Jump,NativeBorgMotionPolicy::WindupEntryPush}){
        setup(policy);departure();const auto tick=world.tick();
        check(!world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.tick()==tick,"older policies cannot borrow departure metadata or air authority");
    }
    setup(NativeBorgMotionPolicy::WalkLedge);departure();const auto valid=offer;const auto tick=world.tick();
    auto refuses=[&](NativeBorgMotionOffer candidate,const char* why){
        check(!world.step({},false,&terrain,nullptr,nullptr,&candidate)&&world.tick()==tick&&world.actors()[0].position==observation.position,why);
    };
    auto wrong=valid;wrong.walk_ledge.reset();refuses(wrong,"ledge kind requires a bound frame-start/geometry/timer payload");
    wrong=valid;wrong.kind=NativeBorgMotionKind::JumpTakeoff;refuses(wrong,"no invented takeoff");
    wrong=valid;wrong.proposed_position[1]-=1;refuses(wrong,"departure retains incoming ground-step height");
    wrong=valid;wrong.vertical_speed=-1;refuses(wrong,"departure is not first falling integration");
    wrong=valid;wrong.walk_ledge->frame_start[2]+=1;refuses(wrong,"frame snapshot is exact accepted start, not an arbitrary previous pose");
    wrong=valid;wrong.walk_ledge->timer698=30;refuses(wrong,"timer is not started before higher-priority correction");
    wrong=valid;wrong.walk_ledge->speed44=11;refuses(wrong,"ground scalar must be one of normal G-Red's qualified walk slots");
    wrong=valid;wrong.walk_ledge->surface.triangle++;refuses(wrong,"selected lower plane is independently checked");
    wrong=valid;wrong.walk_ledge->surface.normal={1,0,0};refuses(wrong,"nonflat/unknown ledges remain outside subset");
    terrain.metadata=false;refuses(valid,"height-only provider refuses");terrain.metadata=true;
    terrain.blocked=true;refuses(valid,"native terrain clipping cannot be acknowledged");terrain.blocked=false;
    offer=valid;accept();
    check(!world.actors()[0].grounded&&observation.position[1]==80&&observation.vertical_speed==0&&
        !world.step({},false,&terrain),"ground departure publishes physical air/no-takeoff and cannot use legacy fallback");
    const auto exit_position=observation.position;
    auto air=[&](bool entry,bool drift=false){
        const auto& previous=*offer.walk_ledge;
        NativeBorgWalkLedge e;e.frame_start=observation.position;e.travel5ae=previous.travel5ae;
        e.speed44=f32(previous.speed44*bits_to_f32(0x3f7d70a4u));e.timer698=entry?30.f:std::max(0.f,previous.timer698-1);
        if(drift)e.drift_heading=0;
        auto next=observation.position;f32 x=0,z=0;
        if(e.drift_heading){source_heading_xz(*e.drift_heading,&x,&z);next[0]=std::fma(4.f,x,next[0]);next[2]=std::fma(4.f,z,next[2]);}
        source_heading_xz(e.travel5ae,&x,&z);next[0]=std::fma(1.f,f32(e.speed44*x),next[0]);
        next[1]=std::fma(observation.vertical_speed,1.f,next[1]);next[2]=std::fma(1.f,f32(e.speed44*z),next[2]);
        check(terrain.static_surface(next[0],next[2],observation.position[1],&e.surface),"next selected flat surface");
        offer={observation,next,observation.facing,BattleMvpAction::Jump,serial+1,producer};
        offer.kind=entry?NativeBorgMotionKind::GroundLossEntry:NativeBorgMotionKind::GroundLossAir;
        offer.vertical_speed=std::max(-30.f,f32(observation.vertical_speed-1));offer.walk_ledge=e;
        if(next[1]<=e.surface.height){offer.kind=NativeBorgMotionKind::GroundLossTouchdown;offer.proposed_position[1]=e.surface.height;offer.vertical_speed=0;}
    };
    air(true);const auto first=offer;const auto before_entry=world.tick();
    wrong=first;wrong.kind=NativeBorgMotionKind::JumpAir;
    check(!world.step({},false,&terrain,nullptr,nullptr,&wrong)&&world.tick()==before_entry,"ordinary air cannot skip the explicit loss entry contract");
    wrong=first;wrong.walk_ledge->timer698=29;
    check(!world.step({},false,&terrain,nullptr,nullptr,&wrong)&&world.tick()==before_entry,"entry resets698 after the frame timer prefix");
    wrong=first;wrong.walk_ledge->frame_start=valid.observed.position;
    check(!world.step({},false,&terrain,nullptr,nullptr,&wrong),"last ground movement's starting position is not the new frame-start snapshot");
    wrong=first;wrong.walk_ledge->drift_heading=0;
    check(!world.step({},false,&terrain,nullptr,nullptr,&wrong),"positive timer suppresses direct camera drift");
    offer=first;accept();
    check(observation.position[1]==exit_position[1]&&observation.vertical_speed==-1&&!world.actors()[0].grounded,
        "first lower falling callback integrates old zero velocity before applying -1");
    for(unsigned i=0;i<120&&!world.actors()[0].grounded;++i){air(false);accept();}
    check(offer.kind==NativeBorgMotionKind::GroundLossTouchdown&&observation.position[1]==0&&world.actors()[0].grounded,
        "actual lower floor touchdown is separate from leaving and entry");
    NativeBorgWalkLedge landed=*offer.walk_ledge;landed.frame_start=observation.position;
    landed.timer698=std::max(0.f,landed.timer698-1);landed.speed44=0;landed.drift_heading.reset();
    offer={observation,observation.position,observation.facing,BattleMvpAction::Land,serial+1,producer};
    offer.kind=NativeBorgMotionKind::JumpLanding;offer.walk_ledge=landed;accept();
    terrain.top=1000;setup(NativeBorgMotionPolicy::WalkLedge);departure();accept();air(true);accept();
    unsigned drift_frames=0;
    for(unsigned i=0;i<120&&!world.actors()[0].grounded;++i){
        const auto timer=offer.walk_ledge->timer698;air(false,timer<=1);if(offer.walk_ledge->drift_heading)++drift_frames;accept();
    }
    check(world.actors()[0].grounded&&drift_frames>0,"bounded long drop permits direct4-unit drift only after698 reaches zero");
    std::printf("native flat walk-ledge/frame-history/timer/kinematics: %u checks PASS; native proxies only\n",checks);
}
