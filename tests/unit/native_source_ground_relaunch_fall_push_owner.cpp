#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include "engine/core/headless_backends.hpp"
#include "engine/gameplay/native_battle_mvp_view.hpp"
#include "engine/gameplay/native_gred_ground_dash_demo.hpp"

namespace {
unsigned checks=0;
std::string error;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"released-relaunch fall push: %s (%s)\n",why,error.c_str());std::abort();}
}
class Files final:public gf::IFileSystem{
public:
    bool file_size(const std::string& n,gf::u64* o)override{
        std::error_code e;const auto s=std::filesystem::file_size(n,e);if(e||!o)return false;*o=s;return true;
    }
    bool read(const std::string& n,std::vector<gf::u8>& o)override{
        gf::u64 s=0;if(!file_size(n,&s)||s>(32u<<20))return false;
        std::ifstream f(n,std::ios::binary);o.assign(std::istreambuf_iterator<char>(f),{});return o.size()==s;
    }
    bool exists(const std::string& n)override{return std::filesystem::exists(n);}
    bool write(const std::string&,const gf::u8*,std::size_t)override{return false;}
    bool erase(const std::string&)override{return false;}
};
}

int main(){
    using namespace gf;
    using Mode=NativeGRedGroundDashDemo::Mode;
    Files files;const std::string art="local/content/native-battle-mvp-v1",pack="local/content/source-gred-ground-v9";
    if(!files.exists(pack+"/motion.gfmm")){std::puts("released-relaunch fall push: optional GFMM9 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);NativeBattleMvp world;NativeBattleMvpView view;
    NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(render.initialize({1152,648,false,true,"released-relaunch fall push"},nullptr,&error)&&
        view.load(files,art,render,&error),"shared original stage, art and renderer");
    BattleMvpConfig config;config.spawn[0]={-606.4511719f,-1373.5872803f};
    const auto bind=[&](Mode mode){
        check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
            source.bind(files,art,pack,world,seed,&error,mode),"grounded original-stage fixture, no state injection");
    };
    struct Route {unsigned press,release,push,touchdown; s16 meter; bool entry;};
    const Route continued{45,60,123,124,6800,false},immediate{81,82,83,84,8200,true};
    const auto prepare=[&](const Route& route,NativeBorgMotionOffer* offer){
        const auto t=world.tick();
        GamePadState pad;pad.b=(t>=5&&t<33)||(t>=route.press&&t<route.release);
        NativeGRedGroundInput input;
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        return source.map_input(pad,&input,&error)&&source.prepare(world,view.source_binding(),frame,input.x,input.y,input.held,offer,&error);
    };
    const auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        if(!world.step({},false,&view,nullptr,&frame,&offer)){error="native tick="+std::to_string(world.tick()+1);check(false,"native acceptance");}
        check(source.commit(world,view.source_binding(),&error)&&source.pose(world,&pose)&&
            view.update_source(world,pose,&error),"matching native receipt before publishing pose, VM, motion and resource");
    };
    const auto fatal_refusal=[&](Mode mode,const Route& route,unsigned stop,u8 selector){
        bind(mode);
        while(world.tick()<stop){NativeBorgMotionOffer offer;check(prepare(route,&offer),"accepted prefix");accept(offer);}
        const auto retained=pose;const auto before=world.actors()[0].position;
        check(source.selector()==selector&&source.relaunches()==1,"expected relaunch lineage at refusal");
        NativeBorgMotionOffer offer;
        check(!prepare(route,&offer)&&world.tick()==stop&&world.actors()[0].position==before&&
            !source.surface_pushes()&&retained.frame_scope.expired()&&!world.motion_receipt_current(retained.receipt)&&
            !world.step({},false,&view),"fatal source refusal retires authority; no fallback");
    };
    fatal_refusal(Mode::GroundAirFallPush,continued,122,6);
    fatal_refusal(Mode::GroundRelaunchFallPush,{81,95,0,0,0,false},82,8);
    for(const auto route:{continued,immediate}){
        bind(Mode::GroundRelaunchFallPush);
        NativeBorgMotionOffer prior;
        std::weak_ptr<const NativeBorgMotionProducer> producer;
        BattleMvpVec landed{};
        for(unsigned t=0;t<600;++t){
            const auto before=world.actors()[0];const auto retained=pose;
            const auto meter=source.resource();const auto count=source.relaunch_fall_pushes();
            NativeBorgMotionOffer offer;
            check(prepare(route,&offer)&&source.resource()==meter&&source.relaunch_fall_pushes()==count,"private staged motion, resource and counters");
            if(!t)producer=offer.producer;
            check(!producer.owner_before(offer.producer)&&!offer.producer.owner_before(producer),"one producer across relaunch, released fall and landing");
            check(bool(offer.static_push)==(t+1==route.push),"only one expected projection");
            if(offer.jump_motion&&offer.jump_motion->relaunch_origin&&offer.jump_motion->phase==6&&
               offer.jump_motion->entry&&!offer.static_push){
                auto wrong=offer;wrong.jump_motion->held_jump=true;wrong.jump_motion->resource_before=0;
                const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==t&&
                    world.actors()[0].position==before.position&&world.motion_receipt_current(retained.receipt),
                    "noncontact release cannot fabricate depletion to seed later push eligibility");
            }
            if(offer.static_push){
                check(offer.jump_motion.has_value(),"fall numeric proof");
                const auto& e=*offer.jump_motion;const auto& p=*offer.static_push;
                check(e.phase==6&&e.entry==route.entry&&e.relaunch_origin&&e.relaunch_coarse57d==0&&!e.ground_loss&&!e.air_dash_origin&&
                    !offer.relaunch_motion&&!offer.relaunch_floor&&!offer.surface_landing&&offer.controller_timer698==0.f&&
                    e.vertical_before==before.vertical_speed&&e.vertical_accel==-1&&e.planar_accel==0&&
                    e.resource_before==route.meter&&e.speed44==f32(e.move_speed*p.damping)&&p.damping==bits_to_f32(0x3f666666u)&&
                    p.angle==11582&&offer.vertical_speed==f32(p.vertical_speed*p.damping),"released fall21 retains its own provenance, gravity and .9 tail");
                if(route.entry)check(prior.relaunch_motion&&e.speed_before==prior.relaunch_motion->speed44&&!e.held_jump,
                    "immediate projection requires accepted relaunch predecessor and actual release");
                else check(prior.jump_motion&&prior.jump_motion->relaunch_origin&&e.speed_before==prior.jump_motion->speed44,
                    "continued projection inherits accepted falling scalar");
                const auto rejects=[&](NativeBorgMotionOffer wrong,const char* why){
                    const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                    check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==t&&
                        world.actors()[0].position==before.position&&source.resource()==meter&&source.relaunch_fall_pushes()==count&&
                        !retained.frame_scope.expired()&&world.motion_receipt_current(retained.receipt),why);
                };
                auto wrong=offer;wrong.jump_motion->relaunch_coarse57d=1;rejects(wrong,"cannot swap relaunch direction/program lineage");
                wrong=offer;wrong.jump_motion->relaunch_origin=false;rejects(wrong,"cannot replace released relaunch with ordinary fall");
                wrong=offer;wrong.jump_motion->entry=!e.entry;rejects(wrong,"entry state must match prior accepted role");
                wrong=offer;wrong.jump_motion->resource_before=0;rejects(wrong,"cannot invent meter depletion");
                wrong=offer;wrong.controller_timer698=1;rejects(wrong,"positive698 is not admitted");
                wrong=offer;wrong.entry_vertical_delta=15;rejects(wrong,"cannot replay relaunch lift");
                wrong=offer;wrong.kind=NativeBorgMotionKind::RelaunchActivePush;rejects(wrong,"active relaunch push remains refused");
                wrong=offer;++wrong.static_push->surface.triangle;rejects(wrong,"must use exact selected plane");
                if(route.entry){wrong=offer;wrong.jump_motion->held_jump=true;rejects(wrong,"held relaunch with meter is not released falling");}
            }
            accept(offer);
            if(offer.static_push)check(source.selector()==6&&source.movement_programs()==std::array<u16,2>{21,21}&&
                source.cached_airborne87()==1&&source.resource()==route.meter&&!source.landings()&&!world.actors()[0].grounded,
                "projection remains airborne program21 with unchanged meter");
            if(t+1==route.touchdown){
                landed=offer.proposed_position;
                check(offer.kind==NativeBorgMotionKind::JumpTouchdown&&world.actors()[0].grounded&&source.cached_airborne87()==1&&
                    source.resource()==route.meter&&source.movement_programs()==std::array<u16,2>{21,21},"separate callback keeps captured fall and cache");
            }
            if(t+1==route.touchdown+1)check(source.cached_airborne87()==0&&source.movement_programs()==std::array<u16,2>{8,8},"separate landing8");
            if(t+1>route.touchdown)check(world.actors()[0].position==landed,"stable landing and idle position");
            prior=offer;
        }
        check(source.relaunch_fall_pushes()==1&&source.surface_pushes()==1&&source.relaunches()==1&&source.landings()==1&&
            !source.air_dash_starts()&&!source.ordered_root().updates&&source.selector()==0&&source.resource()==10000&&
            !world.shots_spawned()&&world.actors()[0].health==100&&world.actors()[1].health==100,"600-tick route completes in idle");
        source.reject();
    }
    std::printf("released-relaunch fall push: %u checks PASS; two 600-tick original-stage routes plus refusal cases\n",checks);
}
