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
    if(!ok){std::fprintf(stderr,"Expiry air dash: %s (%s)\n",why,error.c_str());std::abort();}
}
class Files final:public gf::IFileSystem {
public:
    bool file_size(const std::string& path,gf::u64* out)override{
        std::error_code e;const auto size=std::filesystem::file_size(path,e);
        if(e||!out)return false;
        *out=size;return true;
    }
    bool read(const std::string& path,std::vector<gf::u8>& out)override{
        gf::u64 size=0;if(!file_size(path,&size)||size>(32u<<20))return false;
        std::ifstream in(path,std::ios::binary);out.assign(std::istreambuf_iterator<char>(in),{});return out.size()==size;
    }
    bool exists(const std::string& path)override{return std::filesystem::exists(path);}
    bool write(const std::string&,const gf::u8*,std::size_t)override{return false;}
    bool erase(const std::string&)override{return false;}
};
}

int main(){
    using namespace gf;
    using Kind=NativeBorgMotionKind;
    using Mode=NativeGRedGroundDashDemo::Mode;
    Files files;const std::string art="local/content/native-battle-mvp-v1",pack="local/content/source-gred-ground-v9";
    if(!files.exists(pack+"/motion.gfmm")){std::puts("Expiry air dash: optional GFMM9 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);NativeBattleMvp world;NativeBattleMvpView view;
    NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(render.initialize({1152,648,false,true,"Expiry air dash"},nullptr,&error)&&view.load(files,art,render,&error),
        "qualified stage and model assets");
    view.source_practice_controls();
    auto bind=[&](Mode mode){
        BattleMvpConfig cfg;cfg.arena_half=4500;cfg.spawn[0]={-1250.96863f,-3313.80225f};cfg.spawn_query_y[0]=1846.01367f;
        check(world.start(cfg,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
            source.bind(files,art,pack,world,seed,&error,mode),"fresh grounded placement on the original stage");
    };
    auto prepare=[&](bool flick,NativeBorgMotionOffer* offer,bool lift=false,bool up=false){
        GamePadState pad;
        if(world.tick()>=5&&!source.ground_departures())pad.dpad_left=true;
        if(flick)pad.stick_x=-127;
        pad.b=lift;pad.dpad_up=up;
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);NativeGRedGroundInput input;
        return source.map_input(pad,&input,&error)&&source.prepare(world,view.source_binding(),frame,input.x,input.y,input.held,offer,&error);
    };
    auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        check(world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error)&&
            source.pose(world,&pose)&&view.update_source(world,pose,&error),"native receipt precedes source and renderer publication");
    };
    NativeBorgMotionOffer offer;
    auto prefix=[&](unsigned tick){
        while(world.tick()<tick){check(prepare(false,&offer),"accepted walking ledge prefix");accept(offer);}
    };
    bind(Mode::GroundWalkLedgeExpiryPush);prefix(56);
    check(offer.kind==Kind::GroundLossPush&&offer.walk_ledge->walk_origin&&offer.walk_ledge->timer698==0&&
        prepare(true,&offer)&&offer.kind==Kind::AirDashWindupEntry,"older mode can stage the candidate but lacks the handoff");
    const auto old_frame=NativeBattleMvpView::movement_frame(world,1152,648);
    check(!world.step({},false,&view,nullptr,&old_frame,&offer)&&world.tick()==56&&
        source.resource()==10000&&!source.air_dash_starts()&&world.motion_receipt_current(pose.receipt),
        "older mode publishes no motion, timer, spend or pose on the refused handoff");
    source.reject();check(!world.step({},false,&view),"older refusal does not fall back");

    bind(Mode::GroundWalkLedgeExpiryAirDash);prefix(55);
    check(offer.kind==Kind::GroundLossPush&&source.ground_loss_timer698()==1&&
        !prepare(true,&offer)&&error.find("accepted zero-timer expiry push")!=std::string::npos&&world.tick()==55&&
        source.ground_loss_timer698()==1&&!source.air_dash_starts()&&source.resource()==10000&&!view.practice_readout(world),
        "the next decrement cannot substitute for a previously accepted expiry push");

    bind(Mode::GroundWalkLedgeDirectTimedAirDash);prefix(55);
    check(offer.kind==Kind::GroundLossPush&&source.ground_loss_timer698()==1&&
        !prepare(true,&offer)&&error.find("still-positive walking-push countdown")!=std::string::npos&&world.tick()==55&&
        source.ground_loss_timer698()==1&&!source.air_dash_starts()&&source.resource()==10000,
        "direct timed mode cannot borrow a pending timer expiry either");

    bind(Mode::GroundWalkLedgeExpiryAirDash);prefix(56);
    check(offer.kind==Kind::GroundLossPush&&offer.walk_ledge->walk_origin&&offer.walk_ledge->timer698==0,"exact expiry predecessor");
    unsigned entry=0,handoff=0,active=0,fall=0,touchdown=0,landing=0;
    while(world.tick()<300){
        const auto tick=world.tick()+1;const auto before=world.actors()[0];
        check(prepare(tick==57,&offer),"single left flick and complete post-expiry air-dash route");
        if(native_borg_motion_windup_entry(offer.kind)){
            entry=tick;const auto& a=*offer.air_motion;
            check(tick==57&&offer.kind==Kind::AirDashWindupEntry&&a.entry&&a.phase==15&&offer.controller_timer698==0&&
                offer.proposed_position==before.position&&a.vertical_before==f32(before.vertical_speed*.5f)&&
                a.resource_before==10000&&a.resource_after==10000&&!offer.air_dash_floor&&!offer.static_push,
                "plain entry halves velocity without displacement, contact, timer reset or cost");
            const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
            auto rejects=[&](const NativeBorgMotionOffer& wrong,const char* why){
                check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==56&&
                    world.actors()[0].position==before.position&&source.resource()==10000&&!source.air_dash_starts()&&
                    world.motion_receipt_current(pose.receipt),why);
            };
            auto wrong=offer;wrong.controller_timer698=1;rejects(wrong,"timer substitution");
            wrong=offer;wrong.proposed_position[0]+=1;rejects(wrong,"pre-contact displacement on entry");
            wrong=offer;wrong.air_motion->resource_after-=450;rejects(wrong,"early handoff cost");
            wrong=offer;wrong.kind=Kind::AirDashWindupEntryFloor;wrong.air_dash_floor=NativeBorgAirDashFloor{};
            rejects(wrong,"contact response cannot borrow plain expiry entry");
        }
        if(offer.air_motion&&offer.air_motion->phase==15&&offer.air_motion->next_selector==16){
            handoff=tick;
            check(tick==62&&offer.air_motion->resource_before==10000&&offer.air_motion->resource_after==9550,
                "handoff pays 450 on its own update");
        }
        if(native_borg_motion_active_entry(offer.kind)){
            active=tick;
            check(tick==63&&offer.air_motion->resource_before==9550&&offer.air_motion->resource_after==9500,
                "separate active entry pays 50, not the wind-up cost again");
        }
        if(offer.kind==Kind::JumpAir&&!fall){fall=tick;check(source.resource()==8000,"recovered active costs are retained on release");}
        if(offer.kind==Kind::JumpTouchdown)touchdown=tick;
        if(touchdown&&!landing&&offer.kind==Kind::JumpLanding)landing=tick;
        accept(offer);
    }
    check(entry==57&&handoff==62&&active==63&&fall==123&&touchdown==150&&landing==151&&
        source.walk_ledge_pushes()==2&&source.air_dash_starts()==1&&!source.relaunches()&&
        source.resource()==10000&&world.actors()[0].grounded&&source.selector()==0,"complete accepted route returns to idle and refills normally");

    auto expiry_windup_prefix=[&](Mode mode){
        bind(mode);prefix(56);
        const bool scoped=mode==Mode::GroundExpiryPlainWindupRelaunch;
        while(world.tick()<62){
            const unsigned tick=world.tick();
            const auto before=world.actors()[0];
            check(prepare(tick==56,&offer)&&offer.air_motion&&offer.air_motion->phase==15&&
                offer.air_motion->walk_expiry_windup==scoped&&!offer.air_dash_floor&&!offer.static_push,
                "only the new uninterrupted plain wind-up carries expiry lineage");
            const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
            auto wrong=offer;wrong.air_motion->walk_expiry_windup=!scoped;
            check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==tick&&
                world.actors()[0].position==before.position&&source.resource()==10000&&world.motion_receipt_current(pose.receipt)&&
                !view.practice_readout(world)->plain_windup_relaunch_ready,
                "expiry history cannot be fabricated in the older mode or erased from a scoped frame");
            accept(offer);
            check(view.practice_readout(world)->plain_windup_relaunch_ready==(scoped&&world.tick()==62),
                "the prompt requires the accepted, completed expiry wind-up and the matching policy");
        }
        check(offer.kind==Kind::AirDashWindup&&offer.air_motion->next_selector==16&&offer.air_motion->next_stage==0&&
            !offer.air_motion->timer568&&offer.controller_timer698==0&&source.resource()==9550&&
            !source.air_dash_active_frames(),"plain expiry wind-up completed and paid its own charge");
    };
    expiry_windup_prefix(Mode::GroundTimedPlainWindupRelaunch);
    check(!prepare(false,&offer,true)&&error.find("still-positive ledge countdown")!=std::string::npos&&
        world.tick()==62&&source.resource()==9550&&!source.relaunches(),
        "positive-timer handoff mode still refuses the completed zero-timer wind-up");

    bind(Mode::GroundExpiryPlainWindupRelaunch);prefix(56);
    if(prepare(false,&offer,true)){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        check(!world.step({},false,&view,nullptr,&frame,&offer),"direct relaunch from the expiry push still refuses");
        source.reject();
    }
    check(world.tick()==56&&source.resource()==10000&&!source.relaunches()&&!world.step({},false,&view),
        "expiry relaunch needs a completed wind-up and does not fall back");

    for(const bool held:{false,true}){
        expiry_windup_prefix(Mode::GroundExpiryPlainWindupRelaunch);
        const auto before=world.actors()[0];const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        check(prepare(false,&offer,true,held)&&offer.kind==Kind::RelaunchEntry&&offer.relaunch_motion&&
            offer.controller_timer698==0&&offer.entry_vertical_delta==15&&
            offer.relaunch_motion->vertical_before==f32(before.vertical_speed+15.f)&&offer.relaunch_motion->vertical_before>0&&
            offer.relaunch_motion->resource_before==9550&&offer.relaunch_motion->resource_after==9400&&
            !offer.static_push&&!offer.relaunch_floor&&!offer.air_motion,"qualified expiry history admits plain rising relaunch");
        auto refuses=[&](const NativeBorgMotionOffer& wrong,const char* why){
            check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==62&&world.actors()[0].position==before.position&&
                source.resource()==9550&&!source.relaunches()&&world.motion_receipt_current(pose.receipt),why);
        };
        auto wrong=offer;wrong.controller_timer698=1;refuses(wrong,"expiry handoff cannot invent a positive timer");
        wrong=offer;wrong.relaunch_motion->resource_before=10000;refuses(wrong,"expiry handoff cannot refund the wind-up");
        wrong=offer;wrong.relaunch_motion->resource_after-=50;refuses(wrong,"expiry handoff cannot borrow the active-entry charge");
        wrong=offer;wrong.kind=Kind::RelaunchEntryFloor;wrong.relaunch_floor=NativeBorgRelaunchFloor{};
        refuses(wrong,"expiry lineage does not authorize a relaunch floor response at entry");
        accept(offer);
        check(source.movement_programs()==std::array<u16,2>{14,14}&&source.resource()==9400&&source.relaunches()==1&&
            !source.air_dash_active_frames()&&!view.practice_readout(world)->plain_windup_relaunch_ready,
            "accepted handoff runs relaunch, not active dash, and clears the prompt");
        unsigned active_tick=0,fall_tick=0,hit=0,land=0;
        while(world.tick()<210){
            const unsigned tick=world.tick()+1;
            const bool rise=held&&tick<=74;
            check(prepare(false,&offer,rise,rise),"expiry handoff settles through ordinary fall and landing");
            if(!active_tick&&native_borg_motion_relaunch_active(offer.kind))active_tick=tick;
            if(!fall_tick&&offer.kind==Kind::JumpAir)fall_tick=tick;
            if(offer.kind==Kind::JumpTouchdown)hit=tick;
            if(hit&&!land&&offer.kind==Kind::JumpLanding)land=tick;
            accept(offer);
            if(tick==fall_tick||tick==hit||tick==land)check(source.resource()==(held?8300:9400),"release and landing do not refill early");
        }
        check((held?(active_tick==68&&fall_tick==75&&hit==146):(!active_tick&&fall_tick==64&&hit==126))&&
            land==hit+1&&source.landings()==1&&source.resource()==10000&&world.actors()[0].grounded&&!source.selector()&&
            source.air_dash_starts()==1&&!source.air_dash_active_frames()&&source.relaunches()==1,
            "tap and held expiry handoffs both reach idle without active dash");
    }
    std::printf("Expiry air dash: %u checks PASS, qualified zero-timer handoff and separate costs/landing\n",checks);
}
