#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include "engine/core/headless_backends.hpp"
#include "engine/gameplay/native_battle_mvp_view.hpp"
#include "engine/gameplay/native_gred_ground_dash_demo.hpp"
#include "tests/unit/stage_ground_fixture.hpp"
namespace {
unsigned checks=0;std::string error;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"source wind-up entry push: %s (%s)\n",why,error.c_str());std::abort();}}
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
    if(!files.exists(inputs+"/motion.gfmm")){std::puts("source wind-up entry push: optional qualified GFMM7 absent");return 0;}
    std::vector<u8> bytes;CharacterMovementProfile profile;
    check(files.read(inputs+"/motion.gfmm",bytes)&&character_movement_load({bytes.data(),bytes.size()},&profile,&error),"unchanged qualified GFMM7");
    auto triangle=stage_ground_fixture::triangle(0);const auto length=std::sqrt(10.f);
    for(auto& vertex:triangle.vertices)vertex[1]=-3*vertex[0];
    triangle.plane={3/length,1/length,0,0};StihGrid floor,empty;
    auto load=[&](StihGrid& grid,std::vector<StihTriangle> triangles){
        const auto raw=stage_ground_fixture::bytes(triangles);check(grid.load({raw.data(),raw.size()},&error),"owned numeric grid");
    };
    load(floor,{triangle});load(empty,{});
    CharacterMovementMotionState before;before.position={-200,600,-200};before.speed44=8;before.vertical48=-20;before.planar_change4c=3;before.vertical_change50=-1;
    CharacterMovementStaticGround ground;ground.grids={&floor,&empty,&empty};ground.parameters=stage_ground_fixture::parameters();
    ground.actor.fields_declared=true;ground.actor.current24=ground.actor.previous30=600;ground.query_height668=80;ground.step674=30;
    StageGroundSurfaceResult selected;
    check(ground.surface(before.position,&selected,&error)&&selected.selected,"same selected numeric plane height, not decimal rounding guess");
    before.position[1]=selected.height;ground.actor.current24=ground.actor.previous30=selected.height;
    GRedAirDashState numeric;numeric.timer568=5;GRedAirDashPlan plan;
    check(gred_air_dash_plan(profile,numeric,before,&plan)&&plan.request&&!plan.move&&plan.program==10&&plan.cost_index==255&&
        plan.motion.position==before.position&&plan.motion.speed44==4&&plan.motion.vertical48==-10&&plan.motion.planar_change4c==0&&
        plan.motion.vertical_change50==profile.air[11],"entry halves44/48 and declares future50 without movement, acceleration or entry spend");
    GRedSurfaceContact contact;
    check(gred_air_dash_static_contact(profile,ground,before,plan.motion,15,false,true,&contact,&error)&&
        contact.kind==GRedSurfaceContactKind::Pushed&&contact.motion.position==before.position&&
        contact.motion.speed44==f32(4*profile.steep_damping)&&contact.motion.vertical48==f32(-10*profile.steep_damping)&&
        contact.motion.vertical48!=0&&contact.motion.vertical_change50==profile.air[11]&&contact.motion.planar_change4c==0,
        "steep ret0 applies separate .9 after entry half-speed, retaining50 and not invoking ret1 zero48");
    check(!gred_air_dash_static_contact(profile,ground,before,plan.motion,15,false,false,&contact,&error),
        "qualified operands alone cannot widen caller entry-push permission");
    headless::Render backend;RenderEngine render(backend);
    check(render.initialize({1152,648,false,true,"wind-up entry push fixture"},nullptr,&error),"shared headless renderer");
    NativeBattleMvp world;NativeBattleMvpView view;NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(view.load(files,art,render,&error),"unchanged original stage and normal models");
    BattleMvpConfig config;config.arena_half=4500;config.spawn[0]={-2760.075195312f,-3528.048583984f};config.spawn_query_y[0]=385.716827393f;
    auto bind=[&](NativeGRedGroundDashDemo::Mode mode,const std::string& pack){
        check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
             source.bind(files,art,pack,world,seed,&error,mode),"declared native placement/reference on original triangle145; no source spawn claim");
        check(world.actors()[0].grounded&&world.actors()[0].position[1]>385&&world.actors()[0].position[1]<386&&
            !source.windup_entry_surface_pushes()&&!source.air_dash_starts()&&!source.landings(),"fresh actor and counters");
    };
    unsigned flick=0;
    auto pad_at=[&](unsigned t,bool combined=false){
        GamePadState pad;pad.b=t>=5&&t<32&&(combined||t!=8);
        if(source.surface_pushes()&&!flick)flick=t;
        if(flick&&t<flick+27)pad.stick_y=-127;
        return pad;
    };
    auto prepare=[&](GamePadState pad,NativeBorgMotionOffer* offer){
        NativeGRedGroundInput mapped;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        return source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,offer,&error);
    };
    auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        check(world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error)&&
            source.pose(world,&pose)&&view.update_source(world,pose,&error),"exact entry/continuation receipt before private state/resource/pose publication");
    };
    bind(NativeGRedGroundDashDemo::Mode::WindupPositiveFloor,inputs);bool refused=false;
    for(unsigned t=0;t<120&&!refused;++t){
        NativeBorgMotionOffer offer;const auto tick=world.tick();const auto meter=source.resource();const auto retained=pose;
        if(!prepare(pad_at(t),&offer)){
            check(tick==80&&world.tick()==tick&&source.selector()==6&&source.surface_pushes()==1&&source.resource()==meter&&
                !source.windup_entry_surface_pushes()&&!source.air_dash_starts()&&retained.frame_scope.expired()&&
                !world.motion_receipt_current(retained.receipt)&&!world.step({},false,&view),
                "older mode refuses actual steep entry without staged cost/pose/fallback and retires old leases");
            refused=true;
        }else accept(offer);
    }
    check(refused,"older qualified floor mode retains entry-push refusal");
    const std::string combined_pack="local/content/source-gred-ground-v9";
    const bool has_combined=files.exists(combined_pack+"/motion.gfmm");
    if(has_combined){
        bind(NativeGRedGroundDashDemo::Mode::GroundRelaunchEntryPush,combined_pack);flick=0;
        while(world.tick()<80){NativeBorgMotionOffer offer;check(prepare(pad_at(world.tick(),true),&offer),"previous combined mode follows the approach");accept(offer);}
        NativeBorgMotionOffer offer;const auto retained=pose;const auto meter=source.resource();
        check(!prepare(pad_at(80,true),&offer)&&world.tick()==80&&source.surface_pushes()==1&&source.resource()==meter&&
            !source.air_dash_starts()&&!source.windup_entry_surface_pushes()&&retained.frame_scope.expired()&&
            !world.motion_receipt_current(retained.receipt)&&!world.step({},false,&view),
            "previous combined mode refuses entry contact without spending, advancing or falling back");
    }
    for(bool combined:{false,true}){
    if(combined&&!has_combined)continue;
    if(combined)view.source_practice_controls();
    bind(combined?NativeGRedGroundDashDemo::Mode::GroundWindupEntryPush:NativeGRedGroundDashDemo::Mode::WindupEntryPush,
         combined?combined_pack:inputs);
    check(!view.practice_meter(world),"no practice meter without an accepted source pose");
    flick=0;u32 entry=0,handoff=0,active=0,touchdown=0;unsigned entries=0,continued=0;
    NativeBorgMotionOffer prior;
    for(unsigned t=0;t<600;++t){
        NativeBorgMotionOffer offer;const auto meter=source.resource();const auto count=source.windup_entry_surface_pushes();
        const auto stages=source.air_dash_stages();const auto retained=pose;
        check(prepare(pad_at(t,combined),&offer)&&source.resource()==meter&&source.windup_entry_surface_pushes()==count&&
            source.air_dash_stages().timer568==stages.timer568,"entry half-speed, future50, timer and counters remain staged");
        if(combined&&t){
            const auto presented=view.practice_meter(world);
            check(presented&&presented->value==meter&&presented->limit==10000,"staged motion cannot publish its pending practice meter");
        }
        if(offer.kind==NativeBorgMotionKind::AirDashWindupEntryPush){
            const auto tick=world.tick();const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);auto wrong=offer;
            const auto position=world.actors()[0].position;
            const auto rejects=[&](const NativeBorgMotionOffer& altered,const char* why){
                check(!world.step({},false,&view,nullptr,&frame,&altered)&&world.tick()==tick&&world.actors()[0].position==position&&
                    source.resource()==meter&&source.windup_entry_surface_pushes()==count&&!retained.frame_scope.expired()&&
                    world.motion_receipt_current(retained.receipt),why);
            };
            wrong.static_push->vertical_speed+=profile.air[11];
            rejects(wrong,"entry cannot apply future acceleration or commit staged VM/resource state");
            if(combined){
                check(offer.air_motion&&offer.static_push&&prior.jump_motion&&offer.controller_timer698==0.f,
                    "combined entry retains accepted prior motion and a separate plane record");
                const auto& e=*offer.air_motion;const auto& p=*offer.static_push;
                check(e.phase==15&&e.entry&&e.stage_before==0&&e.next_stage==1&&e.next_selector==15&&!e.callback&&
                    e.speed_before==prior.jump_motion->speed44&&e.move_speed==f32(e.speed_before*.5f)&&
                    e.speed44==f32(e.move_speed*p.damping)&&e.vertical_before==f32(prior.vertical_speed*.5f)&&
                    p.vertical_speed==e.vertical_before&&e.planar_accel==0.f&&e.vertical_accel==profile.air[11]&&
                    p.integrated_position==position&&e.timer568==e.timer_before&&e.resource_before==meter&&e.resource_after==meter,
                    "entry halves incoming speeds, then applies .9 without pre-contact movement, spend or timer decrement");
                wrong=offer;wrong.air_motion->entry=false;rejects(wrong,"entry cannot borrow continuation");
                wrong=offer;wrong.air_motion->timer568-=1;rejects(wrong,"entry cannot advance the wind-up timer");
                wrong=offer;wrong.air_motion->resource_after-=450;rejects(wrong,"entry cannot charge the handoff cost early");
                wrong=offer;wrong.static_push->integrated_position[1]+=e.vertical_before;rejects(wrong,"entry cannot translate before contact");
                wrong=offer;wrong.controller_timer698=1;rejects(wrong,"positive timer remains unsupported");
                wrong=offer;wrong.kind=NativeBorgMotionKind::AirDashActivePush;rejects(wrong,"active motion cannot borrow the wind-up entry receipt");
            }
        }
        accept(offer);const auto tick=world.tick();
        if(combined){
            const auto presented=view.practice_meter(world);
            check(presented&&presented->limit==10000&&presented->value==source.resource(),"accepted source pose publishes exact meter and limit");
            if(tick==86){
                const auto frame=pose.frame_scope.lock();NativeBattleMvp foreign;
                check(frame&&!frame->meter(foreign)&&!view.practice_meter(foreign),"foreign world cannot borrow the practice meter");
                check(world.set_paused(true)&&world.set_paused(true)&&world.paused()&&world.tick()==tick&&
                    view.practice_meter(world)&&view.practice_meter(world)->value==8000,
                    "session pause requires no input tick and preserves the last accepted practice meter");
                check(world.set_paused(false)&&!world.paused()&&world.tick()==tick&&world.motion_receipt_current(pose.receipt)&&
                    view.practice_meter(world)&&view.practice_meter(world)->value==8000,
                    "session resume does not step, refill the meter or revoke its accepted receipt");
            }
        }
        if(native_borg_motion_windup_entry(offer.kind))++entries;
        if(offer.kind==NativeBorgMotionKind::AirDashWindupEntryPush){
            entry=tick;
            check(entry==81&&offer.static_push&&!offer.air_dash_floor&&!offer.relaunch_floor&&
                offer.static_push->integrated_position==offer.observed.position&&
                offer.static_push->vertical_speed==f32(offer.observed.vertical_speed*.5f)&&
                offer.static_push->surface.triangle==145&&offer.static_push->angle>10240&&
                offer.vertical_speed==f32(offer.static_push->vertical_speed*profile.steep_damping)&&offer.vertical_speed<0&&
                !world.actors()[0].grounded&&!source.landings()&&source.resource()==8450&&source.windup_entry_surface_pushes()==1&&
                source.air_dash_starts()==1&&source.selector()==15&&source.air_dash_stages().timer568==5&&
                source.movement_programs()==std::array<u16,2>{10,10},"original steep entry is half-speed then .9, not floor zero/refill or repeated movement");
            NativeBorgMotionReceipt receipt;auto wrong=offer;wrong.kind=NativeBorgMotionKind::AirDashWindupPush;
            check(!world.motion_receipt(wrong,&receipt),"continued wind-up cannot borrow exact entry receipt");
        }
        if(offer.kind==NativeBorgMotionKind::AirDashWindupPush){
            ++continued;
            NativeBorgStaticSurface selected_surface;
            check(tick>=82&&tick<=86&&offer.static_push&&
                view.static_surface(offer.static_push->integrated_position[0],offer.static_push->integrated_position[2],offer.observed.position[1],&selected_surface)&&
                selected_surface==offer.static_push->surface&&
                offer.static_push->vertical_speed==std::max(-30.f,f32(offer.observed.vertical_speed+profile.air[11]))&&
                source.windup_entry_surface_pushes()==1&&source.resource()==(tick==86?8000:8450),
                "continued wind-up reselects the actual pre-plane across authored faces and applies retained50 without replaying entry");
            if(tick==86){handoff=tick;check(source.selector()==16&&!source.air_dash_stages().entered,"deferred cost/select16 occurs before contact");}
        }
        if(offer.kind==NativeBorgMotionKind::AirDashActiveEntry){
            active=tick;check(active==87&&source.resource()==7950&&source.movement_programs()==std::array<u16,2>{11,11},
                "active entry remains separate after timer-completing wind-up push");
        }
        if(offer.kind==NativeBorgMotionKind::JumpTouchdown){
            touchdown=tick;check(touchdown==140&&source.resource()==6450&&source.selector()==7&&world.actors()[0].grounded&&
                source.movement_programs()==std::array<u16,2>{12,12},"normal recovery callback later lands without an entry refill shortcut");
        }
        prior=offer;
    }
    check(entry==81&&handoff==86&&active==87&&touchdown==140&&entries==1&&continued==5&&source.air_dash_starts()==1&&
        source.windup_entry_surface_pushes()==1&&source.windup_pushes()==6&&source.surface_pushes()==7&&
        source.landings()==1&&source.selector()==0&&source.resource()==10000&&!source.windup_floor_contacts()&&!source.active_floor_contacts()&&
        !world.shots_spawned()&&world.actors()[0].health==100&&world.actors()[1].health==100,
        "600-frame original-stage route preserves distinct entry, continuation, recovery and lifetime boundaries");
    const auto retired=pose;source.reject();
    check(retired.frame_scope.expired()&&!world.motion_receipt_current(retired.receipt),"entry producer retirement revokes retained pose and receipt");
    check(!view.practice_meter(world),"retired source scope cannot display a stale meter");
    std::printf("%s entry: entry=%u handoff=%u active=%u touchdown=%u\n",combined?"combined":"legacy",entry,handoff,active,touchdown);
    }
    if(has_combined){
        //The active reset would move horizontally into a raised face. Projection
        //math alone is not permission to cross the native pre-contact boundary.
        BattleMvpConfig raised;raised.arena_half=4500;raised.spawn[0]={-2314,528};raised.spawn_query_y[0]=591.9127f;
        check(world.start(raised,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
            source.bind(files,art,combined_pack,world,seed,&error,NativeGRedGroundDashDemo::Mode::GroundWindupEntryPush),
            "original raised-face fixture uses grounded placement, no injected motion");
        auto raised_pad=[](unsigned t){
            GamePadState pad;pad.b=t>=5&&t<32;if(t>=76&&t<107)pad.stick_x=127;return pad;
        };
        NativeBorgMotionOffer last;
        while(world.tick()<84){check(prepare(raised_pad(world.tick()),&last),"raised-face approach");accept(last);}
        const auto actor=world.actors()[0];const auto current=pose;const auto meter=source.resource();
        const auto pushes=source.surface_pushes();const auto programs=source.movement_programs();
        check(last.air_motion&&last.air_motion->phase==15&&last.air_motion->next_selector==16&&
            last.air_motion->timer568==0&&source.selector()==16&&meter==8000,
            "wind-up handoff has committed once, before the rejected active entry");
        //Fixed heading of the fixture's active-entry probe. The candidate is
        //computed only for the contact check, not applied to the actor.
        f32 x=0,z=0;source_heading_xz(-80,&x,&z);
        auto integrated=actor.position;
        integrated[0]=std::fma(1.f,f32(profile.air[5]*x),integrated[0]);
        integrated[2]=std::fma(1.f,f32(profile.air[5]*z),integrated[2]);
        NativeBorgStaticSurface plane;BattleMvpVec projected;s16 angle=0;
        check(view.static_surface(integrated[0],integrated[2],actor.position[1],&plane)&&plane.triangle==2536&&
            source_surface_angle(plane.normal,&angle)&&angle>10240&&plane.height>integrated[1]&&
            source_surface_push_position(integrated,plane.normal,plane.height,angle,&projected),
            "horizontal active-entry approach overlaps a real steep raised native plane");
        f32 projected_floor=0;
        check(view.height(projected[0],projected[2],actor.position[1],&projected_floor)&&projected[1]>=projected_floor-.01f,
            "a plausible projected endpoint does not excuse blocked pre-contact travel");
        const BattlePoint movement{integrated[0]-actor.position[0],integrated[2]-actor.position[2]};
        const auto& other=world.actors()[1];const auto& bounds=world.config();
        NativeBorgContactRequest request{actor.position,other.position,movement,bounds.arena_half,bounds.actor_radius,
            bounds.actor_height,bounds.step_height,false,other.health!=0};
        request.vertical_end_y=integrated[1];NativeBorgContactResult rejected;
        check(native_borg_resolve_contact(request,&view,&rejected)&&rejected.terrain_blocked&&!rejected.bounds_hit&&!rejected.body_hit,
            "native pre-contact travel rejects the raised surface without relying on bounds or body overlap");
        NativeBorgMotionOffer offer;
        check(!prepare(raised_pad(84),&offer)&&error.find("steep push")!=std::string::npos&&world.tick()==84&&
            world.actors()[0].position==actor.position&&source.selector()==16&&source.resource()==meter&&
            source.surface_pushes()==pushes&&source.movement_programs()==programs&&
            current.frame_scope.expired()&&!world.motion_receipt_current(current.receipt)&&!world.step({},false,&view),
            "unsupported active-entry push does not commit its reset, spend, pose or a fallback frame");
        render.begin_frame({0,.15f,.21f,.3f});render.reset_arena();
        const auto stopped_draws=view.draw_practice_stop(render,1152,648,world.tick());render.end_frame();
        check(stopped_draws==2&&world.tick()==84&&!view.practice_meter(world),
            "stopped practice draws only the text panel, not a retired pose or meter");
        bind(NativeGRedGroundDashDemo::Mode::GroundWindupEntryPush,combined_pack);
        check(world.tick()==0&&!view.practice_meter(world)&&!world.motion_receipt_current(current.receipt),
            "fresh practice round cannot inherit a stopped frame's meter or receipt");
        check(prepare({},&offer),"fresh source input after restart");accept(offer);
        const auto fresh=view.practice_meter(world);
        check(world.tick()==1&&fresh&&fresh->value==10000&&fresh->limit==10000,
            "fresh movement and flight meter resume only on a new accepted source frame");
        source.reject();
        std::puts("combined active entry: original raised-face approach refused at tick=85");
    }
    check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v6",world,seed,&error,NativeGRedGroundDashDemo::Mode::WindupEntryPush),
        "combined entry-push mode retains qualified GFMM7 floor/caller requirements");
    std::printf("source wind-up entry push/VM/resource/pose: %u checks PASS; native proxies only\n",checks);
}
