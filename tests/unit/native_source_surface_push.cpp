#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/native_battle_mvp.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"native static push: %s\n",why);std::abort();}}
struct Plane:gf::IBattleMvpTerrain {
    bool metadata=true;float adjacent=0; //zero: continuous plane; otherwise a native seam
    bool height(float x,float,float,float* out)const override{*out=x>50?0:x>1&&adjacent?adjacent:-3*x;return true;}
    bool static_surface(float x,float,float,gf::NativeBorgStaticSurface* out)const override{
        if(!metadata)return false;
        const float length=std::sqrt(10.f);*out={-3*x,{3/length,1/length,0},7};return true;
    }
};
}
int main(){
    using namespace gf;Plane terrain;NativeBattleMvp world;NativeBorgMotionObservation observation;
    std::shared_ptr<const NativeBorgMotionProducer> producer;NativeBorgMotionOffer offer;NativeBorgMotionReceipt receipt;
    auto make_push=[&](u64 serial){
        NativeBorgStaticPush push;push.integrated_position=observation.position;push.integrated_position[1]=f32(observation.position[1]+observation.vertical_speed);
        push.vertical_speed=std::max(-30.f,f32(observation.vertical_speed-1.f));push.damping=bits_to_f32(0x3f666666u);
        check(terrain.static_surface(push.integrated_position[0],push.integrated_position[2],observation.position[1],&push.surface)&&
            source_surface_angle(push.surface.normal,&push.angle),"independent plane fixture");
        offer={observation,{},observation.facing,BattleMvpAction::Jump,serial,producer};offer.kind=NativeBorgMotionKind::StaticPush;
        offer.static_push=push;offer.vertical_speed=f32(push.vertical_speed*push.damping);
        check(source_surface_push_position(push.integrated_position,push.surface.normal,push.surface.height,push.angle,&offer.proposed_position),"source numeric projection");
    };
    auto setup=[&](NativeBorgMotionPolicy policy,bool nearby=false){
        BattleMvpConfig config;config.spawn={BattlePoint{0,0},nearby?BattlePoint{85,0}:BattlePoint{400,400}};
        check(world.start(config,&terrain),"native plane world");
        producer=world.new_motion_producer(policy);check(producer&&world.observe_motion(&observation),"fresh source producer");
        offer={observation,observation.position,observation.facing,BattleMvpAction::Jump,1,producer};offer.kind=NativeBorgMotionKind::JumpTakeoff;
        check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation),"accepted takeoff");
        offer={observation,observation.position,observation.facing,BattleMvpAction::Jump,2,producer};offer.kind=NativeBorgMotionKind::JumpAir;
        offer.proposed_position[1]+=15;offer.vertical_speed=15;
        check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation),"accepted ordinary launch");
        offer={observation,observation.position,observation.facing,BattleMvpAction::Jump,3,producer};offer.kind=NativeBorgMotionKind::JumpAir;
        offer.proposed_position[1]+=15;offer.vertical_speed=-35;
        check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation),"accepted descending history");
        make_push(4);
    };
    setup(NativeBorgMotionPolicy::Relaunch);
    check(!world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.tick()==3,"older policy cannot opt itself into projection");
    setup(NativeBorgMotionPolicy::StaticPush);
    const auto good=offer;
    auto refuses=[&](const NativeBorgMotionOffer& invalid,const char* why){
        check(!world.step({},false,&terrain,nullptr,nullptr,&invalid)&&world.tick()==3&&world.actors()[0].position==observation.position,why);
    };
    auto invalid=good;invalid.static_push.reset();refuses(invalid,"missing projection evidence refuses atomically");
    invalid=good;invalid.kind=NativeBorgMotionKind::JumpAir;refuses(invalid,"ordinary air cannot borrow push evidence");
    invalid.static_push.reset();refuses(invalid,"ordinary air keeps exact old-velocity and above-floor checks");
    invalid=good;invalid.static_push->integrated_position[1]+=1;refuses(invalid,"pre-projection integration cannot skip accepted old velocity");
    invalid=good;invalid.static_push->integrated_position[1]+=.001f;refuses(invalid,"new push route does not borrow ordinary-air integration tolerance");
    invalid=good;invalid.static_push->surface.normal[0]+=.01f;refuses(invalid,"spoofed selected normal refuses");
    invalid=good;++invalid.static_push->surface.triangle;refuses(invalid,"spoofed selected triangle refuses");
    invalid=good;invalid.static_push->surface.height+=1;refuses(invalid,"spoofed selected height refuses");
    invalid=good;++invalid.static_push->angle;refuses(invalid,"spoofed quantized angle refuses");
    invalid=good;invalid.static_push->damping=.5f;refuses(invalid,"windup damping cannot replace original steep factor");
    invalid=good;invalid.static_push->vertical_speed=-29;refuses(invalid,"pre-push acceleration/clamp evidence is checked");
    invalid=good;invalid.vertical_speed=0;refuses(invalid,"projection cannot zero velocity like landing");
    invalid=good;invalid.proposed_position[0]+=.01f;refuses(invalid,"edited projection cannot be accepted approximately");
    invalid=good;invalid.static_push->integrated_position[0]+=43;refuses(invalid,"pre-projection planar budget is retained");
    terrain.metadata=false;refuses(good,"height-only terrain cannot certify selected plane");terrain.metadata=true;
    terrain.adjacent=100;refuses(good,"raised post-floor/native clipping invalidates projection");
    terrain.adjacent=-4;refuses(good,"post-projection penetration refuses even without terrain blocking");
    terrain.adjacent=-20;
    check(world.step({},false,&terrain,nullptr,nullptr,&good)&&world.motion_receipt(good,&receipt)&&
        !world.actors()[0].grounded&&world.actors()[0].position==good.proposed_position&&world.actors()[0].vertical_speed==-27,
        "push stays airborne above a different lower adjacent floor, no fabricated landing or post-normal identity requirement");
    invalid=good;invalid.static_push->damping=.8f;
    check(!world.motion_receipt(invalid,&receipt),"edited evidence cannot borrow exact receipt");
    invalid=good;invalid.static_push->integrated_position[0]+=1;
    check(!world.motion_receipt(invalid,&receipt),"receipt matches pre-projection position too");
    check(world.motion_receipt(good,&receipt),"matching projection receipt");
    NativeBattleMvp copy=world;
    check(!copy.motion_receipt_current(receipt),"copied world cannot own push receipt");
    check(!world.step({},false,&terrain),"no legacy gravity fallback after push");
    producer.reset();check(!world.motion_receipt_current(receipt),"producer retirement revokes push receipt");
    check(world.start()&&!world.motion_receipt_current(receipt),"restart revokes old projected frame");
    terrain.adjacent=0;setup(NativeBorgMotionPolicy::StaticPush);
    check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&!world.actors()[0].grounded,
        "continuous selected plane admits quantized projection without grounded transition");
    check(world.observe_motion(&observation),"airborne push observation");
    offer={observation,observation.position,observation.facing,BattleMvpAction::AirDash,5,producer};
    offer.kind=NativeBorgMotionKind::AirDashWindupEntry;offer.vertical_speed=observation.vertical_speed*.5f;
    check(world.step({},false,&terrain,nullptr,nullptr,&offer),"ordinary falling push retains existing air-dash entry authority");
    setup(NativeBorgMotionPolicy::StaticPush,true);
    check(!world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.tick()==3,
        "native opponent clipping invalidates projected push instead of silently committing a different position");
    for(auto policy:{NativeBorgMotionPolicy::StaticPush,NativeBorgMotionPolicy::FallHistoryPush}){
        for(bool relaunch:{false,true}){
            setup(policy);
            u64 serial=4;
            auto step=[&](NativeBorgMotionKind kind,BattleMvpAction action,f32 y,f32 speed,f32 entry=0){
                offer={observation,observation.position,observation.facing,action,serial++,producer};offer.kind=kind;
                offer.proposed_position[1]=y;offer.vertical_speed=speed;offer.entry_vertical_delta=entry;
                check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.observe_motion(&observation),"accepted special-callback history before selector6");
            };
            if(relaunch)step(NativeBorgMotionKind::RelaunchEntry,BattleMvpAction::Jump,10,-20,15);
            else{
                step(NativeBorgMotionKind::AirDashWindupEntry,BattleMvpAction::AirDash,30,-17.5f);
                step(NativeBorgMotionKind::AirDashWindup,BattleMvpAction::AirDash,12.5f,-17.57f);
                step(NativeBorgMotionKind::AirDashActiveEntry,BattleMvpAction::AirDash,12.5f,-.07f);
                step(NativeBorgMotionKind::AirDashActive,BattleMvpAction::AirDash,12.43f,-.14f);
                //Neither old nor new policy may pretend active-dash physics
                //is a selector6 transition; recovery must be accepted first.
                const auto tick=world.tick();make_push(serial);
                check(!world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.tick()==tick,"active dash cannot borrow falling-push authority");
                step(NativeBorgMotionKind::AirDashRecovery,BattleMvpAction::AirDash,f32(observation.position[1]+observation.vertical_speed),-20);
            }
            const auto tick=world.tick();make_push(serial);
            if(policy==NativeBorgMotionPolicy::StaticPush){
                check(!world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.tick()==tick,
                    "older ordinary-fall policy cannot borrow immediate special-history fall entry");
            }else{
                const auto expected=offer.proposed_position;
                check(world.step({},false,&terrain,nullptr,nullptr,&offer)&&world.motion_receipt(offer,&receipt)&&
                    world.actors()[0].position==expected&&!world.actors()[0].grounded,
                    "explicit history-fall policy accepts reset acceleration and exact old-velocity projection, not active callback reset");
                auto edited=offer;edited.static_push->vertical_speed+=.001f;
                check(!world.motion_receipt(edited,&receipt),"history-fall projection still requires exact evidence receipt");
            }
        }
    }
    std::printf("native selected-plane push/old-velocity/exact receipt: %u checks PASS; native static proxy, not original whole-frame equivalence\n",checks);
}
