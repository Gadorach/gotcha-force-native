// Exact normal G-Red cue and no-target request inspection. Heading, placement
// and stock are fixture inputs, not live actor observations. No allocator.
#include <cstdio>
#include <filesystem>
#include <fstream>
#include "engine/gameplay/borgs/normal_gred_beam_custody.hpp"
#include "engine/gameplay/borgs/normal_gred_beam_flight.hpp"
#include "engine/gameplay/character_program_directional_control.hpp"
#include "engine/gameplay/stage_projectile_segment.hpp"

namespace {
using namespace gf;
namespace fs=std::filesystem;
constexpr u32 model_hash=0xe2b4439a;
constexpr NormalGRedNeutralBSource source{0x0615,0,3};
constexpr u8 lanes=3;
constexpr f32 step=1;
constexpr s16 heading=0x2345;
constexpr WeaponStockModeZeroState full_stock{5,5,180,0};
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

bool stage_fixture(const fs::path& dir,const GRedBeamFlightSample& from,const GRedBeamFlightSample& to,std::string& error){
    std::array<StihGrid,3> stage;
    constexpr std::array<const char*,3> files{"floor.stih","secondary.stih","tertiary.stih"};
    constexpr std::array<std::size_t,3> sizes{215748,126948,36484};
    constexpr std::array<u32,3> hashes{0x0d913901u,0x01859c01u,0xfe083e8fu};
    for(unsigned i=0;i<stage.size();++i){
        Input bytes;
        if(!bytes.load(dir/files[i],sizes[i],hashes[i])){error="exact stage473 fixture grids required";return false;}
        if(!stage[i].load(bytes.view(),&error))return false;
    }
    // Same declared horizontal bounds as the movement bench, not a live read
    // from the original stage global. These exercise geometry in isolation.
    constexpr StageRayParameters bounds{-4500,4500,-4500,4500,500,true};
    const std::array<const StihGrid*,3> grids{&stage[0],&stage[1],&stage[2]};
    StageProjectileSegment flight,vertical;
    if(!stage_projectile_segment(grids,bounds,to.position20,from.position20,&flight,&error)||
       !stage_projectile_segment(grids,bounds,{0,-100,0},{0,100,0},&vertical,&error))return false;
    if(!flight.queried||flight.surface.grid_code||flight.surface.nearest.tested!=7||
       !vertical.queried||vertical.surface.grid_code!=1||vertical.surface.nearest.triangle!=2301||
       vertical.position!=std::array<f32,3>{0,10,0}){
        error="real-grid flight miss or positive vertical check differed";return false;
    }
    std::printf("fixture stage473: cue segment miss (%u tests); vertical segment hit grid %u triangle %u at (%.3f,%.3f,%.3f)\n",
        flight.surface.nearest.tested,unsigned(vertical.surface.grid_code),vertical.surface.nearest.triangle,
        double(vertical.position[0]),double(vertical.position[1]),double(vertical.position[2]));
    return true;
}

bool witness(const fs::path& assets,const fs::path& profile,const fs::path& height_file,const fs::path& stage_dir,std::string& error){
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

    GRedNeutralBStartup start;
    WeaponStockModeZeroPlan availability;
    const GRedNeutralBChoice no_target_row{GRedNeutralBRoute::Shot,0,44,0};
    if(!normal_gred_neutral_b_start(source,true,0,&start)||
       !normal_gred_neutral_b_availability(source,no_target_row,full_stock,&availability)||
       !availability.accepted||availability.state!=full_stock){
        error="detached startup or check-only stock refused";return false;
    }
    // Startup mask 15 is reduced by the original actor's effective mask 3.
    if(!vm.request_with_actor_controls(start.group,start.program,u8(start.mask&lanes),-1,step,driver,control,&ops,&error))return false;
    Trace trace;
    auto aim_clock=start.aim_clock560;
    for(unsigned visit=0;visit<=budget;++visit){
        if(visit){
            f32 vm_step=0;
            if(!normal_gred_neutral_b_vm_step(source,vm.state().actor.control0[0],step,&vm_step)){
                error="detached VM step refused";return false;
            }
            if(!vm.update_with_actor_controls(lanes,vm_step,driver,control,&ops,&error))return false;
        }
        const auto& state=vm.state();
        if(!trace.visit(state,ops,asset.programs[1],control,driver)){error="accepted pose or cue differed from the exact program";return false;}
        // No-target bench: an actual cue from this accepted VM, a heading
        // supplied by the fixture, and the exact no-target scalar aim path.
        GRedNeutralBNoTargetAim aim;
        GRedNeutralBReleaseGate gate;
        if(!normal_gred_neutral_b_no_target_aim(source,true,heading,step,&aim)||!aim.aim_pass||
           !aim.clear_aim_refs||aim.heading72!=heading||aim.error5aa||
           !normal_gred_neutral_b_release_gate(source,state.actor.control0[0],aim.aim_pass,aim_clock,step,&gate)||
           gate.aim_clock560!=aim_clock||gate.release_due!=(state.actor.control0[0]==1)){
            error="no-target aim or accepted cue gate differed";return false;
        }
        aim_clock=gate.aim_clock560;
        if(!gate.release_due)continue;
        if(visit!=expected_updates||trace.selects!=std::array<unsigned,2>{2,2}){
            error="unexpected cue visit or incomplete release pose lanes";return false;
        }
        // Alternatives at this same cue, not repeated releases or live stock
        // writes. A channel can empty between availability and the cue.
        constexpr WeaponStockModeZeroState last{1,1,180,0},empty{0,0,180,0};
        GRedNeutralBReleasePlan paid,last_round,refused;
        if(!normal_gred_neutral_b_debit(source,gate,availability.state,&paid)||
           !paid.request_beam||paid.owner_slot!=source.slot||paid.object_type!=2||paid.argument!=43||
           paid.stock!=WeaponStockModeZeroState{4,5,180,0}||
           !normal_gred_neutral_b_debit(source,gate,last,&last_round)||!last_round.request_beam||
           last_round.stock!=WeaponStockModeZeroState{0,1,180,180}||
           !normal_gred_neutral_b_debit(source,gate,empty,&refused)||refused.request_beam||refused.stock!=empty){
            error="detached channel-0 debit or beam request differed";return false;
        }
        // The cue fixture has no allocator result. It must stop before the
        // constructor header, even though the separate muzzle math can run.
        GRedBeamFactoryPlan allocation;
        if(!normal_gred_beam_factory_plan(source,paid,aim.clear_aim_refs,GRedBeamPoolSample::Unobserved,&allocation)||
           allocation.call!=GRedBeamPoolCall{3,8,12,16,1,false}||allocation.propose_header){
            error="detached factory request crossed the allocator boundary";return false;
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
        // Unit-scale bench placement, not a reconstruction of when the live
        // initializer sees the parent's world matrices. The two roles use
        // different keys even though both are assigned to the fixture slot.
        constexpr f32 placed[]{125,34,-205};
        const Mat4 frame_base=mat4_multiply(mat4_translation(placed),
            mat4_rotation_y(f32(aim.heading72)*bits_to_f32(0x38c90fdb)));
        CharacterPoseFrame frame;
        CharacterWorldPose fixture_matrices;
        if(!character_pose_initial_frame(rig,frame_base,&frame,&error)||
           !character_pose_direct_world(rig,local,frame,&fixture_matrices,&error))return false;
        if(fixture_matrices.matrices.size()<=7){error="fixture matrix keys missing";return false;}
        const GRedBeamNoTargetBasis basis{source.slot,source.slot,
            fixture_matrices.matrices[7],fixture_matrices.matrices[0]};
        GRedBeamMuzzlePlan muzzle;
        if(!normal_gred_beam_no_target_muzzle(source,paid,aim.clear_aim_refs,basis,&muzzle)){
            error="fixture muzzle inputs refused";return false;
        }
        constexpr std::array<f32,3> expected_origin{238.14360f,226.52010f,-82.33281f};
        constexpr std::array<f32,3> expected_bearing{.761513f,0,.648149f};
        for(unsigned i=0;i<3;++i){
            if(std::abs(muzzle.origin174[i]-expected_origin[i])>.002f||
               std::abs(muzzle.unit_bearing[i]-expected_bearing[i])>.00002f){
                error="fixture muzzle or credited bearing differed";return false;
            }
        }
        GRedBeamFlightSample seeded,advanced;
        if(!normal_gred_beam_flight_seed(source,muzzle,&seeded)||
           !normal_gred_beam_flight_prefix(source,true,seeded,&advanced)||seeded.speed44!=80||advanced.speed44!=80.5f||
           std::abs(seeded.distance180-1.f)>.0001f||std::abs(advanced.distance180-81.f)>.0001f){
            error="detached beam flight prefix differed";return false;
        }
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
        std::printf("candidate model bytes=%u/%u; no live actor pose\n",
            unsigned(state.actor.models[0]),unsigned(state.actor.models[1]));
        std::puts("foreign VM scope and missing zero-weight side clip refused without publishing");
        std::printf("no-target fixture: heading=0x%04x, channel-0 stock %d -> %d, type %u arg %u requested only\n",
            unsigned(u16(aim.heading72)),int(full_stock.current),int(paid.stock.current),unsigned(paid.object_type),unsigned(paid.argument));
        std::puts("last-stock alternative armed 180; empty-stock alternative requested nothing; no allocation or contact");
        std::printf("allocator request only: class %u, base group %u, slot group %u, %u descriptor; no header\n",
            unsigned(allocation.call.pool_class),unsigned(allocation.call.group),unsigned(allocation.call.resolved_group),
            unsigned(allocation.call.descriptors));
        std::printf("fixture only: parent key-7 origin=(%.5f,%.5f,%.5f), credited key-0 bearing=(%.6f,%.6f,%.6f)\n",
            double(muzzle.origin174[0]),double(muzzle.origin174[1]),double(muzzle.origin174[2]),
            double(muzzle.unit_bearing[0]),double(muzzle.unit_bearing[1]),double(muzzle.unit_bearing[2]));
        std::printf("fixture only: distance %.0f -> %.0f; speed for next visit %.1f; numeric prefix only\n",
            double(seeded.distance180),double(advanced.distance180),double(advanced.speed44));
        if(!stage_dir.empty()&&!stage_fixture(stage_dir,seeded,advanced,error))return false;
        return true;
    }
    error="first authored cue not reached within the update budget";return false;
}
}

int main(int argc,char** argv){
    if(argc!=4&&argc!=5){std::fprintf(stderr,"usage: %s MVP_ASSET_DIR NEUTRAL_B_GFMP POSE_HEIGHT_GFHC [STAGE473_DIR]\n",argv[0]);return 2;}
    std::string error;
    if(!witness(argv[1],argv[2],argv[3],argc==5?fs::path(argv[4]):fs::path{},error)){
        std::fprintf(stderr,"G-Red shot cue refused: %s\n",error.c_str());return 1;
    }
}
