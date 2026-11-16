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
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"source wind-up entry floor: %s (%s)\n",why,error.c_str());std::abort();}}
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
    if(!files.exists(inputs+"/motion.gfmm")){std::puts("source wind-up entry floor: optional qualified GFMM7 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);
    check(render.initialize({1152,648,false,true,"source wind-up entry floor fixture"},nullptr,&error),"shared headless renderer");
    NativeBattleMvp world;NativeBattleMvpView view;NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(view.load(files,art,render,&error),"unchanged original stage and normal models");
    auto bind=[&](NativeGRedGroundDashDemo::Mode mode){
        check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
            source.bind(files,art,inputs,world,seed,&error,mode),"ordinary native spawn/bounds/target/reference and current seed");
        check(!source.windup_entry_floor_contacts()&&!source.relaunches()&&!source.air_dash_starts()&&!source.landings(),
            "fresh committed counters");
    };
    auto pad_at=[&](unsigned t){
        GamePadState pad;pad.b=(t>=5&&t<31)||(t>=78&&!source.air_dash_starts());
        if(t>=80&&t<107)pad.stick_x=127;
        return pad;
    };
    auto prepare=[&](GamePadState pad,NativeBorgMotionOffer* offer){
        NativeGRedGroundInput mapped;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        return source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,offer,&error);
    };
    auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        check(world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error)&&
            source.pose(world,&pose)&&view.update_source(world,pose,&error),"matching native receipt before VM/resource/pose publication");
    };
    bind(NativeGRedGroundDashDemo::Mode::AirDashFloor);bool refused=false;
    for(unsigned t=0;t<120&&!refused;++t){
        NativeBorgMotionOffer offer;const auto tick=world.tick();const auto meter=source.resource();const auto retained=pose;
        if(!prepare(pad_at(t),&offer)){
            check(tick==80&&world.tick()==tick&&source.selector()==8&&source.resource()==meter&&source.relaunch_floor_contacts()==2&&
                !source.windup_entry_floor_contacts()&&!source.air_dash_starts()&&!source.landings()&&
                retained.frame_scope.expired()&&!world.motion_receipt_current(retained.receipt)&&!world.step({},false,&view),
                "older GFMM7 air-floor mode still refuses entry contact without cost/pose/fallback and revokes its retired leases");
            refused=true;
        }else accept(offer);
    }
    check(refused,"new kind does not widen the earlier floor policy");
    bind(NativeGRedGroundDashDemo::Mode::WindupEntryFloor);u32 entry=0,handoff=0,active=0,touchdown=0;unsigned entries=0;
    for(unsigned t=0;t<600;++t){
        NativeBorgMotionOffer offer;const auto meter=source.resource();const auto selector=source.selector();const auto retained=pose;
        const auto count=source.windup_entry_floor_contacts();const auto stages=source.air_dash_stages();
        check(prepare(pad_at(t),&offer)&&source.resource()==meter&&source.selector()==selector&&source.windup_entry_floor_contacts()==count&&
            source.air_dash_stages().timer568==stages.timer568,"entry/timer/resource state remains staged before native acceptance");
        if(offer.kind==NativeBorgMotionKind::AirDashWindupEntryFloor){
            const auto tick=world.tick();auto wrong=offer;wrong.air_dash_floor->vertical_speed+=wrong.air_dash_floor->acceleration;
            const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
            check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==tick&&source.resource()==meter&&
                source.windup_entry_floor_contacts()==count&&!retained.frame_scope.expired()&&world.motion_receipt_current(retained.receipt),
                "entry cannot apply future acceleration; rejected native candidate does not commit costs/VM state or retire the live producer");
        }
        accept(offer);const auto tick=world.tick();
        if(native_borg_motion_windup_entry(offer.kind))++entries;
        if(offer.kind==NativeBorgMotionKind::AirDashWindupEntryFloor){
            entry=tick;
            check(entry==81&&offer.air_dash_floor&&!offer.static_push&&!offer.relaunch_floor&&
                offer.air_dash_floor->integrated_position==offer.observed.position&&offer.proposed_position==offer.observed.position&&
                offer.air_dash_floor->vertical_speed==f32(offer.observed.vertical_speed*.5f)&&
                offer.air_dash_floor->acceleration==bits_to_f32(0xbd8f5c29u)&&offer.vertical_speed==0&&
                !world.actors()[0].grounded&&source.resource()==8300&&source.air_dash_starts()==1&&source.windup_entry_floor_contacts()==1&&
                source.selector()==15&&source.air_dash_stages().entered&&source.air_dash_stages().timer568==7&&
                !source.air_dash_stages().contact_callback&&source.movement_programs()==std::array<u16,2>{10,10}&&
                source.relaunch_stages().lower_stage==0&&source.relaunch_stages().upper_stage==0,
                "one floor entry halves old speed but does not move/apply50/spend/refill; ret1 zero48 remains airborne in program10");
            NativeBorgMotionReceipt receipt;auto wrong=offer;wrong.kind=NativeBorgMotionKind::AirDashWindupEntry;
            check(!world.motion_receipt(wrong,&receipt),"ordinary entry cannot borrow new floor entry receipt");
        }
        if(offer.kind==NativeBorgMotionKind::AirDashWindupFloor){
            check(tick>=82&&tick<=88&&source.windup_entry_floor_contacts()==1&&offer.vertical_speed==0&&
                source.movement_programs()==std::array<u16,2>{10,10}&&source.resource()==(tick==88?7850:8300),
                "continued snaps are distinct, do not repeat entry, and spend only at timer handoff");
            if(tick==88){handoff=tick;check(source.selector()==16&&!source.air_dash_stages().entered,"timer selects16 before final wind-up contact");}
        }
        if(offer.kind==NativeBorgMotionKind::AirDashActiveEntryFloor){
            active=tick;check(active==89&&source.resource()==7800&&source.movement_programs()==std::array<u16,2>{11,11}&&
                offer.vertical_speed==bits_to_f32(0xbd8f5c29u),"active entry remains separate and retains -.07 after null contact");
        }
        if(offer.kind==NativeBorgMotionKind::JumpTouchdown){
            touchdown=tick;check(touchdown==119&&source.resource()==6300&&source.selector()==7&&world.actors()[0].grounded&&
                source.movement_programs()==std::array<u16,2>{11,11},"later minimum timer installs landing callback; no earlier floor-entry refill");
        }
    }
    check(entry==81&&handoff==88&&active==89&&touchdown==119&&entries==1&&source.relaunches()==1&&source.relaunch_floor_contacts()==2&&
        source.air_dash_starts()==1&&source.windup_entry_floor_contacts()==1&&source.windup_floor_contacts()==8&&source.active_floor_contacts()==30&&
        source.landings()==1&&source.selector()==0&&source.resource()==10000&&!source.surface_pushes()&&!world.shots_spawned()&&
        world.actors()[0].health==100&&world.actors()[1].health==100,"600-frame route has one entry, preserved spending/history and separate recovery");
    const auto retired=pose;source.reject();
    check(retired.frame_scope.expired()&&!world.motion_receipt_current(retired.receipt),"retiring entry producer revokes retained pose and receipt");
    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v6",world,seed,&error,NativeGRedGroundDashDemo::Mode::WindupEntryFloor),
        "entry-floor mode cannot infer null-callback authority from older packs");
    std::printf("source wind-up entry floor/VM/resource/pose: %u checks PASS; entry=%u handoff=%u active=%u touchdown=%u; native proxies only\n",
        checks,entry,handoff,active,touchdown);
}
