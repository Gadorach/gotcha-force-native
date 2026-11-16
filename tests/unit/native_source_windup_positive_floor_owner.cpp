#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include "engine/core/headless_backends.hpp"
#include "engine/gameplay/native_battle_mvp_view.hpp"
#include "engine/gameplay/native_gred_ground_dash_demo.hpp"
namespace {
unsigned checks=0;std::string error;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"source positive wind-up floor: %s (%s)\n",why,error.c_str());std::abort();}}
class Files final:public gf::IFileSystem{
public:
    bool file_size(const std::string& name,gf::u64* out)override{std::error_code e;const auto size=std::filesystem::file_size(name,e);if(e||!out)return false;*out=size;return true;}
    bool read(const std::string& name,std::vector<gf::u8>& out)override{gf::u64 size=0;if(!file_size(name,&size)||size>(32u<<20))return false;std::ifstream f(name,std::ios::binary);out.assign(std::istreambuf_iterator<char>(f),{});return out.size()==size;}
    bool exists(const std::string& name)override{return std::filesystem::exists(name);}
    bool write(const std::string&,const gf::u8*,std::size_t)override{return false;}
    bool erase(const std::string&)override{return false;}
};
}
int main(){
    using namespace gf;Files files;const std::string art="local/content/native-battle-mvp-v1",inputs="local/content/source-gred-ground-v7";
    if(!files.exists(inputs+"/motion.gfmm")){std::puts("source positive wind-up floor: optional qualified GFMM7 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);
    check(render.initialize({1152,648,false,true,"positive wind-up floor fixture"},nullptr,&error),"shared headless renderer");
    NativeBattleMvp world;NativeBattleMvpView view;NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(view.load(files,art,render,&error),"unchanged original stage and normal models");
    auto bind=[&](NativeGRedGroundDashDemo::Mode mode){
        check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
            source.bind(files,art,inputs,world,seed,&error,mode),"ordinary native spawn/bounds/target/references and current seed");
        check(!source.positive_windup_floor_contacts()&&!source.windup_entry_floor_contacts()&&!source.air_dash_starts()&&!source.landings(),
            "fresh committed counters");
    };
    auto pad_at=[&](unsigned t,unsigned flick=86){
        GamePadState pad;pad.b=(t>=5&&t<31)||(t>=78&&!source.air_dash_starts());
        if(t>=flick&&t<flick+27)pad.stick_x=127;
        return pad;
    };
    auto prepare=[&](GamePadState pad,NativeBorgMotionOffer* offer){
        NativeGRedGroundInput mapped;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        return source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,offer,&error);
    };
    auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        check(world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error)&&
            source.pose(world,&pose)&&view.update_source(world,pose,&error),"exact receipt before private VM/resource/counter/pose publication");
    };
    bind(NativeGRedGroundDashDemo::Mode::WindupEntryFloor);
    for(unsigned t=0;t<86;++t){NativeBorgMotionOffer offer;check(prepare(pad_at(t),&offer),"earlier mode reaches positive relaunch snap");accept(offer);}
    NativeBorgMotionOffer old;const auto meter=source.resource();const auto retained=pose;
    check(world.tick()==86&&world.actors()[0].vertical_speed>0&&prepare(pad_at(86),&old)&&
        old.kind==NativeBorgMotionKind::AirDashWindupEntryFloor&&old.air_dash_floor&&old.air_dash_floor->vertical_speed>0,
        "older owner can stage the numeric positive half-speed but does not gain native authority");
    const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
    check(!world.step({},false,&view,nullptr,&frame,&old)&&world.tick()==86&&source.selector()==8&&source.resource()==meter&&
        !source.windup_entry_floor_contacts()&&!source.positive_windup_floor_contacts()&&!source.air_dash_starts()&&
        !retained.frame_scope.expired()&&world.motion_receipt_current(retained.receipt)&&!world.step({},false,&view),
        "old policy rejection leaves staged state unpublished, preserves current frame and cannot use legacy movement fallback");
    source.reject();
    check(retained.frame_scope.expired()&&!world.motion_receipt_current(retained.receipt),"explicit retirement revokes rejected producer and prior leases");
    bind(NativeGRedGroundDashDemo::Mode::WindupPositiveFloor);u32 entry=0,handoff=0,active=0,touchdown=0;unsigned entries=0;
    for(unsigned t=0;t<600;++t){
        NativeBorgMotionOffer offer;const auto before_meter=source.resource();const auto before_count=source.positive_windup_floor_contacts();
        const auto before_selector=source.selector();const auto stages=source.air_dash_stages();const auto prior_pose=pose;
        check(prepare(pad_at(t),&offer)&&source.resource()==before_meter&&source.positive_windup_floor_contacts()==before_count&&
            source.selector()==before_selector&&source.air_dash_stages().timer568==stages.timer568,
            "positive entry and continuation remain staged before native acceptance");
        if(offer.kind==NativeBorgMotionKind::AirDashWindupPositiveEntryFloor){
            const auto tick=world.tick();auto wrong=offer;wrong.kind=NativeBorgMotionKind::AirDashWindupEntryFloor;
            const auto heading=NativeBattleMvpView::movement_frame(world,1152,648);
            check(!world.step({},false,&view,nullptr,&heading,&wrong)&&world.tick()==tick&&source.resource()==before_meter&&
                source.positive_windup_floor_contacts()==before_count&&!prior_pose.frame_scope.expired()&&world.motion_receipt_current(prior_pose.receipt),
                "old entry tag cannot borrow positive authority or commit pending state");
        }
        accept(offer);const auto tick=world.tick();
        if(tick==86)check(offer.kind==NativeBorgMotionKind::RelaunchActiveFloor&&offer.relaunch_floor&&offer.relaunch_floor->response_factor==1&&
            world.actors()[0].vertical_speed>0&&!world.actors()[0].grounded&&source.relaunch_floor_contacts()==8&&source.resource()==7700,
            "undamped last relaunch snap supplies positive48 at floor height without landing/refill");
        if(native_borg_motion_windup_entry(offer.kind))++entries;
        if(offer.kind==NativeBorgMotionKind::AirDashWindupPositiveEntryFloor){
            entry=tick;
            check(entry==87&&offer.air_dash_floor&&offer.observed.vertical_speed>0&&
                offer.air_dash_floor->vertical_speed==f32(offer.observed.vertical_speed*.5f)&&offer.air_dash_floor->vertical_speed>0&&
                offer.air_dash_floor->acceleration==bits_to_f32(0xbd8f5c29u)&&offer.vertical_speed==0&&
                offer.air_dash_floor->integrated_position==offer.observed.position&&offer.proposed_position==offer.observed.position&&
                !world.actors()[0].grounded&&source.resource()==7700&&source.positive_windup_floor_contacts()==1&&
                source.windup_entry_floor_contacts()==1&&source.air_dash_starts()==1&&source.selector()==15&&
                source.air_dash_stages().timer568==7&&!source.air_dash_stages().contact_callback&&
                source.movement_programs()==std::array<u16,2>{10,10},
                "entry halves positive48 then ret1 zeros it; no translation, acceleration, entry spend or grounded recovery");
            NativeBorgMotionReceipt receipt;auto wrong=offer;wrong.air_dash_floor->vertical_speed=0;
            check(!world.motion_receipt(wrong,&receipt),"zeroed pre-response evidence cannot borrow exact positive receipt");
        }
        if(offer.kind==NativeBorgMotionKind::AirDashWindupFloor){
            check(tick>=88&&tick<=94&&source.positive_windup_floor_contacts()==1&&source.windup_entry_floor_contacts()==1&&
                offer.vertical_speed==0&&source.resource()==(tick==94?7250:7700),
                "continued wind-up does not replay positive entry or charge its deferred cost early");
            if(tick==94){handoff=tick;check(source.selector()==16&&!source.air_dash_stages().entered,"deferred timer spend/select16 before contact");}
        }
        if(offer.kind==NativeBorgMotionKind::AirDashActiveEntryFloor){
            active=tick;check(active==95&&source.resource()==7200&&offer.vertical_speed==bits_to_f32(0xbd8f5c29u),
                "separate active entry preserves the earlier floor response contract");
        }
        if(offer.kind==NativeBorgMotionKind::JumpTouchdown){
            touchdown=tick;check(touchdown==125&&source.resource()==5700&&source.selector()==7&&world.actors()[0].grounded&&
                source.movement_programs()==std::array<u16,2>{11,11},"later active timer callback alone produces actual touchdown");
        }
    }
    check(entry==87&&handoff==94&&active==95&&touchdown==125&&entries==1&&source.relaunches()==1&&source.relaunch_floor_contacts()==8&&
        source.air_dash_starts()==1&&source.positive_windup_floor_contacts()==1&&source.windup_entry_floor_contacts()==1&&
        source.windup_floor_contacts()==8&&source.active_floor_contacts()==30&&source.landings()==1&&source.selector()==0&&source.resource()==10000&&
        !source.surface_pushes()&&!world.shots_spawned()&&world.actors()[0].health==100&&world.actors()[1].health==100,
        "600-frame positive-entry route has one entry and separate spending/landing/refill, not flight or combat substitution");
    bind(NativeGRedGroundDashDemo::Mode::WindupPositiveFloor);
    for(unsigned t=0;t<240;++t){NativeBorgMotionOffer offer;check(prepare(pad_at(t,80),&offer),"inherited nonascending route");accept(offer);}
    check(source.windup_entry_floor_contacts()==1&&!source.positive_windup_floor_contacts()&&source.landings()==1&&source.resource()==10000,
        "new mode retains the separate nonascending kind without mislabelling it as positive");
    const auto retired=pose;source.reject();
    check(retired.frame_scope.expired()&&!world.motion_receipt_current(retired.receipt),"retirement revokes current positive-capable producer leases");
    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v6",world,seed,&error,NativeGRedGroundDashDemo::Mode::WindupPositiveFloor),
        "positive mode still requires reviewed GFMM7 operands and authority");
    std::printf("source positive wind-up floor/VM/resource/pose: %u checks PASS; entry=%u handoff=%u active=%u touchdown=%u; native proxies only\n",
        checks,entry,handoff,active,touchdown);
}
