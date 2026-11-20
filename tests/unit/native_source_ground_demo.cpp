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
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"source ground demo: %s (%s)\n",why,error.c_str());std::abort();}}
class Files final:public gf::IFileSystem {
public:
    bool file_size(const std::string& name,gf::u64* out)override{std::error_code e;const auto size=std::filesystem::file_size(name,e);if(e||!out)return false;*out=size;return true;}
    bool read(const std::string& name,std::vector<gf::u8>& out)override{gf::u64 size=0;if(!file_size(name,&size)||size>(32u<<20))return false;std::ifstream f(name,std::ios::binary);out.assign(std::istreambuf_iterator<char>(f),{});return out.size()==size;}
    bool exists(const std::string& name)override{return std::filesystem::exists(name);}
    bool write(const std::string&,const gf::u8*,std::size_t)override{return false;}
    bool erase(const std::string&)override{return false;}
};
}
int main(){
    using namespace gf;Files files;headless::Render backend;RenderEngine render(backend);
    check(render.initialize({1152,648,false,true,"source ground owned fixture"},nullptr,&error),"shared headless renderer");
    NativeBattleMvp world;NativeBattleMvpView view;NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;
    check(!view.source_seed(world,&seed),"unevaluated assets/world do not provide a source seed");
    NativeGRedPracticeWorldMatrices keys;
    keys.tick=999;
    check(!view.observe_gred_source_world(world,&keys)&&keys.tick==999,"no key snapshot from unevaluated assets");
    if(!files.exists("local/content/source-gred-ground-v1/locomotion.gfmp")){std::printf("source ground demo: %u checks PASS; optional original inputs absent\n",checks);return 0;}
    const std::string art="local/content/native-battle-mvp-v1",inputs="local/content/source-gred-ground-v1";
    check(view.load(files,art,render,&error)&&world.start({},&view)&&view.update(world,&error),"real shared stage/models and initialized native actors");
    check(!view.observe_gred_source_world(world,&keys),"provisional duel pose is not a source practice frame");
    NativeBattleMvp neighbor;check(neighbor.start({},&view),"same-model neighboring world");
    check(!view.source_seed(neighbor,&seed),"renderer seed cannot be borrowed by same-model neighbor with identical coordinates");
    check(view.source_seed(world,&seed)&&source.bind(files,art,inputs,world,seed,&error),"same-world/render-instance source binding");
    NativeGRedGroundDashDemo foreign;
    check(!foreign.bind(files,art,inputs,neighbor,seed,&error),"foreign world cannot bind same source pose seed");
    NativeBorgSourcePose previous;bool moved=false,had_receipt=false;
    const auto start=world.actors()[0].position;
    for(unsigned tick=0;tick<360;++tick){
        const unsigned part=tick/90,phase=tick%90;s16 x=0,y=0;u32 held=0;
        if(phase>=5&&phase<17){const std::array<s16,4> xs={0,-60,0,60},ys={60,0,-60,0};const std::array<u32,4> bits={8,1,4,2};x=xs[part];y=ys[part];held=bits[part]<<16;}
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);NativeBorgMotionOffer offer;
        check(source.prepare(world,view.source_binding(),frame,x,y,held,&offer,&error),"source input/VM/motion candidate");
        check(world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error),"shared native response before source-state commit");
        if(tick<2)check(!view.observe_gred_source_world(world,&keys),"a newer native receipt alone does not publish world keys");
        NativeBorgSourcePose pose;check(source.pose(world,&pose)&&view.update_source(world,pose,&error),"accepted frame and exact render-instance source pose");
        moved|=world.actors()[0].position!=start;
        if(tick==0){
            previous=pose;
            check(view.observe_gred_source_world(world,&keys)&&keys.tick==world.tick()&&
                  character_pose_detail::matrix(keys.key0)&&character_pose_detail::matrix(keys.key7)&&
                  !std::equal(std::begin(keys.key0.m),std::end(keys.key0.m),std::begin(keys.key7.m)),
                  "accepted source world publishes distinct affine key 0 and key 7 together");
            check(neighbor.step({},false,&view)&&neighbor.tick()==world.tick()&&
                  !view.observe_gred_source_world(neighbor,&keys)&&keys.tick==world.tick(),
                  "same-model neighbor at the same tick cannot inspect or replace current keys");
            auto missing=pose;missing.frame_scope.reset();
            check(world.motion_receipt_current(missing.receipt)&&!view.update_source(world,missing,&error)&&
                  !view.observe_gred_source_world(world,&keys),"movement receipt without the owned frame cannot publish matrices");
            check(view.update_source(world,pose,&error),"exact frame still publishes after invalid packet refusal");
            check(!view.update(world,&error),"source actor cannot render through legacy curated-pose fallback");
            check(!view.observe_gred_source_world(world,&keys),"failed legacy publication does not retain authority");
            check(view.update_source(world,pose,&error),"explicit accepted source pose restores publication");
        }
        if(tick==1){
            check(previous.frame_scope.expired()&&!view.update_source(world,previous,&error),"old producer frame cannot be replayed at next world tick");
            check(view.update_source(world,pose,&error),"current source publication after stale refusal");had_receipt=true;
        }
    }
    check(moved&&had_receipt&&world.tick()==360&&source.accepted_frames()==360&&world.actors()[0].health==100&&world.actors()[1].health==100&&!world.shots_spawned(),"scripted source movement in real native world without combat claim");
    NativeBorgSourcePose accepted;check(source.pose(world,&accepted),"current accepted pose before explicit producer retirement");
    source.reject();
    BattleMvpVec low,high;
    check(accepted.frame_scope.expired()&&!world.motion_receipt_current(accepted.receipt)&&!view.actor_bounds(0,&low,&high)&&
          !view.observe_gred_source_world(world,&keys),"producer retirement revokes old pose, receipt and key observation");
    check(!view.update_source(world,accepted,&error),"retired source cannot publish old model state");
    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&source.bind(files,art,inputs,world,seed,&error),"explicit restart rebinds source world and pose generations");
    NativeBorgMotionOffer current;
    check(source.prepare(world,view.source_binding(),NativeBattleMvpView::movement_frame(world,1152,648),0,0,0,&current,&error)&&
          world.step({},false,&view,nullptr,nullptr,&current)&&source.commit(world,view.source_binding(),&error)&&source.pose(world,&accepted)&&view.update_source(world,accepted,&error),"source frame before renderer reload");
    const auto retained_old_binding=accepted.binding.lock();
    check(view.load(files,art,render,&error),"same-model renderer reload");
    check(retained_old_binding&&retained_old_binding!=view.source_binding().lock()&&!view.update_source(world,accepted,&error),"retained old model identity cannot publish into reloaded instance");
    auto retagged=accepted;retagged.binding=view.source_binding();
    check(world.motion_receipt_current(retagged.receipt)&&!retagged.frame_scope.expired()&&
          !view.update_source(world,retagged,&error)&&!view.observe_gred_source_world(world,&keys),
          "retagging the public binding cannot replace the original frame's model owner");
    check(!source.prepare(world,view.source_binding(),NativeBattleMvpView::movement_frame(world,1152,648),0,0,0,&current,&error)&&accepted.frame_scope.expired(),"reloaded render instance retires source producer");
    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&source.bind(files,art,inputs,world,seed,&error),"fresh actor/render generation after reload refusal");
    NativeBorgMotionOffer rejected;
    check(!source.prepare(world,view.source_binding(),NativeBattleMvpView::movement_frame(world,1152,648),0,0,0x100,&rejected,&error)&&world.tick()==0,"unported jump refuses without moving world or falling back");
    const std::string ground_inputs="local/content/source-gred-ground-v2";
    if(files.exists(ground_inputs+"/motion.gfmm")){
        check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed),"fresh general-ground fixture");
        check(!source.bind(files,art,inputs,world,seed,&error,NativeGRedGroundDashDemo::Mode::Ground)&&world.tick()==0,"GFMM1 remains dash-compatible but refuses general-ground binding");
        check(source.bind(files,art,ground_inputs,world,seed,&error,NativeGRedGroundDashDemo::Mode::Ground),"GFMM2 general-ground bind");
        auto camera=NativeBattleMvpView::movement_frame(world,1152,648);
        auto step=[&](GamePadState pad){
            NativeGRedGroundInput sample;NativeBorgMotionOffer offer;NativeBorgSourcePose pose;
            check(source.map_input(pad,&sample,&error),"native source-unit input adaptation");
            check(source.prepare(world,view.source_binding(),camera,sample.x,sample.y,sample.held,&offer,&error),"source general-ground candidate");
            check(world.step({},false,&view,nullptr,&camera,&offer)&&source.commit(world,view.source_binding(),&error),"accept walking/turning/stopping before source commit");
            check(source.pose(world,&pose)&&view.update_source(world,pose,&error),"walk pose uses current accepted receipt and existing renderer");
        };
        const auto initial_facing=world.actors()[0].facing;
        check(native_borg_movement_frame({1,0},&camera),"rotate input camera without input");
        step({});
        check(world.actors()[0].facing==initial_facing&&source.selector()==0,"idle camera rotation does not force Borg body heading");
        GamePadState slow;slow.dpad_up=true;const auto initial_position=world.actors()[0].position;
        step(slow);check(source.selector()==1&&world.actors()[0].position==initial_position,"walk entry requests source program before motion");
        for(unsigned tick=0;tick<25;++tick)step(slow);
        check(source.selector()==2&&world.actors()[0].position!=initial_position,"turn completion selects steady walk without guessed clip completion");
        slow={};slow.dpad_right=true;
        const auto prior_facing=world.actors()[0].facing;step(slow);
        check(source.selector()==2&&world.actors()[0].facing!=prior_facing,"held walk steering rotates body independently of camera");
        step({});check(source.selector()==3&&world.actors()[0].movement_speed==0,"normal stop enters original idle-alias blend with zero speed");
        for(unsigned tick=0;tick<30;++tick)step({});
        check(source.selector()==0,"ordinary stop completes despite looping idle animation");
        step(slow);step({});check(source.selector()==3,"short walk interruption enters stop");
        step(slow);check(source.selector()==1,"held walk interrupts stop with fresh turn entry");
        GamePadState flick;flick.stick_y=-60;
        //Release before the new flick so the original source flick window resets.
        step({});step(flick);check(source.selector()==13,"source flick interrupts stop/walk into ground dash");
        for(unsigned tick=0;tick<80;++tick)step({});
        check(source.selector()==0,"dash recovery returns to general-ground idle");
        NativeBorgSourcePose live;check(source.pose(world,&live),"accepted pose before unsupported native action");
        const auto tick_before=world.tick();GamePadState unsupported;unsupported.b=true;NativeGRedGroundInput refused;
        check(!source.map_input(unsupported,&refused,&error)&&world.tick()==tick_before&&!live.frame_scope.expired()&&source.pose(world,&live),"native jump refusal neither advances simulation nor retires accepted pose");
        step({});
        source.reject();check(live.frame_scope.expired(),"general-ground producer retirement revokes retained poses");
        check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
            source.bind(files,art,ground_inputs,world,seed,&error,NativeGRedGroundDashDemo::Mode::VerticalJump),"explicit vertical-jump capability binds same native world and renderer");
        const auto spawn=world.actors()[0].position;const auto spawn_facing=world.actors()[0].facing;
        bool rose=false,fell=false,depleted=false;f32 previous_height=spawn[1];s16 previous_resource=source.resource();
        auto previous_kind=NativeBorgMotionKind::Ground;NativeBorgSourcePose previous_air;
        for(unsigned t=0;t<600;++t){
            GamePadState pad;pad.b=(t>=5&&t<30)||(t>=150&&t<450);
            NativeGRedGroundInput sample;NativeBorgMotionOffer offer;NativeBorgSourcePose pose;
            const auto committed_selector=source.selector();const auto committed_resource=source.resource();
            check(source.map_input(pad,&sample,&error)&&source.prepare(world,view.source_binding(),camera,sample.x,sample.y,sample.held,&offer,&error),"neutral source takeoff/ascent/fall/landing proposal");
            check(source.resource()==committed_resource&&source.selector()==committed_selector,"pending jump/VM/resource changes remain private until native acceptance");
            check(world.step({},false,&view,nullptr,&camera,&offer)&&source.commit(world,view.source_binding(),&error),"native vertical contact before source/resource commit");
            check(source.pose(world,&pose)&&view.update_source(world,pose,&error),"airborne source pose uses same renderer and current native receipt");
            const auto& actor=world.actors()[0];rose|=actor.position[1]>previous_height;fell|=actor.position[1]<previous_height;previous_height=actor.position[1];
            depleted|=source.resource()==0;
            check(actor.position[0]==spawn[0]&&actor.position[2]==spawn[2]&&actor.facing==spawn_facing,"vertical subset cannot silently use MVP lateral air control or camera-forced body rotation");
            if(actor.position[1]-spawn[1]>250){
                for(auto size:{std::pair<s32,s32>{1152,648},{480,800}}){
                    const auto framing=NativeBattleMvpView::camera_for(world,size.first,size.second);
                    for(const auto& framed_actor:world.actors())for(float offset:{0.f,220.f}){
                        auto point=framed_actor.position;point[1]+=offset;f32 x=0,y=0;
                        check(framing.project(point.data(),&x,&y,nullptr)&&x>0&&x<size.first&&y>0&&y<size.second,"high source ascent remains framed in landscape and portrait without moving actors");
                    }
                }
            }
            if(offer.kind==NativeBorgMotionKind::VerticalAir||offer.kind==NativeBorgMotionKind::VerticalTouchdown)
                check(source.resource()<=previous_resource,"airborne meter cannot refill from current animation/contact classification");
            if(offer.kind==NativeBorgMotionKind::VerticalLanding&&previous_kind==NativeBorgMotionKind::VerticalTouchdown)
                check(source.resource()==previous_resource,"first landing callback retains pre-update cached87 recovery history");
            previous_resource=source.resource();previous_kind=offer.kind;
            if(t==25){
                auto bad=pad;bad.stick_x=127;const auto tick=world.tick();
                check(!source.map_input(bad,&sample,&error)&&world.tick()==tick&&!pose.frame_scope.expired(),"air direction refuses before simulation without retiring accepted pose");
                NativeBorgMotionReceipt receipt;auto edited=offer;edited.vertical_speed+=1;
                check(!world.motion_receipt(edited,&receipt),"edited vertical velocity cannot borrow receipt");
                edited=offer;edited.kind=NativeBorgMotionKind::Ground;
                check(!world.motion_receipt(edited,&receipt),"edited air/ground phase cannot borrow receipt");
                previous_air=pose;
                check(native_borg_movement_frame({0,1},&camera),"camera observation may rotate during vertical motion");
                const auto frozen_tick=world.tick();GamePadState pause;pause.start=true;
                 check(world.step({pause,{}},false,&view)&&world.paused()&&world.tick()==frozen_tick&&source.pose(world,&pose)&&
                       view.observe_gred_source_world(world,&keys)&&keys.tick==frozen_tick,"pause preserves current accepted airborne frame and keys");
                check(world.step({},false,&view)&&world.step({pause,{}},false,&view)&&!world.paused()&&world.tick()==frozen_tick&&
                    source.resource()==previous_resource,"unpause does not spend resource or integrate hidden air updates");
            }
            if(t==26)check(previous_air.frame_scope.expired()&&!view.update_source(world,previous_air,&error)&&
                view.update_source(world,pose,&error),"old airborne pose cannot replay at next accepted tick");
            if(t==45){GamePadState relaunch;relaunch.b=true;
                check(!source.map_input(relaunch,&sample,&error)&&!pose.frame_scope.expired(),"unreviewed airborne relaunch refuses without retiring the displayed pose");}
        }
        check(rose&&fell&&depleted&&source.landings()==2&&source.air_frames()>0&&source.selector()==0&&source.resource()==10000&&
            world.actors()[0].grounded&&world.actors()[0].position==spawn&&!world.shots_spawned(),"release and depletion both land, reset resource using cached history and return idle without combat");
        check(source.pose(world,&live),"retained vertical-mode pose");
        source.reject();check(live.frame_scope.expired(),"vertical producer retirement revokes accepted poses");
        const std::string moving_inputs="local/content/source-gred-ground-v3";
        if(files.exists(moving_inputs+"/motion.gfmm")){
            check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed),"moving jump world fixture");
            check(!source.bind(files,art,ground_inputs,world,seed,&error,NativeGRedGroundDashDemo::Mode::Jump)&&world.tick()==0,"old pack cannot invent missing air steering operands");
            check(source.bind(files,art,moving_inputs,world,seed,&error,NativeGRedGroundDashDemo::Mode::Jump),"explicit moving-jump lease and GFMM3 bind");
            bool steered=false,drifted=false,moving_takeoff=false,neutral_camera_checked=false;const auto beginning=world.actors()[0].position;
            for(unsigned t=0;t<480;++t){
                GamePadState pad;pad.b=(t>=5&&t<35)||(t>=200&&t<230);
                pad.dpad_right=t<20;pad.dpad_up=t>=20&&t<35;pad.dpad_left=t>=35&&t<65;
                if(t>=190&&t<215)pad.dpad_left=true;
                if(t>=215&&t<230)pad.dpad_right=true;
                if(t>=230&&t<260)pad.dpad_down=true;
                auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                const bool rotate_neutral=!neutral_camera_checked&&source.selector()==5&&source.ascent_stage()==2;
                if(rotate_neutral){pad.dpad_up=pad.dpad_down=pad.dpad_left=pad.dpad_right=false;
                    check(native_borg_movement_frame({-1,0},&frame),"rotate movement camera during steady neutral ascent");}
                const auto before=world.actors()[0];NativeGRedGroundInput mapped;NativeBorgMotionOffer offer;NativeBorgSourcePose pose;
                check(source.map_input(pad,&mapped,&error),"mapped jump steering stays below flick threshold");
                check(source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,&offer,&error),"moving source jump proposal");
                const bool accepted=world.step({},false,&view,nullptr,&frame,&offer);
                if(!accepted)error="tick="+std::to_string(world.tick())+" kind="+std::to_string(unsigned(offer.kind))+
                    " before-y="+std::to_string(before.position[1])+" proposed-y="+std::to_string(offer.proposed_position[1])+
                    " old-v="+std::to_string(before.vertical_speed);
                check(accepted&&source.commit(world,view.source_binding(),&error),"moving jump native contact before VM/resource commit");
                check(source.pose(world,&pose)&&view.update_source(world,pose,&error),"moving airborne pose uses same renderer/lease path");
                const auto& after=world.actors()[0];
                if(rotate_neutral){check(after.facing==before.facing,"neutral air camera rotation cannot force body heading");neutral_camera_checked=true;}
                if(offer.kind==NativeBorgMotionKind::JumpTakeoff&&before.movement_speed>0)
                    moving_takeoff=after.movement_speed==0;
                if(offer.kind==NativeBorgMotionKind::JumpAir){
                    steered|=after.facing!=before.facing;
                    if(source.selector()==6){
                        drifted|=after.position[0]!=before.position[0]||after.position[2]!=before.position[2];
                        check(after.facing==before.facing,"ordinary G-Red falling direction does not force body rotation");
                    }
                }
            }
            check(moving_takeoff&&steered&&drifted&&neutral_camera_checked&&source.landings()==2&&source.selector()==0&&source.resource()==10000&&
                world.actors()[0].position!=beginning&&world.actors()[0].grounded,"walk-interrupt takeoff, ascent steering and falling drift complete two accepted landings");
            check(source.pose(world,&live),"retained moving jump pose");
            source.reject();check(live.frame_scope.expired(),"moving jump producer retirement revokes retained pose");
            const std::string air_inputs="local/content/source-gred-ground-v4";
            if(files.exists(air_inputs+"/motion.gfmm")){
                check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed),"source air-dash world fixture");
                check(!source.bind(files,art,moving_inputs,world,seed,&error,NativeGRedGroundDashDemo::Mode::AirDash)&&world.tick()==0,"GFMM3 cannot fabricate windup operands");
                check(source.bind(files,art,air_inputs,world,seed,&error,NativeGRedGroundDashDemo::Mode::AirDash),"distinct air-dash producer and GFMM4 bind");
                bool windup=false,active=false,recovery=false,paused_windup=false;
                for(unsigned t=0;t<480;++t){
                    GamePadState pad;pad.b=t>=5&&t<70;
                    if((t>=45&&t<87)||(t>=90&&t<132))pad.stick_y=-127;
                    NativeGRedGroundInput mapped;NativeBorgMotionOffer offer;NativeBorgSourcePose pose;
                    const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                    const auto before=world.actors()[0];const auto meter=source.resource();
                    check(source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,&offer,&error),"source buffered air-dash candidate");
                    check(source.resource()==meter,"air-dash resource changes remain private before native acceptance");
                    const bool accepted=world.step({},false,&view,nullptr,&frame,&offer);
                    if(!accepted)error="air-dash native refusal tick="+std::to_string(t)+" kind="+std::to_string(unsigned(offer.kind));
                    check(accepted&&source.commit(world,view.source_binding(),&error),"air-dash phases accepted before VM/resource commit");
                    check(source.pose(world,&pose)&&view.update_source(world,pose,&error),"original air-dash clips through same accepted-pose renderer");
                    windup|=offer.kind==NativeBorgMotionKind::AirDashWindupEntry;
                    active|=offer.kind==NativeBorgMotionKind::AirDashActiveEntry;
                    recovery|=offer.kind==NativeBorgMotionKind::AirDashRecovery;
                    if(offer.kind==NativeBorgMotionKind::AirDashWindupEntry){
                        paused_windup|=world.actors()[0].position==before.position;
                        check(source.resource()==meter,"air-dash request alone does not spend windup completion cost");
                    }
                    if(offer.kind==NativeBorgMotionKind::AirDashActiveEntry||offer.kind==NativeBorgMotionKind::AirDashActive)
                        check(source.resource()==std::max(0,int(meter)-50),"policy1 active dash spends50 each accepted update");
                    if(t==71){GamePadState relaunch=pad;relaunch.b=true;NativeGRedGroundInput ignored;
                        const auto tick=world.tick();
                        check(!source.map_input(relaunch,&ignored,&error)&&world.tick()==tick&&!pose.frame_scope.expired(),"fresh relaunch refuses without advancing or retiring accepted active-dash pose");
                    }
                }
                check(windup&&active&&recovery&&paused_windup&&source.air_dash_starts()==2&&source.air_dash_active_frames()>60&&
                    source.landings()==1&&source.selector()==0&&source.resource()==10000&&world.actors()[0].grounded&&
                    !world.shots_spawned()&&world.actors()[0].health==100,"two source dashes in one flight recover and land without a guessed per-flight dash cap");
                check(source.pose(world,&live),"retained air-dash pose");
                source.reject();check(live.frame_scope.expired(),"air-dash producer retirement revokes retained pose");
                check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
                    source.bind(files,art,air_inputs,world,seed,&error,NativeGRedGroundDashDemo::Mode::Relaunch),"distinct relaunch capability reuses same qualified pack and native actors");
                bool entry=false,handoff=false,ascent=false;const auto relaunch_spawn=world.actors()[0].position;
                for(unsigned t=0;t<480;++t){
                    GamePadState pad;pad.b=(t>=5&&t<35)||(t>=36&&t<40)||(t>=45&&t<85)||(t>=110&&t<150);
                    auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                    if(t==61)check(native_borg_movement_frame({-1,0},&frame),"rotate camera during neutral relaunch");
                    const auto before=world.actors()[0];const auto meter=source.resource();const auto count=source.relaunches();
                    NativeGRedGroundInput mapped;NativeBorgMotionOffer offer;NativeBorgSourcePose pose;
                    check(source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,&offer,&error),"fresh input reaches source readiness/resource admission");
                    check(source.resource()==meter&&source.relaunches()==count,"relaunch cost/count remain staged until native acceptance");
                    check(world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error),"native additive entry/continuation before source commit");
                    check(source.pose(world,&pose)&&view.update_source(world,pose,&error),"relaunch programs share exact renderer/frame publication");
                    const auto& after=world.actors()[0];const auto stages=source.relaunch_stages();
                    if(offer.kind==NativeBorgMotionKind::RelaunchEntry){
                        entry=true;check(source.resource()==meter-150&&offer.entry_vertical_delta==15&&
                            std::abs(after.position[1]-before.position[1]-before.vertical_speed-15)<.001f,"entry spends once and moves using old velocity plus source launch value");
                    }else if(native_borg_motion_relaunch(offer.kind))check(source.resource()==std::max(0,int(meter)-100),"continuation spends once per accepted lower callback");
                    if(stages.lower_stage==2){handoff=true;check(stages.upper_stage==2&&offer.kind==NativeBorgMotionKind::RelaunchWindup,"upper latch observes lower completion without requesting ascent early");}
                    if(offer.kind==NativeBorgMotionKind::RelaunchActive){ascent=true;check(stages.lower_stage==3&&stages.upper_stage==3,"both lanes request ascent on the following callback");}
                    if(t>=36&&t<40)check(source.relaunches()==0&&source.resource()==meter,"early fresh press is gated, not queued until readiness or charged");
                    check(after.position[0]==relaunch_spawn[0]&&after.position[2]==relaunch_spawn[2]&&
                        std::hypot(after.facing.x-before.facing.x,after.facing.z-before.facing.z)<.001f,"neutral relaunch cannot gain lateral motion or camera-forced heading beyond initial signed16 heading conversion");
                    if(t==61)check(after.facing==before.facing,"neutral relaunch camera rotation leaves body heading unchanged");
                    if(t==60){const auto tick=world.tick();GamePadState pause;pause.start=true;
                        check(world.step({pause,{}},false,&view)&&world.paused()&&source.pose(world,&pose),"pause keeps accepted relaunch pose");
                        check(world.step({},false,&view)&&world.step({pause,{}},false,&view)&&!world.paused()&&world.tick()==tick&&
                            source.resource()==meter-100,"unpause performs no hidden relaunch spend/update");}
                }
                check(entry&&handoff&&ascent&&source.relaunches()==2&&source.landings()==1&&source.selector()==0&&source.resource()==10000&&
                    world.actors()[0].position==relaunch_spawn&&!world.shots_spawned(),"two predicate-driven relaunches in one flight land and refill without a count-based jump rule");
                check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
                    source.bind(files,art,air_inputs,world,seed,&error,NativeGRedGroundDashDemo::Mode::Relaunch),"active-dash relaunch fixture");
                bool interrupted=false;
                for(unsigned t=0;t<130;++t){
                    GamePadState pad;pad.b=(t>=5&&t<35)||(t>=65&&t<115);if(t>=45&&t<70)pad.stick_x=127;
                    const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                    NativeGRedGroundInput mapped;NativeBorgMotionOffer offer;NativeBorgSourcePose pose;const auto old_selector=source.selector();
                    check(source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,&offer,&error),"source active-dash interruption candidate");
                    check(world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error)&&
                        source.pose(world,&pose)&&view.update_source(world,pose,&error),"active-dash relaunch uses same accepted movement/pose path");
                    if(offer.kind==NativeBorgMotionKind::RelaunchEntry)interrupted=old_selector==16&&source.air_dash_active_frames()<30;
                }
                check(interrupted&&source.relaunches()==1&&source.air_dash_starts()==1,"active-dash relaunch is admitted before the minimum dash timer is ready");
                check(source.pose(world,&live),"retained mid-flight relaunch-mode pose");
                GamePadState restart;restart.y=true;
                check(world.step({restart,{}},false,&view)&&!world.motion_receipt_current(live.receipt)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
                    source.bind(files,art,air_inputs,world,seed,&error,NativeGRedGroundDashDemo::Mode::Relaunch)&&live.frame_scope.expired(),"mid-flight restart revokes old relaunch world/renderer publication");
                source.reject();
                check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
                    source.bind(files,art,air_inputs,world,seed,&error,NativeGRedGroundDashDemo::Mode::Relaunch),"older relaunch-mode dash interruption refusal fixture");
                GamePadState dash_input;dash_input.stick_y=-127;
                auto source_step=[&](GamePadState pad,bool rotate=false){
                    auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                    if(rotate)check(native_borg_movement_frame({-1,0},&frame),"rotate input camera on ground-dash interruption");
                    NativeGRedGroundInput mapped;NativeBorgMotionOffer offer;NativeBorgSourcePose pose;
                    const auto count=source.dash_jumps();const auto selector=source.selector();
                     const auto meter=source.resource();const auto landing_jumps=source.landing_jumps(),landing_dashes=source.landing_dashes();
                     const auto pushes=source.surface_pushes();
                    check(source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,&offer,&error),"source ground dash/jump candidate");
                    check(source.dash_jumps()==count&&source.selector()==selector,"dash interruption count/VM/input remain staged before native response");
                     check(source.resource()==meter&&source.landing_jumps()==landing_jumps&&source.landing_dashes()==landing_dashes,"landing input/refill changes remain staged before native acceptance");
                     check(source.surface_pushes()==pushes,"push counters remain staged until exact native receipt");
                    check(world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error),"existing native takeoff/contact receipt accepts source dash interruption");
                    check(source.pose(world,&pose)&&view.update_source(world,pose,&error),"dash jump publishes through same renderer and frame lease");
                    return offer;
                };
                source_step(dash_input);
                check(source.pose(world,&live),"accepted ground-dash pose before older-mode refusal");
                const auto refused_tick=world.tick();GamePadState jump_input;jump_input.b=true;NativeGRedGroundInput ignored;
                check(!source.map_input(jump_input,&ignored,&error)&&world.tick()==refused_tick&&!live.frame_scope.expired(),"old mode still refuses dash-to-jump before simulation or pose retirement");
                NativeBorgMotionOffer unsupported;
                check(!source.prepare(world,view.source_binding(),NativeBattleMvpView::movement_frame(world,1152,648),0,0,0x100,&unsupported,&error)&&
                    world.tick()==refused_tick&&live.frame_scope.expired()&&!world.step({},false,&view),"raw unsupported old-mode request cannot fall back to MVP movement");
                for(unsigned direction=0;direction<4;++direction){
                    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
                        source.bind(files,art,air_inputs,world,seed,&error,NativeGRedGroundDashDemo::Mode::DashJump),"explicit dash-jump mode reuses qualified pack and initialized native actors");
                    dash_input={};
                    if(direction==0)dash_input.stick_y=-127;
                    if(direction==1)dash_input.stick_x=-127;
                    if(direction==2)dash_input.stick_y=127;
                    if(direction==3)dash_input.stick_x=127;
                    source_step(dash_input);check(source.selector()==13,"original directional dash entry accepted before jump");
                    if(direction==1||direction==3)for(unsigned n=0;n<5;++n)source_step(dash_input);
                    if(direction==2)for(unsigned n=0;n<30&&source.selector()!=14;++n)source_step({});
                    const auto before=world.actors()[0];const auto selector=source.selector();
                    check(selector==(direction==2?14:13),"tested immediate/active dash and recovery interruption boundaries");
                    const auto offer=source_step(jump_input,direction==3);
                    check(offer.kind==NativeBorgMotionKind::JumpTakeoff&&source.selector()==4&&source.dash_jumps()==1&&source.last_dash_jump_selector()==selector&&
                        world.actors()[0].position==before.position&&world.actors()[0].movement_speed==0&&world.actors()[0].vertical_speed==0&&world.actors()[0].grounded,
                        "ground dash selects existing takeoff without old dash advance, MVP momentum carry or early launch");
                    check(world.actors()[0].facing==before.facing,"dash jump does not let camera rotation force body heading");
                    NativeBorgMotionReceipt receipt;auto edited=offer;edited.action=BattleMvpAction::GroundDash;
                    check(!world.motion_receipt(edited,&receipt),"dash-jump packet cannot borrow a differently labelled action receipt");
                    for(unsigned n=0;n<20;++n)source_step(jump_input);
                    for(unsigned n=0;n<200&&source.selector()!=0;++n)source_step({});
                    check(source.selector()==0&&source.landings()==1&&source.dash_jumps()==1&&source.air_dash_starts()==0&&world.actors()[0].position==before.position&&
                        world.actors()[0].grounded&&world.actors()[0].health==100&&!world.shots_spawned(),"directional dash jump lands/refills without stale dash continuation or queued phantom air dash");
                    check(source.pose(world,&live),"retained dash-jump pose before retirement");
                    source.reject();check(live.frame_scope.expired(),"dash-jump retirement revokes retained renderer packet");
                }
                auto bind_landing=[&](NativeGRedGroundDashDemo::Mode mode){
                    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&source.bind(files,art,air_inputs,world,seed,&error,mode),"landing-interruption world/renderer fixture");
                };
                bind_landing(NativeGRedGroundDashDemo::Mode::DashJump);
                for(unsigned n=0;n<20;++n)source_step(jump_input);
                for(unsigned n=0;n<250&&!source.landing_entry_accepted();++n)source_step({});
                check(source.landing_entry_accepted()&&source.pose(world,&live),"accepted landing entry before older-mode refusal");
                const auto old_landing_tick=world.tick();const auto old_landing_meter=source.resource();
                check(!source.map_input(jump_input,&ignored,&error)&&world.tick()==old_landing_tick&&source.resource()==old_landing_meter&&!live.frame_scope.expired(),"older dash-jump mode retains non-destructive landing-jump refusal");
                source.reject();
                bind_landing(NativeGRedGroundDashDemo::Mode::LandingInterrupt);
                for(unsigned n=0;n<20;++n)source_step(jump_input);
                for(unsigned n=0;n<250&&source.selector()!=7;++n)source_step({});
                check(source.selector()==7&&!source.landing_entry_accepted()&&source.pose(world,&live),"current touchdown proof is not a landing-entry callback");
                const auto touchdown_tick=world.tick();const auto touchdown_meter=source.resource();
                check(!source.map_input(jump_input,&ignored,&error)&&world.tick()==touchdown_tick&&source.resource()==touchdown_meter&&!live.frame_scope.expired(),"new mode cannot skip landing entry or spend/refill on refused touchdown cancellation");
                source_step({});
                check(source.landing_entry_accepted()&&source.resource()==touchdown_meter,"first landing entry preserves previous cached87 recovery history");
                GamePadState soft_walk;soft_walk.dpad_up=true;source_step(soft_walk);
                check(source.selector()==7&&source.landing_jumps()==0&&source.landing_dashes()==0,"soft walking does not cancel incomplete source landing program");
                const auto before_rejump=world.actors()[0];const auto before_rejump_meter=source.resource();GamePadState priority_jump=jump_input;priority_jump.stick_x=127;
                const auto landing_jump=source_step(priority_jump);
                check(landing_jump.kind==NativeBorgMotionKind::JumpTakeoff&&source.selector()==4&&source.landing_jumps()==1&&source.landing_dashes()==0&&
                    world.actors()[0].position==before_rejump.position&&world.actors()[0].vertical_speed==0&&source.resource()==std::min(10000,int(before_rejump_meter)+2000),"landing jump uses normal takeoff after cached recovery, without early launch, instant refill or old landing advance");
                NativeBorgMotionReceipt landing_receipt;auto wrong_landing=landing_jump;wrong_landing.kind=NativeBorgMotionKind::JumpLanding;
                check(!world.motion_receipt(wrong_landing,&landing_receipt),"interrupted takeoff cannot borrow an un-interrupted landing receipt");
                for(unsigned n=0;n<20;++n)source_step(jump_input);
                for(unsigned n=0;n<250&&source.selector()!=0;++n)source_step({});
                check(source.selector()==0&&source.landings()==2&&source.landing_jumps()==1&&source.air_dash_starts()==0&&world.actors()[0].position==before_rejump.position,"landing jump returns through flight/landing without phantom buffered air dash");
                source.reject();
                bind_landing(NativeGRedGroundDashDemo::Mode::LandingInterrupt);
                bool emptied=false,released_in_fall=false,late_press=false,buffered_touchdown_refused=false;
                for(unsigned n=0;n<450&&!source.landing_entry_accepted();++n){
                    GamePadState pad=jump_input;
                    if(source.selector()==6&&source.resource()==0&&world.actors()[0].vertical_speed<0){
                        if(!released_in_fall){pad.b=false;released_in_fall=true;}
                        else if(!late_press){
                            if(world.actors()[0].position[1]<=120)late_press=true;
                            else pad.b=false;
                        }
                    }
                    if(source.selector()==7&&!source.landing_entry_accepted()&&late_press){
                        check(source.pose(world,&live),"retained touchdown pose before held-buffer refusal");
                        const auto tick=world.tick();
                        check(!source.map_input(jump_input,&ignored,&error)&&world.tick()==tick&&source.resource()==0&&!live.frame_scope.expired(),"held buffered jump also cannot skip landing entry, even without a new host press edge");
                        buffered_touchdown_refused=true;source_step({});break;
                    }
                    source_step(pad);emptied|=source.resource()==0;
                }
                check(emptied&&buffered_touchdown_refused&&source.landing_entry_accepted()&&source.resource()==0,"depletion and late buffered press land without an instant touchdown cancel or premature refill");
                source_step(soft_walk);
                check(source.selector()==7&&source.resource()==2000&&source.landing_dashes()==0,"ground recovery begins before landing completion, using shared2000 increment");
                source_step({});check(source.resource()==4000,"neutral landing update uses another original recovery increment");
                const auto before_land_dash=world.actors()[0];GamePadState landing_flick;landing_flick.stick_x=-127;
                const auto landed_dash=source_step(landing_flick);
                check(landed_dash.kind==NativeBorgMotionKind::Ground&&landed_dash.action==BattleMvpAction::GroundDash&&source.selector()==13&&source.landing_dashes()==1&&
                    source.landing_jumps()==0&&source.resource()==6000&&world.actors()[0].position==before_land_dash.position,"fresh landing flick uses source ground-dash entry and cached refill, not old landing VM or an air-dash charge");
                for(unsigned n=0;n<5;++n)source_step(landing_flick);
                for(unsigned n=0;n<100&&source.selector()!=0;++n)source_step({});
                check(source.selector()==0&&source.landings()==1&&source.resource()==10000&&source.air_dash_starts()==0&&!world.shots_spawned()&&world.actors()[0].health==100,"landing dash completes through existing source ground controller");
                check(source.pose(world,&live),"retained landing-interruption pose");source.reject();
                check(live.frame_scope.expired(),"landing-interruption retirement revokes retained source pose");
                const std::string surface_inputs="local/content/source-gred-ground-v5";
                if(files.exists(surface_inputs+"/motion.gfmm")){
                    BattleMvpConfig slope_config;slope_config.spawn[0]={-619.2617f,169.4453f}; //declared native placement on original triangle1297
                    auto bind_slope=[&](NativeGRedGroundDashDemo::Mode mode){
                        check(world.start(slope_config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
                            source.bind(files,art,surface_inputs,world,seed,&error,mode),"qualified original-stage slope fixture, native spawn only");
                    };
                    bind_slope(NativeGRedGroundDashDemo::Mode::LandingInterrupt);
                    source_step(jump_input);source_step({});
                    check(source.selector()==4&&source.pose(world,&live),"grounded authored takeoff before legacy re-press refusal");
                    const auto takeoff_tick=world.tick();
                    check(!source.map_input(jump_input,&ignored,&error)&&world.tick()==takeoff_tick&&!live.frame_scope.expired(),"older mode retains non-destructive takeoff re-press refusal");
                    bool legacy_slope_refused=false;
                    for(unsigned n=0;n<200&&!legacy_slope_refused;++n){
                        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);NativeGRedGroundInput mapped;NativeBorgMotionOffer offer;
                        check(source.map_input({},&mapped,&error),"legacy neutral takeoff/flight input");
                        const auto tick=world.tick();
                        if(!source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,&offer,&error)){
                            check(world.tick()==tick&&!world.step({},false,&view),"legacy slope refusal cannot advance or fall back to native gravity");
                            legacy_slope_refused=true;
                        }else{
                            NativeBorgSourcePose pose;
                            check(world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error)&&source.pose(world,&pose)&&
                                view.update_source(world,pose,&error),"legacy flat-only route stays owned until unsupported slope response");
                        }
                    }
                    check(legacy_slope_refused,"old mode cannot acquire new slope authority merely by loading GFMM5");
                    u32 first_air_tick=0;
                    for(bool repress:{false,true}){
                        bind_slope(NativeGRedGroundDashDemo::Mode::Surface);
                        const auto start=world.actors()[0].position;u32 air_tick=0;
                        check(start[1]>30&&start[1]<35,"native terrain producer confirms selected original incline, not flat test data");
                        for(unsigned t=0;t<240;++t){
                            GamePadState pad;pad.b=t<25;
                            if(repress&&t==3)pad.b=false;
                            const auto before=world.actors()[0];const auto selector=source.selector();
                            const auto offer=source_step(pad);
                            if(selector==4)check(source.selector()==4||source.selector()==5,"takeoff re-press does not admit walk/dash/jump cancellation");
                            if(native_borg_motion_phase(offer.kind)==1)check(world.actors()[0].position==before.position&&world.actors()[0].grounded,"authored takeoff continues without early launch or re-press displacement");
                            if(!air_tick&&!world.actors()[0].grounded)air_tick=world.tick();
                            if(offer.kind==NativeBorgMotionKind::JumpTouchdown){
                                check(source.surface_landings()==1&&source.last_surface_angle()==4448&&
                                    std::abs(world.actors()[0].position[1]-start[1])<.001f,"paired selected original surface supplies incline angle and exact native landing height");
                            }
                        }
                        if(!repress)first_air_tick=air_tick;
                        else check(air_tick==first_air_tick&&air_tick!=0,"takeoff re-press samples input without resetting VM completion or creating a new jump");
                        check(source.selector()==0&&source.surface_landings()==1&&source.landings()==1&&source.resource()==10000&&source.relaunches()==0&&
                            world.actors()[0].position==start&&world.actors()[0].health==100&&!world.shots_spawned(),"original-stage slope flight lands/refills through existing source pose and resource systems");
                        check(source.pose(world,&live),"retained selected-surface pose");source.reject();
                        check(live.frame_scope.expired(),"surface producer retirement revokes retained pose");
                    }
                    const std::string steep_inputs="local/content/source-gred-ground-v6";
                    if(files.exists(steep_inputs+"/motion.gfmm")){
                        BattleMvpConfig steep_config;steep_config.spawn[0]={-606.6735f,-1373.0029f};
                        auto bind_steep=[&](NativeGRedGroundDashDemo::Mode mode){
                            check(world.start(steep_config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
                                source.bind(files,art,steep_inputs,world,seed,&error,mode),"original stage triangle1320 fixture; native-only spawn");
                        };
                        bind_steep(NativeGRedGroundDashDemo::Mode::Surface);
                        bool old_refused=false;
                        for(unsigned t=0;t<150&&!old_refused;++t){
                            GamePadState pad;pad.b=(t>=5&&t<8)||(t>=9&&t<33);
                            NativeGRedGroundInput mapped;NativeBorgMotionOffer offer;
                            const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);const auto tick=world.tick();
                            check(source.map_input(pad,&mapped,&error),"older surface mode can sample reviewed flight");
                            if(!source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,&offer,&error)){
                                check(world.tick()==tick&&source.surface_pushes()==0&&!world.step({},false,&view),
                                    "GFMM6 does not widen older surface mode or permit fallback at steep contact");
                                old_refused=true;
                            }else{
                                check(world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error)&&
                                    source.pose(world,&live)&&view.update_source(world,live,&error),"older mode owns flight up to steep refusal");
                            }
                        }
                        check(old_refused,"old surface route refuses actual original steep triangle");
                        bind_steep(NativeGRedGroundDashDemo::Mode::Steep);
                        bool pushed=false;u32 push_tick=0;
                        for(unsigned t=0;t<240;++t){
                            GamePadState pad;pad.b=(t>=5&&t<8)||(t>=9&&t<33);
                            const auto meter=source.resource();const auto landings=source.landings();
                            const auto offer=source_step(pad);
                            if(offer.kind==NativeBorgMotionKind::StaticPush){
                                push_tick=world.tick();pushed=true;
                                check(offer.static_push&&offer.static_push->surface.triangle==1320&&offer.static_push->angle>10240&&
                                    source.selector()==6&&!world.actors()[0].grounded&&source.landings()==landings&&source.resource()==meter&&meter<10000,
                                    "source steep projection preserves falling VM/cached airborne resource, no landing or refill");
                                check(source.surface_pushes()==1&&source.last_push_angle()==offer.static_push->angle&&
                                    world.actors()[0].position==offer.proposed_position&&world.actors()[0].vertical_speed==f32(offer.static_push->vertical_speed*.9f),
                                    "only accepted projection commits source push counter and damped native velocity");
                                NativeBorgMotionReceipt receipt;auto edited=offer;edited.static_push->surface.triangle++;
                                check(!world.motion_receipt(edited,&receipt),"selected original plane is part of exact receipt");
                                NativeBorgStaticSurface final_surface;
                                check(view.static_surface(offer.proposed_position[0],offer.proposed_position[2],offer.observed.position[1],&final_surface)&&
                                    final_surface.triangle!=1320&&offer.proposed_position[1]>=final_surface.height-.01f,
                                    "projection can finish above lower adjacent original floor without invented source post-query or landing");
                            }
                        }
                        check(pushed&&push_tick==82&&source.surface_pushes()==1&&source.landings()==1&&source.selector()==0&&source.resource()==10000&&
                            std::abs(world.actors()[0].position[1]-10)<.001f&&!world.shots_spawned()&&world.actors()[0].health==100,
                            "bounded original steep push crosses to lower floor, then ordinary landing/refill through shared owner");
                        check(source.pose(world,&live),"current push-route pose");source.reject();
                        check(live.frame_scope.expired(),"push-route retirement revokes retained renderer lease");
                        //Original centroid with longer fall: accept its first
                        //push, then refuse the next unresolved seam penetration.
                        bind_steep(NativeGRedGroundDashDemo::Mode::Steep);
                        bool seam_refused=false;
                        for(unsigned t=0;t<120&&!seam_refused;++t){
                            GamePadState pad;pad.b=(t>=5&&t<8)||(t>=9&&t<30);
                            NativeGRedGroundInput mapped;NativeBorgMotionOffer offer;
                            const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                            check(source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,&offer,&error),
                                "reviewed numeric candidate before native seam decision");
                            const auto tick=world.tick();const auto pushes=source.surface_pushes();const auto meter=source.resource();
                            if(!world.step({},false,&view,nullptr,&frame,&offer)){
                                check(world.tick()==tick&&source.surface_pushes()==pushes&&pushes==1&&source.resource()==meter&&
                                    !source.commit(world,view.source_binding(),&error)&&!world.step({},false,&view),
                                    "unresolved original seam cannot commit staged projection, resource, counters or legacy gravity");
                                seam_refused=true;
                            }else check(source.commit(world,view.source_binding(),&error)&&source.pose(world,&live)&&view.update_source(world,live,&error),
                                "accepted pre-seam source frames keep same renderer");
                        }
                        check(seam_refused,"shortfall original seam refusal is retained, not relaxed for demo");
                    }
                }
            }
        }
    }
    std::printf("source ground world/pose/transaction ownership: %u checks PASS; real stage/models; native proxy contacts, no original whole-frame equivalence\n",checks);
}
