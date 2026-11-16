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
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"combined ground ledge: %s (%s)\n",why,error.c_str());std::abort();}}
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
    if(!files.exists(pack+"/motion.gfmm")){std::puts("combined ground ledge: optional qualified GFMM9 absent");return 0;}
    headless::Render backend;RenderEngine render(backend);NativeBattleMvp world;NativeBattleMvpView view;
    NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(render.initialize({1152,648,false,true,"one ground controller"},nullptr,&error)&&view.load(files,art,render,&error),"shared renderer/original stage/models");
    enum class Route {Walk,Mixed,Recovery,Sustained};
    for(auto route:{Route::Walk,Route::Mixed,Route::Recovery,Route::Sustained}){
        BattleMvpConfig config;config.arena_half=4500;
        config.spawn[0]=route==Route::Recovery?BattlePoint{1012.360657f,1600.f}:BattlePoint{1010.166015625f,2315.240722656f};
        config.spawn_query_y[0]=482.747558594f;
        check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
            source.bind(files,art,pack,world,seed,&error,NativeGRedGroundDashDemo::Mode::GroundLedge),
            "one fresh ground world/VM/driver/producer bind per route; never airborne teleport/rebind");
        std::weak_ptr<const NativeBorgMotionProducer> producer;const auto binding=view.source_binding();const auto world_scope=world.presentation_scope();
        u32 departure=0,entry=0,drift=0,touchdown=0;unsigned direct=0;f32 scalar=0;BattlePoint fall_facing{};BattleMvpVec landed{};
        const bool dash=route==Route::Mixed||route==Route::Recovery;bool saw_walk=false,saw_stop=false,saw_dash=false;
        for(unsigned t=0;t<600;++t){
            auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
            if(route==Route::Walk&&source.ground_loss_entries())check(native_borg_movement_frame({1,0},&frame),"camera rotation during walking-origin fall");
            GamePadState pad;
            if(t>=5&&(!source.ground_loss_entries()||source.ground_loss_timer698()>0)){
                if(route==Route::Recovery){if(t==5)pad.stick_y=-127;}
                else if(route==Route::Mixed){if(t<70)pad.dpad_up=true;else if(t>=72)pad.stick_y=-127;}
                else if(route==Route::Sustained&&t>=72)pad.stick_y=-127;
                else pad.dpad_up=true;
            }
            NativeGRedGroundInput mapped;NativeBorgMotionOffer offer;const auto old_pose=pose;
            const auto roots=source.ordered_root().updates,walks=source.walk_departures(),dashes=source.dash_departures();
            const auto flags=source.movement_flags();const auto timer=source.ground_loss_timer698();
            check(source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,&offer,&error)&&
                source.ordered_root().updates==roots&&source.walk_departures()==walks&&source.dash_departures()==dashes&&
                source.movement_flags()==flags&&source.ground_loss_timer698()==timer,
                "walk/dash/input/VM/history/root/flags/counters all stay staged");
            if(!t)producer=offer.producer;
            check(!producer.owner_before(offer.producer)&&!offer.producer.owner_before(producer)&&
                !binding.owner_before(view.source_binding())&&!view.source_binding().owner_before(binding)&&
                !world_scope.owner_before(offer.observed.scope)&&!offer.observed.scope.owner_before(world_scope),
                "same producer, render binding and world across walk/stop/flick/dash/fall/landing/idle");
            auto refuses=[&](NativeBorgMotionOffer wrong,const char* why){
                const auto position=world.actors()[0].position;const auto tick=world.tick();
                check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==tick&&world.actors()[0].position==position&&
                    source.ordered_root().updates==roots&&!old_pose.frame_scope.expired()&&world.motion_receipt_current(old_pose.receipt),why);
            };
            if(route==Route::Mixed&&t==72){
                check(offer.ordered_ground&&offer.ordered_ground->phase==12&&offer.ordered_ground->move_speed==25&&
                    offer.ordered_ground->retained_speed==24.5f&&offer.ordered_ground->speed_before==0,
                    "genuine release/flick cancels stop before dispatch; exactly one ordered entry motion, not walk plus dash");
            }
            if(route==Route::Sustained&&t==72)check(!offer.ordered_ground&&!roots,
                "pushing a long-held walk past threshold without release does not fabricate the recovered flick window");
            if(offer.kind==NativeBorgMotionKind::GroundDeparture){
                departure=world.tick()+1;scalar=offer.walk_ledge->speed44;fall_facing=offer.facing;
                check(bool(offer.ordered_ground)==dash&&offer.vertical_speed==0&&offer.walk_ledge->timer698==0&&
                    offer.proposed_position[1]==offer.observed.position[1],"distinct walking versus ordered-dash evidence on unchanged-height departure");
                auto wrong=offer;wrong.walk_ledge->frame_start[2]+=1;refuses(wrong,"both origins require exact current frame-start history");
                wrong=offer;wrong.walk_ledge->surface.triangle++;refuses(wrong,"both origins independently verify static geometry");
                if(!dash){
                    wrong=offer;wrong.walk_ledge->speed44=25;refuses(wrong,"combined air budget cannot widen walking's qualified scalar slots");
                    wrong=offer;wrong.ordered_ground=NativeBorgOrderedGround{};
                    refuses(wrong,"walking cannot manufacture an ordered root/VM capability");
                }else{
                    wrong=offer;wrong.ordered_ground.reset();refuses(wrong,"ordered dash cannot disguise itself as a walking departure");
                }
            }
            if(offer.kind==NativeBorgMotionKind::GroundLossEntry){
                entry=world.tick()+1;
                check(!offer.ordered_ground&&offer.walk_ledge->speed44==f32(scalar*bits_to_f32(0x3f7d70a4u))&&
                    offer.walk_ledge->timer698==30&&offer.vertical_speed==-1&&
                    offer.proposed_position[1]==offer.observed.position[1],"one shared following priority handoff, separate from departure and takeoff");
                auto wrong=offer;wrong.walk_ledge->speed44=dash?12:24.5f;
                refuses(wrong,"falling cannot swap walking and dash momentum families");
            }
            if(offer.kind==NativeBorgMotionKind::GroundLossAir&&offer.walk_ledge->drift_heading){drift=world.tick()+1;++direct;}
            if(offer.kind==NativeBorgMotionKind::GroundLossTouchdown){touchdown=world.tick()+1;landed=offer.proposed_position;}
            check(world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error)&&
                source.pose(world,&pose)&&view.update_source(world,pose,&error),"matching native receipt before publishing one controller/pose transaction");
            saw_walk|=source.movement_programs()==std::array<u16,2>{1,1};
            saw_stop|=source.movement_programs()==std::array<u16,2>{31,31};
            saw_dash|=source.movement_programs()==std::array<u16,2>{27,27};
            if(entry&&(!touchdown||world.tick()<=touchdown))check(world.actors()[0].facing==fall_facing&&source.resource()==10000,
                "camera-relative drift does not force body heading or spend jump/flight resource");
            if(touchdown&&world.tick()>touchdown)check(world.actors()[0].position==landed,"stable shared landing/idle after native touchdown");
            if(world.tick()==entry&&entry)check(source.movement_programs()==std::array<u16,2>{13,13}&&
                source.movement_flags()==0x40&&source.cached_airborne87()==1,"both origins enter the same reviewed falling program and flags/cache order");
        }
        const auto expected_departure=route==Route::Mixed?80u:route==Route::Recovery?14u:85u;
        const auto expected_touchdown=route==Route::Mixed?112u:route==Route::Recovery?46u:117u;
        check(departure==expected_departure&&entry==departure+1&&touchdown==expected_touchdown&&
            source.ground_departures()==1&&source.walk_departures()==u64(!dash)&&source.dash_departures()==u64(dash)&&
            source.ground_loss_entries()==1&&source.ground_loss_landings()==1&&source.landings()==1&&source.selector()==0&&
            source.cached_airborne87()==0&&source.movement_flags()==0&&source.resource()==10000&&!world.shots_spawned()&&
            world.actors()[0].health==100&&world.actors()[1].health==100,"four600-update original-stage routes preserve distinct origins and stable idle");
        check(route==Route::Recovery?(!direct&&!drift&&!source.suppressed_ground_drifts()):
            (direct==1&&drift==entry+30&&source.suppressed_ground_drifts()==30),"same falling timer semantics do not infer an identical input history");
        check(saw_dash==dash&&(route==Route::Recovery||saw_walk)&&
            (route!=Route::Mixed||(saw_walk&&saw_stop&&saw_dash&&source.ordered_entries()==1&&source.ordered_root().updates==7)),
            "one route really walks, releases/stops and dashes without model/controller replacement");
        if(!dash)check(!source.ordered_root().updates,"walking-only route never claims ordered AObj/root authority");
        GamePadState jump;jump.b=true;NativeGRedGroundInput mapped;const auto tick=world.tick();
        check(!source.map_input(jump,&mapped,&error)&&world.tick()==tick&&!pose.frame_scope.expired(),"unified ground permission still refuses jump atomically");
        source.reject();check(pose.frame_scope.expired()&&!world.motion_receipt_current(pose.receipt),"unified producer retirement revokes pose/receipt leases");
    }
    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v8",world,seed,&error,NativeGRedGroundDashDemo::Mode::GroundLedge),
        "combined permission requires GFMM9, never silently upgrades an older pack");
    std::printf("one ground-ledge controller/walk/stop/flick/dash/history/pose: %u checks PASS; four600-update original-stage routes; native proxies only\n",checks);
}
