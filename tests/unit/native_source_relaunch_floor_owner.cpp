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
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"source relaunch floor: %s (%s)\n",why,error.c_str());std::abort();}}
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
    if(!files.exists(inputs+"/motion.gfmm")){std::puts("source relaunch floor: optional qualified GFMM7 absent");return 0;}
    std::vector<u8> bytes;CharacterMovementProfile profile;CharacterMovementInputProfile input;
    check(files.read(inputs+"/motion.gfmm",bytes)&&character_movement_load({bytes.data(),bytes.size()},&profile,&error)&&profile.relaunch_floor_operands,
        "qualified GFMM7 caller operand");
    check(files.read(inputs+"/movement.gfmi",bytes)&&character_movement_input_load({bytes.data(),bytes.size()},&input,&error),"unchanged input/resource profile");
    StihGrid floor,empty;auto load=[&](StihGrid& grid,std::vector<StihTriangle> triangles){
        const auto data=stage_ground_fixture::bytes(triangles);check(grid.load({data.data(),data.size()},&error),"owned numeric flat grid");
    };
    load(floor,{stage_ground_fixture::triangle(0)});load(empty,{});
    for(bool entry:{false,true})for(bool direction:{false,true})for(bool crosses_zero:{false,true}){
        CharacterMovementMotionState before;before.position={-200,crosses_zero?.05f:5.f,-200};before.speed44=4;
        before.vertical48=entry?-30.f:crosses_zero?-.1f:-10.f;
        GRedRelaunchState state=entry?GRedRelaunchState{}:GRedRelaunchState{1,1};GRedRelaunchPlan plan;
        check(gred_relaunch_plan(profile,state,before,&plan),"existing relaunch plan");
        auto accelerated=plan.motion;check(gred_jump_acceleration(profile,direction,&accelerated),"reviewed jump acceleration");
        CharacterMovementCeiling ceiling;CharacterMovementMotionState proposed;
        check(character_movement_air_step(profile,accelerated,1,1,true,ceiling,&proposed,&error),"shared clamped source integration");
        CharacterMovementStaticGround ground;ground.grids={&floor,&empty,&empty};ground.parameters=stage_ground_fixture::parameters();
        ground.actor.fields_declared=true;ground.actor.current24=ground.actor.previous30=before.position[1];ground.query_height668=80;ground.step674=30;
        GRedSurfaceContact contact;
        check(gred_relaunch_static_contact(profile,ground,before,proposed,&contact,&error)&&contact.kind==GRedSurfaceContactKind::Snapped&&
            contact.motion.position[1]==0&&contact.motion.vertical48==(proposed.vertical48<0?f32(proposed.vertical48*profile.relaunch_floor_damping):proposed.vertical48)&&
            contact.motion.vertical_change50==proposed.vertical_change50&&contact.motion.speed44==proposed.speed44&&
            contact.motion.planar_change4c==proposed.planar_change4c,
            "null754 snaps height; only negative48 gets caller .8, preserving50/44/4C rather than push or65E64");
        GRedRelaunchUpperPlan upper;GRedRelaunchState lower;
        check(gred_relaunch_lower_finished(plan,false,&lower)&&gred_relaunch_upper_plan(lower,&upper)&&upper.program==14,
            "floor snap does not reset lower/upper latches or request landing animation");
        GRedSurfaceContact normal;
        check(gred_surface_contact(profile,ground,before,proposed,{0,0,true,true},&normal,&error)&&normal.kind==GRedSurfaceContactKind::Landed&&
            normal.motion.vertical48==0&&normal.motion.vertical_change50==0,"normal callback candidate remains explicitly distinct from null callback");
        auto legacy=profile;legacy.relaunch_floor_operands=false;
        check(!gred_relaunch_static_contact(legacy,ground,before,proposed,&normal,&error),"GFMM6 cannot fabricate null-callback caller response");
    }
    headless::Render backend;RenderEngine render(backend);
    check(render.initialize({1152,648,false,true,"source relaunch floor fixture"},nullptr,&error),"shared headless renderer");
    NativeBattleMvp world;NativeBattleMvpView view;NativeGRedGroundDashDemo source;NativeBorgAnimationSeed seed;NativeBorgSourcePose pose;
    check(view.load(files,art,render,&error),"qualified unchanged original stage/models");
    auto bind=[&](NativeGRedGroundDashDemo::Mode mode){
        check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&source.bind(files,art,inputs,world,seed,&error,mode),
            "ordinary native spawn/bounds/target/references and same-world source seed");
        check(!source.relaunch_floor_contacts()&&!source.relaunches()&&!source.landings(),"fresh committed floor counters");
    };
    auto pad_at=[&](unsigned t){GamePadState pad;pad.b=(t>=5&&t<31)||(t>=78&&source.relaunch_floor_contacts()<2);return pad;};
    auto prepare=[&](GamePadState pad,NativeBorgMotionOffer* offer){
        NativeGRedGroundInput mapped;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        return source.map_input(pad,&mapped,&error)&&source.prepare(world,view.source_binding(),frame,mapped.x,mapped.y,mapped.held,offer,&error);
    };
    auto accept=[&](const NativeBorgMotionOffer& offer){
        const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
        check(world.step({},false,&view,nullptr,&frame,&offer)&&source.commit(world,view.source_binding(),&error)&&
            source.pose(world,&pose)&&view.update_source(world,pose,&error),"exact floor receipt before private source state/pose publication");
    };
    bind(NativeGRedGroundDashDemo::Mode::RelaunchEntrySurface);bool old_refused=false;
    for(unsigned t=0;t<120&&!old_refused;++t){
        NativeBorgMotionOffer offer;const auto tick=world.tick();const auto meter=source.resource();
        if(!prepare(pad_at(t),&offer)){
            check(tick==78&&world.tick()==tick&&source.resource()==meter&&!source.relaunch_floor_contacts()&&!source.relaunches()&&
                !world.step({},false,&view),"older mode loading GFMM7 still refuses null-callback floor response without cost/commit/fallback");
            old_refused=true;
        }else accept(offer);
    }
    check(old_refused,"GFMM7 alone grants no new actor floor authority");
    bind(NativeGRedGroundDashDemo::Mode::RelaunchFloor);u32 first=0,second=0,touchdown=0;
    for(unsigned t=0;t<240;++t){
        NativeBorgMotionOffer offer;const auto tick=world.tick();const auto meter=source.resource();const auto count=source.relaunch_floor_contacts();
        const auto stages=source.relaunch_stages();const auto programs=source.movement_programs();const auto retained=pose;
        check(prepare(pad_at(t),&offer)&&source.resource()==meter&&source.relaunch_floor_contacts()==count&&source.movement_programs()==programs&&
            source.relaunch_stages().lower_stage==stages.lower_stage&&source.relaunch_stages().upper_stage==stages.upper_stage,
            "floor costs, latches and publication remain staged before native acceptance");
        if(native_borg_motion_relaunch_floor(offer.kind)){
            auto edited=offer;edited.relaunch_floor->response_factor=.9f;const auto frame=NativeBattleMvpView::movement_frame(world,1152,648);
            check(!world.step({},false,&view,nullptr,&frame,&edited)&&world.tick()==tick&&source.resource()==meter&&
                source.relaunch_floor_contacts()==count&&!retained.frame_scope.expired()&&world.motion_receipt_current(retained.receipt),
                "steep damping cannot borrow floor response or commit staged resource/VM state");
        }
        accept(offer);
        if(native_borg_motion_relaunch_floor(offer.kind)){
            if(!first){first=world.tick();}else second=world.tick();
            const auto current=source.relaunch_stages();
            check(offer.relaunch_floor&&!offer.static_push&&source.selector()==8&&current.lower_stage==1&&current.upper_stage==1&&
                source.movement_programs()==std::array<u16,2>{14,14}&&source.last_relaunch_floor_programs()==std::array<u16,2>{14,14}&&
                !world.actors()[0].grounded&&!source.landings()&&!source.surface_pushes()&&world.actors()[0].position[1]==10&&
                offer.vertical_speed<0&&offer.relaunch_floor->acceleration==profile.air[0]&&offer.relaunch_floor->response_factor==profile.relaunch_floor_damping&&
                source.resource()==meter-(first==world.tick()?input.costs[0]:input.costs[1]),
                "actual original floor snap keeps8/14/airborne cache and independent latches, damps48 only and charges correct entry/continuation cost");
            if(first==world.tick())check(offer.kind==NativeBorgMotionKind::RelaunchEntryFloor&&source.resource()==8400&&source.relaunches()==1,
                "first null-callback floor receipt is entry, not landing");
            else check(offer.kind==NativeBorgMotionKind::RelaunchWindupFloor&&source.resource()==8300&&source.relaunches()==1,
                "held relaunch repeats floor response without replaying entry or refill");
            NativeBorgMotionReceipt receipt;auto wrong=offer;wrong.kind=NativeBorgMotionKind::JumpTouchdown;
            check(!world.motion_receipt(wrong,&receipt),"landing cannot borrow a null-callback floor receipt");
        }
        if(offer.kind==NativeBorgMotionKind::JumpTouchdown){
            touchdown=world.tick();
            check(second&&touchdown==second+1&&source.selector()==7&&world.actors()[0].grounded&&source.resource()==8300&&
                source.movement_programs()==std::array<u16,2>{21,21},"release enters falling21 and real touchdown later, not inside relaunch");
        }
    }
    check(first==79&&second==80&&touchdown==81&&source.relaunch_floor_contacts()==2&&source.relaunches()==1&&source.landings()==1&&
        source.selector()==0&&source.resource()==10000&&!world.shots_spawned()&&world.actors()[0].health==100&&world.actors()[1].health==100,
        "bounded null-callback route holds two snaps, then separate landing/refill through existing owner and renderer");
    const auto retained=pose;source.reject();
    check(retained.frame_scope.expired()&&!world.motion_receipt_current(retained.receipt),"floor producer retirement revokes retained pose/receipt");
    check(world.start({},&view)&&view.update(world,&error)&&view.source_seed(world,&seed)&&
        !source.bind(files,art,"local/content/source-gred-ground-v6",world,seed,&error,NativeGRedGroundDashDemo::Mode::RelaunchFloor)&&world.tick()==0,
        "GFMM6 cannot supply the caller's separately recovered floor operand");
    std::printf("source null-callback floor/lane/resource/pose: %u checks PASS; snaps=%u/%u touchdown=%u; native proxies, no whole-frame equivalence\n",
        checks,first,second,touchdown);
}
