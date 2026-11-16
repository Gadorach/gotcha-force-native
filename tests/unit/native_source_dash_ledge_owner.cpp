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
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"dash ledge owner: %s (%s)\n",why,error.c_str());std::abort();}}
class Files final:public gf::IFileSystem{
public:
    bool file_size(const std::string& n,gf::u64* o)override{std::error_code e;const auto s=std::filesystem::file_size(n,e);if(e||!o)return false;*o=s;return true;}
    bool read(const std::string& n,std::vector<gf::u8>& o)override{gf::u64 s=0;if(!file_size(n,&s)||s>(32u<<20))return false;std::ifstream f(n,std::ios::binary);o.assign(std::istreambuf_iterator<char>(f),{});return o.size()==s;}
    bool exists(const std::string& n)override{return std::filesystem::exists(n);}
    bool write(const std::string&,const gf::u8*,std::size_t)override{return false;}
    bool erase(const std::string&)override{return false;}
};
}
int main(){
    using namespace gf;Files files;const std::string art="local/content/native-battle-mvp-v1",pack="local/content/source-gred-ground-v9";
    if(!files.exists(pack+"/motion.gfmm")){std::puts("dash ledge owner: optional GFMM9 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);NativeBattleMvp world;NativeBattleMvpView view;
    NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(render.initialize({1152,648,false,true,"owned ordered dash ledge"},nullptr,&error)&&view.load(files,art,render,&error),"shared renderer and original models/stage");
    auto bind=[&](f32 z,NativeGRedGroundDashDemo::Mode mode){
        BattleMvpConfig config;config.arena_half=4500;config.spawn[0]={1012.360657f,z};config.spawn_query_y[0]=482.747589f;
        check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
            source.bind(files,art,pack,world,seed,&error,mode),"fresh native ground spawn/reference on original roof; not an airborne teleport");
        check(!source.ordered_root().updates&&!source.ground_departures()&&!source.ground_loss_entries(),"fresh controller/history/counter state");
    };
    auto accept=[&](const NativeBorgMotionOffer& offer,const NativeBorgMovementFrame& frame){
        check(world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error)&&
            source.pose(world,&pose)&&view.update_source(world,pose,&error),"exact native geometry/root/history receipt before source and pose publication");
    };
    //Data alone never upgrades an older mode, even at the same physical edge.
    for(auto mode:{NativeGRedGroundDashDemo::Mode::Ground,NativeGRedGroundDashDemo::Mode::OrderedGround,NativeGRedGroundDashDemo::Mode::WalkLedge}){
        bind(1403.599731f,mode);bool refused=false;
        for(unsigned t=0;t<20&&!refused;++t){
            const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);NativeBorgMotionOffer offer;
            const auto tick=world.tick();const auto position=world.actors()[0].position;const auto old_pose=pose;
            if(!source.prepare(world,view.source_binding(),frame,0,t>=5?60:0,t>=5?8u<<16:0,&offer,&error)){
                check(world.tick()==tick&&world.actors()[0].position==position&&!source.ground_departures()&&
                    old_pose.frame_scope.expired()&&!world.step({},false,&view),"older modes refuse edge/flick without fake takeoff, partial state or fallback");
                refused=true;
            }else accept(offer,frame);
        }
        check(refused,"older actor policies are not widened by GFMM9");
    }
    struct Route {f32 z;unsigned release;u8 phase;u32 departure,touchdown;u64 roots;};
    const std::array<Route,4> routes={Route{1403.599731f,600,12,6,38,1},Route{1520,600,13,11,43,6},
        Route{1600,6,14,14,46,9},Route{1600,600,13,14,46,7}};
    for(unsigned index=0;index<routes.size();++index){
        const auto route=routes[index];bind(route.z,NativeGRedGroundDashDemo::Mode::DashLedge);
        std::weak_ptr<const NativeBorgOrderedRootScope> root_lease;
        std::optional<NativeBorgOrderedGround> departure_evidence;
        BattleMvpVec departure_start{},landing_position{};BattlePoint fall_facing{};s16 travel=0;
        f32 retained=0;u32 departed=0,entered=0,drift=0,touchdown=0;unsigned direct_frames=0;
        u64 roots_at_departure=0;bool held_departure=false;
        for(unsigned t=0;t<600;++t){
            auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
            if(index==3&&source.ground_loss_entries())check(native_borg_movement_frame({1,0},&frame),"rotated camera during fall fixture");
            GamePadState pad;if(t>=5&&t<route.release&&(!source.ground_loss_entries()||source.ground_loss_timer698()>0))pad.stick_y=-127;
            NativeGRedGroundInput mapped;NativeBorgMotionOffer offer;const auto old_pose=pose;
            const auto old_root=source.ordered_root();const auto old_flags=source.movement_flags();const auto old_timer=source.ground_loss_timer698();
            const auto old_departures=source.ground_departures(),old_entries=source.ground_loss_entries();
            check(source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,&offer,&error)&&
                source.ordered_root().updates==old_root.updates&&source.movement_flags()==old_flags&&source.ground_loss_timer698()==old_timer&&
                source.ground_departures()==old_departures&&source.ground_loss_entries()==old_entries,
                "native frame-start, root/VM, priority, flags, timer and counters remain staged");
            check(offer.walk_ledge&&offer.walk_ledge->frame_start==offer.observed.position,"current observed start is owned history, not the previous movement's start");
            auto refuses=[&](NativeBorgMotionOffer wrong,const char* why){
                const auto tick=world.tick();const auto position=world.actors()[0].position;
                check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==tick&&world.actors()[0].position==position&&
                    source.ordered_root().updates==old_root.updates&&source.movement_flags()==old_flags&&
                    !old_pose.frame_scope.expired()&&world.motion_receipt_current(old_pose.receipt),why);
            };
            if(offer.ordered_ground&&offer.ordered_ground->root_sampled)root_lease=offer.ordered_ground->root_scope;
            if(offer.kind==NativeBorgMotionKind::GroundDeparture){
                const auto& e=*offer.ordered_ground;departed=world.tick()+1;departure_start=offer.observed.position;
                departure_evidence=e;
                retained=e.retained_speed;travel=e.travel5ae;fall_facing=offer.facing;roots_at_departure=e.root_updates;
                held_departure=!e.root_sampled;
                check(e.phase==route.phase&&e.retained_speed==std::max(0.f,std::fma(e.change4c,1.f,e.move_speed))&&
                    offer.walk_ledge->speed44==e.retained_speed&&e.root_updates==route.roots&&offer.vertical_speed==0&&
                    offer.proposed_position[1]==offer.observed.position[1]&&offer.walk_ledge->timer698==0&&
                    offer.walk_ledge->surface.height<offer.proposed_position[1]-60,
                    "ordered entry/active/held/recovery motion precedes actual departure; retained post-motion scalar is explicit");
                auto wrong=offer;wrong.ordered_ground.reset();refuses(wrong,"dash policy cannot borrow a walking departure without ordered evidence");
                wrong=offer;wrong.walk_ledge.reset();refuses(wrong,"ordered root permission alone grants no ground-loss history");
                wrong=offer;wrong.walk_ledge->timer698=30;refuses(wrong,"no early falling timer on dash departure");
                wrong=offer;wrong.walk_ledge->frame_start[0]+=1;refuses(wrong,"edited frame-start history refuses");
                wrong=offer;wrong.walk_ledge->surface.triangle++;refuses(wrong,"native independently checks lower selected surface identity");
                wrong=offer;wrong.proposed_position[1]-=1;refuses(wrong,"departure cannot apply an early falling callback");
                if(e.move_speed!=e.retained_speed){
                    wrong=offer;wrong.walk_ledge->speed44=e.move_speed;
                    refuses(wrong,"last ground distance/root speed is not the post-motion falling scalar");
                }
                if(e.root_sampled){wrong=offer;wrong.ordered_ground->root_scope.reset();refuses(wrong,"fresh VM/root operation still needs its sealed lease at departure");}
                else check(e.control_before&1u,"held active departure carries no invented root update");
                if(route.phase==14)check(e.recovery546==1&&e.move_speed==e.root_speed&&e.change4c==-.5f,
                    "recovery leaves using owned root speed, then applies -.5 before the next priority correction");
            }
            if(offer.kind==NativeBorgMotionKind::GroundLossEntry){
                entered=world.tick()+1;
                check(!offer.ordered_ground&&!root_lease.expired()&&offer.walk_ledge->speed44==f32(retained*bits_to_f32(0x3f7d70a4u))&&
                    offer.walk_ledge->travel5ae==travel&&offer.walk_ledge->timer698==30&&offer.vertical_speed==-1&&
                    offer.proposed_position[1]==offer.observed.position[1]&&!offer.walk_ledge->drift_heading,
                    "next priority correction preserves accepted dash scalar, then program13 damps it once and integrates old zero48");
                auto wrong=offer;wrong.walk_ledge->frame_start=departure_start;
                refuses(wrong,"previous dash motion's start cannot impersonate the falling frame-start snapshot");
                wrong=offer;wrong.walk_ledge->speed44=f32(25.f*bits_to_f32(0x3f7d70a4u));
                refuses(wrong,"cannot infer falling momentum from entry distance25 or earlier rendered distance");
                wrong=offer;wrong.walk_ledge->drift_heading=0;
                refuses(wrong,"positive698 suppresses direct drift without erasing retained scalar");
                wrong=offer;wrong.ordered_ground=departure_evidence;
                refuses(wrong,"retained dash root evidence cannot authorize a fresh airborne root/controller operation");
            }
            if(offer.kind==NativeBorgMotionKind::GroundLossAir&&offer.walk_ledge->drift_heading){drift=world.tick()+1;++direct_frames;}
            if(offer.kind==NativeBorgMotionKind::GroundLossTouchdown){touchdown=world.tick()+1;landing_position=offer.proposed_position;}
            accept(offer,frame);
            const auto tick=world.tick();
            if(offer.kind==NativeBorgMotionKind::GroundDeparture){
                check(!world.actors()[0].grounded&&source.movement_programs()==std::array<u16,2>{27,27}&&
                    source.movement_flags()==0x20&&source.cached_airborne87()==0&&source.selector()==offer.ordered_ground->next_selector,
                    "departure still captures dash program27, flags20/cache0; physical air is not a takeoff/program13 receipt");
            }
            if(offer.kind==NativeBorgMotionKind::GroundLossEntry){
                check(root_lease.expired()&&source.ordered_root().updates==roots_at_departure&&source.selector()==6&&
                    source.movement_programs()==std::array<u16,2>{13,13}&&source.movement_flags()==0x40&&source.cached_airborne87()==1,
                    "accepted falling handoff retires the old root lease and atomically publishes program13/flags40/cache1");
            }
            if(entered&&tick>=entered&&(!touchdown||tick<=touchdown)){
                check(source.ordered_root().updates==roots_at_departure&&world.actors()[0].facing==fall_facing&&source.resource()==10000,
                    "falling has no extra ordered-root updates, camera-forced body heading or jump/flight resource spend");
            }
            if(touchdown&&tick==touchdown)check(source.selector()==7&&source.cached_airborne87()==1&&source.movement_flags()==0x40&&
                world.actors()[0].grounded&&landing_position[1]==10,"physical touchdown is separate from later landing flags/cache");
            if(touchdown&&tick==touchdown+1)check(source.movement_programs()==std::array<u16,2>{8,8}&&
                source.cached_airborne87()==0&&source.movement_flags()==0x200,"landing VM/flags happen on the following accepted update");
            if(touchdown&&tick>touchdown)check(world.actors()[0].position==landing_position,"stable native recovery after actual floor touchdown");
        }
        check(departed==route.departure&&entered==departed+1&&touchdown==route.touchdown&&
            source.ground_departures()==1&&source.ground_loss_entries()==1&&source.ground_loss_landings()==1&&
            source.ordered_root().updates==route.roots&&source.landings()==1&&source.selector()==0&&
            source.movement_flags()==0&&source.cached_airborne87()==0&&!world.shots_spawned()&&
            world.actors()[0].health==100&&world.actors()[1].health==100,
            "600-update original-stage entry/active/held/recovery departure routes retain their exact ownership and stable idle");
        check(route.release==6?(drift==0&&direct_frames==0&&!source.suppressed_ground_drifts()):
            (drift==entered+30&&direct_frames==1&&source.suppressed_ground_drifts()==30),
            "retained momentum and held-input timer698/direct drift are separate semantics");
        check(index==3?held_departure:!held_departure,"actual active hold is distinct from a fresh AObj/root evaluation");
        GamePadState jump;jump.b=true;NativeGRedGroundInput mapped;const auto tick=world.tick();
        check(!source.map_input(jump,&mapped,&error)&&world.tick()==tick&&!pose.frame_scope.expired(),"jump mapping still refuses without a partial frame");
        source.reject();check(pose.frame_scope.expired()&&!world.motion_receipt_current(pose.receipt),"producer retirement revokes new-mode pose/receipt authority");
    }
    //A fresh buffered flick coinciding with recovery ground loss must not run
    //the dash again before the higher-priority correction. Its later airborne
    //admission is still explicitly outside this mode.
    bind(1600,NativeGRedGroundDashDemo::Mode::DashLedge);NativeBorgMotionOffer pending_entry;
    for(unsigned t=0;t<14;++t){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);NativeBorgMotionOffer offer;
        check(source.prepare(world,view.source_binding(),frame,0,t==5?60:0,t==5?8u<<16:0,&offer,&error),"recovery departure priority fixture");
        accept(offer,frame);
    }
    check(source.ground_departures()==1&&!source.ground_loss_entries(),"accepted recovery departure before pending input/priority conflict");
    const auto priority_frame=NativeBattleMvpView::movement_frame(world,1152,648);
    check(source.prepare(world,view.source_binding(),priority_frame,60,0,1u<<16,&pending_entry,&error)&&
        pending_entry.kind==NativeBorgMotionKind::GroundLossEntry&&!pending_entry.ordered_ground,
        "ground loss wins before a newly buffered flick; no repeated ground dash or fabricated air dash");
    accept(pending_entry,priority_frame);
    const auto priority_position=world.actors()[0].position;const auto priority_roots=source.ordered_root().updates;
    NativeBorgMotionOffer unsupported;
    check(!source.prepare(world,view.source_binding(),priority_frame,0,0,0,&unsupported,&error)&&world.tick()==15&&
        world.actors()[0].position==priority_position&&source.ordered_root().updates==priority_roots&&pose.frame_scope.expired()&&
        !world.step({},false,&view),"later buffered air dash refuses atomically instead of being silently discarded or granted");
    bind(1403.599731f,NativeGRedGroundDashDemo::Mode::DashLedge);
    bool walking_refused=false;
    for(unsigned t=0;t<20&&!walking_refused;++t){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);NativeBorgMotionOffer offer;
        if(!source.prepare(world,view.source_binding(),frame,0,t>=5?32:0,t>=5?8u<<16:0,&offer,&error))walking_refused=true;
        else accept(offer,frame);
    }
    check(walking_refused&&!source.ground_departures(),"dash-ledger permission does not silently authorize a walking ledge");
    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v8",world,seed,&error,NativeGRedGroundDashDemo::Mode::DashLedge),
        "dash-ledger owner requires GFMM9 ordered-root capability");
    std::printf("owned source dash ledge/history/root/priority/fall/landing: %u checks PASS; four600-frame original-stage routes; native proxies only\n",checks);
}
