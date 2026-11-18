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
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"source wind-up surface: %s (%s)\n",why,error.c_str());std::abort();}}
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
    using namespace gf;Files files;const std::string art="local/content/native-battle-mvp-v1",inputs="local/content/source-gred-ground-v6";
    if(!files.exists(inputs+"/motion.gfmm")){std::puts("source wind-up surface: optional qualified original pack absent");return 0;}
    std::vector<u8> bytes;CharacterMovementProfile profile;CharacterMovementInputProfile input;
    check(files.read(inputs+"/motion.gfmm",bytes)&&character_movement_load({bytes.data(),bytes.size()},&profile,&error),"unchanged qualified GFMM6");
    check(files.read(inputs+"/movement.gfmi",bytes)&&character_movement_input_load({bytes.data(),bytes.size()},&input,&error),"unchanged resource/input profile");
    auto triangle=stage_ground_fixture::triangle(0);const f32 length=std::sqrt(10.f);
    for(auto& v:triangle.vertices)v[1]=-3*v[0];
    triangle.plane={3/length,1/length,0,0};
    StihGrid floor,empty;auto load=[&](StihGrid& grid,std::vector<StihTriangle> triangles){
        const auto data=stage_ground_fixture::bytes(triangles);check(grid.load({data.data(),data.size()},&error),"owned numeric grid");
    };
    load(floor,{triangle});load(empty,{});
    for(f32 timer:{2.f,1.f})for(bool done:{false,true}){
        GRedAirDashState state;state.selector=15;state.entered=true;state.stage=1;state.timer568=timer;
        CharacterMovementMotionState before;before.position={-200,605,-200};before.speed44=8;before.vertical48=-10;before.vertical_change50=profile.air[11];
        GRedAirDashPlan plan;
        check(gred_air_dash_plan(profile,state,before,&plan)&&plan.program==10&&plan.advance&&!plan.request&&plan.move&&
            plan.motion.vertical48==-10&&plan.motion.vertical_change50==bits_to_f32(0xbd8f5c29u)&&
            plan.state.selector==(timer==1?16:15),"continued wind-up keeps program10 and old velocity; timer, not clip completion, owns handoff");
        CharacterMovementMotionState integrated;CharacterMovementCeiling ceiling;
        check(character_movement_air_step(profile,plan.motion,1,1,false,ceiling,&integrated,&error)&&integrated.position[1]==595&&
            integrated.vertical48==f32(-10+profile.air[11]),"wind-up integrates before its -.07 acceleration, without recovery -1 or entry half-speed reset");
        CharacterAirResourceState resource;resource.value736=5000;
        if(plan.cost_after_move)check(character_air_resource_consume(resource,input.costs[plan.cost_index],input.recovery_delay,1,&resource),
            "timer completion cost is charged after movement and before contact");
        check(gred_air_dash_spent(plan,resource.value736,&state),"wind-up post-spend selector");
        CharacterMovementStaticGround ground;ground.grids={&floor,&empty,&empty};ground.parameters=stage_ground_fixture::parameters();
        ground.actor.fields_declared=true;ground.actor.current24=ground.actor.previous30=605;ground.query_height668=80;ground.step674=30;
        GRedSurfaceContact contact;GRedAirDashContactTail tail;
        check(gred_surface_contact(profile,ground,before,integrated,{0,0,true},&contact,&error)&&contact.kind==GRedSurfaceContactKind::Pushed&&
            contact.motion.vertical48==f32(integrated.vertical48*.9f)&&contact.motion.vertical48!=0&&
            contact.motion.vertical_change50==profile.air[11]&&contact.motion.speed44==f32(integrated.speed44*.9f)&&
            gred_air_dash_contact_tail(15,false,done,false,&tail)&&!tail.landing&&!tail.falling,
            "steep return0 retains acceleration and damped velocity, not source's landed-only vertical zero or a recovery/fall shortcut");
        check(resource.value736==(timer==1?4550:5000),"wind-up push never adds landing refill or repeats entry cost");
        GRedAirDashPlan next;
        check(gred_air_dash_plan(profile,state,contact.motion,&next)&&
            (timer==1?(next.program==11&&next.request&&next.motion.vertical48==0):
                       (next.program==10&&!next.request&&next.motion.vertical48==contact.motion.vertical48)),
            "after push, continue wind-up or perform a separate accepted active-entry reset");
        const auto retained=contact.motion.position;
        check(!gred_surface_contact(profile,ground,before,integrated,{0,0,false},&contact,&error)&&contact.motion.position==retained,
            "GFMM6 alone cannot widen earlier wind-up contact authority");
    }
    //Entry contact is intentionally NOT enabled by this continued-frame mode.
    CharacterMovementMotionState under;under.position={-200,595,-200};under.speed44=8;under.vertical48=-10;
    GRedAirDashState entry;entry.timer568=6;GRedAirDashPlan plan;
    check(gred_air_dash_plan(profile,entry,under,&plan)&&!plan.move&&plan.motion.speed44==4&&plan.motion.vertical48==-5,
        "separate source entry still halves old speeds without translation");
    CharacterMovementStaticGround ground;ground.grids={&floor,&empty,&empty};ground.parameters=stage_ground_fixture::parameters();
    ground.actor.fields_declared=true;ground.actor.current24=ground.actor.previous30=595;ground.query_height668=80;ground.step674=30;GRedSurfaceContact contact;
    check(!gred_surface_contact(profile,ground,under,plan.motion,{0,0,false},&contact,&error),
        "unaccepted below-plane numeric entry cannot borrow continued-frame push permission");
    headless::Render backend;RenderEngine render(backend);
    check(render.initialize({1152,648,false,true,"source wind-up surface fixture"},nullptr,&error),"shared headless renderer");
    NativeBattleMvp world;NativeBattleMvpView view;NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(view.load(files,art,render,&error),"qualified shared original stage and normal Borg models");
    BattleMvpConfig config;config.arena_half=4500;config.spawn[0]={-2302.447265625f,528.269836426f};config.spawn_query_y[0]=591.912719727f;
    auto bind=[&](NativeGRedGroundDashDemo::Mode mode,const std::string& pack){
        check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&source.bind(files,art,pack,world,seed,&error,mode),
            "explicit native wider demo bounds and real original-floor query reference; no source spawn claim");
        check(world.actors()[0].grounded&&world.actors()[0].vertical_speed==0&&world.actors()[0].position[1]>590&&
            world.actors()[0].position[1]<594&&!source.windup_pushes()&&!source.recovery_pushes()&&!source.surface_pushes(),
            "actual selected original mountain floor and fresh committed counters");
    };
    auto pad_at=[](unsigned t){GamePadState pad;pad.b=t>=5&&t<33;if(t>=79&&t<129)pad.stick_x=-127;return pad;};
    auto prepare=[&](GamePadState pad,NativeBorgMotionOffer* offer){
        NativeGRedGroundInput mapped;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        return source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,offer,&error);
    };
    auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        check(world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error)&&
            source.pose(world,&pose)&&view.update_source(world,pose,&error),"matching native wind-up receipt before VM/resource/counter/pose publication");
    };
    bind(NativeGRedGroundDashDemo::Mode::RecoverySurface,inputs);bool old_refused=false;
    for(unsigned t=0;t<180&&!old_refused;++t){
        NativeBorgMotionOffer offer;const auto tick=world.tick();const auto selector=source.selector();const auto programs=source.movement_programs();
        if(!prepare(pad_at(t),&offer)){
            check(tick==85&&selector==15&&programs==std::array<u16,2>{10,10}&&world.tick()==tick&&!source.windup_pushes()&&
                !world.step({},false,&view),"earlier recovery mode refuses actual continued wind-up push without commit or legacy fallback");
            old_refused=true;
        }else accept(offer);
    }
    check(old_refused,"same GFMM6 cannot widen earlier mode's wind-up authority");
    const std::string combined_pack="local/content/source-gred-ground-v9";
    const bool has_combined=files.exists(combined_pack+"/motion.gfmm");
    if(has_combined){
        bind(NativeGRedGroundDashDemo::Mode::GroundRecoveryPush,combined_pack);
        while(world.tick()<85){NativeBorgMotionOffer offer;check(prepare(pad_at(world.tick()),&offer),"previous combined mode follows the approach");accept(offer);}
        NativeBorgMotionOffer refused;const auto current=pose;
        check(!prepare(pad_at(85),&refused)&&world.tick()==85&&!source.windup_pushes()&&
            current.frame_scope.expired()&&!world.motion_receipt_current(current.receipt)&&!world.step({},false,&view),
            "previous combined mode still refuses continued wind-up without fallback");
    }
    for(const bool combined:{false,true}){
    if(combined&&!has_combined)continue;
    const auto mode=combined?NativeGRedGroundDashDemo::Mode::GroundWindupPush:NativeGRedGroundDashDemo::Mode::WindupSurface;
    const auto& pack=combined?combined_pack:inputs;
    bind(mode,pack);u32 first_push=0,last_push=0,handoff=0;bool separate_active=false;unsigned entry_frames=0;
    NativeBorgMotionOffer prior;
    for(unsigned t=0;t<360;++t){
        NativeBorgMotionOffer offer;const auto meter=source.resource();const auto pushes=source.windup_pushes();const auto programs=source.movement_programs();
        check(prepare(pad_at(t),&offer)&&source.resource()==meter&&source.windup_pushes()==pushes&&source.movement_programs()==programs,
             "wind-up timer/cost/contact pose remain staged before native acceptance");
        if(combined&&offer.kind==NativeBorgMotionKind::AirDashWindupPush){
            check(offer.air_motion&&prior.air_motion&&offer.static_push&&offer.controller_timer698==0.f,"combined path requires both motion and plane records");
            const auto& air=*offer.air_motion;const auto& contact=*offer.static_push;
            check(air.phase==15&&!air.entry&&!air.callback&&air.timer_before==prior.air_motion->timer568&&
                air.timer568==air.timer_before-1.f&&air.speed_before==prior.air_motion->speed44&&
                air.speed44==f32(air.move_speed*contact.damping)&&air.vertical_accel==profile.air[11]&&
                offer.vertical_speed==f32(contact.vertical_speed*contact.damping)&&offer.vertical_speed!=0.f&&
                air.resource_before==meter&&air.resource_after==meter-(air.timer568==0.f?input.costs[2]:0),
                "continued push retains damping rather than null-floor zero; timer alone decides once-only cost");
            const auto retained=pose;const auto position=world.actors()[0].position;
            const auto rejects=[&](NativeBorgMotionOffer wrong,const char* why){
                const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
                check(!world.step({},false,&view,nullptr,&frame,&wrong)&&world.tick()==t&&world.actors()[0].position==position&&
                    source.resource()==meter&&source.windup_pushes()==pushes&&source.movement_programs()==programs&&
                    !retained.frame_scope.expired()&&world.motion_receipt_current(retained.receipt),why);
            };
            auto wrong=offer;wrong.air_motion->timer568=air.timer_before;rejects(wrong,"contact cannot stall wind-up timer");
            wrong=offer;wrong.air_motion->resource_after+=1;rejects(wrong,"contact cannot change cost");
            wrong=offer;wrong.air_motion->callback=true;rejects(wrong,"wind-up cannot become a real landing callback");
            wrong=offer;wrong.vertical_speed=0;rejects(wrong,"push cannot use null-floor zero response");
            wrong=offer;wrong.controller_timer698=1;rejects(wrong,"positive698 remains unsupported");
            wrong=offer;wrong.kind=NativeBorgMotionKind::AirDashWindupEntryPush;rejects(wrong,"entry push cannot borrow continued authority");
            wrong=offer;wrong.air_motion->entry=true;rejects(wrong,"continued kind cannot fake wind-up entry");
            wrong=offer;wrong.kind=NativeBorgMotionKind::AirDashActivePush;rejects(wrong,"active push remains unsupported");
            wrong=offer;wrong.static_push->surface.normal={0,1,0};rejects(wrong,"exact selected plane is required");
        }
        accept(offer);
        if(offer.kind==NativeBorgMotionKind::AirDashWindupEntry){
            ++entry_frames;
            check(!offer.static_push&&world.actors()[0].position==offer.observed.position&&
                world.actors()[0].vertical_speed==f32(offer.observed.vertical_speed*.5f)&&source.resource()==meter&&!source.windup_pushes(),
                "entry is a distinct no-movement half-speed step, not repeated on contact");
        }
        if(offer.kind==NativeBorgMotionKind::AirDashWindupPush){
            if(!first_push){first_push=world.tick();}
            last_push=world.tick();
            check(offer.static_push&&source.movement_programs()==std::array<u16,2>{10,10}&&source.last_push_programs()==std::array<u16,2>{10,10}&&
                !world.actors()[0].grounded&&!source.landings()&&source.windup_pushes()==pushes+1&&!source.recovery_pushes()&&
                !source.dash_fall_pushes()&&!source.relaunch_fall_pushes()&&offer.static_push->surface.triangle==2536&&
                offer.static_push->vertical_speed==std::max(-30.f,f32(offer.observed.vertical_speed+profile.air[11])),
                "actual original triangle2536 push keeps program10 and wind-up -.07 acceleration, no landing/refill or history relabel");
            check((source.selector()==15&&source.resource()==meter)||(source.selector()==16&&source.resource()==meter-input.costs[2]),
                "wind-up contact neither spends early nor refills; timer-completion cost remains exactly once");
            if(source.selector()==16)handoff=world.tick();
            NativeBorgMotionReceipt receipt;auto wrong=offer;wrong.kind=NativeBorgMotionKind::AirDashRecoveryPush;
            check(!world.motion_receipt(wrong,&receipt),"recovery cannot borrow accepted wind-up receipt");
        }
        if(handoff&&world.tick()==handoff+1){
            check(offer.kind==NativeBorgMotionKind::AirDashActiveEntry&&source.movement_programs()==std::array<u16,2>{11,11}&&
                world.actors()[0].position[1]==offer.observed.position[1]&&world.actors()[0].vertical_speed==profile.air[11]&&
                source.resource()==meter-input.costs[3],"separate active-entry reset follows timer-completing push without an invented extra wind-up tick");
            separate_active=true;
        }
        prior=offer;
    }
    check(first_push==86&&last_push==87&&handoff==87&&separate_active&&entry_frames==1&&source.windup_pushes()==2&&source.surface_pushes()==2&&
        source.air_dash_starts()==1&&source.landings()==1&&source.selector()==0&&source.resource()==10000&&
        !world.shots_spawned()&&world.actors()[0].health==100&&world.actors()[1].health==100,
        "bounded original mountain route pushes twice, enters active dash/recovery and lands/refills normally");
    const auto retained=pose;source.reject();
    check(retained.frame_scope.expired()&&!world.motion_receipt_current(retained.receipt),"wind-up producer retirement revokes retained pose and receipt");
    bind(mode,pack);bool relaunched=false;
    for(unsigned t=0;t<110&&!relaunched;++t){
        auto pad=pad_at(t);const bool admitted_handoff=source.windup_pushes()&&source.selector()==16;const auto tick=world.tick();
        if(admitted_handoff)pad.b=true;
        NativeBorgMotionOffer offer;check(prepare(pad,&offer),"source candidate around completed wind-up handoff");accept(offer);
        if(admitted_handoff){
            check(offer.kind==NativeBorgMotionKind::RelaunchEntry&&source.relaunches()==1&&source.windup_pushes()==2&&world.tick()==tick+1&&
                offer.entry_vertical_delta==15&&source.movement_programs()==std::array<u16,2>{14,14},
                "source input may admit relaunch after timer completion, not while wind-up still owns continuation");
            relaunched=true;
        }
    }
    check(relaunched,"real source/native relaunch handoff after timer-completing wind-up push");source.reject();
    std::printf("%s wind-up surface: pushes=%u..%u handoff=%u\n",combined?"combined":"legacy",first_push,last_push,handoff);
    }
    check(world.start(config,&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v5",world,seed,&error,NativeGRedGroundDashDemo::Mode::WindupSurface)&&world.tick()==0,
        "legacy pack cannot fabricate wind-up steep operands");
    std::printf("source continued wind-up/timer/contact/pose: %u checks PASS; native proxies, no whole-frame equivalence\n",checks);
}
