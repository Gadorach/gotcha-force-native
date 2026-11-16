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
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"source walk ledge: %s (%s)\n",why,error.c_str());std::abort();}}
class Files final:public gf::IFileSystem{
public:
    bool file_size(const std::string& n,gf::u64* o)override{std::error_code e;const auto size=std::filesystem::file_size(n,e);if(e||!o)return false;*o=size;return true;}
    bool read(const std::string& n,std::vector<gf::u8>& out)override{gf::u64 size=0;if(!file_size(n,&size)||size>(32u<<20))return false;std::ifstream f(n,std::ios::binary);out.assign(std::istreambuf_iterator<char>(f),{});return out.size()==size;}
    bool exists(const std::string& n)override{return std::filesystem::exists(n);}
    bool write(const std::string&,const gf::u8*,std::size_t)override{return false;}
    bool erase(const std::string&)override{return false;}
};
}
int main(){
    using namespace gf;Files files;const std::string art="local/content/native-battle-mvp-v1",pack="local/content/source-gred-ground-v8";
    if(!files.exists(pack+"/motion.gfmm")){std::puts("source walk ledge: optional qualified GFMM8 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);
    check(render.initialize({1152,648,false,true,"owned frame-start walk ledge"},nullptr,&error),"shared headless renderer");
    NativeBattleMvp world;NativeBattleMvpView view;NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(view.load(files,art,render,&error),"original stage and unchanged normal models");
    BattleMvpConfig config;config.arena_half=4500;config.spawn[0]={1010.166015625f,2315.240722656f};config.spawn_query_y[0]=482.747558594f;
    auto bind=[&](NativeGRedGroundDashDemo::Mode mode){
        check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&source.bind(files,art,pack,world,seed,&error,mode),
            "native placement/reference on original roof249; not an original actor spawn");
        check(!source.ground_departures()&&!source.ground_loss_entries()&&!source.ground_loss_landings()&&source.ground_loss_timer698()==0,
            "fresh owned history/timer counters");
    };
    auto pad_at=[&](unsigned t){
        GamePadState pad;pad.dpad_up=t>=5&&(!source.ground_loss_entries()||source.ground_loss_timer698()>0);return pad;
    };
    auto prepare=[&](GamePadState pad,NativeBorgMotionOffer* offer){
        NativeGRedGroundInput input;const auto camera=NativeBattleMvpView::movement_frame(world,1152,648);
        return source.map_input(pad,&input,&error)&&source.prepare(world,view.source_binding(),camera,input.x,input.y,input.held,offer,&error);
    };
    auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto camera=NativeBattleMvpView::movement_frame(world,1152,648);
        check(world.step({},false,&view,nullptr,&camera,&offer)&&source.commit(world,view.source_binding(),&error)&&
            source.pose(world,&pose)&&view.update_source(world,pose,&error),"exact owned receipt before history/VM/timer/resource/pose publication");
    };
    bind(NativeGRedGroundDashDemo::Mode::Ground);bool refused=false;
    for(unsigned t=0;t<120&&!refused;++t){
        NativeBorgMotionOffer offer;const auto tick=world.tick();const auto meter=source.resource();const auto prior=pose;
        if(!prepare(pad_at(t),&offer)){
            check(tick==84&&world.tick()==tick&&source.resource()==meter&&!source.ground_departures()&&
                prior.frame_scope.expired()&&!world.motion_receipt_current(prior.receipt)&&!world.step({},false,&view),
                "older ground mode still refuses ledge with no partial state/fallback and retires prior leases");
            refused=true;
        }else accept(offer);
    }
    check(refused,"GFMM8 data alone did not enable an older actor policy");
    bind(NativeGRedGroundDashDemo::Mode::WalkLedge);u32 departure=0,entry=0,drift=0,touchdown=0,landing_entry=0;BattleMvpVec departure_start{};
    BattlePoint fall_facing{};unsigned direct_frames=0;
    for(unsigned t=0;t<600;++t){
        NativeBorgMotionOffer offer;const auto timer=source.ground_loss_timer698();const auto flags=source.movement_flags();
        const auto departures=source.ground_departures(),entries=source.ground_loss_entries();const auto prior=pose;const auto meter=source.resource();
        check(prepare(pad_at(t),&offer)&&source.ground_loss_timer698()==timer&&source.movement_flags()==flags&&
            source.ground_departures()==departures&&source.ground_loss_entries()==entries&&source.resource()==meter,
            "frame snapshot, priority selection, flag writes and timer remain staged");
        if(offer.kind==NativeBorgMotionKind::GroundLossEntry){
            auto wrong=offer;wrong.walk_ledge->frame_start=departure_start;const auto tick=world.tick();
            const auto camera=NativeBattleMvpView::movement_frame(world,1152,648);
            check(!world.step({},false,&view,nullptr,&camera,&wrong)&&world.tick()==tick&&source.ground_loss_entries()==entries&&
                source.movement_flags()==flags&&!prior.frame_scope.expired()&&world.motion_receipt_current(prior.receipt),
                "previous movement's start is not the new frame-start history; native rejection publishes nothing and preserves live prior frame");
        }
        accept(offer);const auto tick=world.tick();
        check(offer.walk_ledge&&offer.walk_ledge->frame_start==offer.observed.position,"every role is bound to the exact observed native frame start");
        if(offer.kind==NativeBorgMotionKind::GroundDeparture){
            departure=tick;departure_start=offer.observed.position;fall_facing=offer.facing;
            check(departure==85&&offer.proposed_position[1]==offer.observed.position[1]&&offer.vertical_speed==0&&!world.actors()[0].grounded&&
                source.selector()==2&&source.movement_programs()==std::array<u16,2>{1,1}&&source.cached_airborne87()==0&&source.movement_flags()==0x10&&
                source.ground_loss_timer698()==0&&offer.walk_ledge->surface.height<offer.proposed_position[1]-60,
                "last ground motion retains Y/walking VM/cache0; no invented takeoff or immediate lower falling callback");
        }
        if(offer.kind==NativeBorgMotionKind::GroundLossEntry){
            entry=tick;
            check(entry==86&&offer.proposed_position[1]==offer.observed.position[1]&&offer.vertical_speed==-1&&!world.actors()[0].grounded&&
                source.selector()==6&&source.movement_programs()==std::array<u16,2>{13,13}&&source.cached_airborne87()==1&&source.movement_flags()==0x40&&
                source.ground_loss_timer698()==30&&!offer.walk_ledge->drift_heading,
                "higher-priority correction then program13/air flag/cache1; old zero48 integrates before -1, timer starts30");
        }
        if(offer.kind==NativeBorgMotionKind::GroundLossEntry||offer.kind==NativeBorgMotionKind::GroundLossAir){
            check(offer.facing==fall_facing&&source.resource()==10000&&!source.landings(),"fall retains body heading and meter without takeoff/flight spending");
            if(source.ground_loss_timer698()>0)check(!offer.walk_ledge->drift_heading,"positive698 suppresses direct camera drift while retaining scalar damping");
            else if(offer.walk_ledge->drift_heading){drift=tick;++direct_frames;}
        }
        if(offer.kind==NativeBorgMotionKind::GroundLossTouchdown){
            touchdown=tick;
            check(touchdown==117&&world.actors()[0].grounded&&world.actors()[0].position[1]==10&&source.selector()==7&&
                source.movement_programs()==std::array<u16,2>{13,13}&&source.cached_airborne87()==1&&source.movement_flags()==0x40,
                "real flat touchdown stays on captured falling program; landing entry and cached-ground recovery are later");
        }
        if(!landing_entry&&touchdown&&tick==touchdown+1){
            landing_entry=tick;
            check(source.movement_programs()==std::array<u16,2>{8,8}&&source.cached_airborne87()==0&&source.movement_flags()==0x200,
                "scoped60228 flag portion and landing prefix run after accepted touchdown");
        }
    }
    check(departure==85&&entry==86&&drift==116&&direct_frames==1&&touchdown==117&&landing_entry==118&&
        source.ground_departures()==1&&source.ground_loss_entries()==1&&source.ground_loss_landings()==1&&source.suppressed_ground_drifts()==30&&
        source.landings()==1&&source.selector()==0&&source.cached_airborne87()==0&&source.movement_flags()==0&&source.resource()==10000&&
        !source.air_dash_starts()&&!source.relaunches()&&!source.surface_pushes()&&!world.shots_spawned()&&
        world.actors()[0].health==100&&world.actors()[1].health==100,"bounded600-frame roof-to-floor route, separate flags/cache/receipt lifetimes and stable idle");
    const auto stable=pose;const auto tick=world.tick();GamePadState jump;jump.b=true;NativeGRedGroundInput mapped;
    check(!source.map_input(jump,&mapped,&error)&&world.tick()==tick&&!stable.frame_scope.expired(),
        "walk-only input mapper refuses jump without advancing simulation or claiming a callback");
    GamePadState flick;flick.stick_x=127;
    check(!source.map_input(flick,&mapped,&error)&&world.tick()==tick,"walk-only mapper keeps dash/flick/flight refusal");
    source.reject();check(stable.frame_scope.expired()&&!world.motion_receipt_current(stable.receipt),"producer retirement revokes retained new-mode leases");
    check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v7",world,seed,&error,NativeGRedGroundDashDemo::Mode::WalkLedge),
        "new actor mode requires distinct GFMM8 ground-loss operands, not old equal numeric values");
    std::printf("source live walk-ledge/history/VM/timer/pose: %u checks PASS; departure=%u entry=%u drift=%u touchdown=%u; native proxies only\n",
        checks,departure,entry,drift,touchdown);
}
