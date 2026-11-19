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
    if(!ok){std::fprintf(stderr,"Recovery recontact: %s (%s)\n",why,error.c_str());std::abort();}
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
class DifferentTriangle final:public gf::IBattleMvpTerrain {
    const gf::NativeBattleMvpView& view_;
    gf::BattleMvpVec start_;
public:
    DifferentTriangle(const gf::NativeBattleMvpView& view,gf::BattleMvpVec start):view_(view),start_(start){}
    bool height(gf::f32 x,gf::f32 z,gf::f32 y,gf::f32* out)const override{return view_.height(x,z,y,out);}
    bool static_surface(gf::f32 x,gf::f32 z,gf::f32 y,gf::NativeBorgStaticSurface* out)const override{
        if(!view_.static_surface(x,z,y,out))return false;
        if(x==start_[0]&&z==start_[2])++out->triangle;
        return true;
    }
};
}

int main(){
    using namespace gf;
    using Kind=NativeBorgMotionKind;
    using Mode=NativeGRedGroundDashDemo::Mode;
    Files files;const std::string art="local/content/native-battle-mvp-v1",pack="local/content/source-gred-ground-v9";
    if(!files.exists(pack+"/motion.gfmm")){std::puts("Recovery recontact: optional GFMM9 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);NativeBattleMvp world;NativeBattleMvpView view;
    NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(render.initialize({1152,648,false,true,"Recovery recontact"},nullptr,&error)&&view.load(files,art,render,&error),
        "qualified stage and models");
    view.source_practice_controls();
    auto bind=[&](Mode mode){
        check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
            source.bind(files,art,pack,world,seed,&error,mode),"ordinary native start");
    };
    NativeBorgMotionOffer offer;
    auto prepare=[&](GamePadState pad={}){
        const unsigned t=world.tick();
        if(t>=5&&t<39)pad.b=true;
        if(t==42)pad.stick_y=-127;
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        NativeGRedGroundInput input;
        return source.map_input(pad,&input,&error)&&source.prepare(world,view.source_binding(),frame,input.x,input.y,input.held,&offer,&error);
    };
    auto accept=[&](){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        check(world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error)&&
            source.pose(world,&pose)&&view.update_source(world,pose,&error),"native receipt precedes motion, pose and readout");
    };
    auto prefix=[&](unsigned t){
        while(world.tick()<t){check(prepare(),"approach via takeoff, genuine backward flick and neutral recovery");accept();}
    };
    bind(Mode::GroundWalkLedgeExpiryAirDash);prefix(107);
    check(!prepare()&&error.find("simultaneous recovery-completion/landing")!=std::string::npos&&world.tick()==107&&
        source.resource()==5750&&source.selector()==17&&!view.practice_readout(world)&&!world.step({},false,&view),
        "earlier mode refuses the overlap without publication or fallback");

    bind(Mode::GroundRecoveryRecontact);prefix(106);
    check(prepare(),"actual recovery prior to completion");
    const auto stale_completion=offer.recovery_completion.lock();
    check(stale_completion&&offer.kind==Kind::AirDashRecovery&&!offer.air_motion->lower_done,"actual incomplete lane is sealed");
    accept();
    auto rejects=[&](const NativeBorgMotionOffer& wrong,const IBattleMvpTerrain& terrain,const char* why){
        const auto before=world.actors()[0];const auto tick=world.tick();
        const auto shown=view.practice_readout(world);const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        check(shown&&!world.step({},false,&terrain,nullptr,&frame,&wrong)&&world.tick()==tick&&
            world.actors()[0].position==before.position&&world.actors()[0].vertical_speed==before.vertical_speed&&
            world.motion_receipt_current(pose.receipt)&&source.resource()==5750&&!source.landings()&&
            view.practice_readout(world)->kind==shown->kind&&
            view.practice_readout(world)->recovery_drift_active==shown->recovery_drift_active,why);
    };
    check(prepare()&&offer.kind==Kind::AirDashRecoveryFloorComplete&&offer.air_motion&&offer.surface_landing,
        "staged completion with physical contact but not touchdown");
    check(offer.air_motion->lower_done&&offer.air_motion->phase==17&&offer.air_motion->next_selector==6&&offer.air_motion->callback&&
        offer.surface_landing->angle==0&&offer.surface_landing->surface.triangle==1631&&offer.controller_timer698==0&&
        offer.proposed_position[1]==10&&offer.surface_landing->integrated_position[1]<10&&!offer.vertical_speed,
        "lower completion follows the real callback and retains its zero vertical speed");
    auto wrong=offer;wrong.recovery_completion.reset();rejects(wrong,view,"completion requires its sealed lower-lane result");
    wrong=offer;wrong.recovery_completion=stale_completion;rejects(wrong,view,"a live proof from the last tick does not match");
    wrong=offer;wrong.kind=Kind::JumpTouchdown;wrong.air_motion->next_selector=7;wrong.air_motion->lower_done=false;wrong.action=BattleMvpAction::Jump;
    rejects(wrong,view,"ordinary touchdown cannot substitute a different lower-lane outcome");
    wrong=offer;wrong.air_motion->travel5ae=source_heading_wrap(s32(wrong.air_motion->travel5ae)+256);
    f32 x=0,z=0;source_heading_xz(wrong.air_motion->travel5ae,&x,&z);
    const auto& origin=world.actors()[0].position;
    wrong.proposed_position[0]=wrong.surface_landing->integrated_position[0]=std::fma(1.f,f32(wrong.air_motion->move_speed*x),origin[0]);
    wrong.proposed_position[2]=wrong.surface_landing->integrated_position[2]=std::fma(1.f,f32(wrong.air_motion->move_speed*z),origin[2]);
    NativeBorgStaticSurface turned_surface;
    check(view.static_surface(wrong.proposed_position[0],wrong.proposed_position[2],origin[1],&turned_surface)&&
        turned_surface==offer.surface_landing->surface,"altered travel also reaches the original flat triangle");
    rejects(wrong,view,"completion must retain the previously accepted travel heading");
    const DifferentTriangle changed_floor(view,world.actors()[0].position);
    rejects(offer,changed_floor,"same height on a different selected triangle is not sufficient");
    accept();
    check(!world.actors()[0].grounded&&source.selector()==6&&source.movement_programs()==std::array<u16,2>{12,12}&&
        source.resource()==5750&&!source.landings()&&view.practice_readout(world)->kind==Kind::AirDashRecoveryFloorComplete,
        "both program12 lanes finish while selector6 and the airborne resource history are retained");

    check(prepare()&&offer.kind==Kind::JumpTouchdown&&offer.jump_motion&&offer.surface_landing&&
        offer.jump_motion->entry&&offer.jump_motion->air_dash_origin&&!offer.jump_motion->vertical_before&&
        offer.surface_landing->surface.triangle==1631&&offer.surface_landing->integrated_position[1]==10,
        "one neutral program13 update recontacts the same floor without a new descent");
    wrong=offer;wrong.jump_motion->drift_heading=0;rejects(wrong,view,"recontact is not an authorization to steer at rest");
    wrong=offer;--wrong.jump_motion->resource_before;rejects(wrong,view,"recontact inherits the exact meter");
    accept();
    check(world.actors()[0].grounded&&source.selector()==7&&source.movement_programs()==std::array<u16,2>{13,13}&&
        source.landings()==1&&source.resource()==5750,"only the second contact publishes touchdown");
    prefix(110);
    check(offer.kind==Kind::JumpLanding&&source.movement_programs()==std::array<u16,2>{8,8}&&source.resource()==5750,
        "landing8 and subsequent meter refill are separate");
    prefix(200);
    check(source.selector()==0&&source.landings()==1&&source.resource()==10000&&world.actors()[0].grounded,"route settles and refills");
    const auto neutral=world.actors()[0].position;

    bind(Mode::GroundRecoveryRecontact);prefix(108);
    GamePadState drift;drift.dpad_right=true;
    check(!prepare(drift)&&error.find("requires a neutral recontact")!=std::string::npos&&world.tick()==108&&
        source.resource()==5750&&!source.landings()&&!world.step({},false,&view),"live steering is outside the verified recontact");

    bind(Mode::GroundRecoveryRecontactDrift);prefix(107);
    GamePadState up;up.dpad_up=true;
    for(const unsigned tick:{108u,109u}){
        const auto before=world.actors()[0];const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        check(prepare(up)&&offer.kind==(tick==108?Kind::AirDashRecoveryFloorComplete:Kind::JumpTouchdown)&&
            offer.surface_landing&&offer.surface_landing->surface.triangle==1631&&offer.controller_timer698==0&&
            offer.facing==before.facing,"Up steers both scoped updates without changing plane, timer or body");
        const auto direction=tick==108?offer.air_motion->drift_heading:offer.jump_motion->drift_heading;
        const auto travel=tick==108?offer.air_motion->travel5ae:offer.jump_motion->travel5ae;
        const auto speed=tick==108?offer.air_motion->move_speed:offer.jump_motion->move_speed;
        s16 forward=0;f32 dx=0,dz=0,tx=0,tz=0;
        check(source_heading_s16(frame.forward.x,frame.forward.z,&forward)&&direction==forward,"Up follows the observed camera");
        source_heading_xz(*direction,&dx,&dz);source_heading_xz(travel,&tx,&tz);
        check(offer.proposed_position[0]==std::fma(1.f,f32(speed*tx),std::fma(4.f,dx,before.position[0]))&&
            offer.proposed_position[2]==std::fma(1.f,f32(speed*tz),std::fma(4.f,dz,before.position[2])),
            "exact four-unit drift runs before the retained planar momentum");
        auto absent=offer;absent.recovery_completion.reset();rejects(absent,view,"each scoped update needs its own proof");
        auto erased=offer;
        if(tick==108)erased.air_motion->drift_heading.reset();else erased.jump_motion->drift_heading.reset();
        rejects(erased,view,"an issued drift proof cannot be relabelled as neutral");
        NativeBorgMovementFrame turned;
        check(native_borg_movement_frame(frame.right,&turned)&&!world.step({},false,&view,nullptr,&turned,&offer)&&
            world.tick()==tick-1&&world.actors()[0].position==before.position&&world.motion_receipt_current(pose.receipt)&&
            view.practice_readout(world)->recovery_drift_active==(tick==109),
            "native refuses the same offered displacement against a different camera without publishing the cue");
        accept();
        check(view.practice_readout(world)->recovery_drift_active&&source.resource()==5750&&
            world.actors()[0].grounded==(tick==109),"drift cue, stable meter and touchdown publish only on acceptance");
    }
    prefix(110);
    check(offer.kind==Kind::JumpLanding&&!view.practice_readout(world)->recovery_drift_active&&source.resource()==5750,
        "drift does not leak into landing");
    prefix(200);
    const auto& steered=world.actors()[0].position;
    check(steered[0]<neutral[0]-7&&steered[2]>neutral[2]+1&&steered[1]==10&&
        world.actors()[0].grounded&&source.landings()==1&&source.resource()==10000&&!source.selector(),
        "steered route lands and refills normally at a distinct position");

    bind(Mode::GroundRecoveryRecontactDrift);prefix(107);
    check(prepare(drift)&&offer.kind==Kind::AirDashRecoveryFloorComplete,"Right is accepted at the first contact");
    accept();
    const auto at_floor=world.actors()[0].position;
    check(!prepare(drift)&&error.find("same flat floor")!=std::string::npos&&world.tick()==108&&
        world.actors()[0].position==at_floor&&!source.landings()&&source.resource()==5750&&!world.step({},false,&view),
        "a later change of triangle still does not authorize the recontact");

    GamePadState diagonal;diagonal.dpad_up=diagonal.dpad_right=true;
    GamePadState flick;flick.stick_x=127;
    GamePadState jump;jump.b=true;
    for(const auto pad:{diagonal,flick,jump}){
        bind(Mode::GroundRecoveryRecontactDrift);prefix(108);
        check(!prepare(pad)&&error.find("neutral or soft cardinal drift")!=std::string::npos&&
            world.tick()==108&&!source.landings()&&source.resource()==5750&&!world.step({},false,&view),
            "recontact still refuses diagonal, flick and lift inputs");
    }
    std::printf("Recovery recontact: %u checks PASS, scoped completion, floor contact and landing\n",checks);
}
