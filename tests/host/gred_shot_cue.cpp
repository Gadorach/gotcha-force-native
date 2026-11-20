// Exact normal G-Red pose inspection. No input dispatcher, live actor, world,
// aim, stock, allocator, renderer or shot publication participates.
#include <cstdio>
#include <filesystem>
#include <fstream>
#include "engine/gameplay/character_program_directional_control.hpp"

namespace {
using namespace gf;
namespace fs=std::filesystem;
constexpr u32 model_hash=0xe2b4439a;
constexpr u8 lanes=3;
constexpr f32 step=1;
constexpr unsigned budget=64;
constexpr unsigned expected_updates=16; // detached step-1 regression, not retail timing
constexpr std::array<u8,4> opening{0,0,1,0},release{1,0,1,0};

struct Input {
    std::vector<u8> bytes;
    ByteView view()const{return {bytes.data(),bytes.size()};}
    bool load(const fs::path& path,std::size_t size,u32 hash){
        std::error_code ec;
        if(!fs::is_regular_file(path,ec)||fs::file_size(path,ec)!=size||ec)return false;
        std::ifstream stream(path,std::ios::binary);
        bytes.resize(size);
        return bool(stream.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(size)))&&
            stream.peek()==std::char_traits<char>::eof()&&!stream.bad()&&character_program_model_hash(view())==hash;
    }
};

struct Trace {
    std::array<unsigned,2> selects{};
    unsigned controls=0;
    bool visit(const CharacterProgramState& state,const std::vector<CharacterProgramOperation>& ops,
               const CharacterProgram& shot,const CharacterProgramDirectionalControl& control,
               const CharacterMotionDriver& driver){
        auto command_at=[&](const auto& op,unsigned offset){
            return offset+op.command.size()<=shot.bytes.size()&&
                std::equal(op.command.begin(),op.command.end(),shot.bytes.begin()+offset);
        };
        for(const auto& op:ops){
            if(op.kind==CharacterProgramOperationKind::DirectionalSelect){
                if(op.lane>=2||selects[op.lane]>=2||!command_at(op,selects[op.lane]?0x24:0))return false;
                ++selects[op.lane];
            }else if(op.kind==CharacterProgramOperationKind::Control0){
                if(op.lane||controls>=2||selects[0]!=controls+1||!command_at(op,controls?0x2c:0x08))return false;
                ++controls;
            }
        }
        if(!controls||state.actor.control0!=(controls==2?release:opening))return false;
        for(unsigned lane=0;lane<2;++lane){
            const auto& accepted=state.lanes[lane];
            const auto* direction=control.directional_snapshot(accepted.motion);
            if(!direction||direction->lane()!=lane||!selects[lane]||direction->side_weight()!=0||
               accepted.program!=1||accepted.descriptor_group!=2)return false;
            const bool second=selects[lane]==2;
            const auto centre=direction->primary_target(),side=direction->side_target();
            const auto* base=driver.snapshot(direction->primary());
            const auto* lateral=driver.snapshot(direction->side());
            if(!base||!lateral||base->lane()!=lane||lateral->lane()!=lane||
               centre.group!=2||centre.index!=(second?6:0)||side.group!=2||side.index!=(second?7:1)||
               base->group()!=3||lateral->group()!=3||base->index()!=(second?11:0)||
               lateral->index()!=(second?12:1))return false;
        }
        return true;
    }
};

bool unchanged(const CharacterProgramState& before,const CharacterProgramState& after){
    if(before.actor.control0!=after.actor.control0||before.actor.models!=after.actor.models)return false;
    for(unsigned i=0;i<2;++i){
        const auto& a=before.lanes[i];const auto& b=after.lanes[i];
        if(a.program!=b.program||a.cursor!=b.cursor||a.wait!=b.wait||a.clock!=b.clock||
           a.motion.context!=b.motion.context||a.motion.frame!=b.motion.frame)return false;
    }
    return true;
}

bool witness(const fs::path& assets,const fs::path& profile,const fs::path& height_file,std::string& error){
    Input model,prefix,pose,bank_bytes,program,height_bytes;
    if(!model.load(assets/"g-red.hsd",193095,model_hash)||
       !prefix.load(assets/"g-red.prefix",256,0x5e71a4a7)||
       !pose.load(assets/"g-red.pose",188,0x5b1ea8fe)||
       !bank_bytes.load(assets/"g-red.motions",355264,0x276b4e9e)||
       !program.load(profile,436,0xf10c9e6d)||!height_bytes.load(height_file,28,0x0c0b1313)){
        error="exact normal 0615/0 model, rig, bank, shot profile and GFHC required";return false;
    }

    HsdModelBones bones;
    CharacterPoseRig rig;
    CharacterLocalPose initial;
    CharacterMotionBank bank;
    CharacterProgramAsset asset;
    CharacterPoseHeightProfile height;
    if(!hsd_bones_load(prefix.view(),model.view(),&bones,&error)||
       !character_pose_rig_load(pose.view(),bones,&rig,&error)||
       !character_pose_initial_local(rig,&initial,&error)||
       !character_motion_bank_load(bank_bytes.view(),&bank,&error)||
       !character_program_load_control_flow(program.view(),model_hash,&asset,&error)||
       !character_pose_height_profile_load(height_bytes.view(),&height,&error))return false;

    constexpr std::array<std::array<u8,2>,9> keys{{
        {0,0},{0,1},{0,6},{3,0},{3,1},{3,2},{3,11},{3,12},{3,13}
    }};
    std::vector<CharacterMotionClipBinding> clips;
    for(const auto& [group,index]:keys){
        CharacterMotionSelection selected;
        if(!character_motion_select(bank,group,index,&selected,&error))return false;
        if(selected.fallback){error="exact clip required (no default fallback)";return false;}
        CharacterMotionClipBinding clip{group,index,{}};
        if(!hsd_bone_motion_load(selected.archive,bones,&clip.clip,&error))return false;
        clips.push_back(std::move(clip));
    }

    std::vector<CharacterProgramOperation> ops;
    auto seed=[&](const CharacterMotionDriver& driver,CharacterProgramVm& vm,CharacterProgramDirectionalControl& control){
        CharacterPoseHeightFields fields;fields.scale_b4=1;
        // Explicit settled inspection seed, not idle-to-shot action dispatch.
        if(!vm.load_fresh_actor_directional_controls(asset,{0,1,height.zero},&error)||
           !control.bind(vm,driver,height,fields,{},CharacterProgramDirectionalControlMode::DeclaredHeightFields,&error)||
           !vm.request_with_actor_controls(0,0,lanes,-1,step,driver,control,&ops,&error,{0,0}))return false;
        for(unsigned lane=0;lane<2;++lane){
            const auto& idle=vm.state().lanes[lane];
            const auto* selected=control.snapshot(idle.motion);
            if(!selected||selected->lane()!=lane||idle.program||idle.descriptor_group||idle.descriptor_choice){
                error="idle lane seed did not bind";return false;
            }
        }
        if(vm.state().actor.control0[0]||vm.state().actor.steering1dfc||vm.actor_root_control_mode()){
            error="idle release/steering/root scope";return false;
        }
        return true;
    };
    CharacterMotionDriver driver;
    CharacterProgramVm vm,foreign;
    CharacterProgramDirectionalControl control,foreign_control;
    if(!driver.load_directional_transitions(model.view(),rig,asset,clips,initial,{7,11},&error)||
       !seed(driver,vm,control)||!seed(driver,foreign,foreign_control))return false;
    const auto before=vm.state();
    if(vm.request_with_actor_controls(2,0,lanes,-1,step,driver,foreign_control,&ops,&error)||
       !ops.empty()||!unchanged(before,vm.state())){
        error="foreign VM scope must refuse without publishing state";return false;
    }

    // Startup mask 15 is reduced by the original actor's effective mask 3.
    if(!vm.request_with_actor_controls(2,0,lanes,-1,step,driver,control,&ops,&error))return false;
    Trace trace;
    for(unsigned visit=0;visit<=budget;++visit){
        if(visit&&!vm.update_with_actor_controls(lanes,step,driver,control,&ops,&error))return false;
        const auto& state=vm.state();
        if(!trace.visit(state,ops,asset.programs[1],control,driver)){error="accepted pose or cue differed from the exact program";return false;}
        if(state.actor.control0[0]==0)continue;
        if(visit!=expected_updates||trace.selects!=std::array<unsigned,2>{2,2}){
            error="unexpected cue visit or incomplete release pose lanes";return false;
        }

        // At zero weight the original side target still has to bind. Remove it
        // only from a separate bench library, not from the qualified inputs.
        auto incomplete=clips;
        std::erase_if(incomplete,[](const auto& clip){return clip.group==3&&clip.index==12;});
        CharacterMotionDriver missing_driver;
        CharacterProgramVm missing_vm;
        CharacterProgramDirectionalControl missing_control;
        if(!missing_driver.load_directional_transitions(model.view(),rig,asset,incomplete,initial,{7,11},&error)||
           !seed(missing_driver,missing_vm,missing_control)||
           !missing_vm.request_with_actor_controls(2,0,lanes,-1,step,missing_driver,missing_control,&ops,&error))return false;
        for(unsigned update=1;update<visit;++update){
            if(!missing_vm.update_with_actor_controls(lanes,step,missing_driver,missing_control,&ops,&error))return false;
            if(missing_vm.state().actor.control0!=opening){error="premature cue without the side clip";return false;}
        }
        const auto pre_cue=missing_vm.state();
        if(missing_vm.update_with_actor_controls(lanes,step,missing_driver,missing_control,&ops,&error)||
           !ops.empty()||!unchanged(pre_cue,missing_vm.state())){
            error="missing side clip must refuse without publishing the cue";return false;
        }

        CharacterLocalPose local;
        if(!control.compose_local_pose(state,&local,&error))return false;
        for(unsigned lane=0;lane<2;++lane){
            const auto& l=state.lanes[lane];
            const auto& d=*control.directional_snapshot(l.motion);
            if(l.wait!=0x40||l.cursor!=0x44||d.primary().end_frame!=35||d.side().end_frame!=36){
                error="cue cursor or release endpoints differed";return false;
            }
        }
        if(state.actor.models!=std::array<u8,2>{4,0}){error="cue model bytes differed";return false;}
        std::printf("normal G-Red 0615/0: first authored +1CEF=1 after %u VM updates (step %.0f); local pose only\n",visit,double(step));
        for(unsigned lane=0;lane<2;++lane){
            const auto& l=state.lanes[lane];
            const auto& direction=*control.directional_snapshot(l.motion);
            const auto* base=driver.snapshot(direction.primary());
            const auto* side=driver.snapshot(direction.side());
            std::printf("lane %u P2:0 wait=0x%02x next=0x%02x D2:%u/%u clips=%u:%u/%u:%u frames=%.2f/%.2f ends=%d/%d\n",
                lane,unsigned(l.wait),unsigned(l.cursor),unsigned(direction.primary_target().index),unsigned(direction.side_target().index),
                unsigned(base->group()),unsigned(base->index()),unsigned(side->group()),unsigned(side->index()),
                double(direction.primary().frame),double(direction.side().frame),
                int(direction.primary().end_frame),int(direction.side().end_frame));
        }
        std::printf("candidate model bytes=%u/%u; no actor, world pose, stock, beam or hit\n",
            unsigned(state.actor.models[0]),unsigned(state.actor.models[1]));
        std::puts("foreign VM scope and missing zero-weight side clip refused without publishing");
        return true;
    }
    error="first authored cue not reached within the update budget";return false;
}
}

int main(int argc,char** argv){
    if(argc!=4){std::fprintf(stderr,"usage: %s MVP_ASSET_DIR NEUTRAL_B_GFMP POSE_HEIGHT_GFHC\n",argv[0]);return 2;}
    std::string error;
    if(!witness(argv[1],argv[2],argv[3],error)){std::fprintf(stderr,"G-Red shot cue refused: %s\n",error.c_str());return 1;}
}
