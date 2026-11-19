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
            view.practice_readout(world)->kind==shown->kind,why);
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

    bind(Mode::GroundRecoveryRecontact);prefix(108);
    GamePadState drift;drift.dpad_right=true;
    check(!prepare(drift)&&error.find("requires a neutral recontact")!=std::string::npos&&world.tick()==108&&
        source.resource()==5750&&!source.landings()&&!world.step({},false,&view),"live steering is outside the verified recontact");
    std::printf("Recovery recontact: %u checks PASS, scoped completion, floor contact and landing\n",checks);
}
