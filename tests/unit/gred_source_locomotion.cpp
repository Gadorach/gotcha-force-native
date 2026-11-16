#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <set>
#include "engine/gameplay/character_motion_driver.hpp"
#include "engine/gameplay/character_program_height_control.hpp"
#include "engine/gameplay/gred_movement_admission.hpp"
#include "engine/gameplay/gred_ground_dash.hpp"
#include "engine/gameplay/native_borg_movement.hpp"
namespace {
unsigned checks=0;std::string error;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"source G-Red locomotion: %s (%s)\n",why,error.c_str());std::abort();}}
std::vector<gf::u8> read(const std::string& name){std::ifstream f(name,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
gf::ByteView view(const std::vector<gf::u8>& bytes){return {bytes.data(),bytes.size()};}
}
int main(){
    using namespace gf;CharacterProgramAsset asset;
    check(!character_program_load_control_flow({},0,&asset,&error),"missing profile refuses");
    auto encoded=read("local/recomp-evidence/gred-movement-admission-v1/locomotion.gfmp");
    if(encoded.empty()){std::printf("source G-Red locomotion: %u checks PASS; optional source profile absent, no actor execution\n",checks);return 0;}
    auto model=read("local/content/native-battle-mvp-v1/g-red.hsd");
    auto prefix=read("local/content/native-battle-mvp-v1/g-red.prefix");
    auto pose=read("local/content/native-battle-mvp-v1/g-red.pose");
    auto motions=read("local/content/native-battle-mvp-v1/g-red.motions");
    check(character_program_model_hash(view(model))==0xe2b4439au,"normal G-Red model identity");
    check(character_program_load_control_flow(view(encoded),0xe2b4439au,&asset,&error)&&asset.programs.size()==32,"32 unaltered source base-program entries load through existing GFMP3 reader");
    check(character_program_detail::valid_control_flow(asset)&&asset.lane_mask==3,"existing structural execution domain, no relaxed validation");
    for(unsigned i=0;i<32;++i)check(asset.programs[i].group==1&&asset.programs[i].choice==i,"source base-group choice identity");
    CharacterProgramAsset refused;
    check(!character_program_load(view(encoded),0xe2b4439au,&refused,&error)&&!character_program_load_transitions(view(encoded),0xe2b4439au,&refused,&error),"legacy reader refusals remain intact");
    auto truncated=encoded;truncated.pop_back();
    check(!character_program_load_control_flow(view(truncated),0xe2b4439au,&refused,&error)&&!refused.loaded,"truncated source profile refuses");
    check(!character_program_load_control_flow(view(encoded),0xeab3632au,&refused,&error),"G-Red source profile cannot bind Sasuke");
    HsdModelBones bones;CharacterPoseRig rig;CharacterMotionBank bank;
    check(hsd_bones_load(view(prefix),view(model),&bones,&error)&&character_pose_rig_load(view(pose),bones,&rig,&error),"shared normal skeleton and lane rig");
    check(character_motion_bank_load(view(motions),&bank,&error),"owned original motion bank");
    std::set<std::pair<u8,u8>> needed;
    for(const auto& descriptor:asset.descriptors)for(unsigned lane=0;lane<2;++lane){
        const unsigned at=4+lane*8;check(descriptor.bytes[at]<16,"no unqualified common-bank clip substitution");
        needed.emplace(descriptor.bytes[at],descriptor.bytes[at+1]);
    }
    std::vector<CharacterMotionClipBinding> clips;
    for(auto [group,index]:needed){
        CharacterMotionSelection selected;CharacterMotionClipBinding clip;clip.group=group;clip.index=index;
        check(character_motion_select(bank,group,index,&selected,&error)&&!selected.fallback,"exact source clip exists without idle fallback");
        check(hsd_bone_motion_load(selected.archive,bones,&clip.clip,&error),"shared bone-motion decoder");
        clips.push_back(std::move(clip));
    }
    CharacterLocalPose initial;CharacterMotionDriver driver;
    check(character_pose_initial_local(rig,&initial,&error)&&driver.load_frozen_target_transitions(view(model),rig,asset,clips,initial,&error),"existing source motion driver accepts recovered program library");
    CharacterProgramVm vm;std::vector<CharacterProgramOperation> operations;unsigned requests=0,updates=0;
    for(u8 choice:{u8(0),u8(1),u8(2),u8(4),u8(6),u8(8),u8(10),u8(11),u8(12),u8(13),u8(14)}){
        check(vm.load_fresh_actor_control_flow(asset,&error)&&vm.request(1,choice,3,-1,1,driver,&operations,&error),"existing VM evaluates an explicit source request, not gameplay input admission");
        ++requests;
        for(unsigned tick=0;tick<60;++tick){
            check(vm.update(3,1,driver,&operations,&error),"bounded source wait/transition update");++updates;
            for(unsigned lane=0;lane<2;++lane)check(driver.snapshot(vm.state().lanes[lane].motion)!=nullptr,"source lane snapshot remains library-bound");
        }
        if(choice==6)for(unsigned lane=0;lane<2;++lane)
            check(driver.snapshot(vm.state().lanes[lane].motion)->index()==24,"original apex program reaches falling clip24 through its waits");
        if(choice==10||choice==14)for(unsigned lane=0;lane<2;++lane)
            check(driver.snapshot(vm.state().lanes[lane].motion)->index()==14,"source wind-up/alternate launch uses ambiguous clip14");
    }
    check(vm.load_fresh_actor_control_flow(asset,&error),"fresh VM before ground-dash refusal");
    operations.push_back({});
    check(!vm.request(1,27,3,-1,1,driver,&operations,&error)&&!vm.ready()&&operations.empty(),"ground dash still refuses without its scoped pose-height consumer; no fabricated execution");
    auto height_bytes=read("local/recomp-evidence/gred-movement-admission-v1/pose-height.gfhc");
    CharacterPoseHeightProfile height;
    check(character_pose_height_profile_load(view(height_bytes),&height,&error),"qualified source pose-height operands load through shared reader");
    CharacterPoseHeightFields fields;fields.model_group3e8=6; //normal0615; no world positions declared
    const auto mode=CharacterProgramHeightControlMode::DeclaredFields;
    {
        CharacterProgramVm flag_vm;CharacterProgramHeightControl controls;CharacterProgramClassFields before,after;
        check(flag_vm.load_fresh_actor_height_controls(asset,{},&error)&&controls.bind(flag_vm,height,fields,{},mode,&error)&&
            flag_vm.observe_class_fields(&before),"scoped ordinary ground flag fixture");
        check(flag_vm.write_ground_walk_flag(before,true,&error)&&flag_vm.observe_class_fields(&after)&&
            after.behaviour==(before.behaviour|0x10u)&&after.default_group==before.default_group&&after.effect_control==before.effect_control,
            "ground handler can write only bit10, not unrelated actor/class fields");
        auto stale=flag_vm;
        check(!stale.write_ground_walk_flag(before,false,&error),"old ground behaviour observation cannot write through changed state");
        CharacterProgramVm foreign;check(foreign.load_fresh_actor_height_controls(asset,{},&error),"neighboring ground flag VM");
        check(!foreign.write_ground_walk_flag(after,false,&error),"foreign ground flag scope cannot borrow state");
        check(flag_vm.write_ground_walk_flag(after,false,&error)&&!(flag_vm.state().actor.behaviour&0x10u),"ordinary stop clears its represented ground flag");
    }
    for(u8 choice:{u8(27),u8(28),u8(29),u8(30)}){
        CharacterProgramHeightControl controls;
        check(vm.load_fresh_actor_height_controls(asset,{},&error)&&controls.bind(vm,height,fields,{},mode,&error),"explicit ground-dash VM/height scope without invented world observations");
        check(vm.request_with_actor_controls(1,choice,3,-1,1,driver,controls,&operations,&error),"unaltered ground-dash prefix executes with its scoped height consumer");
        ++requests;
        unsigned height_commands=0;
        for(const auto& operation:operations)if(operation.kind==CharacterProgramOperationKind::PoseHeight){
            ++height_commands;check(operation.lane==1&&operation.command[3]==1,"source height command retains lane and operand");
        }
        check(height_commands==1&&vm.state().actor.pose_height.mode71d==1&&vm.state().actor.pose_height.offset6d0==0,"grounded branch records mode1 and returns zero offset without fake bone positions");
        check(vm.state().actor.behaviour==0x20&&vm.state().actor.control1[0]==0&&vm.state().actor.control1[1]==2*(choice-27),"source dash entry controls and direction metadata");
        const u8 expected=choice==27?2:choice==28?4:choice==29?3:5;
        for(unsigned lane=0;lane<2;++lane)check(driver.snapshot(vm.state().lanes[lane].motion)->index()==expected,"source directional clip matches each dash program");
        bool active=false,recovery=false;
        for(unsigned tick=0;tick<60;++tick){
            const auto old_frame=vm.state().lanes[1].motion.frame;
            const auto old_control=vm.state().actor.control1[0];
            check(vm.update_with_actor_controls(3,1,driver,controls,&operations,&error),"bounded source dash wait/control update");++updates;
            const auto current=vm.state().actor.control1[0];
            if(current!=old_control){
                if(current==1){active=true;check(old_frame>=5,"source active phase waits for frame5");}
                else if(current==3){recovery=true;check(old_frame>=19,"source recovery phase waits for frame19");}
                else check(false,"no unexpected dash control phase");
            }
            check(vm.state().actor.pose_height.offset6d0==0,"ground dash never fabricates a world-height adjustment");
        }
        check(active&&recovery&&vm.state().lanes[0].done&&vm.state().lanes[1].done,"original dash controls traverse entry/active/recovery and terminal wait");
        CharacterProgramVm neighbor;CharacterProgramHeightControl foreign;
        check(neighbor.load_fresh_actor_height_controls(asset,{},&error)&&foreign.bind(neighbor,height,fields,{},mode,&error),"same-model neighboring scope fixture");
        const auto clock=vm.state().lanes[1].clock;
        check(!vm.update_with_actor_controls(3,1,driver,foreign,&operations,&error)&&!vm.ready()&&operations.empty()&&vm.state().lanes[1].clock==clock,"foreign same-model height scope cannot advance the dash");
        check(vm.load_fresh_actor_height_controls(asset,{},&error)&&!vm.request_with_actor_controls(1,choice,3,-1,1,driver,controls,&operations,&error)&&operations.empty(),"reload invalidates old height scope");
    }
    CharacterProgramHeightControl split;
    check(vm.load_fresh_actor_height_controls(asset,{},&error)&&split.bind(vm,height,fields,{},mode,&error),"independent-lane fixture");
    check(vm.request_with_actor_controls(1,27,1,-1,1,driver,split,&operations,&error)&&vm.state().actor.pose_height.mode71d==0,"upper-only request cannot execute lower height command");
    check(vm.request_with_actor_controls(1,27,2,-1,1,driver,split,&operations,&error)&&vm.state().actor.pose_height.mode71d==1,"separate lower request owns height command");
    requests+=2;
    const auto upper_frame=vm.state().lanes[0].motion.frame;
    for(unsigned tick=0;tick<10;++tick){check(vm.update_with_actor_controls(2,1,driver,split,&operations,&error),"lower-only dash update");++updates;}
    check(vm.state().lanes[0].motion.frame==upper_frame&&vm.state().lanes[1].motion.frame>upper_frame,"lower continuation does not retime upper-body lane");
    auto input_bytes=read("local/recomp-evidence/gred-input-resource-v1/movement.gfmi");unsigned input_links=0;
    if(!input_bytes.empty()){
        CharacterMovementInputProfile input_profile;
        check(character_movement_input_load(view(input_bytes),&input_profile,&error),"original source movement-input/resource profile");
        CharacterMovementInputState input_state;CharacterAirResourceState meter{input_profile.resource_limit,0,0};
        GRedMovementAdmissionFields admission_fields;admission_fields.prior_handlers_declined=true;GRedMovementAdmission admission;
        check(character_movement_input_step(input_profile,input_state,{0,60,8u<<16,0},&input_state,&error)&&
              gred_movement_admit(input_profile,input_state,meter,admission_fields,&admission,&error)&&admission.selector==12,"source forward flick reaches ground-dash admission");
        u8 source_program=27,direction_class=0,coarse_direction=0;
        auto motion_bytes=read("local/recomp-evidence/gred-motion-integration-v1/motion-v2.gfmm");
        CharacterMovementProfile source_motion;
        if(!motion_bytes.empty()){
            check(character_movement_load(view(motion_bytes),&source_motion,&error),"qualified source heading/motion profile");
            NativeBorgMovementFrame native_frame;CharacterMovementHeadingFrame source_frame;CharacterMovementHeading selected;
            check(native_borg_movement_frame({0,-1},&native_frame)&&native_borg_source_heading_frame(native_frame,{0,-1},&source_frame),"explicit native camera/body heading observation");
            const auto body=source_frame.body72;
            check(character_movement_heading(input_profile,source_motion,admission.direction58f,CharacterMovementHeadingMode::CardinalDash,source_frame,&selected,&error)&&
                  gred_ground_dash_program(source_motion,selected,&source_program,&direction_class)&&source_program==27&&source_frame.body72==body,"source tables derive forward dash program without hardcoded direction classification or camera-forced body turn");
            coarse_direction=u8(source_heading_sector(selected.relative734,2));
        }
        CharacterProgramHeightControl connected;
        check(vm.load_fresh_actor_height_controls(asset,{},&error)&&connected.bind(vm,height,fields,{},mode,&error)&&
              vm.request_with_actor_controls(1,source_program,3,-1,1,driver,connected,&operations,&error),"admitted ground-dash selector joins the original program through the existing VM");
        ++requests;input_state.directions5d8=admission.remaining_directions;admission_fields.selector=12;admission_fields.flags5e0=vm.state().actor.behaviour;
        for(unsigned tick=0;tick<6;++tick){
            check(character_movement_input_step(input_profile,input_state,{0,60,8u<<16,admission_fields.flags5e0},&input_state,&error)&&
                  gred_movement_admit(input_profile,input_state,meter,admission_fields,&admission,&error)&&!admission.changed,"held source direction cannot repeatedly request the dash");
            check(vm.update_with_actor_controls(3,1,driver,connected,&operations,&error),"input-linked original dash timeline update");++updates;
            admission_fields.flags5e0=vm.state().actor.behaviour;
        }
        check(character_movement_input_step(input_profile,input_state,{0,60,(8u<<16)|0x100u,admission_fields.flags5e0},&input_state,&error)&&
              gred_movement_admit(input_profile,input_state,meter,admission_fields,&admission,&error)&&admission.selector==4,"new jump edge interrupts admitted ground dash with original priority");
        check(vm.request_with_actor_controls(1,2,3,-1,1,driver,connected,&operations,&error)&&
              driver.snapshot(vm.state().lanes[1].motion)->index()==6,"admitted jump joins original takeoff program/clip without removing height opcode from dash");
        ++requests;
        check(meter.value736==input_profile.resource_limit,"input admission does not invent actor-handler resource spending");
        if(source_motion.loaded){
            check(vm.load_fresh_actor_height_controls(asset,{},&error)&&connected.bind(vm,height,fields,{},mode,&error),"fresh scope for source dash handler planning");
            GRedGroundDashState dash;dash.direction_class=direction_class;dash.coarse57d=coarse_direction;
            f32 speed=0,change=0;unsigned held_callbacks=0;bool resumed=false;f32 held_frame=-1;
            for(unsigned tick=0;tick<100&&dash.selector;++tick){
                GRedGroundDashPlan plan;const auto previous=vm.state().lanes[1].motion.frame;
                check(gred_ground_dash_plan(source_motion,dash,speed,change,vm.state().actor.control1[0],&plan),"source ground-dash handler plan");
                if(plan.request_program){
                    check(vm.request_with_actor_controls(1,plan.program,2,-1,1,driver,connected,&operations,&error)&&
                          vm.request_with_actor_controls(1,plan.program,1,-1,1,driver,connected,&operations,&error),"handler requests unaltered dash in source lower/upper order");requests+=2;
                }else if(plan.advance_program){
                    check(vm.update_with_actor_controls(2,1,driver,connected,&operations,&error)&&
                          vm.update_with_actor_controls(1,1,driver,connected,&operations,&error),"handler-qualified source lower/upper VM advance");updates+=2;if(dash.selector==14)resumed=true;
                }
                else{
                    ++held_callbacks;if(held_frame<0)held_frame=previous;
                    check(vm.state().lanes[1].motion.frame==held_frame,"source active dash holds animation across continuing handler callbacks");
                }
                speed=plan.speed44;change=plan.change4c;if(plan.move)speed=std::max(0.f,std::fma(change,1.f,speed));
                check(gred_ground_dash_finish(source_motion,plan,vm.state().actor.control1[0],vm.state().lanes[1].done,8u<<16,1,&dash),"source timer/held-input handler finish");
            }
            check(dash.selector==0&&held_callbacks>=20&&resumed&&vm.state().lanes[1].done,"source active hold, timer expiry and recovery differ from unconditional in-place replay");
        }
        ++input_links;
    }
    std::printf("source G-Red locomotion: %u checks PASS; original profile=1, clips=%zu, explicit requests=%u, VM updates=%u; input-program links=%u; scoped ground-dash height/timing/lane/refusal checks; no terrain/weapon/contact/physics or actor admission\n",checks,clips.size(),requests,updates,input_links);
}
