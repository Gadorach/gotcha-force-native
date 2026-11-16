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
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"source air-dash floor: %s (%s)\n",why,error.c_str());std::abort();}}
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
    if(!files.exists(inputs+"/motion.gfmm")){std::puts("source air-dash floor: optional qualified GFMM7 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);
    check(render.initialize({1152,648,false,true,"source air-dash floor fixture"},nullptr,&error),"shared renderer");
    NativeBattleMvp world;NativeBattleMvpView view;NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(view.load(files,art,render,&error),"unchanged original stage and normal models");
    auto bind=[&](NativeGRedGroundDashDemo::Mode mode){
        check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
            source.bind(files,art,inputs,world,seed,&error,mode),"ordinary native spawn/bounds/target/reference and current source seed");
        check(!source.windup_floor_contacts()&&!source.active_floor_contacts()&&!source.air_dash_starts()&&!source.landings(),
            "fresh committed source counters");
    };
    auto pad_at=[](unsigned t,bool left=false){
        GamePadState pad;pad.b=t>=5&&t<31;if(t>=78&&t<105)pad.stick_x=left?-127:127;return pad;
    };
    auto prepare=[&](GamePadState pad,NativeBorgMotionOffer* offer){
        NativeGRedGroundInput mapped;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        return source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,offer,&error);
    };
    auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        check(world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error)&&
            source.pose(world,&pose)&&view.update_source(world,pose,&error),"exact native receipt before private state/resource/pose publication");
    };
    bind(NativeGRedGroundDashDemo::Mode::RelaunchFloor);bool refused=false;
    for(unsigned t=0;t<120&&!refused;++t){
        NativeBorgMotionOffer offer;const auto tick=world.tick();const auto meter=source.resource();const auto retained=pose;
        if(!prepare(pad_at(t),&offer)){
            check(tick==79&&world.tick()==tick&&source.selector()==15&&source.resource()==meter&&
                !source.windup_floor_contacts()&&!source.active_floor_contacts()&&!source.landings()&&
                retained.frame_scope.expired()&&!world.motion_receipt_current(retained.receipt)&&!world.step({},false,&view),
                "older GFMM7 mode refuses without staged cost/pose/fallback and retires its previous producer/frame lease");
            refused=true;
        }else accept(offer);
    }
    check(refused,"new operands do not widen earlier producer authority");
    bind(NativeGRedGroundDashDemo::Mode::AirDashFloor);u32 first=0,handoff=0,active=0,touchdown=0;unsigned entries=0;
    const auto acceleration=bits_to_f32(0xbd8f5c29u);
    for(unsigned t=0;t<600;++t){
        NativeBorgMotionOffer offer;const auto meter=source.resource();const auto selector=source.selector();const auto stages=source.air_dash_stages();
        const auto windup=source.windup_floor_contacts(),running=source.active_floor_contacts();const auto retained=pose;
        check(prepare(pad_at(t),&offer)&&source.resource()==meter&&source.selector()==selector&&
            source.windup_floor_contacts()==windup&&source.active_floor_contacts()==running&&source.air_dash_stages().timer568==stages.timer568,
            "wind-up/active stage, timer, spending and floor counters stay staged");
        if(native_borg_motion_air_dash_floor(offer.kind)){
            auto wrong=offer;wrong.vertical_speed=f32(offer.air_dash_floor->vertical_speed*.8f);const auto tick=world.tick();
            const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
            check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==tick&&source.resource()==meter&&
                source.windup_floor_contacts()==windup&&source.active_floor_contacts()==running&&
                !retained.frame_scope.expired()&&world.motion_receipt_current(retained.receipt),
                "relaunch damping cannot borrow either air-dash floor contract or commit its pending VM/resource state");
        }
        accept(offer);const auto tick=world.tick();
        if(offer.kind==NativeBorgMotionKind::AirDashWindupEntry){
            ++entries;check(tick==79&&!offer.air_dash_floor&&offer.proposed_position==offer.observed.position&&
                offer.vertical_speed==f32(offer.observed.vertical_speed*.5f)&&source.resource()==8550,
                "separate wind-up entry halves speed without translation, contact response or start cost");
        }
        if(native_borg_motion_air_dash_floor(offer.kind)){
            check(offer.air_dash_floor&&!offer.relaunch_floor&&!offer.static_push&&!world.actors()[0].grounded&&
                world.actors()[0].position[1]==10&&!source.landings()&&!source.air_dash_stages().contact_callback&&
                offer.air_dash_floor->acceleration==acceleration,
                "null754 floor receipt stays airborne on the selected original floor, retaining50 rather than65E64");
            if(offer.kind==NativeBorgMotionKind::AirDashWindupFloor){
                if(!first)first=tick;
                check(tick>=80&&tick<=86&&source.movement_programs()==std::array<u16,2>{10,10}&&offer.vertical_speed==0&&
                    source.resource()==(tick==86?8100:8550),"seven wind-up snaps clear48 only; no .8 response, landing or refill");
                if(tick==86){
                    handoff=tick;check(source.selector()==16&&!source.air_dash_stages().entered&&source.air_dash_stages().timer568==0,
                        "source timer spend/select16 occurs before contact while captured program10 completes this tick");
                }
            }else{
                if(!active)active=tick;
                check(tick>=87&&tick<=116&&source.selector()==16&&source.movement_programs()==std::array<u16,2>{11,11}&&
                    offer.vertical_speed==offer.air_dash_floor->vertical_speed&&offer.vertical_speed<0&&source.resource()==8100-s16(50*(tick-86)),
                    "early active snaps preserve accumulating negative speed and ordinary per-update spending");
                check((tick==87)==(offer.kind==NativeBorgMotionKind::AirDashActiveEntryFloor),
                    "only one active entry reset; continued floor contacts never replay entry");
            }
            NativeBorgMotionReceipt receipt;auto wrong=offer;wrong.air_dash_floor->surface.triangle++;
            check(!world.motion_receipt(wrong,&receipt),"edited selected floor cannot borrow an exact receipt");
        }
        if(offer.kind==NativeBorgMotionKind::JumpTouchdown){
            touchdown=tick;
            check(touchdown==117&&source.selector()==7&&world.actors()[0].grounded&&source.resource()==6550&&
                source.movement_programs()==std::array<u16,2>{11,11}&&source.air_dash_stages().contact_callback,
                "minimum active timer installs65E64 before contact; real touchdown keeps captured program11 and has no early refill");
        }
    }
    check(first==80&&handoff==86&&active==87&&touchdown==117&&entries==1&&source.air_dash_starts()==1&&source.windup_floor_contacts()==7&&
        source.active_floor_contacts()==30&&source.last_air_dash_floor_programs()==std::array<u16,2>{11,11}&&source.landings()==1&&
        source.selector()==0&&source.resource()==10000&&!source.relaunches()&&!source.surface_pushes()&&!world.shots_spawned()&&
        world.actors()[0].health==100&&world.actors()[1].health==100,
        "600-frame one-shot route stays stable through separate landing recovery, with no fabricated flight/refill/combat");
    const auto retired=pose;source.reject();
    check(retired.frame_scope.expired()&&!world.motion_receipt_current(retired.receipt),"retiring floor producer revokes retained pose and receipt");
    bind(NativeGRedGroundDashDemo::Mode::AirDashFloor);bool side_refused=false;
    for(unsigned t=0;t<140&&!side_refused;++t){
        NativeBorgMotionOffer offer;const auto tick=world.tick();const auto meter=source.resource();const auto retained=pose;
        const auto count=source.active_floor_contacts();
        if(!prepare(pad_at(t,true),&offer)){
            check(tick==104&&world.tick()==tick&&source.resource()==meter&&source.active_floor_contacts()==count&&
                !source.landings()&&retained.frame_scope.expired()&&!world.motion_receipt_current(retained.receipt),
                "leftward raised seam refuses staged cost/pose and revokes its retired producer/frame lease");
            side_refused=true;
        }else accept(offer);
    }
    check(side_refused,"new floor mode does not widen raised-side/unknown geometry authority");
    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v6",world,seed,&error,NativeGRedGroundDashDemo::Mode::AirDashFloor),
        "combined null-callback mode requires qualified GFMM7, not inferred constants from earlier packs");
    std::printf("source air-dash floor/resource/pose: %u checks PASS; windup80-86 active87-116 touchdown=%u; native proxies, no whole-frame equivalence\n",checks,touchdown);
}
