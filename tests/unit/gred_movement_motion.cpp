#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>
#include "engine/gameplay/character_movement_motion.hpp"
#include "engine/gameplay/gred_ground_dash.hpp"
#include "engine/gameplay/gred_ground_walk.hpp"
#include "engine/gameplay/native_gred_ground_input.hpp"
#include "engine/gameplay/native_borg_movement.hpp"
#include "tests/unit/stage_ground_fixture.hpp"
namespace {
unsigned checks=0;std::string error;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"source G-Red motion: %s (%s)\n",why,error.c_str());std::abort();}}
std::vector<gf::u8> read(const std::string& path){std::ifstream f(path,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
gf::CharacterMovementProfile fixture(){
    using namespace gf;CharacterMovementProfile p;p.loaded=true;p.model=0x615;p.model_hash=123;p.class_flags=12;
    p.rank.fill(1);p.walk_speeds.fill(5);p.turn_counts.fill(11);p.dimensions.fill(20);p.turn_gains.fill(.2f);
    p.epsilon=.0001f;p.special_min_vertical=-35;p.angle_unit=bits_to_f32(0x38c90fdbu);p.limits={8,18,-30};
    for(unsigned i=0;i<8;++i){p.direction_classes[i]=i<2||i==7?0:i<4?1:i==4?2:3;p.octant_headings[i]=u8(i*32);p.cardinal_headings[i]=i==0?0:i<4?64:i==4?128:192;}
    for(auto& row:p.ground_dash)row={25,-.5f,22,30};
    p.held_dash_masks={8u<<16,1u<<16,4u<<16,2u<<16};p.dash_body_gain=.2f;p.dash_angle_factor=182;p.dash_near_delta=5;p.dash_steering={100,200,0,10,15};
    return p;
}
gf::CharacterMovementInputProfile input_fixture(){
    using namespace gf;CharacterMovementInputProfile p;p.loaded=true;p.model=0x615;p.model_hash=123;p.held_mask=0xf0100;p.resource_policy=1;
    p.low_threshold=20;p.high_threshold=52;p.axis_window=2;p.other_window=3;p.direction_window=20;p.release_window=16;
    for(unsigned i=0;i<8;++i)p.octants[i]=1;
    p.nibble_indices.fill(-1);const std::array<u8,8> directions={8,9,1,5,4,6,2,10};for(unsigned i=0;i<8;++i)p.nibble_indices[directions[i]]=gf::s8(i+1);p.nibble_indices[0]=0;
    for(auto& c:p.commands){c.kind=1;c.pulse=1;c.masks={2,0};}p.resource_limit=1000;return p;
}
}
int main(){
    using namespace gf;auto p=fixture();auto ip=input_fixture();
    check(character_movement_profile_valid(p)&&character_movement_input_profile_valid(ip),"owned numeric source-domain fixtures");
    for(unsigned raw=0;raw<65536;++raw){
        const auto angle=std::bit_cast<s16>(u16(raw));const unsigned magnitude=angle<0?unsigned(-s32(angle)):unsigned(angle);
        check(source_heading_sector(angle,0)==((raw+4096)&65535)/8192&&
              source_heading_sector(angle,1)==(magnitude+4096)/8192&&
              source_heading_sector(angle,2)==((raw+8192)&65535)/16384,"all signed16 sector wrap boundaries");
    }
    for(BattlePoint forward:{BattlePoint{0,-1},BattlePoint{1,0},BattlePoint{0,1},BattlePoint{-1,0}}){
        NativeBorgMovementFrame native;CharacterMovementHeadingFrame frame;
        check(native_borg_movement_frame(forward,&native)&&native_borg_source_heading_frame(native,forward,&frame),"camera-relative source heading observation");
        const auto body=frame.body72;
        for(auto [nibble,expected]:{std::pair<u8,u8>{8,27},{1,28},{4,29},{2,30}}){
            CharacterMovementHeading heading;u8 program=0,klass=0;
            check(character_movement_heading(ip,p,nibble,CharacterMovementHeadingMode::CardinalDash,frame,&heading,&error)&&
                  gred_ground_dash_program(p,heading,&program,&klass)&&program==expected,"camera/body-relative original directional program selection");
            check(frame.body72==body,"heading selection cannot assign body orientation");
        }
        CharacterMovementHeading idle;
        check(character_movement_heading(ip,p,0,CharacterMovementHeadingMode::Held,frame,&idle,&error)&&!idle.has_direction&&frame.body72==body,"idle has no camera-forced body rotation");
    }
    CharacterMovementHeadingFrame reference;reference.use_player_reference=true;CharacterMovementHeading heading;
    check(!character_movement_heading(ip,p,8,CharacterMovementHeadingMode::Held,reference,&heading,&error),"missing alternate player-heading producer refuses");
    reference.player_reference_declared=true;reference.player_heading=4096;
    check(character_movement_heading(ip,p,8,CharacterMovementHeadingMode::Held,reference,&heading,&error)&&heading.desired5ac==4096,"explicit alternate heading reference");
    NativeBorgMovementFrame reflected;reflected.right={-1,0};CharacterMovementHeadingFrame rejected;
    check(!native_borg_source_heading_frame(reflected,{0,-1},&rejected),"reflected native input basis is not silently converted to source heading");
    CharacterMovementTurn turn{0,1000,0},next;bool done=false;
    check(character_movement_turn_begin(turn,10,&next)&&next.rate7e==100,"source turn duration is a divisor");
    for(unsigned i=0;i<10;++i)check(character_movement_turn_step(next,1,&next,&done),"bounded source heading turn");
    check(done&&next.body72==1000&&!next.delta5aa&&!next.rate7e,"turn completion retains exact residual");
    check(character_movement_turn_step({0,10,1},0,&next,&done)&&done&&next.body72==10,"source zero-step positive residual behavior retained");
    check(character_movement_turn_step({0,-10,-1},0,&next,&done)&&!done&&next.body72==0,"source zero-step negative residual differs");
    GRedGroundWalkEntry walk_entry;heading={};heading.has_direction=true;
    check(!gred_ground_walk_entry(p,heading,{},&walk_entry),"walk refuses profiles without GFMM2 operands");
    p.walk_operands=true;p.walk_angle_divisor=32768;p.stop_damping_base=.96f;p.stop_damping_gain=1.f-.96f;
    p.turn_counts[4]=19;
    for(s32 angle:{0,4095,4096,16384,32767,-32768}){
        heading.delta5aa=s16(angle);heading.desired5ac=s16(angle);
        check(gred_ground_walk_entry(p,heading,{},&walk_entry)&&
              walk_entry.blend_duration==u8(11+f32(std::abs(angle))/32768*8)&&
              walk_entry.travel5ae==angle&&walk_entry.speed44==5,"source signed-angle walk speed/duration/heading operands");
    }
    check(gred_ground_walk_steer(p,{100,0,0},1000,1,1,&next)&&next.body72==300&&next.delta5aa==800&&next.rate7e==200,"steady source walk keeps body/desired/travel domains separate");
    float damped=0;
    check(gred_ground_stop_speed(p,10,1,&damped)&&std::abs(damped-9.6f)<.00001f,"stop source damping operands");
    const auto saved=walk_entry;
    heading.has_direction=false;
    check(!gred_ground_walk_entry(p,heading,{},&walk_entry)&&walk_entry.speed44==saved.speed44,"directionless entry refuses without output mutation");
    //Input adapter is native convenience policy; the source input kernel below
    //still owns flick/held command interpretation.
    auto adapted_profile=ip;adapted_profile.octants={2,10,8,9,1,5,4,6};
    GamePadState device;NativeGRedGroundInput mapped;
    device.dpad_up=true;
    check(native_gred_ground_input(adapted_profile,device,&mapped,&error)&&mapped.x==0&&mapped.y==32&&mapped.held==(8u<<16),"native arrow walk stays below source flick threshold");
    device={};device.stick_y=-127;
    check(native_gred_ground_input(adapted_profile,device,&mapped,&error)&&mapped.y==127&&mapped.held==(8u<<16),"native stick Y is converted to source-positive up");
    device.b=true;
    check(!native_gred_ground_input(adapted_profile,device,&mapped,&error),"unsupported native action refuses before source preparation");
    GRedDashSteering steering;steering.camera5b0=-32768;steering.body72=16384;steering.distance_declared=true;steering.distance75c=300;steering.speed44=0;
    GRedDashSteering steered;check(gred_ground_dash_steer(p,steering,&steered)&&steered.body72==steering.body72,"zero-speed dash steering does not rotate body");
    steering.speed44=25;check(gred_ground_dash_steer(p,steering,&steered)&&steered.body72<steering.body72&&steered.body72>0,"source dash body turn uses bounded fractional correction");
    steering.distance_declared=false;check(!gred_ground_dash_steer(p,steering,&steered),"needed target-distance producer cannot be invented");
    steering.completed547=true;steering.distance75c=NAN;check(gred_ground_dash_steer(p,steering,&steered),"completed steering branch does not demand unused target data");
    using namespace stage_ground_fixture;
    auto floor_bytes=bytes({triangle(0)}),empty_bytes=bytes({});StihGrid floor,empty;
    check(floor.load({floor_bytes.data(),floor_bytes.size()},&error)&&empty.load({empty_bytes.data(),empty_bytes.size()},&error),"owned static source grids");
    CharacterMovementStaticGround ground;ground.grids={&floor,&empty,&empty};ground.parameters=parameters();ground.actor.fields_declared=true;ground.query_height668=80;ground.step674=30;
    CharacterMovementMotionState motion;motion.position={-200,0,-200};motion.speed44=25;motion.planar_change4c=-.5f;
    CharacterMovementMotionState moved;
    check(character_movement_ground_step(p,motion,1,ground,&moved,&error)&&std::abs(moved.position[2]+175)<.001f&&moved.position[1]==0&&moved.speed44==24.5f,"ground move integrates old speed then applies source acceleration");
    auto missing=ground;missing.grids[2]=nullptr;moved.position={99,99,99};
    check(!character_movement_ground_step(p,motion,1,missing,&moved,&error)&&moved.position==CharacterMotionVector{99,99,99},"missing third source grid refuses atomically");
    auto stale=ground;stale.actor.current24=1;
    check(!character_movement_ground_step(p,motion,1,stale,&moved,&error),"different-height source observation cannot be reused as current ground state");
    motion.vertical_only598=1;check(character_movement_ground_step(p,motion,1,missing,&moved,&error)&&moved.position==motion.position&&moved.speed44==motion.speed44,"suppressed planar branch does not invent grid reads or acceleration");motion.vertical_only598=0;
    for(float level:{100.f,-100.f}){
        auto data=bytes({triangle(level)});StihGrid step_grid;check(step_grid.load({data.data(),data.size()},&error),"owned step grid");
        ground.grids[0]=&step_grid;ground.query_height668=200;
        check(character_movement_ground_step(p,motion,1,ground,&moved,&error),"source step restoration query");
        if(level>0)check(moved.position==motion.position&&moved.speed44==24.5f,"too-high step restores position but not already updated speed");
        else check(moved.position[1]==0&&moved.position[2]>motion.position[2],"large downward step retains Y without discarding horizontal motion");
    }
    motion={};motion.speed44=6;motion.vertical48=15;motion.vertical_change50=-1;
    CharacterMovementCeiling ceiling;
    check(!character_movement_air_step(p,motion,1,1,false,ceiling,&moved,&error),"upward integration requires declared ceiling policy");
    ceiling.mode=CharacterMovementCeilingMode::DeclaredStatic;ceiling.stage_top34=1000;ceiling.query_height668=20;
    check(character_movement_air_step(p,motion,1,1,false,ceiling,&moved,&error)&&moved.position==CharacterMotionVector{0,15,6}&&moved.vertical48==14,"air move integrates old velocities then updates vertical acceleration");
    ceiling.stage_top34=25;
    check(character_movement_air_step(p,motion,1,1,false,ceiling,&moved,&error)&&moved.position[1]==5&&moved.vertical48==-1,"ceiling clamp precedes source acceleration update");
    ceiling.previous_y30=20;
    check(character_movement_air_step(p,motion,1,1,false,ceiling,&moved,&error)&&moved.position[1]==20,"ceiling restoration preserves higher previous source Y");
    motion.vertical48=-50;
    check(character_movement_air_step(p,motion,1,1,true,{},&moved,&error)&&moved.vertical48==-30,"descending normal clamp needs no fabricated ascending ceiling observation");
    motion.field741=1;
    check(character_movement_air_step(p,motion,1,1,true,{},&moved,&error)&&moved.vertical48==-35,"source exceptional fall clamp stays distinct");
    motion={};motion.speed44=7;motion.planar_change4c=2;motion.vertical48=20;
    ceiling.mode=CharacterMovementCeilingMode::DeclaredSourceBypass;
    check(character_movement_air_step(p,motion,1,1,true,ceiling,&moved,&error)&&moved.position[2]==7&&moved.speed44==8&&moved.vertical48==18,"air wrapper clamps velocities after old-velocity position integration");
    GRedGroundDashState dash;GRedGroundDashPlan plan;
    check(gred_ground_dash_plan(p,dash,0,0,0,&plan)&&plan.request_program&&!plan.move&&plan.program==27,"dash entry requests program without inventing motion");
    dash=plan.state;float speed=plan.speed44,change=plan.change4c;
    for(unsigned tick=0;tick<40&&dash.selector==13;++tick){
        const u8 control=tick<5?0:1;
        check(gred_ground_dash_plan(p,dash,speed,change,control,&plan),"source active dash plan");
        check(plan.advance_program==(control==0)&&plan.move,"active control1 holds animation but still plans motion");
        speed=std::max(0.f,plan.speed44+plan.change4c);change=plan.change4c;
        check(gred_ground_dash_finish(p,plan,control,false,8u<<16,1,&dash),"source active timer/held-direction finish");
    }
    check(dash.selector==14&&dash.timer568==0&&speed==22,"held dash expires from source timer and reaches source minimum speed");
    check(gred_ground_dash_plan(p,dash,speed,change,1,&plan)&&plan.advance_program,"recovery resumes the held program");
    check(gred_ground_dash_finish(p,plan,3,true,0,1,&dash)&&dash.selector==0,"recovery completion proposes return to idle");
    dash.selector=14;dash.recovery_entered=false;check(!gred_ground_dash_plan(p,dash,speed,change,3,&plan),"unreviewed forced-continuation recovery entry refuses");
    unsigned originals=0;
    auto encoded=read("local/recomp-evidence/gred-motion-integration-v1/motion-v2.gfmm");
    if(!encoded.empty()){
        CharacterMovementProfile original;
        check(character_movement_load({encoded.data(),encoded.size()},&original,&error),"qualified source motion operands");
        check(original.walk_speeds[0]==10&&original.ground_dash[0][0]==25&&original.rank[2]==1,"source numeric operands and constructor rank");
        for(unsigned n=0;n<encoded.size();++n){auto bad=encoded;bad[n]^=1;check(!character_movement_load({bad.data(),bad.size()},&p,&error)&&!p.loaded,"edited source motion profile refuses");}
        std::array<StihGrid,3> real;
        unsigned loaded=0;for(const char* name:{"floor","secondary","tertiary"}){auto data=read(std::string("local/recomp-evidence/gred-motion-integration-v1/stage/")+name+".stih");check(!data.empty()&&real[loaded].load({data.data(),data.size()},&error),"three distinct original static grids");++loaded;}
        ground.grids={&real[0],&real[1],&real[2]};ground.parameters={-100000,100000,500,0,-.5f,20000,-20000,true};ground.query_height668=original.dimensions[2];ground.step674=original.dimensions[4];
        motion={};motion.position={-240,10,300};motion.speed44=original.walk_speeds[0];motion.travel5ae=-32768;
        f32 previous_y=motion.position[1];
        for(unsigned tick=0;tick<60;++tick){
            ground.actor.current24=motion.position[1];ground.actor.previous30=previous_y;
            check(character_movement_ground_step(original,motion,1,ground,&moved,&error)&&character_movement_motion_valid(moved),"real-grid source motion proposal sequence");
            previous_y=motion.position[1];motion=moved;
        }
        check(motion.position[2]<300,"real original grid sequence produces world displacement");originals=3;
    }
    for(const char* pack:{"source-gred-ground-v1","source-gred-ground-v2","source-gred-ground-v3","source-gred-ground-v4","source-gred-ground-v5","source-gred-ground-v6","source-gred-ground-v7","source-gred-ground-v8"}){
        auto bytes=read(std::string("local/content/")+pack+"/motion.gfmm");
        if(bytes.empty())continue;
        CharacterMovementProfile original;
        check(character_movement_load({bytes.data(),bytes.size()},&original,&error),"qualified GFMM1/2 pack backward compatibility");
        check(original.walk_operands==(bytes[5]>=2)&&original.air_operands==(bytes[5]>=3)&&original.air_dash_operands==(bytes[5]>=4)&&original.surface_operands==(bytes[5]>=5)&&original.steep_operands==(bytes[5]>=6)&&original.relaunch_floor_operands==(bytes[5]>=7)&&original.ground_loss_operands==(bytes[5]==8),"versioned movement/contact capabilities remain distinct");
        if(original.relaunch_floor_operands)check(original.relaunch_floor_damping==bits_to_f32(0x3f4ccccdu),"GFMM7 keeps the caller's8043751C .8 factor separate from .9 push and .8 data44 acceleration");
        if(original.ground_loss_operands)check(original.ground_loss_zero==0&&original.ground_loss_contact_delay==30,
            "GFMM8 keeps ground correction's8043762C zero and80437648 timer separate from air-dash69C operands");
        if(original.steep_operands)check(original.steep_damping==bits_to_f32(0x3f666666u),"GFMM6 source steep damping is .9, not windup .5");
        if(original.surface_operands)check(original.landing_angles==std::array<s16,2>{10240,6144}&&original.contact_motion_epsilon==bits_to_f32(0x2edbe6feu),"GFMM5 retains original classAC/AE and motion-squared threshold");
        if(original.air_dash_operands)check(original.air_dash_windup_factor==.5f&&original.air_dash_contact_delay==30,"GFMM4 preserves windup factor and source contact timer");
        if(original.air_operands)check(original.ascent_pitch==8192&&original.air_ready_delay==20&&
            original.air_damping[0]==bits_to_f32(0x3f7ae148u)&&original.air_damping[2]==bits_to_f32(0x3f7d70a4u),"GFMM3 preserves source air damping, pitch and timer operands");
        if(original.walk_operands)check(original.walk_angle_divisor==32768&&original.stop_damping_base==bits_to_f32(0x3f75c28fu)&&
            original.stop_damping_gain==bits_to_f32(0x3d23d710u),"new tail preserves original source walk/stop floats exactly");
        for(unsigned at=0;at<bytes.size();++at){
            auto edited=bytes;edited[at]^=1;
            check(!character_movement_load({edited.data(),edited.size()},&original,&error)&&!original.loaded,"GFMM1/2 edited operand/identity/header refuses");
        }
    }
    std::printf("source heading/turn/motion/dash planning: %u checks PASS; original static grids=%u; proposals only, no body/contact/frame admission\n",checks,originals);
}
