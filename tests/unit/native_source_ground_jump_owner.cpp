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
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"ground jump owner: %s (%s)\n",why,error.c_str());std::abort();}}
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
    if(!files.exists(pack+"/motion.gfmm")){std::puts("ground jump owner: optional GFMM9 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);NativeBattleMvp world;NativeBattleMvpView view;
    NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(render.initialize({1152,648,false,true,"one ground/jump controller"},nullptr,&error)&&view.load(files,art,render,&error),"shared renderer and original model/stage inputs");
    enum class Route {Neutral,Walk,Dash,Recovery,Mixed,Depletion,WalkLedge,DashLedge};
    for(auto route:{Route::Neutral,Route::Walk,Route::Dash,Route::Recovery,Route::Mixed,Route::Depletion,Route::WalkLedge,Route::DashLedge}){
        const bool ledge=route==Route::WalkLedge||route==Route::DashLedge;
        BattleMvpConfig config;
        if(ledge){config.arena_half=4500;config.spawn[0]={1012.360657f,route==Route::WalkLedge?1520.f:1403.599731f};config.spawn_query_y[0]=482.747589f;}
        check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
            source.bind(files,art,pack,world,seed,&error,NativeGRedGroundDashDemo::Mode::GroundJump),
            "fresh native ground placement; same scoped controller through ground/jump/landing");
        std::weak_ptr<const NativeBorgMotionProducer> producer;std::weak_ptr<const NativeBorgOrderedRootScope> root_lease;
        u32 takeoff=0,ascent=0,fall=0,touchdown=0,idle=0;u64 takeoff_roots=0;bool depleted=false,spent=false,restored=false;
        bool saw_walk=false,saw_stop=false,checked_cancel=false;BattlePoint fall_facing{};BattleMvpVec landed{};
        s16 resource_at_touchdown=0;unsigned takeoff_frames=0;
        for(unsigned t=0;t<600;++t){
            auto frame=NativeBattleMvpView::movement_frame(world,1152,648);GamePadState pad;
            if(ledge){
                if(t>=5&&(!source.ground_loss_entries()||source.ground_loss_timer698()>0)){
                    if(route==Route::WalkLedge)pad.dpad_up=true;else pad.stick_y=-127;
                }
            }else if(route==Route::Mixed){
                if(t>=5&&t<25)pad.dpad_down=true;
                if(t>=27&&t<35)pad.stick_y=127;
                if(t>=35&&t<71){pad.b=true;pad.dpad_down=true;}
            }else if(route==Route::Dash||route==Route::Recovery){
                const unsigned press=route==Route::Recovery?21:17;
                if(t>=5&&t<17)pad.stick_y=127;
                if(t>=press&&t<56){pad.b=true;pad.dpad_down=true;}
            }else if(route==Route::Walk){
                if(t>=5&&t<17)pad.dpad_down=true;
                if(t>=17&&t<56){pad.b=true;pad.dpad_down=true;}
            }else pad.b=t>=5&&t<(route==Route::Depletion?330u:40u);
            if(route==Route::Neutral&&source.selector()==5&&source.ascent_stage()==2)
                check(native_borg_movement_frame({1,0},&frame),"rotating native camera during neutral ascent");
            const auto old_actor=world.actors()[0];const auto old_pose=pose;const auto old_resource=source.resource();const auto old_roots=source.ordered_root().updates;
            const auto old_jumps=source.dash_jumps();NativeGRedGroundInput mapped;NativeBorgMotionOffer offer;
            check(source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,&offer,&error)&&
                source.resource()==old_resource&&source.ordered_root().updates==old_roots&&source.dash_jumps()==old_jumps,
                "VM/resource/root/interruption state remains staged until matching native motion acceptance");
            if(!t)producer=offer.producer;
            check(!producer.owner_before(offer.producer)&&!offer.producer.owner_before(producer),"one producer across ground, jump, ledge and landing");
            auto refuses=[&](NativeBorgMotionOffer wrong,const char* why){
                const auto tick=world.tick();
                check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==tick&&world.actors()[0].position==old_actor.position&&
                    source.resource()==old_resource&&source.ordered_root().updates==old_roots&&!old_pose.frame_scope.expired()&&
                    world.motion_receipt_current(old_pose.receipt),why);
            };
            if(offer.ordered_ground&&offer.ordered_ground->root_sampled)root_lease=offer.ordered_ground->root_scope;
            if(offer.kind==NativeBorgMotionKind::JumpTakeoff){
                ++takeoff_frames;const auto& e=*offer.jump_motion;
                if(!takeoff){
                    takeoff=world.tick()+1;takeoff_roots=old_roots;
                    check(e.entry&&!offer.ordered_ground&&!offer.walk_ledge&&offer.proposed_position==old_actor.position&&
                        !offer.vertical_speed&&!e.move_speed&&!e.speed44&&!e.planar_accel&&!e.vertical_accel,
                        "takeoff selects existing program2 before old walk/dash motion, root advance, integration or launch spending");
                    auto wrong=offer;wrong.jump_motion.reset();refuses(wrong,"new jump permission requires exact phase/kinematic evidence");
                    wrong=offer;wrong.jump_motion->move_speed=25;refuses(wrong,"dash scalar cannot carry through normal class12 takeoff");
                    wrong=offer;wrong.proposed_position[0]+=1;refuses(wrong,"old ground motion cannot run before takeoff");
                    wrong=offer;wrong.jump_motion->next_selector=5;refuses(wrong,"cannot skip source takeoff completion");
                    if(old_roots)check(!root_lease.expired(),"accepted old dash root lease stays live while jump is only staged");
                }
            }
            if(offer.jump_motion){
                const auto& e=*offer.jump_motion;
                if(e.phase==5&&!ascent){
                    ascent=world.tick()+1;check(e.entry&&e.vertical_before==15&&offer.proposed_position[1]==old_actor.position[1]+15,
                        "source launch uses old15 velocity before acceleration; not an MVP hop");
                    auto wrong=offer;wrong.jump_motion->vertical_accel+=.1f;refuses(wrong,"source ascent acceleration is independently checked");
                    wrong=offer;wrong.vertical_speed+=1;refuses(wrong,"edited launch velocity refuses");
                    wrong=offer;wrong.jump_motion->speed44+=1;refuses(wrong,"edited horizontal acceleration/clamp refuses");
                }
                if(e.phase==6&&!e.ground_loss&&!fall){
                    fall=world.tick()+1;fall_facing=offer.facing;
                    check(e.entry&&e.planar_accel==0&&e.vertical_accel==-1,"release/depletion enters shared falling rather than relaunch or air dash");
                    auto wrong=offer;wrong.jump_motion->ground_loss=true;refuses(wrong,"normal jump fall cannot borrow ledge lineage/timer semantics");
                }
                if(e.phase==6&&!e.ground_loss)check(offer.facing==fall_facing,"falling camera/input do not force Borg body heading");
                if(e.phase==7&&e.next_selector==7){
                    auto wrong=offer;wrong.kind=NativeBorgMotionKind::Ground;wrong.jump_motion.reset();
                    refuses(wrong,"cannot return idle before source landing completion");
                }
                if(e.phase==6&&!e.entry&&!checked_cancel&&!pad.b){
                    auto cancel=pad;cancel.b=true;NativeGRedGroundInput rejected;
                    check(!source.map_input(cancel,&rejected,&error)&&!old_pose.frame_scope.expired(),"new airborne/relaunch/landing press refuses before simulation");
                    checked_cancel=true;
                }
            }
            const bool hit=offer.kind==NativeBorgMotionKind::JumpTouchdown||offer.kind==NativeBorgMotionKind::GroundLossTouchdown;
            if(hit){touchdown=world.tick()+1;landed=offer.proposed_position;}
            check(world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error)&&
                source.pose(world,&pose)&&view.update_source(world,pose,&error),"native geometry/phase/kinematics before VM/resource/root/pose publication");
            if(world.tick()==takeoff&&takeoff)check(source.movement_programs()==std::array<u16,2>{2,2}&&source.resource()==old_resource&&
                source.ordered_root().updates==takeoff_roots&&root_lease.expired(),"accepted takeoff retires old root authority without extra evaluation or resource cost");
            if(takeoff&&world.tick()>=takeoff)check(source.ordered_root().updates==takeoff_roots,"airborne controller never advances the interrupted ordered dash");
            if(route==Route::Neutral&&source.selector()==5&&source.ascent_stage()==2)check(world.actors()[0].facing==old_actor.facing,
                "neutral ascent does not let camera rotation force body heading");
            if(offer.jump_motion&&offer.jump_motion->phase==5)spent|=source.resource()<old_resource;
            depleted|=source.resource()==0;
            if(hit)resource_at_touchdown=source.resource();
            if(touchdown&&world.tick()==touchdown)check(source.cached_airborne87()==1&&world.actors()[0].grounded,
                "physical touchdown precedes cached-ground resource recovery/landing entry");
            if(touchdown&&world.tick()>touchdown){
                check(world.actors()[0].position==landed,"stable landing/idle after the selected real floor, no forced floor10");
                restored|=source.resource()>resource_at_touchdown;
                if(!source.selector()&&!idle)idle=world.tick();
            }
            saw_walk|=source.movement_programs()==std::array<u16,2>{1,1};
            saw_stop|=source.movement_programs()==std::array<u16,2>{31,31};
            if(offer.jump_motion){auto wrong=offer;wrong.jump_motion->resource_before^=1;NativeBorgMotionReceipt receipt;
                check(!world.motion_receipt(wrong,&receipt)&&world.motion_receipt(offer,&receipt),"receipt matches all jump evidence, not just endpoint/velocity");}
        }
        if(ledge)check(!takeoff&&!ascent&&source.ground_departures()==1&&source.ground_loss_entries()==1&&source.ground_loss_landings()==1&&
            source.walk_departures()==u64(route==Route::WalkLedge)&&source.dash_departures()==u64(route==Route::DashLedge),
            "both prior ground-loss origins remain distinct from deliberate takeoff");
        else check(takeoff&&ascent>takeoff&&takeoff_frames>1&&fall>ascent&&touchdown>fall&&idle>touchdown&&spent&&restored,
            "source takeoff/ascent/release-or-depletion/fall/landing/resources complete without fabricated actor callbacks");
        if(route==Route::Mixed)check(saw_walk&&saw_stop&&source.dash_jumps()==1&&source.last_dash_jump_selector()==13&&
            takeoff==36&&ascent==48&&fall==72&&touchdown==124&&source.ordered_root().updates==7,
            "same-world walk/stop/ordered dash/priority jump route keeps exact source ordering");
        if(route==Route::Dash)check(source.dash_jumps()==1&&source.last_dash_jump_selector()==13,"active ordered dash jump is counted only after acceptance");
        if(route==Route::Recovery)check(source.dash_jumps()==1&&source.last_dash_jump_selector()==14&&takeoff_roots==10,"recovery jump interrupts before another root/ground update");
        if(route==Route::Depletion)check(depleted,"held neutral rise really depletes source resource before recovery");
        check(source.landings()==1&&source.selector()==0&&source.resource()==10000&&source.cached_airborne87()==0&&source.movement_flags()==0&&
            !source.air_dash_starts()&&!source.relaunches()&&!world.shots_spawned()&&world.actors()[0].health==100&&world.actors()[1].health==100,
            "600-update shared-controller route returns stable idle without air-dash/relaunch/combat authority");
        source.reject();check(pose.frame_scope.expired()&&!world.motion_receipt_current(pose.receipt),"new-mode retirement revokes retained pose/receipt authority");
    }
    BattleMvpConfig edge;edge.arena_half=4500;edge.spawn[0]={1012.360657f,1403.599731f};edge.spawn_query_y[0]=482.747589f;
    check(world.start(edge,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        source.bind(files,art,pack,world,seed,&error,NativeGRedGroundDashDemo::Mode::GroundJump),"fresh ground-loss versus late-jump priority fixture");
    for(unsigned t=0;t<7;++t){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);GamePadState pad;
        if(t==5)pad.stick_y=-127;
        if(t==6)pad.b=true;
        NativeGRedGroundInput mapped;NativeBorgMotionOffer offer;
        check(source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,&offer,&error),
            "late held jump is sampled before priority selection, not used as an airborne takeoff");
        if(t==6)check(offer.kind==NativeBorgMotionKind::GroundLossEntry&&offer.jump_motion->ground_loss&&
            !offer.ordered_ground,"pending correction wins over late jump intent");
        check(world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error)&&source.pose(world,&pose),
            "priority handoff accepted without new root or jump callback");
    }
    check(source.ground_loss_entries()==1&&!source.dash_jumps()&&source.ordered_root().updates==1,"ground-loss priority does not claim dash jump");
    GamePadState late;late.b=true;NativeGRedGroundInput rejected;const auto late_position=world.actors()[0].position;
    check(!source.map_input(late,&rejected,&error)&&world.tick()==7&&!pose.frame_scope.expired(),
        "continued late jump in ground-loss falling has no relaunch authority");
    NativeBorgMotionOffer unsupported;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
    check(!source.prepare(world,view.source_binding(),frame,0,0,0x100u,&unsupported,&error)&&world.tick()==7&&
        world.actors()[0].position==late_position&&pose.frame_scope.expired()&&!world.step({},false,&view),
        "raw API cannot bypass unsupported late-jump refusal or publish partial state");
    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        source.bind(files,art,pack,world,seed,&error,NativeGRedGroundDashDemo::Mode::GroundLedge)&&
        !source.map_input(late,&rejected,&error),"older combined ground controller remains jump-refusing");
    source.reject();
    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v8",world,seed,&error,NativeGRedGroundDashDemo::Mode::GroundJump),
        "ordered ground/jump controller requires GFMM9, not a weaker jump-only data capability");
    std::printf("one ground/jump/root/interruption/kinematic/resource/pose controller: %u checks PASS; eight600-update original-stage routes; native proxies only\n",checks);
}
