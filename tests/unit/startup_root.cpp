#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/gameplay/startup_root.hpp"
#include "engine/gameplay/startup_world.hpp"
#include "engine/gameplay/startup_actor_dispatch.hpp"
#include "tests/unit/character_motion_driver_fixture.hpp"
namespace {
unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"startup root: %s\n",why);std::abort();}}
std::vector<gf::u8> read(const std::string& p){std::ifstream f(p,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};}
gf::StartupRootProfile owned(){gf::StartupRootProfile p;p.loaded=true;p.reset_scale=1;p.lifetime=10;p.step=1;p.angle_span=16384;p.amplitude=gf::bits_to_f32(0x3f666666);p.bias=gf::bits_to_f32(0x3dcccccd);p.y_base=5;p.y_factor=4;p.angle_unit=gf::bits_to_f32(0x38c90fdb);return p;}
}
int main(){
    std::string error;auto p=owned();gf::StartupRootState root;check(gf::startup_root_reset(p,&root)&&root.actor_scale==std::array<float,3>{1,1,1}&&root.root_scale==root.actor_scale&&root.scalar_b4==1&&root.scalar_b8==1,"source reset writes ones, not zero scales");
    unsigned substate=0,fade=0,sound=0;float previous=1;
    for(unsigned i=0;i<10;++i){gf::StartupRootStep next;check(gf::startup_root_exit_step(p,root,gf::u8(substate),&next),"source exit step");root=next.state;substate=next.substate;fade+=next.fade12;sound+=next.sound242;
        check(root.remaining==9-float(i)&&root.root_scale[0]==root.root_scale[2]&&root.root_scale[0]<=previous&&root.root_scale[1]>0,"fixed-one update counter and anisotropic XZ/Y shape");
        check(next.hide==(i==9)&&substate==(i==9?2u:1u),"hide exactly on tenth update, not at scale zero");previous=root.root_scale[0];}
    check(fade==1&&sound==1&&root.root_scale[0]==p.bias&&root.root_scale[1]==std::fma(-p.y_factor,p.bias,p.y_base),"curve12/sound242 required once; final source XZ=.1 and Y=4.6");
    gf::StartupRootStep idle;check(gf::startup_root_exit_step(p,root,2,&idle)&&!idle.updated&&!idle.fade12&&!idle.sound242&&!idle.hide,"completed substate holds without repeated side effects");
    auto invalid=p;invalid.step=.5f;check(!gf::startup_root_reset(invalid,&root),"unreviewed counter step rejects, no invented animation-rate scaling");
    root={};check(!gf::startup_root_exit_step(p,root,0,&idle),"root initialization is explicit");
    character_motion_driver_fixture::Fixture f;check(f.load(&error),"owned actor model/rig/profile");gf::CharacterLocalPose local;local.bones.resize(2);
    for(unsigned i=0;i<2;++i){local.bones[i].joint_offset=f.rig.binding.bones[i].joint_offset;for(unsigned k=0;k<3;++k)local.bones[i].translation[k]=f.rig.binding.bones[i].translation[k];}
    local.bones[0].translation[0]=3;gf::CharacterPoseFrame modifiers;check(gf::character_pose_initial_frame(f.rig,gf::Mat4{},&modifiers,&error),"common supplied bone modifiers");modifiers.alternate_root=true;
    gf::StartupActorTransform actor;actor.position[0]=10;actor.scale[0]=2;gf::CharacterWorldPose world;
    check(gf::character_pose_startup_world(f.rig,local,actor,modifiers,&world,&error)&&world.matrices[0].at(0,3)==16&&world.matrices[1].at(0,3)==18,"source alternate root includes scaled root-local translation and then child hierarchy");
    check(!gf::character_pose_direct_world(f.rig,local,modifiers,&world,&error),"legacy direct-anchor gate not broadly relaxed");
    modifiers.scales[0]={.5f,2,.5f};check(gf::character_pose_startup_world(f.rig,local,actor,modifiers,&world,&error)&&world.matrices[0].at(0,0)==1&&world.matrices[1].at(0,3)==17,"root scale is a separate final local multiplier, not a universal inverse bind");
    modifiers.actor_base.at(0,3)=7;check(!gf::character_pose_startup_world(f.rig,local,actor,modifiers,&world,&error)&&world.matrices.empty(),"new source branch rejects hidden extra opaque anchor");modifiers.actor_base={};
    actor.model_flags=2;check(!gf::character_pose_startup_world(f.rig,local,actor,modifiers,&world,&error),"retained bone1-adjust branch remains explicit refusal");actor.model_flags=0;
    modifiers.inverse_kinematics=true;check(!gf::character_pose_startup_world(f.rig,local,actor,modifiers,&world,&error),"IK not accidentally admitted");modifiers.inverse_kinematics=false;
    modifiers.enabled_bones=1;check(!gf::character_pose_startup_world(f.rig,local,actor,modifiers,&world,&error),"disabled-bone retained state not filled with identity guesses");modifiers.enabled_bones=3;
    actor.scale[0]=0;check(!gf::character_pose_startup_world(f.rig,local,actor,modifiers,&world,&error),"singular global scale refuses, no renderer normal fallback");actor.scale[0]=2;
    gf::CharacterProgramVm vm;gf::CharacterProgramActor vm_actor;vm_actor.default_group=5;auto program=f.profile;program.programs[0].bytes={1,5,0,0,0,0,0,0,2,255,1,0,0,0,0,0,0,1,255,255};
    check(vm.load(program,vm_actor,{},&error),"owned program entering exit");gf::CharacterMotionDriver driver;check(f.driver(driver,&error),"owned concrete motion driver");
    gf::StartupActorDispatch dispatch;dispatch.bind(7);check(dispatch.set_root_profile(p),"root feature opt-in via bounded source profile");std::vector<gf::StartupActorEffect> effects;
    std::vector<gf::CharacterProgramOperation> ops;check(!dispatch.request(vm,0,-1,1,driver,&ops,&error),"root-configured actor requires source reset before request");
    check(dispatch.reset(&effects)&&dispatch.request(vm,0,-1,1,driver,&ops,&error)&&dispatch.update(vm,1,driver,&ops,&effects,&error)&&dispatch.update(vm,1,driver,&ops,&effects,&error)&&dispatch.state().phase==gf::StartupActorDispatchState::ScaleOut,"source start delay/control1 transition, no early scale-exit execution");
    fade=sound=0;
    for(unsigned i=0;i<10;++i){check(dispatch.update(vm,.5f,driver,&ops,&effects,&error)&&ops.empty(),"exit updates root only, not motion VM; fixed source step independent of supplied .5 motion delta");for(auto e:effects){fade+=e.kind==gf::StartupActorEffectKind::FadeCurve12Required;sound+=e.kind==gf::StartupActorEffectKind::Sound242Required;}
        check(dispatch.root_state().remaining==9-float(i)&&dispatch.state().render_group==(i==9?0:72),"source render group clears on final exit step");}
    check(dispatch.ready()&&dispatch.state().substate==2&&fade==1&&sound==1,"configured exit no longer falsely refuses; pending audio/fade remain explicit");
    check(dispatch.update(vm,1,driver,&ops,&effects,&error)&&effects.empty(),"post-exit state does not replay numerical/audio/fade effects");
    auto encoded=read("local/content/startup-root/root.gfrs");unsigned real=0,actors=0,actor_updates=0,world_frames=0;
    if(!encoded.empty()){
        gf::StartupRootProfile source;check(gf::startup_root_load({encoded.data(),encoded.size()},&source,&error)&&source.lifetime==10&&source.reset_scale==1,"actual source-bound root profile");
        for(std::size_t n=0;n<encoded.size();++n)check(!gf::startup_root_load({encoded.data(),n},&source,&error)&&!source.loaded,"all truncated root profiles refuse");
        for(unsigned at:{0u,4u,6u,8u,12u,16u,20u,24u,28u,32u,36u,40u,44u,48u}){auto bad=encoded;bad[at]=255;check(!gf::startup_root_load({bad.data(),bad.size()},&source,&error)&&!source.loaded,"corrupt/nonfinite/unsupported profile values refuse");}
        real=1;
    }
    if(real)for(const char* name:{"g-red","sasuke"}){
        const auto base=std::string("local/content/startup-characters/")+name+"/";auto model=read(base+"model.hsd"),prefix=read(base+"model-prefix.bin"),layout=read(std::string("local/content/character-pose-bindings/")+name+".gfcp"),bytes=read(std::string("local/content/character-program-bindings/")+name+".gfmp");if(model.empty()||layout.empty()||bytes.empty())continue;
        gf::HsdModelBones bones;gf::CharacterPoseRig rig;gf::CharacterProgramAsset profile;check(gf::hsd_bones_load({prefix.data(),prefix.size()},{model.data(),model.size()},&bones,&error)&&gf::character_pose_rig_load({layout.data(),layout.size()},bones,&rig,&error)&&gf::character_program_load({bytes.data(),bytes.size()},gf::character_program_model_hash({model.data(),model.size()}),&profile,&error),"actual rig and program binding for configured root exit");
        std::vector<gf::CharacterMotionClipBinding> clips;for(unsigned index:{0,1,3,4,7,8,9}){auto data=read(std::string("local/content/startup-motion-programs/")+name+"/motions/motion-0"+std::to_string(index)+".hsd");gf::CharacterMotionClipBinding c;c.index=gf::u8(index);check(gf::hsd_bone_motion_load({data.data(),data.size()},bones,&c.clip,&error),"actual source clip");clips.push_back(std::move(c));}
        gf::CharacterMotionDriver concrete;check(concrete.load({model.data(),model.size()},rig,profile,clips,&error)&&vm.load(profile,vm_actor,{},&error),"actual retained driver and fresh VM");gf::StartupActorDispatch controlled;controlled.bind(7);
        gf::StartupRootProfile source;check(gf::startup_root_load({encoded.data(),encoded.size()},&source,&error)&&controlled.set_root_profile(source)&&controlled.reset(&effects)&&controlled.request(vm,7,-1,1,concrete,&ops,&error),"source reset/exit profile opts actor into numeric root path");
        unsigned entered=0;bool hidden=false;
        for(unsigned tick=0;tick<256;++tick){check(controlled.update(vm,1,concrete,&ops,&effects,&error),"configured source actor subdispatch/root step");++actor_updates;
            for(auto e:effects)entered+=e.kind==gf::StartupActorEffectKind::FadeCurve12Required;
            std::array<gf::CharacterPoseLane,4> requested;for(unsigned lane=0;lane<2;++lane){const auto* state=concrete.snapshot(vm.state().lanes[lane].motion);check(state!=nullptr,"owned source motion snapshot");requested[lane].current=&state->sample();requested[lane].mirrored=state->mirrored();}
            gf::CharacterLocalPose pose;gf::CharacterPoseFrame frame;gf::StartupActorTransform supplied;supplied.position[0]=7;supplied.position[1]=13;supplied.position[2]=-21;
            for(unsigned k=0;k<3;++k)supplied.scale[k]=controlled.root_state().actor_scale[k];
            check(gf::character_pose_sample(rig,requested,&pose,&error)&&gf::character_pose_initial_frame(rig,gf::Mat4{},&frame,&error),"source local channels with supplied actor fields, not source placement claim");frame.alternate_root=true;frame.scales[0]=controlled.root_state().root_scale;
            check(gf::character_pose_startup_world(rig,pose,supplied,frame,&world,&error),"source alternate world chain consumes driven root-scale values");++world_frames;
            if(controlled.state().substate==2){hidden=controlled.state().render_group==0;break;}
        }
        if(std::string(name)=="sasuke")check(hidden&&entered==1&&controlled.ready()&&vm.state().lanes[0].descriptor_choice==8,"Sasuke completes numeric10-update exit and source hide, no longer blocked on root arithmetic");
        else check(!hidden&&!entered&&vm.state().lanes[0].motion.frame==10&&vm.state().lanes[0].threshold==24,"G-Red held source program remains held; root feature does not force a clip/exit transition");
        ++actors;
    }
    std::printf("{\"startup_root_checks\":%u,\"source_root_profile\":%u,\"source_actors\":%u,\"actor_updates\":%u,\"alternate_world_frames\":%u,\"exit_updates\":10,\"world_chain_and_exit_subset_only\":true,\"visual_startup_or_trail_camera_material_events_admitted\":false,\"all_passed\":true}\n",checks,real,actors,actor_updates,world_frames);
}
