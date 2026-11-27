#pragma once
#include "engine/assets/borg_practice_profile.hpp"
#include "engine/assets/stage_package.hpp"
#include "engine/assets/stage_view_fade.hpp"
#include "engine/gameplay/animated_model_scene.hpp"
#include "engine/gameplay/battle_roster_series.hpp"
#include "engine/gameplay/borg_mobility_hud.hpp"
#include "engine/gameplay/borg_stock_hud.hpp"
#include "engine/gameplay/stage_inclined_ledge.hpp"
#include "engine/gameplay/native_battle_mvp_borg.hpp"
#include "engine/gameplay/attachment_model_scene.hpp"
#include "engine/gameplay/stage_ground_surface.hpp"
#include "engine/gameplay/stage_projectile_segment.hpp"
#include "engine/graphics/scene_draw.hpp"
#include "engine/graphics/text.hpp"
#include "engine/graphics/ink_splatter.hpp"
#include "engine/core/backends.hpp"
namespace gf {
enum class PracticeBackScreen {Close,OwnedCollection,ForceBrowser};
struct BorgBattleVisualAssets {
    ByteView model,prefix,pose,motions;
    std::array<ByteView,kBorgBattleMoves> field_texture{};
    ByteView impact_record_bank{};
};
inline bool borg_impact_record_fade_supported(const HsdMeshProperties& mesh){
    if(mesh.hidden)return true;
    const u32 alpha=(mesh.render_mode>>13)&3u;
    const u32 alpha_source=alpha?alpha:(mesh.render_mode&3u);
    // The authored fade only admits a material-bearing alpha lane that affects
    // RGB blending, never a write to depth or a replacement render policy.
    return alpha_source!=2&&mesh.colour_write&&mesh.blend&&mesh.blend_src==5&&(mesh.blend_dst==2||mesh.blend_dst==6)&&
           !mesh.depth_write&&(mesh.texture==kHsdNoTexture||mesh.texture_stage_inactive||
                               mesh.texture_alpha_mode==0||mesh.texture_alpha_mode==3);
}
class NativeBattleMvpView final:public IBattleMvpTerrain {
public:
    NativeBattleMvpView()=default;
    NativeBattleMvpView(const NativeBattleMvpView&)=delete;
    NativeBattleMvpView& operator=(const NativeBattleMvpView&)=delete;
    void source_ground_controls(bool interactive){source_general_=true;source_interactive_=interactive;}
    void source_air_dash_floor_controls(){source_air_dash_floor_=true;}
    void source_windup_entry_floor_controls(){source_windup_entry_floor_=true;}
    void source_windup_positive_floor_controls(){source_windup_positive_floor_=true;}
    void source_windup_entry_push_controls(){source_windup_entry_push_=true;}
    void source_walk_ledge_controls(){source_walk_ledge_=true;}
    void source_ordered_ground_controls(){source_ordered_ground_=true;}
    void source_dash_ledge_controls(){source_dash_ledge_=true;}
    void source_ground_ledge_controls(){source_ground_ledge_=true;}
    void source_ground_jump_controls(){source_ground_jump_=true;}
    void source_ground_landing_controls(){source_ground_landing_=true;}
    void source_ground_air_controls(){source_ground_air_=true;}
    void source_ground_air_floor_controls(){source_ground_air_floor_=true;}
    void source_ground_relaunch_controls(){source_ground_relaunch_=true;}
    void source_ground_relaunch_floor_controls(){source_ground_relaunch_floor_=true;}
    void source_ground_windup_entry_floor_controls(){source_ground_windup_entry_floor_=true;}
    void source_ground_surface_controls(){source_ground_surface_=true;}
    void source_ground_loss_surface_controls(){source_ground_loss_surface_=true;}
    void source_ground_loss_jump_controls(){source_ground_loss_jump_=true;}
    void source_ground_loss_landing_controls(){source_ground_loss_landing_=true;}
    void source_ground_loss_air_controls(){source_ground_loss_air_=true;}
    void source_ground_loss_relaunch_controls(){source_ground_loss_relaunch_=true;}
    void source_ground_fall_push_controls(){source_ground_fall_push_=true;}
    void source_ground_air_fall_push_controls(){source_ground_air_fall_push_=true;}
    void source_ground_relaunch_fall_push_controls(){source_ground_relaunch_fall_push_=true;}
    void source_ground_recovery_push_controls(){source_ground_recovery_push_=true;}
    void source_ground_windup_push_controls(){source_ground_windup_push_=true;}
    void source_ground_active_push_controls(){source_ground_active_push_=true;}
    void source_ground_relaunch_push_controls(){source_ground_relaunch_push_=true;}
    void source_ground_relaunch_entry_push_controls(){source_ground_relaunch_entry_push_=true;}
    void source_ground_windup_entry_push_controls(){source_ground_windup_entry_push_=true;}
    void source_practice_controls(){source_practice_=true;}
    void source_practice_back(PracticeBackScreen screen){source_practice_back_=screen;}
    void source_practice_close_camera(bool close){source_practice_close_camera_=close;}
    void source_practice_owned_copy(u64 instance){source_practice_owned_instance_=instance;}
    void source_practice_force_slot(unsigned force,unsigned slot){source_practice_force_slot_={force,slot};}
    void source_practice_start(std::string_view hint){
        source_practice_start_label_="DEMO START / LIVE INPUT";
        if(!hint.empty()){source_practice_start_label_+=" - ";source_practice_start_label_+=hint;}
    }
    std::optional<NativeBorgSourceReadout> practice_readout(const NativeBattleMvp& world)const{
        const auto frame=source_practice_frame_.lock();
        return source_practice_&&frame?frame->readout(world):std::nullopt;
    }
    std::optional<NativeBorgSourceMeter> practice_meter(const NativeBattleMvp& world)const{
        const auto frame=practice_readout(world);
        return frame?std::optional{frame->meter}:std::nullopt;
    }
    static constexpr const char* windup_handoff_hint="SHIFT/B RELAUNCH / NEUTRAL DASH";
    static constexpr const char* dash_interrupt_hint="DASH CANCELED";
    static constexpr const char* landing_dash_hint="DASH FROM LANDING";
    static constexpr const char* direct_expiry_relaunch_hint="DIRECT EXPIRY RELAUNCH";
    static const char* practice_direct_timed_relaunch_cue(const NativeBorgSourceReadout& frame){
        if(!frame.direct_timed_relaunch)return nullptr;
        return frame.direct_timed_relaunch_up?"AIRBORNE / DIRECT TIMED UP RELAUNCH":"AIRBORNE / DIRECT TIMED RELAUNCH";
    }
    static constexpr const char* direct_expiry_request_hint="UP+SHIFT/B TO REQUEST RELAUNCH";
    static constexpr const char* direct_expiring_request_hint="UP+SHIFT/B TO REQUEST RELAUNCH ON EXPIRY";
    static constexpr const char* direct_expiring_relaunch_hint="AIRBORNE / RELAUNCH ON EXPIRY";
    static constexpr const char* direct_expiring_entry_push_hint="AIRBORNE / RELAUNCH ENTRY PUSH ON EXPIRY";
    static constexpr const char* expiring_walk_air_dash_hint="AIRBORNE / DASH WIND-UP ON EXPIRY";
    static constexpr const char* expiring_walk_air_dash_relaunch_hint="AIRBORNE / EXPIRY WIND-UP TO RELAUNCH";
    static constexpr const char* expiring_walk_air_dash_snap_hint="AIRBORNE / WIND-UP ENTRY SNAP ON EXPIRY";
    static constexpr const char* windup_from_relaunch_snap_hint="AIRBORNE / RELAUNCH TO WIND-UP";
    static constexpr const char* relaunch_from_active_snap_hint="AIRBORNE / ACTIVE DASH TO RELAUNCH";
    static const char* practice_timed_release_push_cue(const NativeBorgSourceReadout& frame){
        if(!frame.timed_release_push)return nullptr;
        if(frame.down_timed_release_push)return "AIRBORNE / TIMED DOWN-DRIFT PUSH";
        if(frame.third_timed_release_push)return "AIRBORNE / THIRD TIMED RELAUNCH-FALL PUSH";
        return frame.second_timed_release_push?"AIRBORNE / SECOND TIMED RELAUNCH-FALL PUSH":"AIRBORNE / TIMED RELAUNCH-FALL PUSH";
    }
    static const char* practice_relaunch_floor_cue(const NativeBorgSourceReadout& frame){
        return frame.relaunch_floor_damped?
            (*frame.relaunch_floor_damped?"AIRBORNE / DESCENT DAMPED":"AIRBORNE / VERTICAL SPEED RETAINED"):nullptr;
    }
    static const char* practice_contact_suffix(NativeBorgMotionKind kind){
        if(native_borg_motion_push(kind))return " / PUSH";
        if(native_borg_motion_air_dash_floor(kind)||native_borg_motion_relaunch_floor(kind))return " / FLOOR SNAP";
        return "";
    }
    static const char* practice_phase(const NativeBorgSourceReadout& frame){
        const auto kind=frame.kind;
        if(kind==NativeBorgMotionKind::Ground){
            if(frame.action==BattleMvpAction::GroundDash)return "GROUND DASH";
            return frame.action==BattleMvpAction::Move?"WALKING":"IDLE";
        }
        if(native_borg_motion_phase(kind)==3)return "TOUCHDOWN";
        if(native_borg_motion_phase(kind)==4)return "LANDING";
        if(native_borg_motion_phase(kind)==1)return "TAKEOFF";
        if(kind==NativeBorgMotionKind::GroundDeparture)return "LEDGE DEPARTURE";
        if(kind==NativeBorgMotionKind::GroundLossEntry)return "LEDGE FALL ENTRY";
        if(kind==NativeBorgMotionKind::GroundLossAir||kind==NativeBorgMotionKind::GroundLossPush)return "LEDGE FALL";
        if(native_borg_motion_windup_entry(kind)||native_borg_motion_windup_update(kind))return "DASH WIND-UP";
        if(native_borg_motion_active_entry(kind)||native_borg_motion_active_update(kind))return "AIR DASH";
        if(kind==NativeBorgMotionKind::AirDashRecoveryFloorComplete)return "DASH RECOVERY / FLOOR CONTACT";
        if(native_borg_motion_recovery(kind))return "DASH RECOVERY";
        if(native_borg_motion_relaunch(kind))return "RELAUNCH";
        return frame.ascent?"RISING":"FALLING";
    }
    static std::string practice_ledge_cue(const NativeBorgSourceReadout& frame){
        if(!frame.ledge_timer_ticks)return {};
        const auto ticks=*frame.ledge_timer_ticks;
        if(frame.kind==NativeBorgMotionKind::GroundLossEntry||frame.kind==NativeBorgMotionKind::GroundLossAir||
            (frame.kind==NativeBorgMotionKind::GroundLossPush&&ticks))
            return ticks?"DRIFT LOCKED "+std::to_string(ticks)+" TICKS":
                frame.ledge_drift_active?"DRIFT ACTIVE":"DRIFT UNLOCKED";
        return "LEDGE TIMER "+std::to_string(ticks)+" TICKS";
    }
    unsigned draw_practice_stop(RenderEngine& render,s32 width,s32 height,u32 tick)const{
        if(!source_practice_||width<=0||height<=0)return 0;
        const auto saved=render.state();
        render.state()={};render.state().blend=true;render.state().blend_src=5;render.state().blend_dst=6;
        TextRenderer::Style title;title.scale=width>=360?2:1;title.r=255;title.g=210;title.b=80;
        TextRenderer::Style body;body.scale=width>=640?2:1;
        const s32 y=std::max(0,height/2-42);
        auto draws=text_.draw_centered(render,"MOVEMENT PRACTICE STOPPED",width/2,y,title);
        const auto detail="MOTION REFUSED / LAST TICK "+std::to_string(tick)+"\nQ RESTART   "+practice_exit_hint();
        draws+=text_.draw_centered(render,detail,width/2,y+text_.line_height(title)+12,body);
        render.state()=saved;
        return draws;
    }
    void source_vertical_jump_controls(s16 resource,bool moving=false,bool air_dash=false,bool relaunch=false,bool dash_jump=false,bool landing_interrupt=false,bool surface=false,bool steep=false,bool fall_surface=false,bool recovery_surface=false,bool source_windup=false,bool active_surface=false,bool relaunch_surface=false,bool entry_surface=false,bool relaunch_floor=false){source_vertical_jump_=true;source_moving_jump_=moving;source_air_dash_=air_dash;source_relaunch_=relaunch;source_dash_jump_=dash_jump;source_landing_interrupt_=landing_interrupt;source_surface_=surface;source_steep_=steep;source_fall_surface_=fall_surface;source_recovery_surface_=recovery_surface;source_windup_surface_=source_windup;source_active_surface_=active_surface;source_relaunch_surface_=relaunch_surface;source_relaunch_entry_surface_=entry_surface;source_relaunch_floor_=relaunch_floor;source_resource_=resource;}
    bool load(IFileSystem& files,const std::string& directory,RenderEngine& render,std::string* error,
              const NativeGRedPracticeAssets* practice=nullptr){
        clear_stage();
        source_practice_package_=false;source_practice_owned_instance_=0;source_practice_force_slot_={};
        source_practice_frame_.reset();
        if(practice&&!practice->qualified()){if(error)*error="unqualified packaged practice view inputs";return false;}
        auto read=[&](const std::string& name,std::vector<u8>* out){u64 size=0;const auto path=directory+"/"+name;if(!files.file_size(path,&size)||!size||size>(32u<<20)||!files.read(path,*out)||out->size()!=size){if(error)*error="native MVP: missing/oversized asset "+path;return false;}return true;};
        if(!load_stage(read,render,error))return false;
        for(unsigned i=0;i<2;++i){
            if(!i&&practice){
                using A=NativeGRedPracticeAssets;const auto& p=*practice;
                if(!borgs_[0].load(0x615,p[A::Body],p[A::RigPrefix],p[A::PoseProfile],p[A::Bank],error))return false;
            }else{
                const std::string name=i?"sasuke":"g-red";std::vector<u8> model,prefix,pose,motions;
                if(!read(name+".hsd",&model)||!read(name+".prefix",&prefix)||!read(name+".pose",&pose)||!read(name+".motions",&motions)||!borgs_[i].load(i?10:0x615,{model.data(),model.size()},{prefix.data(),prefix.size()},{pose.data(),pose.size()},{motions.data(),motions.size()},error))return false;
            }
            borgs_[i].upload(render,10000+i*2000);
        }
        if(render.shared_texture_upload_failed())return false;
        source_practice_package_=practice!=nullptr;ready_=true;return true;
    }
    bool load_packages(IFileSystem& files,const std::string& directory,RenderEngine& render,
                       std::span<const std::shared_ptr<const BorgGameplayProfile>> definitions,
                       std::span<const BorgBattleVisualAssets> assets,std::string* error,const std::string& stage_collision={}){
        clear_stage();
        auto read=[&](const std::string& name,std::vector<u8>* out){
            u64 size=0;const auto path=directory+"/"+name;
            if(files.file_size(path,&size)&&size&&size<=(32u<<20)&&files.read(path,*out)&&out->size()==size)return true;
            if(error)*error="Borg Battle Test: missing or oversized stage asset "+path;
            return false;
        };
        if(!load_stage(read,render,error))return false;
        if(needs_projectile_stage(definitions)&&!bind_projectile_stage(files,directory,stage_collision,error))return false;
        if(files.exists(directory+"/view-fade.txt")){
            std::vector<u8> bytes;u64 length=0;const auto path=directory+"/view-fade.txt";
            if(floor_hash_==source_floor_hash_||stage_bank_.hash()==source_stage_hash_||
               !files.file_size(path,&length)||!length||length>4096||!files.read(path,bytes)||bytes.size()!=length){
                if(error)*error="stage viewport fade: authored local descriptor required";
                return false;
            }
            if(!bind_stage_fades(render,{reinterpret_cast<const char*>(bytes.data()),bytes.size()},error))return false;
        }
        return bind_package_borgs(render,definitions,assets,error);
    }
    bool load_provisional_walk(IFileSystem& files,const std::string& directory,RenderEngine& render,
                               ByteView model,ByteView prefix,ByteView motions,
                               const std::array<BattleMvpMotionChoice,2>& roles,const BattleMvpConfig& config,std::string* error,
                               const ProvisionalBorgJumpClips* jump=nullptr,std::span<const CharacterPreviewMount> mounts={},
                               const ProvisionalBorgDashClips* dash=nullptr,const ProvisionalBorgAirDashClips* air_dash=nullptr,
                                const ProvisionalBorgGlideClips* glide=nullptr,const BattleMvpMotionChoice* cruise_entry=nullptr){
        if(render.frame_in_progress()||!config.provisional_walk||
           config.provisional_jump!=bool(jump)||config.provisional_ground_dash!=bool(dash)||
           config.provisional_air_dash!=bool(air_dash)||config.provisional_glide!=bool(glide)){
            if(error)*error="movement lab: matching configuration between frames required";
            return false;
        }
        const bool replacing=provisional_walk_view_&&ready_;
        if(!replacing)clear_stage();
        auto read=[&](const std::string& name,std::vector<u8>* out){
            u64 size=0;const auto path=directory+"/"+name;
            if(files.file_size(path,&size)&&size&&size<=(32u<<20)&&files.read(path,*out)&&out->size()==size)return true;
            if(error)*error="walk lab: missing or oversized stage asset "+path;
            return false;
        };
        if(!replacing&&!load_stage(read,render,error))return false;
        NativeBattleMvpBorg next;
        if(!next.load_provisional_walk(model,prefix,motions,roles,error,jump,mounts,dash,air_dash,glide,
                                       bool(config.provisional_cruise),cruise_entry))return false;
        // Qualify the initial pose on this same retained stage before replacing
        // any live body state or consuming one of the two private texture ranges.
        NativeBattleMvp probe;
        ready_=true;
        if(!probe.start(config,this)||!next.update(probe.actors()[0],probe.tick(),error)){
            ready_=replacing;
            if(error&&error->empty())*error="movement lab: initial body pose refused";
            return false;
        }
        const auto textures=next.scene_.asset().scene.textures.size();
        const auto next_slot=replacing?1u-walk_texture_slot_:0u;
        const auto next_base=walk_texture_base_+1024u*next_slot;
        if(textures>1024){ready_=replacing;if(error)*error="movement lab: texture budget";return false;}
        for(u32 i=0;i<textures;++i)if(render.texture_is_shared(next_base+i)){
            ready_=replacing;if(error)*error="movement lab: private texture range unavailable";return false;
        }
        if(next.scene_.upload_textures(render,next_base)!=textures){
            for(u32 i=0;i<textures;++i)render.release_texture(next_base+i);
            ready_=replacing;if(error)*error="movement lab: body texture upload refused";return false;
        }
        if(replacing)for(u32 i=0;i<walk_textures_;++i)render.release_texture(walk_texture_base_+1024u*walk_texture_slot_+i);
        borgs_[0]=std::move(next);walk_texture_slot_=next_slot;walk_textures_=unsigned(textures);
        provisional_jump_view_=jump!=nullptr;provisional_dash_view_=dash!=nullptr;provisional_air_dash_view_=air_dash!=nullptr;
        provisional_glide_view_=glide!=nullptr;
        provisional_cruise_view_=bool(config.provisional_cruise);
        provisional_walk_view_=ready_=true;return true;
    }
    // Bind only the selected stage. No package path is consulted at runtime or
    // on restart; the renderer and collision queries own the accepted data.
    bool bind_stage(const StagePackageSnapshot& stage,RenderEngine& render,std::string* error){
        const auto fail=[&](const char* reason){if(error)*error=reason;return false;};
        if(source_practice_)return fail("unsupported portable stage mode: source movement practice has its own stage");
        clear_stage();
        if(!stage.valid())return fail("malformed stage snapshot");
        const auto model=stage.asset(StageAsset::Model),floor=stage.asset(StageAsset::Floor);
        if(character_program_model_hash(floor)==source_floor_hash_||character_program_model_hash(model)==source_stage_hash_)
            return fail("unsupported portable stage: pinned retail stage inputs require the exact legacy qualification");
        if(!load_stage(model,floor,render,error))return false;
        if(floor_.cell_size_x()!=500||floor_.cell_size_z()!=500)
            return fail("unsupported portable stage floor: expected a 500-unit STIH grid");
        const auto declaration=stage.asset(StageAsset::ProjectileStage);
        if(declaration.size){
            const auto secondary=stage.asset(StageAsset::Secondary),tertiary=stage.asset(StageAsset::Tertiary);
            const auto source_grid=[&](ByteView bytes){
                const auto hash=character_program_model_hash(bytes);
                return std::find(source_grid_hashes_.begin(),source_grid_hashes_.end(),hash)!=source_grid_hashes_.end();
            };
            if(source_grid(secondary)||source_grid(tertiary))
                return fail("unsupported portable stage: pinned retail projectile grids require the exact legacy qualification");
            if(!bind_authored_projectile_stage(declaration,{secondary,tertiary},error))return false;
        }
        const auto fade=stage.asset(StageAsset::ViewFade);
        if(fade.size&&!bind_stage_fades(render,{reinterpret_cast<const char*>(fade.data),fade.size},error))return false;
        if(render.shared_texture_upload_failed())return fail("stage texture upload failed");
        stage_manifest_=stage.manifest;
        if(error)error->clear();
        return true;
    }
    std::string_view stage_id()const{return stage_manifest_?stage_manifest_->id:std::string_view{};}
    bool configure_stage_duel(std::string_view id,const std::array<std::string_view,2>& marker_ids,
                              BattleMvpConfig* config,std::string* error)const{
        return configure_stage_roster(id,marker_ids,config,error);
    }
    // Preflight resolves placement before loading Borg visuals. start()
    // still owns initial facing and final atomic admission of the world.
    bool configure_stage_roster(std::string_view id,std::span<const std::string_view> marker_ids,
                                BattleMvpConfig* config,std::string* error)const{
        const auto fail=[&](std::string reason){if(error)*error=std::move(reason);return false;};
        if(!config||!stage_manifest_||id!=stage_manifest_->id)return fail("exact bound stage ID required");
        if(config->solo()||marker_ids.size()<2||marker_ids.size()>kBattleMvpMaxActors||
           marker_ids.size()!=config->declared_actors)return fail("one stage spawn marker per battle actor required");
        for(unsigned i=0;i<marker_ids.size();++i)for(unsigned j=0;j<i;++j)
            if(marker_ids[i]==marker_ids[j])return fail("stage spawn markers must be distinct: "+std::string(marker_ids[i]));
        auto next=*config;next.arena_half=stage_manifest_->arena_half;
        if(!std::isfinite(next.actor_radius)||next.actor_radius<=0||4*next.actor_radius>=next.arena_half)
            return fail("stage arena is too small for the native battle body bounds");
        for(unsigned i=0;i<marker_ids.size();++i){
            const auto* marker=stage_manifest_->spawn(marker_ids[i]);
            if(!marker)return fail("missing stage spawn marker: "+std::string(marker_ids[i]));
            const auto& p=marker->query;
            if(std::abs(p[0])>next.arena_half-next.actor_radius||std::abs(p[2])>next.arena_half-next.actor_radius)
                return fail("stage spawn outside arena body bounds: "+marker->id);
            f32 clearance=0;
            if(!native_borg_partition_portion(this,{p[0],p[2]},{},next.actor_radius,&clearance)||clearance<1)
                return fail("stage spawn intersects a body partition: "+marker->id);
            f32 support=0;
            if(!height(p[0],p[2],p[1],&support)||!std::isfinite(support)||std::abs(support)>20000)
                return fail("stage spawn has no supported floor: "+marker->id);
            next.spawn[i]={p[0],p[2]};next.spawn_query_y[i]=p[1];
            for(unsigned j=0;j<i;++j){
                const f32 dx=p[0]-next.spawn[j].x,dz=p[2]-next.spawn[j].z;
                if(dx*dx+dz*dz<=4*next.actor_radius*next.actor_radius)
                    return fail("stage spawn markers overlap the native bodies: "+std::string(marker_ids[j])+" and "+marker->id);
            }
        }
        *config=std::move(next);if(error)error->clear();return true;
    }
    bool load_packages(RenderEngine& render,std::span<const std::shared_ptr<const BorgGameplayProfile>> definitions,
                       std::span<const BorgBattleVisualAssets> assets,std::string* error){
        ready_=false;
        if(!stage_manifest_){
            if(error)*error="exact portable stage binding required";
            return false;
        }
        for(const auto& def:definitions)if(def&&def->movement==BorgBattleMovement::NormalGRed){
            if(error)*error="unsupported portable stage movement: normal G-Red requires the exact legacy stage";
            return false;
        }
        if(needs_projectile_stage(definitions)&&!projectile_stage_ready_){
            if(error)*error="unsupported stage projectile capability: declare projectile-stage, secondary and tertiary";
            return false;
        }
        return bind_package_borgs(render,definitions,assets,error);
    }
private:
    bool bind_package_borgs(RenderEngine& render,std::span<const std::shared_ptr<const BorgGameplayProfile>> definitions,
                            std::span<const BorgBattleVisualAssets> assets,std::string* error){
        projectile_records_={};
        impact_record_scenes_={};
        u32 next_projectile_texture=118000;
        const auto bind_record=[&](AnimatedModelScene& scene,const HsdRecordAnimationBank& bank,u16 record,bool fade=false){
            if(!scene.load_compact_static(bank,record,error)||!scene.request(0,0,error))return false;
            const auto& meshes=scene.asset()->meshes;
            if(fade&&!std::all_of(meshes.begin(),meshes.end(),borg_impact_record_fade_supported)){
                if(error)*error="Borg Battle Test: impact growth requires material-alpha blending without depth writes";
                return false;
            }
            scene.set_enabled(true);scene.set_untextured_billboards(true);
            const auto count=scene.asset()->textures.size();
            if(count>2048||next_projectile_texture+count>120048||
               scene.upload_textures_shared(render,next_projectile_texture)!=count){
                if(error)*error="Borg Battle Test: effect record texture budget or upload";
                return false;
            }
            next_projectile_texture+=u32(count);return true;
        };
        if(definitions.size()!=assets.size()||definitions.empty()||definitions.size()>kBattleMvpMaxActors){
            if(error)*error="Borg Battle Test: package/visual count";
            return false;
        }
        for(unsigned i=0;i<definitions.size();++i){
            const auto& a=assets[i];
            if(!borgs_[i].load_package(definitions[i],a.model,a.prefix,a.pose,a.motions,error))return false;
            borgs_[i].upload(render,10000+i*2000);
            HsdRecordAnimationBank projectile_bank,impact_bank;
            const auto bind_impact=[&](u8 slot,u16 record,u16 scale,u16 ticks,u16 end_scale){
                if(!impact_bank.records()&&!impact_bank.load(a.impact_record_bank,error,true))return false;
                auto& entry=impact_record_scenes_[i][slot];
                entry.scale_percent=scale;entry.end_scale=end_scale;entry.ticks=ticks;
                return bind_record(entry.scene,impact_bank,record,end_scale!=0);
            };
            for(u8 j=0;j<definitions[i]->projectiles.size();++j){
                if(const auto& effect=definitions[i]->projectiles[j].impact_record;effect){
                    if(!bind_impact(j,effect->record,effect->scale_percent,effect->ticks,effect->end_scale))return false;
                }
                const auto& look=definitions[i]->projectiles[j].record_look;
                if(!look)continue;
                if(!projectile_bank.records()&&!projectile_bank.load(a.model,error,true))return false;
                auto& entry=projectile_records_[i][j];entry.scale_percent=look->scale_percent;
                if(!bind_record(entry.scene,projectile_bank,look->record))return false;
                if(const auto& t=definitions[i]->projectiles[j].record_trail;t){
                    entry.trail.emplace();entry.trail_scale=t->scale_percent;entry.trail_capacity=t->count;
                    if(!bind_record(*entry.trail,projectile_bank,t->record))return false;
                }
            }
            for(unsigned m=0;m<definitions[i]->moves.size();++m){
                const auto& contact=definitions[i]->moves[m].contact_impacts;
                for(unsigned team=0;team<contact.size();++team)
                    for(unsigned layer=0;layer<contact[team].size();++layer){
                        const auto& effect=contact[team][layer];
                        if(!bind_impact(borg_contact_impact_slot(m,team,layer),effect.record,effect.scale_percent,
                                        effect.ticks,0))return false;
                    }
                const auto& field=definitions[i]->moves[m].field;
                if(!field||field->texture.empty())continue;
                const auto tex=a.field_texture[m];
                if(tex.size!=kBorgFieldTextureBytes||!tex.data||
                   !render.upload_texture_shared(field_texture(i,m),{kBorgFieldTextureSide,kBorgFieldTextureSide,tex.data,0,0,true})){
                    if(error)*error="Borg Battle Test: field texture needs a local 32x32 RGBA8 image";
                    return false;
                }
            }
        }
        bool needs_ink=false;
        for(const auto& def:definitions)if(def)for(const auto& shot:def->projectiles)needs_ink|=shot.ink.has_value();
        if(needs_ink){
            const auto rgba=ink_splatter_rgba();
            if(!render.upload_texture_shared(ink_texture_,{kInkSplatterSize,kInkSplatterSize,rgba.data(),0,0,true})){
                if(error)*error="Borg Battle Test: ink splatter upload failed";
                return false;
            }
            ink_ready_=true;
        }
        ready_=!render.shared_texture_upload_failed();return ready_;
    }
public:
    bool supports_projectile_stage()const override{return (ready_||stage_manifest_)&&projectile_stage_ready_;}
    bool projectile_stage(const BattleMvpVec& start,const BattleMvpVec& end,NativeBorgStageProjectileHit* out)const override{
        if(!out||!supports_projectile_stage())return false;
        StageProjectileSegment hit;
        if(!stage_projectile_segment({&floor_,&projectile_grids_[0],&projectile_grids_[1]},projectile_bounds_,end,start,&hit,nullptr)||
           hit.surface.nearest.excluded_bounds)return false;
        NativeBorgStageProjectileHit next;
        if(hit.surface.grid_code){
            // The parked face remains outside this native acceptance seam.
            if(projectile_source_stage_&&hit.surface.grid_code==1&&hit.surface.nearest.triangle==1130)return false;
            f64 numerator=0,denominator=0;
            for(unsigned i=0;i<3;++i){
                const f64 delta=f64(end[i])-start[i];
                numerator+=delta*(f64(hit.position[i])-start[i]);denominator+=delta*delta;
            }
            const f64 fraction=denominator>0?numerator/denominator:-1;
            if(!std::isfinite(fraction)||fraction<-.0001||fraction>1.0001)return false;
            next={true,f32(std::clamp(fraction,0.,1.)),hit.surface.grid_code,hit.surface.nearest.triangle};
        }
        *out=next;return true;
    }
    std::span<const StageBodyPartition> body_partitions()const override{
        return stage_manifest_?std::span<const StageBodyPartition>(stage_manifest_->body_partitions):
                               std::span<const StageBodyPartition>{};
    }
    bool height(f32 x,f32 z,f32 current_y,f32* out)const override{
        NativeBorgStaticSurface surface;
        if(!out||!static_surface(x,z,current_y,&surface))return false;
        *out=surface.height;return true;
    }
    bool coplanar_crossing(u32 from,u32 to,const BattleMvpVec& start,const BattleMvpVec& end)const override{
        return ready_&&stage_ground_coplanar_crossing(floor_,from,to,start,end);
    }
    bool gentle_slope_crossing(u32 from,u32 to,const BattleMvpVec& start,const BattleMvpVec& end)const override{
        return ready_&&stage_ground_slope_crossing(floor_,from,to,start,end,true);
    }
    bool inclined_ledge_crossing(u32 from,u32 to,const BattleMvpVec& start,const BattleMvpVec& end,f32 step)const override{
        return ready_&&stage_inclined_ledge_crossing(floor_.triangles(),from,to,start,end,step);
    }
    bool same_face_push_exit(u32 face,const BattleMvpVec& start,const BattleMvpVec& integrated,const BattleMvpVec& projected)const override{
        const StageGroundParameters p{-100000,100000,500,0,-.5f,20000,-20000,true};
        return ready_&&stage_ground_same_face_push_exit(floor_,p,face,start,integrated,projected);
    }
    bool trace_floor_approach(u32 face,const BattleMvpVec& start,const BattleMvpVec& end,StageFloorApproachTrace* out)const{
        return ready_&&stage_ground_trace_floor_approach(floor_,face,start,end,out);
    }
    bool static_surface(f32 x,f32 z,f32 current_y,NativeBorgStaticSurface* out)const override{
        const bool authored=stage_manifest_.has_value();
        if((!ready_&&!authored)||!out)return false;
        if(authored&&(x<floor_.minimum_x()||z<floor_.minimum_z()))return false;
        const StageGroundParameters p{-100000,100000,500,0,-.5f,20000,-20000,true};std::span<const u32> refs;
        const std::array<f32,3> point{x,current_y,z};if(!stage_ground_detail::cell(floor_,p,point,&refs))return false;
        bool found=false,below=false;f32 best=-20000,lowest=20000;
        NativeBorgStaticSurface selected{-20000},low{20000};
        for(u32 index:refs){const auto& triangle=floor_.triangles()[index];std::array<f32,2> delta;f32 y=0;
            if(authored){
                const auto& a=triangle.vertices[0];const auto& b=triangle.vertices[1];const auto& c=triangle.vertices[2];
                const auto area=(f64(b[0])-a[0])*(f64(c[2])-a[2])-(f64(b[2])-a[2])*(f64(c[0])-a[0]);
                if(triangle.plane[1]<=0||area>=0)continue;
            }
            if(!stage_ground_detail::projected(triangle,point,false,&delta))continue;
            if(!stage_ground_detail::height(triangle,delta,&y))return false;
            if(authored&&std::abs(y)>20000)continue;
            const NativeBorgStaticSurface candidate{y,{triangle.plane[0],triangle.plane[1],triangle.plane[2]},index};
            //Tie metadata is last-wins, like the source paired query; height's
            //numeric max/min behavior remains unchanged for ordinary MVP users.
            found=true;if(y<=lowest){lowest=y;low=candidate;}
            if(y<=current_y+45&&(!below||y>=best)){below=true;best=y;selected=candidate;}}
        if(!found)return false;
        *out=below?selected:low;return true;
    }
    bool update(const NativeBattleMvp& world,std::string* error){
        source_practice_frame_.reset();rendered_scope_.reset();
        if(!ready_||!world.ready()||world.config().provisional_walk!=provisional_walk_view_||
           world.config().provisional_jump!=provisional_jump_view_||world.config().provisional_ground_dash!=provisional_dash_view_||
           world.config().provisional_air_dash!=provisional_air_dash_view_||world.config().provisional_glide!=provisional_glide_view_||
           bool(world.config().provisional_cruise)!=provisional_cruise_view_)return false;
        for(unsigned i=0;i<world.actor_count();++i)
            if(!borgs_[model_slot(world,i)].update(world.actors()[i],world.tick(),error,this))return false;
        rendered_scope_=world.presentation_scope();rendered_tick_=world.tick();publish_movement_fx(world);
        return true;
    }
    void reset_control_test_pose(){
        for(auto& borg:borgs_)borg.reset_control_test_pose();
        source_practice_frame_.reset();rendered_scope_.reset();
        movement_pulses_={};
        for(auto& obstacle:stage_fades_)obstacle.views={};
    }
    bool source_seed(const NativeBattleMvp& world,NativeBorgAnimationSeed* out)const{return source_seed(0,world,out);}
    bool source_seed(unsigned actor,const NativeBattleMvp& world,NativeBorgAnimationSeed* out)const{
        if(!ready_||actor>=world.actor_count()||!world.ready()||!out||rendered_tick_!=world.tick()||!world.owns_motion_scope(rendered_scope_)||
           !borgs_[model_slot(world,actor)].animation_seed(world.actors()[actor],world.tick(),out))return false;
        out->stage_hash=stage_bank_.hash();out->floor_hash=floor_hash_;out->world_scope=rendered_scope_;return true;
    }
    std::weak_ptr<const CharacterModelBinding> source_binding()const{return source_binding(0);}
    std::weak_ptr<const CharacterModelBinding> source_binding(unsigned actor)const{
        return actor<borgs_.size()?borgs_[actor].render_binding():std::weak_ptr<const CharacterModelBinding>{};
    }
    bool package_socket(unsigned owner,const BattleMvpActor& actor,u32 tick,u8 move,u16 frame,
                        const BorgBattleSocket& spec,const CharacterLocalPose* source_local,BattleMvpVec* out)const{
        return ready_&&owner<borgs_.size()&&borgs_[owner].package_socket(actor,tick,move,frame,spec,source_local,out);
    }
    struct SourcePoseView {unsigned actor=0;const NativeBorgSourcePose* pose=nullptr;};
    bool update_source(const NativeBattleMvp& world,const NativeBorgSourcePose& source,std::string* error){
        const SourcePoseView one{0,&source};
        return update_sources(world,std::span<const SourcePoseView>(&one,1),error);
    }
    bool update_sources(const NativeBattleMvp& world,std::span<const SourcePoseView> poses,std::string* error){
        source_practice_frame_.reset();rendered_scope_.reset();
        if(!ready_){if(error)*error="source view: not ready";return false;}
        if(source_publication_==std::numeric_limits<u64>::max()){if(error)*error="source view: publication exhausted";return false;}
        std::array<bool,kBattleMvpMaxActors> posed{};
        for(const auto& entry:poses){
            if(!entry.pose||entry.actor>=world.actor_count()){if(error)*error="source view: actor/pose outside the world";return false;}
            const unsigned slot=model_slot(world,entry.actor);
            if(!world.motion_receipt_current(entry.pose->receipt,entry.actor)){
                borgs_[slot].refuse_source_pose();if(error)*error="source view: stale/foreign native contact receipt";return false;
            }
            if(!borgs_[slot].update_source_pose(entry.actor,world,*entry.pose,error))return false;
            posed[entry.actor]=true;
        }
        for(unsigned i=0;i<world.actor_count();++i)
            if(!posed[i]&&!borgs_[model_slot(world,i)].update(world.actors()[i],world.tick(),error))return false;
        if(source_practice_&&!poses.empty())source_practice_frame_=poses[0].pose->frame_scope;
        rendered_scope_=world.presentation_scope();rendered_tick_=world.tick();++source_publication_;publish_movement_fx(world);return true;
    }
    // Proves publication of the exact source frame without sampling any bones.
    // A republish at the same tick has a distinct serial.
    bool source_pose_publication(const NativeBattleMvp& world,const NativeBorgSourcePose& pose,u64* out)const{
        // Package upper-body overlays are authored presentation, not a
        // qualified source muzzle or source-trial pose publication.
        if(!out||!ready_||world.config().packages[0]||!source_publication_||rendered_tick_!=world.tick()||!world.owns_motion_scope(rendered_scope_)||
           !borgs_[0].source_pose_published(world,pose))return false;
        *out=source_publication_;return true;
    }
    bool observe_gred_source_world(const NativeBattleMvp& world,NativeGRedPracticeWorldMatrices* out)const{
        return ready_&&!world.config().packages[0]&&rendered_tick_==world.tick()&&world.owns_motion_scope(rendered_scope_)&&
            borgs_[0].observe_source_world_matrices(world,out);
    }
    bool actor_bounds(unsigned actor,BattleMvpVec* low,BattleMvpVec* high)const{return actor<borgs_.size()&&borgs_[actor].bounds(low,high);}
    static Camera camera_for_actors(const BattleMvpActor& player,const BattleMvpActor& target,s32 width,s32 height){
        Camera camera;const auto line=native_borg_direction({player.position[0],player.position[2]},{target.position[0],target.position[2]});
        camera.eye[0]=player.position[0]-line.x*650+line.z*220;camera.eye[1]=player.position[1]+340;camera.eye[2]=player.position[2]-line.z*650-line.x*220;
        for(unsigned k=0;k<3;++k){camera.target[k]=target.position[k];}
        camera.target[1]+=120;
        camera.fov_y_degrees=55;camera.near_plane=5;camera.far_plane=25000;camera.viewport_width=width;camera.viewport_height=height;
        if(player.source_ground_motion&&std::abs(player.position[1]-target.position[1])>250&&width>0&&height>0){
            //Native presentation only: fit both model-center proxies in the
            //existing camera when source ascent exceeds the MVP hop's range.
            //No input/body heading or recovered source-camera claim is made.
            const f32 dx=player.position[0]-target.position[0],dy=player.position[1]-target.position[1],dz=player.position[2]-target.position[2];
            const f32 radius=.5f*std::hypot(dx,dy,dz)+180;
            const f32 half_angle=std::atan(std::tan(55.f*3.14159265f/360)*std::min(1.f,f32(width)/height));
            const f32 distance=radius/std::sin(half_angle)*1.08f,planar=distance*.9396926f;
            for(unsigned k=0;k<3;++k)camera.target[k]=(player.position[k]+target.position[k])*.5f;
            camera.target[1]+=120;
            camera.eye[0]=camera.target[0]+planar*(-line.x*.9486833f+line.z*.3162278f);
            camera.eye[1]=camera.target[1]+distance*.3420201f;
            camera.eye[2]=camera.target[2]+planar*(-line.z*.9486833f-line.x*.3162278f);
        }
        return camera;
    }
    static Camera camera_for(const NativeBattleMvp& world,s32 width,s32 height){
        if(world.actor_count()!=1){
            const u8 target=world.target_of(0);
            const unsigned other=target==255?1u:unsigned(target);
            return camera_for_actors(world.actors()[0],world.actors()[other],width,height);
        }
        Camera camera;const auto& p=world.actors()[0].position;
        camera.target[0]=p[0];camera.target[1]=p[1]+76;camera.target[2]=p[2];
        camera.eye[0]=p[0]+168;camera.eye[1]=p[1]+244;camera.eye[2]=p[2]+490;
        camera.fov_y_degrees=55;camera.near_plane=5;camera.far_plane=25000;
        camera.viewport_width=width;camera.viewport_height=height;
        return camera;
    }
    static NativeBorgMovementFrame roster_movement_frame(const NativeBattleMvp& world){
        // Derive the view from the opening placements, not live targets.
        // For one local or allied locals, orient toward the opposition.
        const auto& spawn=world.config().spawn;
        const unsigned locals=world.config().roster_local_players;
        auto from=spawn[0],to=spawn[1];
        if(world.actor_count()>2&&(locals==1||world.actors()[0].team==world.actors()[1].team)){
            BattlePoint enemies{};
            unsigned count=0,first=0;
            for(unsigned i=locals;i<world.actor_count();++i)if(world.actors()[i].team!=world.actors()[0].team){
                if(!count)first=i;
                enemies.x+=spawn[i].x;enemies.z+=spawn[i].z;++count;
            }
            if(count){
                if(locals==2)from={(from.x+to.x)*.5f,(from.z+to.z)*.5f};
                to={enemies.x/count,enemies.z/count};
                if(from==to){from=spawn[0];to=spawn[first];}
            }
        }
        const auto line=native_borg_direction(from,to);
        NativeBorgMovementFrame frame;
        native_borg_movement_frame({.6f*line.x-.8f*line.z,.6f*line.z+.8f*line.x},&frame);
        return frame;
    }
    struct RosterBodyBounds {BattleMvpVec low,high;};
    using RosterBounds=std::array<std::optional<RosterBodyBounds>,kBattleMvpMaxActors>;
    struct RosterFraming {Camera camera;RosterBounds bodies;};
    static Camera roster_camera_for(const NativeBattleMvp& world,s32 width,s32 height,const RosterBounds& bodies={}){
        if(world.actor_count()<=2||width<1||height<1)return camera_for(world,width,height);
        Camera camera;
        camera.fov_y_degrees=55;camera.near_plane=5;camera.far_plane=25000;
        camera.viewport_width=width;camera.viewport_height=height;
        const auto& config=world.config();
        const auto forward=roster_movement_frame(world).forward;
        BattleMvpVec low{},high{};
        bool found=false;
        for(unsigned i=0;i<world.actor_count();++i){
            const auto& actor=world.actors()[i];
            if(!actor.health)continue;
            const auto& p=actor.position;
            BattleMvpVec min{p[0]-config.actor_radius,p[1],p[2]-config.actor_radius};
            BattleMvpVec max{p[0]+config.actor_radius,p[1]+config.actor_height,p[2]+config.actor_radius};
            if(bodies[i])for(unsigned k=0;k<3;++k){
                min[k]=std::min(min[k],bodies[i]->low[k]);max[k]=std::max(max[k],bodies[i]->high[k]);
            }
            max[1]+=18; // leave room for the aim label above the body
            for(unsigned k=0;k<3;++k){low[k]=found?std::min(low[k],min[k]):min[k];high[k]=found?std::max(high[k],max[k]):max[k];}
            found=true;
        }
        if(!found)return camera_for(world,width,height);
        constexpr f32 distance=800.f;
        const f32 horizontal=distance*.9f;
        for(unsigned k=0;k<3;++k)camera.target[k]=(low[k]+high[k])*.5f;
        camera.eye[0]=camera.target[0]-forward.x*horizontal;
        camera.eye[1]=camera.target[1]+distance*std::sqrt(1.f-.9f*.9f);
        camera.eye[2]=camera.target[2]-forward.z*horizontal;
        auto fitted=camera;
        if(camera_dolly_to_fit_bounds(fitted,low,high,true))return fitted;
        return camera;
    }
    RosterFraming roster_framing(const NativeBattleMvp& world,s32 width,s32 height)const{
        RosterFraming frame{};
        // A restart can publish another pose for the same tick. Check the
        // world scope as well as the tick before using any rendered geometry.
        if(ready_&&rendered_tick_==world.tick()&&world.owns_motion_scope(rendered_scope_)){
            for(unsigned i=0;i<world.actor_count();++i)if(world.actors()[i].health){
                RosterBodyBounds bounds{};
                if(!package_body_bounds(world,i,&bounds.low,&bounds.high))continue;
                bool valid=true;
                for(unsigned k=0;k<3;++k)
                    valid&=std::isfinite(bounds.low[k])&&std::isfinite(bounds.high[k])&&
                           bounds.low[k]>=-1e5f&&bounds.high[k]<=1e5f&&bounds.low[k]<=bounds.high[k];
                if(valid)frame.bodies[i]=bounds;
            }
        }
        frame.camera=roster_camera_for(world,width,height,frame.bodies);
        return frame;
    }
    static Camera close_practice_camera(const NativeBattleMvp& world,s32 width,s32 height){
        auto camera=camera_for(world,width,height);
        //Keep the existing target-relative azimuth. Reframe G-Red for
        //inspection without changing the separate movement-frame calculation.
        const auto forward=native_borg_direction({camera.eye[0],camera.eye[2]},{camera.target[0],camera.target[2]});
        const auto& player=world.actors()[0].position;
        for(unsigned i=0;i<3;++i)camera.target[i]=player[i];
        camera.target[1]+=95;
        camera.eye[0]=camera.target[0]-forward.x*430;
        camera.eye[1]=camera.target[1]+170;
        camera.eye[2]=camera.target[2]-forward.z*430;
        return camera;
    }
    static NativeBorgMovementFrame movement_frame(const Camera& camera){
        NativeBorgMovementFrame frame;
        native_borg_movement_frame({camera.target[0]-camera.eye[0],camera.target[2]-camera.eye[2]},&frame);
        return frame;
    }
    static NativeBorgMovementFrame movement_frame(const NativeBattleMvp& world,s32 width,s32 height){
        return movement_frame(camera_for(world,width,height));
    }
    static NativeBorgMovementFrame movement_frame_for(const NativeBattleMvp& world,unsigned actor,s32 width,s32 height){
        if(actor>=world.actor_count())actor=0;
        const u8 target=world.target_of(actor);
        const unsigned other=target==255?(actor==0?1:0):unsigned(target);
        const auto camera=camera_for_actors(world.actors()[actor],world.actors()[other],width,height);
        NativeBorgMovementFrame frame;native_borg_movement_frame({camera.target[0]-camera.eye[0],camera.target[2]-camera.eye[2]},&frame);return frame;
    }
    std::optional<NativeBorgMotion> provisional_motion(const NativeBattleMvp& world)const{
        if(!provisional_walk_view_||!world.ready()||rendered_tick_!=world.tick()||
           !world.owns_motion_scope(rendered_scope_)||!borgs_[0].presentation_ready_)return std::nullopt;
        if(borgs_[0].provisional_animated_cruise_&&world.actors()[0].package_boosting)return NativeBorgMotion::LabBoost;
        return NativeBorgMotion(borgs_[0].base_);
    }
    unsigned draw(RenderEngine& render,const NativeBattleMvp& world,s32 width,s32 height,std::string* error,
                   bool practice_input_refused=false,unsigned viewer=0,const Camera* viewer_camera=nullptr,
                   bool shared_roster_view=false,const BattleRosterSeries* series=nullptr){
        projectile_record_draws_=0;
        projectile_trail_draws_=0;
        impact_record_draws_=0;
        package_part_draws_={};
        package_body_draws_={};
        if(!ready_||!world.ready()||width<=0||height<=0||viewer>=world.actor_count())return 0;
        if(!stage_fades_.empty()&&viewer>=stage_fades_.front().views.size()){
            if(error)*error="stage fade: viewport outside the two-view scope";
            return 0;
        }
        if(world.config().provisional_walk!=provisional_walk_view_||world.config().provisional_jump!=provisional_jump_view_||
           world.config().provisional_ground_dash!=provisional_dash_view_||
           world.config().provisional_air_dash!=provisional_air_dash_view_||
           world.config().provisional_glide!=provisional_glide_view_||
           bool(world.config().provisional_cruise)!=provisional_cruise_view_||
           (provisional_walk_view_&&(rendered_tick_!=world.tick()||!world.owns_motion_scope(rendered_scope_))))return 0;
        const bool roster_view=shared_roster_view&&world.actor_count()>2&&bool(world.config().packages[0]);
        const unsigned locals=world.config().roster_local_players;
        if(!roster_view||(series&&!series->owns_round(world)))series=nullptr;
        const auto round_result=series&&series->ended()?series->outcome():world.result();
        const auto framing=roster_view?roster_framing(world,width,height):RosterFraming{};
        auto camera=viewer_camera?*viewer_camera:roster_view?framing.camera:
            source_practice_&&source_practice_close_camera_?close_practice_camera(world,width,height):
            viewer==1?camera_for_actors(world.actors()[1],world.actors()[0],width,height):camera_for(world,width,height);
        camera.viewport_width=width;camera.viewport_height=height;
        if(provisional_walk_view_&&!viewer_camera&&!fit_walk_camera(camera,error))return 0;
        if(!viewer_camera&&world.config().packages[0]&&world.actor_count()==2){
            for(unsigned i=0;i<world.actor_count();++i)if(world.actors()[i].health){
                BattleMvpVec bottom{},top{};
                const auto& body=borgs_[model_slot(world,i)].scene_;
                if(body.bounds(&bottom,&top)&&!camera_dolly_to_fit_bounds(camera,bottom,top,true)){
                    if(error)*error="native Battle Test: posed Borg bounds exceed the combat camera domain";
                    return 0;
                }
            }
        }
        unsigned draws=0;
        bool replace_default=false;
        const auto focus=roster_view?shared_roster_fade_focus(world,camera,framing.bodies):
                                     std::array<std::optional<BattleMvpVec>,2>{};
        for(auto& obstacle:stage_fades_){
            const auto& spec=obstacle.profile;auto& fade=obstacle.views[viewer];
            if(spec.gate==StageViewFadeGate::Occlusion){
                const std::array eye{camera.eye[0],camera.eye[1],camera.eye[2]};
                bool have_subject=false,covered=false;
                for(const auto& point:focus)if(point){
                    have_subject=true;
                    if(stage_view_fade_occludes(spec,eye,*point)){covered=true;break;}
                }
                // A single-player view or a shared view with no local in
                // frame keeps the ordinary camera-target fallback.
                if(!have_subject)covered=stage_view_fade_occludes(spec,eye,{camera.target[0],camera.target[1],camera.target[2]});
                fade.update_gate(spec,covered,world.tick());
            }else if(roster_view){
                bool near=false;
                for(unsigned i=0;i<locals;++i)
                    near|=world.actors()[i].health&&stage_view_fade_near(spec,world.actors()[i].position);
                fade.update_gate(spec,near,world.tick());
            }else fade.update(spec,world.actors()[viewer].position,world.tick());
            replace_default|=spec.normal==2;
            if(fade.alpha<1&&!obstacle.faded.replace_static_alpha(fade.alpha,error))return 0;
        }
        const SceneLighting* old=render.scene_lighting();const bool had=old!=nullptr;const SceneLighting saved=had?*old:SceneLighting{};
        SceneLighting light;light.declared=true;SceneLight ambient;ambient.colour[0]=ambient.colour[1]=ambient.colour[2]=180;ambient.colour[3]=255;light.lights.push_back(ambient);
        SceneLight sun;sun.flags=5;sun.colour[0]=sun.colour[1]=sun.colour[2]=190;sun.colour[3]=255;sun.position[0]=-1;sun.position[1]=2;sun.position[2]=1;light.lights.push_back(sun);
        if(!render.set_scene_lighting(light))return 0;
        unsigned actor_draws=0;
        for(u8 pass:{u8(1),u8(4),u8(2)}){
            if(!render.begin_model_pass(pass))return 0;
            draws+=stage_[0].draw(render,camera);
            if(!replace_default)draws+=stage_[1].draw(render,camera);
            for(auto& obstacle:stage_fades_){
                if(obstacle.views[viewer].alpha<1)draws+=obstacle.faded.draw(render,camera);
                else draws+=(obstacle.profile.normal==2?stage_[1]:obstacle.normal).draw(render,camera);
            }
            for(unsigned i=0;i<world.actor_count();++i)
                if(world.actors()[i].health)actor_draws+=borgs_[model_slot(world,i)].draw(render,camera,&package_part_draws_[i],&package_body_draws_[i]);
            if(!render.end_model_pass()){if(error)*error="native MVP model pass failed";return 0;}
        }
        draws+=actor_draws;
        if(had)render.set_scene_lighting(saved);else render.clear_scene_lighting();
        for(unsigned i=0;i<world.actor_count();++i){
            const auto& profile=world.config().packages[i];
            const auto& body=borgs_[model_slot(world,i)].scene_;
            if(world.actors()[i].health&&profile&&!profile->zero_reflections.empty()&&!body.error().empty()){
                if(error)*error=body.error();
                return 0;
            }
        }
        if(world.actor_count()==1){
            if(!world.config().provisional_walk)return draws;
            if(!actor_draws){
                if(error)*error=borgs_[0].scene_.error().empty()?"walk lab: body did not draw":borgs_[0].scene_.error();
                return 0;
            }
            const auto state=render.state();render.state()={};render.state().blend=true;
            render.state().blend_src=5;render.state().blend_dst=6;
            TextRenderer::Style style;style.scale=width>=640?2:1;
            std::string hint=provisional_cruise_view_?(borgs_[0].provisional_animated_cruise_?
                "PROVISIONAL CRUISE LAB / ENTRY CLIP\nARROWS STEER   SHIFT BOOST":
                "PROVISIONAL CRUISE LAB / STATIC BODY\nARROWS STEER   SHIFT BOOST"):
                "PROVISIONAL MOVEMENT LAB / ONE BORG\nARROWS WALK";
            if(provisional_jump_view_)hint+="   SHIFT JUMP";
            if(provisional_dash_view_||provisional_air_dash_view_)hint+="   L DASH";
            hint+="\nQ RESTART   ENTER PAUSE";
            if(provisional_air_dash_view_)hint+="\nONE AIR BURST PER AIRTIME";
            if(provisional_glide_view_)hint+="\nHOLD SHIFT WHILE FALLING TO GLIDE";
            if(world.config().provisional_ground_pace)hint+="\nSOURCE-RELATIVE PACE / NOT RETAIL SPEED";
            if(provisional_cruise_view_){
                const auto& actor=world.actors()[0];
                hint+="\nBOOST "+std::to_string(actor.package_boost_meter.value736)+"/"+
                    std::to_string(world.config().provisional_cruise->capacity)+"   NO COMBAT OR SAVES";
            }else hint+="\nNO BOOST, COMBAT OR SAVES";
            const s32 lines=5+unsigned(provisional_air_dash_view_||provisional_glide_view_)+unsigned(bool(world.config().provisional_ground_pace));
            draws+=text_.draw(render,hint,24,std::max(8,height-lines*text_.line_height(style)),style);
            render.state()=state;
            return draws;
        }
        const auto state=render.state();render.state()={};render.state().depth_test=true;render.state().depth_write=false;render.state().cull_backface=false;render.state().blend=true;render.state().blend_src=5;render.state().blend_dst=6;Mat4 identity;render.upload_matrix(0,identity.m);geometry_.clear();
        for(const auto& shot:world.projectiles())if(shot.active){
            if(shot.from_package&&shot.owner<projectile_records_.size()){
                auto& records=projectile_records_[shot.owner];
                if(auto it=records.find(shot.package_projectile);it!=records.end()){
                    auto& visual=it->second;
                    const auto position_record=[&](AnimatedModelScene& scene,const BattleMvpVec& position,u16 percent){
                        const f32 scale=f32(percent)*.01f;
                        const f32 xyz[]{scale,scale,scale};
                        return scene.set_root_world(mat4_multiply(mat4_translation(position.data()),mat4_scale(xyz)),error);
                    };
                    if(visual.trail)for(unsigned j=0;j<shot.record_trail_count;++j){
                        const auto slot=(shot.record_trail_cursor+visual.trail_capacity-shot.record_trail_count+j)%visual.trail_capacity;
                        if(!position_record(*visual.trail,shot.record_trail_positions[slot],visual.trail_scale)){
                            render.state()=state;return 0;
                        }
                        const auto submitted=visual.trail->draw(render,camera);
                        projectile_trail_draws_+=submitted;draws+=submitted;
                    }
                    if(!position_record(visual.scene,shot.position,visual.scale_percent)){
                        render.state()=state;
                        return 0;
                    }
                    const auto submitted=visual.scene.draw(render,camera);
                    projectile_record_draws_+=submitted;draws+=submitted;
                    continue; // A declared record never falls back to a procedural orb.
                }
            }
            const Tint colour=shot.package_look.size?Tint{shot.package_look.rgb[0],shot.package_look.rgb[1],shot.package_look.rgb[2],255}:
                shot.from_package&&shot.landed?Tint{255,210,80,255}:
                shot.owner?Tint{120,180,255,255}:Tint{255,100,65,255};
            const auto visual=borg_projectile_visual_sample(shot,world.tick());
            const f32 base_size=shot.package_look.size?f32(shot.package_look.size):
                shot.weapon==BattleMvpWeapon::Bomb?28:shot.weapon==BattleMvpWeapon::EnergyBeam?18:12;
            const f32 size=base_size*visual.scale;
            if(shot.sustained){
                const auto ribbon=[&](f32 first,f32 last,f32 width){
                    if(last<=first)return;
                    auto start=shot.position,segment=shot.velocity;
                    for(unsigned i=0;i<3;++i){start[i]+=shot.velocity[i]*first;segment[i]*=last-first;}
                    ScreenDraw::beam_ribbon(camera,start.data(),segment.data(),width,colour,geometry_);
                };
                const auto split=shot.beam_power_visual>100?std::min(shot.beam_fraction,shot.beam_power_entry):shot.beam_fraction;
                ribbon(0,split,size);
                ribbon(split,shot.beam_fraction,size*shot.beam_power_visual*.01f);
                continue;
            }
            if(shot.from_package&&shot.weapon==BattleMvpWeapon::Shuriken){
                ScreenDraw::shuriken(camera,shot.position.data(),size,visual.angle,colour,geometry_);
                continue;
            }
            if(shot.from_package&&shot.weapon==BattleMvpWeapon::Orb){
                ScreenDraw::energy_orb(camera,shot.position.data(),size,visual.angle,colour,geometry_);
                continue;
            }
            std::size_t first=0,count=0;ScreenDraw::billboard(camera,shot.position.data(),size,colour,geometry_,&first,&count);
            if(shot.weapon==BattleMvpWeapon::EnergyBeam){BattleMvpVec trail=shot.position;for(unsigned i=0;i<3;++i)trail[i]-=shot.velocity[i]*.7f;ScreenDraw::billboard(camera,trail.data(),size*.65f,colour,geometry_,&first,&count);}
        }
        for(const auto& owned:world.emitters())for(const auto& emitter:owned)if(emitter.id){
            std::size_t first=0,count=0;
            ScreenDraw::billboard(camera,emitter.position.data(),18,{112,225,255,255},geometry_,&first,&count);
        }
        for(const auto& cloud:world.smoke_clouds())if(cloud.parent){
            const auto age=world.tick()-cloud.born;
            const f32 radius=cloud.spec.radius;
            const u8 alpha=u8(38+70*(1.f-std::min(1.f,f32(age)/cloud.spec.ticks)));
            std::size_t first=0,count=0;
            for(unsigned i=0;i<5;++i){
                const f32 a=f32(i)*2.39996323f;
                const BattleMvpVec centre{cloud.centre[0]+radius*.4f*std::cos(a),
                    cloud.centre[1]+radius*.15f*f32(i%3),cloud.centre[2]+radius*.4f*std::sin(a)};
                ScreenDraw::disc(camera,centre.data(),radius*.7f,12,{150,158,170,alpha},geometry_,&first,&count);
            }
        }
        for(const auto& field:world.fields())if(field.id&&field.spec.texture.empty()){
            const auto& c=field.spec.tint;const Tint tint{c[0],c[1],c[2],c[3]};
            const auto& r=field.spec.extent;
            if(field.spec.shape==BorgFieldShape::Box){
                const f32 half[]{f32(r[0]),f32(r[1]),f32(r[2])};
                ScreenDraw::box(render,camera,field.centre.data(),half,tint,geometry_);
            }else{
                std::size_t first=0,count=0;
                ScreenDraw::disc(camera,field.centre.data(),r[0],24,tint,geometry_,&first,&count);
                ScreenDraw::ring(camera,field.centre.data(),r[0],4,24,{c[0],c[1],c[2],u8(std::min(255,2*int(c[3])))},geometry_,&first,&count);
            }
        }
        for(const auto& link:world.fence_links()){
            const BattleMvpVec delta{link.end[0]-link.start[0],link.end[1]-link.start[1],link.end[2]-link.start[2]};
            ScreenDraw::beam_ribbon(camera,link.start.data(),delta.data(),5,{100,222,255,230},geometry_);
        }
        for(const auto& owned:world.anchor_paths())for(const auto& path:owned)for(unsigned i=0;i<path.count;++i){
            std::size_t first=0,count=0;
            ScreenDraw::ring(camera,path.points[i].position.data(),14,3,12,{135,245,188,255},geometry_,&first,&count);
        }
        draw_movement_fx(world,camera);
        for(const auto& wave:world.walk_pulses())if(wave.id){
            auto origin=wave.origin;origin[1]+=2.f;
            const auto alpha=u8(224u*(wave.spec.ticks-(world.tick()-wave.born))/wave.spec.ticks);
            ScreenDraw::ground_ring(camera,origin.data(),wave.radius(world.tick()),7,24,{255,183,86,alpha},geometry_);
        }
        for(const auto& impact:world.impacts())if(impact.ticks){std::size_t first=0,count=0;ScreenDraw::ring(camera,impact.position.data(),
            impact.radius?impact.radius:impact.explosion?140:28,impact.radius?std::min(12.f,impact.radius*.25f):impact.explosion?12:5,
             16,{255,210,70,u8(252u*impact.ticks/impact.total_ticks)},geometry_,&first,&count);}
        for(const auto& effect:world.impact_records())if(effect.ticks){
            auto& records=impact_record_scenes_[effect.owner];
            const auto it=records.find(effect.binding);
            if(it==records.end()){
                if(error)*error="Borg Battle Test: impact record not bound";
                render.state()=state;return 0;
            }
            auto& visual=it->second;
            const auto sample=borg_impact_record_appearance(visual.scale_percent,visual.end_scale,visual.ticks,effect.ticks);
            const f32 xyz[]{sample.scale,sample.scale,sample.scale};
            if(!visual.scene.set_root_world(mat4_multiply(mat4_translation(effect.position.data()),mat4_scale(xyz)),error)){
                render.state()=state;return 0;
            }
            if(visual.end_scale&&!visual.scene.multiply_static_alpha(sample.alpha,error)){
                render.state()=state;return 0;
            }
            const auto submitted=visual.scene.draw(render,camera);
            impact_record_draws_+=submitted;draws+=submitted;
        }
        if(!geometry_.empty()){const auto base=render.append_vertices(geometry_.data(),geometry_.size());draws+=render.draw(Primitive::Triangles,base,u32(geometry_.size()))?1:0;}
        for(const auto& field:world.fields())if(field.id&&!field.spec.texture.empty()){
            f32 x=0,y=0,z=0;
            if(!camera.project(field.centre.data(),&x,&y,&z))continue;
            const auto& c=field.spec.tint;const auto& r=field.spec.extent;
            const f32 dx=camera.size_scale(r[0],z),dy=camera.size_scale(r[1],z);
            geometry_.clear();ScreenDraw::textured_quad(geometry_,x-dx,y-dy,x+dx,y+dy,z,0,0,1,1,{c[0],c[1],c[2],c[3]});
            render.state().texture=field_texture(field.owner,field.move);
            const auto base=render.append_vertices(geometry_.data(),geometry_.size());draws+=render.draw(Primitive::Triangles,base,u32(geometry_.size()))?1:0;
        }
        render.state()={};render.state().depth_test=render.state().depth_write=false;render.state().blend=true;render.state().blend_src=5;render.state().blend_dst=6;render.state().cull_backface=false;render.upload_matrix(0,identity.m);geometry_.clear();
        if(ink_ready_)if(const auto* ink=world.ink_overlay(viewer)){
            const f32 scale=f32(ink->spec.coverage)*.01f;
            const f32 x=f32(camera.viewport_x)+width*.5f,y=f32(camera.viewport_y)+height*.5f;
            const f32 area_w=width*scale*.5f,area_h=height*scale*.5f;
            const u8 alpha=u8(u32(ink->spec.opacity)*(ink->spec.ticks-(world.tick()-ink->born))/ink->spec.ticks);
            ScreenDraw::textured_quad(geometry_,x-area_w,y-area_h,x+area_w,y+area_h,0,0,0,1,1,{255,255,255,alpha});
            render.state().texture=ink_texture_;
            const auto first=render.append_vertices(geometry_.data(),geometry_.size());
            draws+=render.draw(Primitive::Triangles,first,u32(geometry_.size()))?1:0;
            render.state().texture=0;geometry_.clear();
        }
        if(!roster_view)for(unsigned i=0;i<2;++i){const f32 x=i?f32(width)-310:24;ScreenDraw::textured_quad(geometry_,x,24,x+286,42,0,0,0,1,1,{35,35,45,230});ScreenDraw::textured_quad(geometry_,x,24,x+286*f32(world.actors()[i].health)/world.maximum_health(i),42,0,0,0,1,1,i?Tint{100,170,255,255}:Tint{255,100,70,255});}
        const auto practice=practice_readout(world);
        if(practice&&width>=200&&height>=190){
            const auto meter=practice->meter;
            const f32 width_px=f32(std::min(286,width-48));
            ScreenDraw::textured_quad(geometry_,24,52,24+width_px,62,0,0,0,1,1,{35,35,45,230});
            if(meter.value)ScreenDraw::textured_quad(geometry_,24,52,24+width_px*f32(meter.value)/meter.limit,62,0,0,0,1,1,{255,210,80,255});
        }
        if(!geometry_.empty()){
            const auto base=render.append_vertices(geometry_.data(),geometry_.size());
            draws+=render.draw(Primitive::Triangles,base,u32(geometry_.size()))?1:0;
        }
        TextRenderer::Style style;style.scale=width>=1000?2:1;
        if(roster_view&&height<220)style.scale=1;
        s32 practice_notice_line=3;
        if(practice&&width>=200&&height>=190){
            const auto meter=practice->meter;
            std::string text="FLIGHT "+std::to_string(meter.value)+"/"+std::to_string(meter.limit);
            text+="\n";text+=practice_phase(*practice);
            text+=practice_contact_suffix(practice->kind);
            if(const auto ticks=practice->dash_release_ticks)
                text+=*ticks?"\nRELEASE IN "+std::to_string(*ticks)+" TICKS":"\nCENTER TO RECOVER";
            if(const auto cue=practice_ledge_cue(*practice);!cue.empty())text+="\n"+cue;
            if(practice->recovery_drift_active)text+="\nDRIFT ACTIVE";
            if(practice->recovery_seam)text+="\nFLAT SEAM";
            if(practice->recovery_push_complete)text+="\nRECOVERY COMPLETE";
            if(practice->recovery_push_exit_drift)text+="\nEXIT DRIFT";
            if(practice->plain_windup_relaunch_ready||practice->floor_windup_relaunch_ready){text+="\n";text+=windup_handoff_hint;}
            if(const char* cue=practice_relaunch_floor_cue(*practice)){text+="\n";text+=cue;}
            if(practice->dash_interrupted_takeoff){text+="\n";text+=dash_interrupt_hint;}
            if(practice->landing_dash_started){text+="\n";text+=landing_dash_hint;}
            if(practice->direct_expiry_relaunch){text+="\n";text+=direct_expiry_relaunch_hint;}
            if(const char* cue=practice_direct_timed_relaunch_cue(*practice)){text+="\n";text+=cue;}
            if(practice->direct_expiry_relaunch_ready){text+="\n";text+=direct_expiry_request_hint;}
            if(practice->direct_expiring_relaunch_ready){text+="\n";text+=direct_expiring_request_hint;}
            if(practice->direct_expiring_relaunch){text+="\n";text+=direct_expiring_relaunch_hint;}
            if(practice->direct_expiring_entry_push){text+="\n";text+=direct_expiring_entry_push_hint;}
            if(practice->expiring_walk_air_dash){text+="\n";text+=expiring_walk_air_dash_hint;}
            if(practice->expiring_walk_air_dash_relaunch){text+="\n";text+=expiring_walk_air_dash_relaunch_hint;}
            if(practice->expiring_walk_air_dash_snap){text+="\n";text+=expiring_walk_air_dash_snap_hint;}
            if(practice->windup_from_relaunch_snap){text+="\n";text+=windup_from_relaunch_snap_hint;}
            if(practice->relaunch_from_active_snap){text+="\n";text+=relaunch_from_active_snap_hint;}
            if(const char* cue=practice_timed_release_push_cue(*practice)){text+="\n";text+=cue;}
            practice_notice_line=std::max(practice_notice_line,1+s32(std::count(text.begin(),text.end(),'\n')));
            draws+=text_.draw(render,text,24,68,style);
        }
        if(source_practice_&&practice_input_refused&&!world.paused()&&width>=260&&height>=248){
            auto notice=style;notice.r=255;notice.g=210;notice.b=80;
            draws+=text_.draw(render,"INPUT REFUSED - RELEASE AND RETRY",24,68+practice_notice_line*text_.line_height(style),notice);
        }
        std::string status="G-RED VS SASUKE - NATIVE MVP / PROVISIONAL RULES\nWASD MOVE  L DASH  SHIFT JUMP  SPACE PRIMARY  E SECONDARY  Q RESTART  ENTER PAUSE";
        if(world.actors()[0].source_ground_motion)status="G-RED SOURCE GROUND-DASH LAB - SCRIPTED / NATIVE CONTACT PROXY\nENTER PAUSE  Q RESTART  ESC CLOSE - NO SOURCE WALK/JUMP/COMBAT";
        if(source_general_)status=std::string("G-RED SOURCE GROUND LAB - ")+(source_interactive_?"INTERACTIVE":"SCRIPTED")+" / NATIVE CONTACT PROXY\nARROWS WALK  WASD FLICK DASH  ENTER PAUSE  Q RESTART - NO JUMP/FLIGHT/COMBAT";
        if(source_vertical_jump_)status="G-RED SOURCE VERTICAL JUMP SUBSET / NATIVE FLAT CONTACT\nSHIFT JUMP FROM IDLE - RELEASE TO FALL - NEUTRAL STICK IN AIR\nNO MOVING TAKEOFF / AIR STEERING / RELAUNCH / AIR DASH / COMBAT\nRESOURCE "+std::to_string(source_resource_);
        if(source_moving_jump_)status="G-RED SOURCE MOVING JUMP SUBSET / NATIVE FLAT CONTACT\nSHIFT JUMP - WASD / ARROWS STEER - RELEASE SHIFT TO FALL\nNO RELAUNCH / AIR DASH / DASH-TO-JUMP / COMBAT\nRESOURCE "+std::to_string(source_resource_);
        if(source_air_dash_)status="G-RED SOURCE AIR DASH / NATIVE FLAT CONTACT\nSHIFT JUMP - ARROWS STEER - AIRBORNE WASD FLICK DASH\nCENTER DIRECTION TO RECOVER AFTER MINIMUM ACTIVE TIMER\nNO RELAUNCH / DASH-TO-JUMP / COMBAT - RESOURCE "+std::to_string(source_resource_);
        if(source_relaunch_)status="G-RED SOURCE RELAUNCH / NATIVE FLAT CONTACT\nSHIFT JUMP - RELEASE / PRESS AGAIN AIRBORNE TO RELAUNCH\nARROWS STEER - AIRBORNE WASD FLICK DASH\nNO DASH-TO-JUMP / LANDING INTERRUPTION / COMBAT - METER "+std::to_string(source_resource_);
        if(source_dash_jump_)status="G-RED SOURCE DASH-JUMP / NATIVE CONTACT PROXY\nWASD FLICK DASH - SHIFT INTERRUPTS GROUND DASH\nARROWS STEER - RELEASE / PRESS SHIFT FOR AIR RELAUNCH\nNO LANDING / TAKEOFF INTERRUPTION OR COMBAT - METER "+std::to_string(source_resource_);
        if(source_landing_interrupt_)status="G-RED SOURCE LANDING INPUT / NATIVE CONTACT PROXY\nAFTER LANDING ENTRY: SHIFT JUMP OR WASD FLICK DASH\nARROWS STEER / WAIT - SOFT WALK DOES NOT CANCEL LANDING\nNO TOUCHDOWN SKIP / TAKEOFF INTERRUPT / COMBAT - METER "+std::to_string(source_resource_);
        if(source_surface_)status="G-RED SOURCE STATIC SURFACE / NATIVE CONTACT PROXY\nSHIFT JUMP - ARROWS STEER - WASD FLICK DASH\nTAKEOFF RE-PRESS IS SAMPLED, NOT A LOCOMOTION CANCEL\nNO STEEP RESPONSE / PLATFORMS / COMBAT - METER "+std::to_string(source_resource_);
        if(source_steep_)status="G-RED SOURCE STATIC PUSH / NATIVE CONTACT PROXY\nSHIFT JUMP - ARROWS STEER - WASD FLICK DASH\nSTEEP PUSH STAYS AIRBORNE - NO LANDING REFILL\nNO DASH/RELAUNCH PUSH / PLATFORMS / COMBAT - METER "+std::to_string(source_resource_);
        if(source_fall_surface_)status="G-RED SOURCE FALLING SURFACE / NATIVE CONTACT PROXY\nSHIFT JUMP/RELAUNCH - ARROWS STEER - WASD FLICK DASH\nPOST-DASH/RELAUNCH FALL PUSH STAYS AIRBORNE\nNO ACTIVE PUSH / PLATFORMS / COMBAT - METER "+std::to_string(source_resource_);
        if(source_recovery_surface_)status="G-RED SOURCE RECOVERY SURFACE / NATIVE CONTACT PROXY\nSHIFT JUMP/RELAUNCH - ARROWS STEER - WASD FLICK DASH\nRECOVERY PUSH PRECEDES PROGRAM12 COMPLETION\nNO ACTIVE PUSH / PLATFORMS / COMBAT - METER "+std::to_string(source_resource_);
        if(source_windup_surface_)status="G-RED SOURCE WINDUP SURFACE / NATIVE CONTACT PROXY\nSHIFT JUMP/RELAUNCH - ARROWS STEER - WASD FLICK DASH\nPROGRAM10 CONTINUED PUSH: -.07 ACCEL, .9 DAMPING\nNO ENTRY/ACTIVE PUSH / PLATFORMS / COMBAT - METER "+std::to_string(source_resource_);
        if(source_active_surface_)status="G-RED SOURCE ACTIVE SURFACE / NATIVE CONTACT PROXY\nSHIFT JUMP/RELAUNCH - ARROWS STEER - WASD FLICK DASH\nPROGRAM11: SPEND BEFORE MOTION/PUSH - NO REFILL\nNO ENTRY/RELAUNCH PUSH / PLATFORMS / COMBAT - METER "+std::to_string(source_resource_);
        if(source_relaunch_surface_)status="G-RED SOURCE RELAUNCH SURFACE / NATIVE CONTACT PROXY\nSHIFT JUMP/RELAUNCH - ARROWS STEER - WASD FLICK DASH\nPROGRAM14/15 PUSH: POSITIVE ACCEL, SEPARATE LANES\nNO ENTRY PUSH / RELAUNCH LANDING / COMBAT - METER "+std::to_string(source_resource_);
        if(source_relaunch_entry_surface_)status="G-RED SOURCE ENTRY PUSH / NATIVE CONTACT PROXY\nSHIFT JUMP/RELAUNCH - ARROWS STEER - WASD FLICK DASH\nADD15 BEFORE MOTION/ACCEL/CLAMP/PUSH - NO REFILL\nNO RISING ENTRY / RELAUNCH LANDING / COMBAT - METER "+std::to_string(source_resource_);
        if(source_relaunch_floor_)status="G-RED SOURCE NULL-CALLBACK FLOOR / NATIVE PROXY\nSHIFT JUMP/RELAUNCH - ARROWS STEER - WASD FLICK DASH\nSNAP IS NOT LANDING - NEGATIVE SPEED X .8\nRELAUNCH STAYS AIRBORNE - NO REFILL - METER "+std::to_string(source_resource_);
        if(source_air_dash_floor_)status="G-RED SOURCE AIR-DASH FLOOR / NATIVE PROXY\nSHIFT JUMP/RELAUNCH - ARROWS STEER - WASD FLICK DASH\nSNAP IS NOT LANDING - WINDUP ZERO / ACTIVE KEEPS SPEED\nNO REFILL ON SNAPS - REAL CALLBACK LANDS - METER "+std::to_string(source_resource_);
        if(source_windup_entry_floor_)status="G-RED SOURCE WINDUP ENTRY FLOOR / NATIVE PROXY\nSHIFT JUMP/RELAUNCH - ARROWS STEER - WASD FLICK DASH\nENTRY HALVES SPEED THEN ZEROS VERTICAL - NO MOTION\nSNAP IS NOT LANDING OR REFILL - METER "+std::to_string(source_resource_);
        if(source_windup_positive_floor_)status="G-RED SOURCE POSITIVE WINDUP FLOOR / NATIVE PROXY\nSHIFT JUMP/RELAUNCH - ARROWS STEER - WASD FLICK DASH\nPOSITIVE SPEED HALVED THEN ZEROED - ENTRY DOES NOT RISE\nNO LANDING OR REFILL ON SNAP - METER "+std::to_string(source_resource_);
        if(source_windup_entry_push_)status="G-RED SOURCE WINDUP ENTRY PUSH / NATIVE PROXY\nSHIFT JUMP/RELAUNCH - ARROWS STEER - WASD FLICK DASH\nENTRY HALF SPEED THEN STEEP X .9 - NOT FLOOR ZERO\nNO LANDING OR REFILL ON PUSH - METER "+std::to_string(source_resource_);
        if(source_walk_ledge_)status="G-RED SOURCE WALK LEDGE / NATIVE PROXY\nARROWS/DPAD WALK - NO JUMP/FLICK/DASH/FLIGHT\nGROUND HEIGHT HOLD THEN PRIORITY FALLING PROGRAM13\nOWNED FRAME HISTORY - DRIFT DELAY30 - METER "+std::to_string(source_resource_);
        if(source_ordered_ground_)status="G-RED ORDERED GROUND DASH / NATIVE PROXY\nARROWS/DPAD WALK - WASD/STICK FLICK DASH\nENTRY MOTION - OWNED ROOT SPEED - POST-MOTION IDLE\nNO LEDGE/JUMP/FLIGHT/COMBAT - NO CLIPPING";
        if(source_dash_ledge_)status="G-RED SOURCE DASH LEDGE / NATIVE PROXY\nWASD/STICK DASH OFF EDGE - NO WALKING LEDGES\nPOST-DASH SCALAR THEN PRIORITY PROGRAM13 / DELAY30\nLOW FALL DRIFT - NO JUMP/FLIGHT/COMBAT/CLIPPING";
        if(source_ground_ledge_)status="G-RED GROUND LEDGE CONTROLLER / NATIVE PROXY\nARROWS/DPAD WALK - RELEASE THEN WASD/STICK FLICK DASH\nWALK OR DASH OFF EDGE - SHARED FALL/DRIFT/LANDING\nNO JUMP/AIR-DASH/FLIGHT/COMBAT/CLIPPING";
        if(source_ground_jump_)status="G-RED GROUND/JUMP CONTROLLER / NATIVE PROXY\nARROWS WALK - WASD/STICK FLICK DASH - SHIFT JUMP\nDASH JUMP CANCELS OLD MOTION - SOURCE JUMP/METER\nNO AIR-DASH/RELAUNCH/LANDING CANCEL/COMBAT";
        if(source_ground_landing_)status="G-RED GROUND/LANDING CONTROLLER / NATIVE PROXY\nSHIFT JUMP - WASD/STICK FLICK DASH - ARROWS WALK\nACCEPT LANDING ENTRY THEN JUMP/DASH CANCEL\nNO TOUCHDOWN SKIP/AIR-DASH/RELAUNCH/COMBAT";
        if(source_ground_air_)status="G-RED GROUND/AIR CONTROLLER / NATIVE PROXY\nSHIFT JUMP/RISE - WASD/STICK FLICK GROUND/AIR DASH\nSOURCE WINDUP/ACTIVE/RECOVERY - ACCEPTED LANDING INPUT\nNO RELAUNCH/FLOOR SNAP/PUSH/CLIPPING/COMBAT";
        if(source_ground_air_floor_)status="G-RED GROUND/AIR/FLOOR CONTROLLER / NATIVE PROXY\nSHIFT JUMP/RISE - WASD/STICK FLICK GROUND/AIR DASH\nWINDUP ZERO48 / EARLY ACTIVE RETAINS48 / LATER LANDING\nNO ENTRY SNAP/RELAUNCH/PUSH/CLIPPING/COMBAT";
        if(source_ground_relaunch_)status="G-RED GROUND/AIR/RELAUNCH CONTROLLER / NATIVE PROXY\nSHIFT JUMP/RISE - REPRESS AIRBORNE TO RELAUNCH\nSOURCE ADDITIVE LIFT / LOWER-UPPER14-15 / RELEASED21-24\nNO RELAUNCH FLOOR/ENTRY SNAP/PUSH/CLIPPING/COMBAT";
        if(source_ground_relaunch_floor_)status="G-RED GROUND/RELAUNCH FLOOR / NATIVE PROXY\nSHIFT JUMP/RISE - REPRESS AIRBORNE TO RELAUNCH\nNULL FLOOR: NEGATIVE48 X .8 / RETAIN50-LATCHES-CACHE\nNO WINDUP ENTRY SNAP/PUSH/CLIPPING/COMBAT";
        if(source_ground_windup_entry_floor_)status="G-RED GROUND/WINDUP ENTRY FLOOR / NATIVE PROXY\nSHIFT JUMP/RISE/RELAUNCH - WASD/STICK FLICK DASH\nENTRY HALF44-48 / NO MOTION-COST / FLOOR ZERO48\nNO PUSH/POSITIVE698/CLIPPING/COMBAT";
        if(source_ground_surface_)status="G-RED COMBINED SURFACE CONTROLLER / NATIVE PROXY\nSHIFT JUMP/RISE/RELAUNCH - WASD/STICK FLICK DASH\nPAIRED HEIGHT/NORMAL - SOURCE LANDING ANGLE GATE\nNO PUSH/POSITIVE698/CLIPPING/COMBAT";
        if(source_ground_loss_surface_)status="G-RED GROUND LOSS SURFACE / NATIVE PROXY\nSOURCE TIMER698 GATE / PAIRED PLANE / FALLING13\nSEPARATE TOUCHDOWN - LANDING8 - TIMER EXPIRY\nNO PUSH/POS698 AIR OR LANDING INPUT/COMBAT";
        if(source_ground_loss_jump_)status="G-RED GROUND LOSS LANDING JUMP / NATIVE PROXY\nSHIFT JUMP AFTER ACCEPTED LANDING8 / RETAIN698\nTAKEOFF2 - ACTUAL ASCENT4 COST - SEPARATE FALL6\nNO POS698 DASH/FLIGHT/RELAUNCH/PUSH/COMBAT";
        if(source_ground_loss_landing_)status="G-RED GROUND LOSS LANDING INPUT / NATIVE PROXY\nSHIFT JUMP - WASD/STICK DASH AFTER LANDING8\nRETAIN698 / EXACT SHARED ROOT-VM-RESOURCE PATH\nNO POS698 FLIGHT/PUSH/NONFLAT DASH/COMBAT";
        if(source_ground_loss_air_)status="G-RED GROUND LOSS AIR / NATIVE PROXY\nWASD/STICK AIR DASH AFTER ACCEPTED FALLING ENTRY\nRETAIN698 / TIMER-BOUND PLANE / RECOVERY DRIFT GATE\nNO POS698 RELAUNCH/PUSH/NONFLAT DASH/COMBAT";
        if(source_ground_loss_relaunch_)status="G-RED GROUND LOSS RELAUNCH / NATIVE PROXY\nSHIFT RELAUNCH / RETAIN698 / INDEPENDENT14-15\nACTUAL SCALAR-CLAMP / TIMER FLOOR / RELEASED21\nNO PUSH/NONFLAT DASH/CLIPPING/COMBAT";
        if(source_ground_fall_push_)status="G-RED ORDINARY FALL PUSH / NATIVE PROXY\nZERO698 FALL6 / SELECTED PLANE / .9 SPEED-V TAIL\nRETAIN GRAVITY-CACHE / SEPARATE CALLBACK LANDING\nNO LOSS/AIR/RELAUNCH/DASH PUSH OR COMBAT";
        if(source_ground_air_fall_push_)status="G-RED POST-AIR FALL PUSH / NATIVE PROXY\nCOMPLETED RECOVERY -> FALL13 / ZERO698 / .9 TAIL\nRETAIN GRAVITY-CACHE / SEPARATE CALLBACK LANDING\nNO LOSS/RELAUNCH/ACTIVE PUSH OR COMBAT";
        if(source_ground_relaunch_fall_push_)status="G-RED RELAUNCH RELEASE PUSH / NATIVE PROXY\nRELEASE -> FALL21-24 / ZERO698 / .9 TAIL\nSEPARATE CALLBACK LANDING / NO ACTIVE PUSH";
        if(source_ground_recovery_push_)status="G-RED RECOVERY PUSH / NATIVE PROXY\nAIRBORNE RECOVERY12 / ZERO698 / .9 TAIL\nCONTACT BEFORE COMPLETION / SEPARATE LANDING";
        if(source_ground_windup_push_)status="G-RED WIND-UP PUSH / NATIVE PROXY\nCONTINUED WIND-UP10 / ZERO698 / .9 TAIL\nTIMER HANDOFF / NO ENTRY OR ACTIVE PUSH";
        if(source_ground_active_push_)status="G-RED ACTIVE DASH PUSH / NATIVE PROXY\nCONTINUED ACTIVE11 / ZERO698 / .9 TAIL\nCOST BEFORE CONTACT / SEPARATE RECOVERY";
        if(source_ground_relaunch_push_)status="G-RED CONTINUED RELAUNCH PUSH / NATIVE PROXY\nRELAUNCH14-15 / ZERO698 / .9 TAIL\nNO ENTRY REPLAY / SEPARATE RELEASE AND LANDING";
        if(source_ground_relaunch_entry_push_)status="G-RED RELAUNCH ENTRY PUSH / NATIVE PROXY\nDESCENDING AFTER +15 / ZERO698 / .9 TAIL\nONE ENTRY COST / SEPARATE RELEASE AND LANDING";
        if(source_ground_windup_entry_push_)status="G-RED WIND-UP ENTRY PUSH / NATIVE PROXY\nHALF-SPEED THEN .9 / NO PRE-CONTACT MOVE\nDEFERRED HANDOFF / SEPARATE ACTIVE ENTRY";
        if(source_practice_){
            status="G-RED MOVEMENT PRACTICE / NATIVE PROXY\nARROWS WALK  WASD FLICK DASH  SHIFT JUMP/RISE\nRELEASE THEN PRESS SHIFT TO RELAUNCH\nENTER PAUSE  P STEP WHILE PAUSED\n";
            status+=std::string("Q RESTART  ")+practice_exit_hint()+" / NO COMBAT";
            if(source_interactive_)status+=source_practice_close_camera_?"\nC CAMERA: CLOSE":"\nC CAMERA: STANDARD";
            if(source_practice_package_)status+=source_practice_owned_instance_?
                "\nOWNED PACKAGE COPY "+std::to_string(source_practice_owned_instance_)+" / NORMAL":
                "\nPACKAGE PRACTICE / NORMAL VARIANT";
            if(source_practice_force_slot_[0])status+="\nFORCE "+std::to_string(source_practice_force_slot_[0])+
                " / SLOT "+std::to_string(source_practice_force_slot_[1])+" / PRACTICE ONLY";
            if(!source_practice_start_label_.empty())status+="\n"+source_practice_start_label_;
        }
        const bool package=bool(world.config().packages[0]);
        if(package){
            const auto& moves=world.config().packages[viewer]->moves;
            const bool has_x=std::any_of(moves.begin(),moves.end(),[](const auto& move){return move.button==BorgBattleButton::X;});
            const bool has_b=std::any_of(moves.begin(),moves.end(),[](const auto& move){return move.button==BorgBattleButton::B;});
            status=std::string("PACKAGE BATTLE TEST / TEST COMBAT\n")+
                (has_b?"SPACE B":"B NOT IMPLEMENTED")+"   Q RESTART\n"+
                (has_x?"E: X MOVE":"X NOT IMPLEMENTED");
            if(world.config().local_aim_cycle){
                status+="   Z TARGET";
                if(locals==2)status+="\nP2: IJK/SEMI  U B  H JUMP  O X  . TARGET";
            }
            else if(world.actor_count()>2)status+="\nP2: I J K ; MOVE   U B   H JUMP   O X";
        }
        if(round_result!=BattleMvpResult::Fighting){
            if(series){
                const auto winner=series->winner();
                status+='\n';
                if(series->timed_out())status+="TIME - ";
                if(winner!=255)status+="TEAM "+std::to_string(winner)+" WINS MATCH";
                else{
                    if(round_result==BattleMvpResult::Draw)status+="ROUND DRAW";
                    else status+="TEAM "+std::to_string(round_result==BattleMvpResult::GRedWins?0:1)+" WINS ROUND";
                }
                status+=locals==2?" - Q/N":" - Q";
                status+=winner!=255?" REMATCH":series->keeps_score()?" NEXT ROUND":" RESTART";
            }else status+="\n"+std::string(
                round_result==BattleMvpResult::GRedWins?(roster_view?"TEAM 0 WINS":package?"PLAYER 1 WINS":"G-RED WINS"):
                round_result==BattleMvpResult::SasukeWins?(roster_view?"TEAM 1 WINS":package?"PLAYER 2 WINS":"SASUKE WINS"):"DRAW")+" - Q TO RESTART";
        }
        else if(world.paused())status+=source_practice_?"\nPAUSED AT TICK "+std::to_string(world.tick()):"\nPAUSED";
        if(roster_view)draws+=draw_roster_health(render,world,width,height,style,series);
        else if(package&&world.actor_count()>0){
            const auto& self=world.actors()[0];
            // A 20-cell bar per actor. The maximum comes from that actor's own
            // profile health, so the bar cannot imply a shared scale.
            const auto health_bars=[&](unsigned actor){
                const auto& a=world.actors()[actor];
                const auto& profile=world.config().packages[actor];
                const unsigned maximum=profile?profile->health:0;
                constexpr unsigned cells=20;
                const unsigned filled=battle_health_cells(u32(a.health),u32(maximum),cells);
                std::string bar(cells,'-');
                for(unsigned i=0;i<filled;++i)bar[i]='#';
                return "P"+std::to_string(actor+1)+" ["+bar+"] "+
                       std::to_string(int(a.health))+"/"+std::to_string(int(maximum));
            };
            std::string hud;
            for(unsigned i=0;i<world.actor_count();++i){
                if(i)hud+=world.actor_count()>2?"\n":"   ";
                hud+=health_bars(i);
            }
            const auto gauge=battle_charge_gauge(self.package_charge_total,self.package_action_born,world.tick());
            if(gauge.total){
                constexpr unsigned cells=16;
                const unsigned filled=unsigned((u32(gauge.elapsed)*cells)/gauge.total);
                std::string bar(cells,'-');
                for(unsigned i=0;i<filled;++i)bar[i]='#';
                hud+="\nCHARGE ["+bar+"] "+std::to_string(int(gauge.elapsed))+"/"+std::to_string(int(gauge.total));
            }
            draws+=text_.draw(render,hud,24,16,style);
        }
        if(series&&series->tick_limit()&&width>=90&&height>=48){
            const u32 seconds=(series->remaining_ticks(world)+kBattleRosterTicksPerSecond-1)/kBattleRosterTicksPerSecond;
            const auto clock="TIME "+std::to_string(seconds/60)+(seconds%60<10?":0":":")+std::to_string(seconds%60);
            draws+=text_.draw_centered(render,clock,width/2,width<248||height<156?16:4,style);
        }
        if(world.config().local_aim_cycle&&round_result==BattleMvpResult::Fighting){
            const std::array targets{world.aim_target_of(0),locals==2?world.aim_target_of(1):u8(255)};
            const bool shared=targets[0]!=255&&targets[0]==targets[1];
            for(unsigned local=0;local<locals;++local){
                const auto target=targets[local];
                if(target==255)continue;
                auto point=world.actors()[target].position;
                point[1]+=world.config().actor_height;
                if(const auto& bounds=framing.bodies[target]){
                    point[0]=(bounds->low[0]+bounds->high[0])*.5f;
                    point[1]=std::max(point[1],bounds->high[1]);
                    point[2]=(bounds->low[2]+bounds->high[2])*.5f;
                }
                point[1]+=18;
                f32 x=0,y=0;
                if(!camera.project(point.data(),&x,&y,nullptr)||!(x>=0&&x<width&&y>=0&&y<height))continue;
                auto label_style=style;label_style.scale=2;
                label_style.r=local?115:255;label_style.g=local?220:205;label_style.b=local?255:90;
                auto shadow=label_style;shadow.r=shadow.g=shadow.b=24;
                const auto label=local?"P2 AIM":"P1 AIM";
                const s32 margin=1+text_.measure_width(label,label_style)/2;
                if(width<=2*margin)continue;
                const s32 sx=std::clamp(s32(x),margin,width-margin);
                const s32 line=text_.line_height(label_style);
                const s32 sy=shared?std::max(0,s32(y)-2*line)+s32(local)*line:std::max(0,s32(y)-line);
                draws+=text_.draw_centered(render,label,sx+1,sy+1,shadow);
                draws+=text_.draw_centered(render,label,sx,sy,label_style);
            }
        }
        const s32 status_y=(source_practice_||roster_view)?std::max(0,height-16-
            s32(1+std::count(status.begin(),status.end(),'\n'))*text_.line_height(style)):height-80;
        if(roster_view)draws+=draw_roster_resources(render,world,width,status_y,style);
        draws+=text_.draw(render,status,24,status_y,style);render.state()=state;return draws;
    }
    unsigned projectile_record_draws()const{return projectile_record_draws_;}
    unsigned package_part_draws(unsigned actor)const{return actor<package_part_draws_.size()?package_part_draws_[actor]:0;}
    unsigned package_body_draws(unsigned actor)const{return actor<package_body_draws_.size()?package_body_draws_[actor]:0;}
    bool package_body_bounds(const NativeBattleMvp& world,unsigned actor,BattleMvpVec* low,BattleMvpVec* high)const{
        if(actor>=world.actor_count()||actor>=borgs_.size())return false;
        return borgs_[model_slot(world,actor)].scene_.bounds(low,high);
    }
    std::array<unsigned,3> package_texture_tally(const NativeBattleMvp& world,unsigned actor)const{
        if(actor>=world.actor_count()||actor>=borgs_.size())return {};
        return borgs_[model_slot(world,actor)].texture_tally();
    }
    const BorgTrailingChain* package_trailing_chain(const NativeBattleMvp& world,unsigned actor)const{
        if(actor>=world.actor_count()||rendered_tick_!=world.tick()||!world.owns_motion_scope(rendered_scope_))return nullptr;
        const auto& borg=borgs_[model_slot(world,actor)];
        return borg.presentation_ready_&&borg.trailing_chain_.tip()?&borg.trailing_chain_:nullptr;
    }
    unsigned projectile_trail_draws()const{return projectile_trail_draws_;}
    unsigned impact_record_draws()const{return impact_record_draws_;}
    bool package_pose_held(const NativeBattleMvp& world,unsigned actor)const{
        return actor<world.actor_count()&&rendered_tick_==world.tick()&&world.owns_motion_scope(rendered_scope_)&&
            borgs_[model_slot(world,actor)].package_pose_held();
    }
    std::optional<f32> stage_view_alpha(unsigned viewer,u16 record=2)const{
        if(viewer>=2)return {};
        for(const auto& obstacle:stage_fades_)if(obstacle.profile.normal==record&&obstacle.views[viewer].sampled)
            return obstacle.views[viewer].alpha;
        return {};
    }
private:
    static std::array<std::optional<BattleMvpVec>,2> shared_roster_fade_focus(
        const NativeBattleMvp& world,const Camera& camera,const RosterBounds& bodies){
        std::array<std::optional<BattleMvpVec>,2> focus{};
        const f32 left=f32(camera.viewport_x),top=f32(camera.viewport_y);
        for(unsigned i=0;i<world.config().roster_local_players;++i){
            const auto& actor=world.actors()[i];
            if(!actor.health)continue;
            auto point=actor.position;
            point[1]+=world.config().actor_height*.5f;
            if(bodies[i])for(unsigned k=0;k<3;++k)point[k]=(bodies[i]->low[k]+bodies[i]->high[k])*.5f;
            f32 x=0,y=0;
            if(camera.project(point.data(),&x,&y,nullptr)&&x>=left&&x<left+camera.viewport_width&&
               y>=top&&y<top+camera.viewport_height)focus[i]=point;
        }
        return focus;
    }
    unsigned draw_roster_resources(RenderEngine& render,const NativeBattleMvp& world,s32 width,s32 status_y,
                                   const TextRenderer::Style& style){
        constexpr s32 margin=24,gap=16;
        const s32 column=(width-2*margin-gap)/2;
        const s32 line=text_.line_height(style);
        // Leave room for the largest team health panel (including both
        // charges) and for the session/control status underneath us.
        const s32 clear_top=16+line+3+3*(line+8)+2*line+12;
        const s32 rows=(status_y-clear_top-8)/line-1;
        if(column<100||rows<1)return 0;
        unsigned draws=0;
        for(unsigned local=0;local<world.config().roster_local_players;++local){
            const auto& profile=*world.config().packages[local];
            if(!world.actors()[local].health)continue;
            const auto stock=borg_stock_hud(profile,world.package_action(local));
            const auto mobility=borg_mobility_hud(profile,world.actors()[local]);
            const bool show_mobility=mobility&&(rows>1||!stock.count);
            const unsigned shown=std::min(stock.count,unsigned(rows)-unsigned(show_mobility));
            if(!shown&&!show_mobility)continue;
            const s32 x=local?margin+column+gap:margin;
            s32 y=status_y-s32(shown+unsigned(show_mobility)+1)*line-8;
            auto title="P"+std::to_string(local+1)+(show_mobility?" STATUS":" AMMO");
            if(shown<stock.count)title+=" +"+std::to_string(stock.count-shown);
            draws+=text_.draw(render,title,x,y,style);y+=line;
            if(show_mobility){
                auto tint=style;tint.r=137;tint.g=205;tint.b=246;
                draws+=text_.draw(render,borg_mobility_text(*mobility),x,y,tint);y+=line;
            }
            for(unsigned i=0;i<shown;++i,y+=line){
                const auto& row=stock.rows[i];
                auto text=std::string(borg_stock_hud_buttons(row.buttons))+" "+
                    std::to_string(row.current)+"/"+std::to_string(row.capacity);
                auto tint=style;
                std::string_view status;
                if(row.phase==BorgStockHudPhase::Refill){tint.r=255;tint.g=222;tint.b=92;status=" REFILL";}
                else if(row.phase==BorgStockHudPhase::Return){tint.r=130;tint.g=222;tint.b=217;status=" RETURN";}
                else if(row.phase==BorgStockHudPhase::Empty){tint.r=tint.g=tint.b=160;status=" EMPTY";}
                if(text_.measure_width(text+std::string(status),style)<=column)text+=status;
                // Two pools used by the same button are real separate
                // channels. Keep their names, not an invented combined count.
                for(unsigned j=0;j<stock.count;++j)if(j!=i&&(row.buttons&stock.rows[j].buttons)){
                    auto id=profile.stocks[row.channel].id;
                    for(auto& c:id){
                        if(c>='a'&&c<='z')c=char(c-'a'+'A');
                        if(!TextRenderer::covers(c))c='?';
                    }
                    while(!id.empty()&&text_.measure_width(text+" "+id,style)>column)id.pop_back();
                    if(!id.empty())text+=" "+id;
                    break;
                }
                draws+=text_.draw(render,text,x,y,tint);
            }
        }
        return draws;
    }
    unsigned draw_roster_health(RenderEngine& render,const NativeBattleMvp& world,s32 width,s32 height,
                                const TextRenderer::Style& style,const BattleRosterSeries* series){
        constexpr std::array<Tint,2> colors{{{255,175,83,255},{108,178,255,255}}};
        constexpr s32 margin=16,gap=16;
        const s32 column=std::min(286,(width-2*margin-gap)/2);
        const s32 line=text_.line_height(style);
        const bool compact=column<100||height<156;
        const bool scored=series&&series->keeps_score();
        const unsigned local_count=world.config().roster_local_players;
        unsigned draws=0;
        geometry_.clear();
        for(unsigned team=0;team<colors.size();++team){
            const auto color=colors[team];
            auto team_style=style;team_style.r=color.r;team_style.g=color.g;team_style.b=color.b;
            const auto score=scored?std::to_string(series->wins()[team])+"/"+std::to_string(series->goal()):std::string{};
            const auto heading="TEAM "+std::to_string(team)+(scored?" WINS "+score:"");
            if(compact){
                if(scored){
                    const auto summary="T"+std::to_string(team)+" W"+score;
                    draws+=text_.draw_centered(render,summary,s32(team?3:1)*width/4,4,team_style);
                    continue;
                }
                unsigned total=0,alive=0;
                for(unsigned i=0;i<world.actor_count();++i)if(world.actors()[i].team==team){
                    ++total;alive+=world.actors()[i].health!=0;
                }
                const auto summary="T"+std::to_string(team)+" "+std::to_string(alive)+"/"+std::to_string(total);
                draws+=text_.draw_centered(render,summary,s32(team?3:1)*width/4,4,team_style);
                continue;
            }
            const s32 x=team?width-margin-column:margin;
            s32 y=margin;
            draws+=text_.draw(render,heading,x,y,team_style);y+=line+3;
            for(unsigned i=0;i<world.actor_count();++i){
                const auto& actor=world.actors()[i];
                if(actor.team!=team)continue;
                const auto slot=std::string(i<local_count?"P":"CPU ")+std::to_string(i+1);
                const auto maximum=world.maximum_health(i);
                auto row_style=style;
                if(!actor.health)row_style.r=row_style.g=row_style.b=160;
                const auto label=slot+(actor.health?"  "+std::to_string(actor.health)+"/"+std::to_string(maximum):"  KO");
                // On a narrow resize keep at least the slot label.
                draws+=text_.draw(render,text_.measure_width(label,row_style)<=column?label:slot,x,y,row_style);
                const f32 bar_y=f32(y+line);
                ScreenDraw::textured_quad(geometry_,x,bar_y,x+column,bar_y+4,0,0,0,1,1,{35,35,45,230});
                if(maximum&&actor.health){
                    const auto filled=f32(column)*std::min(actor.health,maximum)/maximum;
                    ScreenDraw::textured_quad(geometry_,x,bar_y,x+filled,bar_y+4,0,0,0,1,1,color);
                }
                y+=line+8;
                if(i<local_count&&actor.health){
                    const auto gauge=battle_charge_gauge(actor.package_charge_total,actor.package_action_born,world.tick());
                    if(gauge.total){
                        const auto charge="CHARGE "+std::to_string(gauge.elapsed)+"/"+std::to_string(gauge.total);
                        draws+=text_.draw(render,charge,x,y,team_style);y+=line;
                    }
                }
            }
        }
        if(!geometry_.empty()){
            const auto base=render.append_vertices(geometry_.data(),geometry_.size());
            if(render.draw(Primitive::Triangles,base,u32(geometry_.size())))++draws;
        }
        return draws;
    }
    bool fit_walk_camera(Camera& camera,std::string* error)const{
        BattleMvpVec low,high;
        if(!borgs_[0].scene_.bounds(&low,&high)){if(error)*error="walk lab: current body bounds required";return false;}
        // Camera-relative travel still uses the original camera frame.
        if(!camera_fit_bounds(camera,low,high,550,12000)){if(error)*error="walk lab: body framing budget";return false;}
        return true;
    }
    struct MovementPulse {BattleMvpVec position{};u32 born=0;bool active=false;};
    void publish_movement_fx(const NativeBattleMvp& world){
        for(unsigned slot=0;slot<world.actor_count();++slot){
            const auto* profile=world.config().packages[slot].get();
            if(!profile)continue;
            const auto& actor=world.actors()[slot];
            for(unsigned i=0;i<profile->movement_fx.size();++i){
                const auto& rule=profile->movement_fx[i];auto& pulse=movement_pulses_[slot][i];
                if(!rule.duration||!actor.health)continue;
                if((actor.package_movement_cues&u8(rule.event))&&(!pulse.active||pulse.born!=world.tick())){
                    BattleMvpVec point;
                    if(borgs_[slot].bone_point(rule.bone,rule.local,&point))pulse={point,world.tick(),true};
                }
            }
        }
    }
    void draw_movement_fx(const NativeBattleMvp& world,const Camera& camera){
        if(rendered_tick_!=world.tick()||!world.owns_motion_scope(rendered_scope_))return;
        for(unsigned slot=0;slot<world.actor_count();++slot){
            const auto* profile=world.config().packages[slot].get();const auto& actor=world.actors()[slot];
            if(!profile||!actor.health)continue;
            for(unsigned i=0;i<profile->movement_fx.size();++i){
                const auto& rule=profile->movement_fx[i];const auto& pulse=movement_pulses_[slot][i];
                BattleMvpVec center;f32 age=0;std::size_t first=0,count=0;
                if(!rule.duration){
                    if(!(actor.package_movement_cues&u8(rule.event))||!borgs_[slot].bone_point(rule.bone,rule.local,&center))continue;
                }else{
                    if(!pulse.active||world.tick()-pulse.born>=rule.duration)continue;
                    age=f32(world.tick()-pulse.born)/rule.duration;center=pulse.position;
                }
                const f32 size=f32(rule.size);
                if(rule.style==BorgMovementLook::Jet){
                    auto tail=center;tail[1]-=size*.46f;
                    ScreenDraw::disc(camera,tail.data(),size*.34f,9,{255,120,36,135},geometry_,&first,&count);
                    ScreenDraw::disc(camera,center.data(),size*.21f,9,{255,234,111,240},geometry_,&first,&count);
                }else{
                    for(int arm:{-1,0,1}){
                        auto particle=center;particle[0]+=f32(arm)*size*(.14f+.34f*age);
                        particle[1]+=size*(.1f+.16f*age);
                        ScreenDraw::disc(camera,particle.data(),size*(.16f+.2f*age),9,
                            {223,213,196,u8((1-age)*150)},geometry_,&first,&count);
                    }
                }
            }
        }
    }
    static bool needs_projectile_stage(std::span<const std::shared_ptr<const BorgGameplayProfile>> definitions){
        for(const auto& def:definitions)if(def)for(const auto& shot:def->projectiles)
            if(borg_projectile_uses_stage(shot.floor)||shot.attraction.has_value())return true;
        return false;
    }
    bool bind_projectile_stage(IFileSystem& files,const std::string& arena,const std::string& directory,std::string* error){
        auto refuse=[&](){if(error)*error="stage projectile collision requires the qualified matching stage and all three st42 grids";return false;};
        projectile_source_stage_=false;
        const bool source_floor=floor_hash_==source_floor_hash_,source_model=stage_bank_.hash()==source_stage_hash_;
        const auto descriptor=arena+"/projectile-stage.gfsr";
        const auto read=[&](const std::string& path,std::size_t cap,std::vector<u8>* out){
            u64 size=0;return files.file_size(path,&size)&&size&&size<=cap&&files.read(path,*out)&&out->size()==size;
        };
        constexpr std::array names{"secondary.stih","tertiary.stih"};
        // Never let an authored declaration broaden the parked retail query
        // or mix the pinned retail floor/model with authored grids.
        if(source_floor||source_model){
            if(!source_floor||!source_model||directory.empty()||files.exists(descriptor))return refuse();
            for(unsigned i=0;i<names.size();++i){
                std::vector<u8> bytes;
                if(!read(directory+"/"+names[i],16u<<20,&bytes)||
                   character_program_model_hash({bytes.data(),bytes.size()})!=source_grid_hashes_[i]||
                   !projectile_grids_[i].load({bytes.data(),bytes.size()},error))return refuse();
            }
            projectile_bounds_={-10000,9999,-10000,9999,500,true};projectile_source_stage_=true;
            projectile_stage_ready_=true;return true;
        }
        if(!files.exists(descriptor))return refuse();
        std::vector<u8> declaration;
        std::array<std::vector<u8>,2> bytes;
        if(!read(descriptor,48,&declaration))return refuse_authored_projectile_stage(error);
        std::array<ByteView,2> grids;
        for(unsigned i=0;i<names.size();++i){
            if(!read(arena+"/"+names[i],16u<<20,&bytes[i]))return refuse_authored_projectile_stage(error);
            grids[i]={bytes[i].data(),bytes[i].size()};
        }
        return bind_authored_projectile_stage({declaration.data(),declaration.size()},grids,error);
    }
    static bool refuse_authored_projectile_stage(std::string* error){
        if(error)*error="authored stage projectile collision: expected bounded local GFSR and matching 500-unit grids";
        return false;
    }
    bool bind_authored_projectile_stage(ByteView declaration,const std::array<ByteView,2>& grids,std::string* error){
        StageRayParameters bounds;
        if(!stage_ray_parameters_load(declaration,&bounds,error))return refuse_authored_projectile_stage(error);
        const auto covered=[&](const StihGrid& grid){
            return grid.cell_size_x()==500&&grid.cell_size_z()==500&&bounds.min_x>=grid.minimum_x()&&bounds.min_z>=grid.minimum_z()&&
                bounds.max_x<f64(grid.minimum_x())+500.*grid.cells_x()&&bounds.max_z<f64(grid.minimum_z())+500.*grid.cells_z();
        };
        if(!covered(floor_))return refuse_authored_projectile_stage(error);
        for(unsigned i=0;i<grids.size();++i)
            if(!projectile_grids_[i].load(grids[i],error)||!covered(projectile_grids_[i]))return refuse_authored_projectile_stage(error);
        projectile_bounds_=bounds;projectile_stage_ready_=true;return true;
    }
    bool bind_stage_fades(RenderEngine& render,std::string_view text,std::string* error){
        std::vector<StageViewFadeProfile> profiles;
        if(!stage_view_fades_load(text,&profiles,error))return false;
        std::vector<StageViewObstacle> pending;
        pending.reserve(profiles.size());
        AttachmentModelBank bank;
        for(const auto& profile:profiles){
            auto& obstacle=pending.emplace_back();obstacle.profile=profile;
            AttachmentBoneTransform root;root.valid=true;
            if(profile.normal!=2){
                if((!bank.ready()&&!bank.load_prepared(stage_bank_,error))||
                   !obstacle.normal.load_shared(bank,profile.normal,error)||!obstacle.normal.set_world_static(root,error))return false;
                obstacle.normal.upload_textures_shared(render,1000);
            }
            HsdRecordAnimationTables tables;
            if(!stage_bank_.animation_tables(profile.alternate,&tables)||tables.joint||tables.material||tables.shape){
                if(error)*error="stage viewport fade: alternate record must be static";
                return false;
            }
            if(!obstacle.faded.load_compact_static(stage_bank_,profile.alternate,error))return false;
            const auto* asset=obstacle.faded.asset();
            if(!asset||asset->joints.size()!=1||!std::all_of(asset->meshes.begin(),asset->meshes.end(),
                [](const auto& mesh){return (mesh.render_mode&0x60000000u)==0x60000000u&&mesh.blend&&!mesh.depth_write&&mesh.depth_test;})){
                if(error)*error="stage viewport fade: static depth-tested translucent alternate required";
                return false;
            }
            obstacle.faded.set_enabled(true);
            if(!obstacle.faded.set_root_world(root.world,error)||!obstacle.faded.request(0,0,error))return false;
            obstacle.faded.upload_textures_shared(render,1000);
        }
        stage_fades_=std::move(pending);return true;
    }
    template<class Read>
    bool load_stage(Read read,RenderEngine& render,std::string* error){
        std::vector<u8> stage,floor;
        return read("stage.hsd",&stage)&&read("floor.stih",&floor)&&
            load_stage({stage.data(),stage.size()},{floor.data(),floor.size()},render,error);
    }
    void clear_stage(){
        ready_=false;provisional_walk_view_=provisional_jump_view_=provisional_dash_view_=provisional_air_dash_view_=provisional_glide_view_=provisional_cruise_view_=false;projectile_stage_ready_=false;projectile_source_stage_=false;
        source_practice_frame_.reset();rendered_scope_.reset();
        stage_manifest_.reset();
        ink_ready_=false;
        stage_fades_.clear();
        projectile_records_={};
        impact_record_scenes_={};
    }
    bool load_stage(ByteView stage,ByteView floor,RenderEngine& render,std::string* error){
        if(!floor_.load(floor,error)||!stage_bank_.load(stage,error,true))return false;
        floor_hash_=character_program_model_hash(floor);
        AttachmentModelBank geometry;if(!geometry.load_prepared(stage_bank_,error))return false;
        AttachmentBoneTransform root;root.valid=true;
        for(unsigned i=0;i<2;++i){
            if(!stage_[i].load_shared(geometry,u16(i*2),error)||!stage_[i].set_world_static(root,error))return false;
            stage_[i].upload_textures_shared(render,1000);
        }
        return true;
    }
    static unsigned model_slot(const NativeBattleMvp& world,unsigned actor){
        return world.actor_count()==1?(world.config().solo_model==0x000a?1u:0u):actor;
    }
    friend class NativeGRedClass3TrialBridge;
    bool observe_sasuke_trial_key2(const NativeBattleMvp& world,const NativeBorgSourcePose& paid_pose,
                                   u64 publication,Mat4* out)const{
        u64 current=0;
        return source_pose_publication(world,paid_pose,&current)&&current==publication&&
            borgs_[1].observe_receiver_key2(world.actors()[1],world.tick(),out);
    }
    const char* practice_exit_hint()const{
        switch(source_practice_back_){
        case PracticeBackScreen::OwnedCollection:return "ESC COLLECTION";
        case PracticeBackScreen::ForceBrowser:return "ESC FORCE BROWSER";
        default:return "ESC CLOSE";
        }
    }
    bool ready_=false,source_general_=false,source_interactive_=false,source_vertical_jump_=false;u32 floor_hash_=0,rendered_tick_=0;std::weak_ptr<const NativeBorgMotionScope> rendered_scope_;
    bool provisional_walk_view_=false,provisional_jump_view_=false,provisional_dash_view_=false,provisional_air_dash_view_=false,
         provisional_glide_view_=false,provisional_cruise_view_=false;
    struct ProjectileRecordDraw {AnimatedModelScene scene;std::optional<AnimatedModelScene> trail;
                                 u16 scale_percent=100,trail_scale=100;u8 trail_capacity=0;};
    std::array<std::map<u8,ProjectileRecordDraw>,kBattleMvpMaxActors> projectile_records_;
    struct ImpactRecordDraw {AnimatedModelScene scene;u16 scale_percent=100,end_scale=0,ticks=0;};
    std::array<std::map<u8,ImpactRecordDraw>,kBattleMvpMaxActors> impact_record_scenes_;
    unsigned projectile_record_draws_=0;
    unsigned projectile_trail_draws_=0;
    unsigned impact_record_draws_=0;
    std::array<unsigned,kBattleMvpMaxActors> package_part_draws_{},package_body_draws_{};
    static constexpr u32 walk_texture_base_=76000;
    unsigned walk_texture_slot_=0,walk_textures_=0;
    u64 source_publication_=0;
    s16 source_resource_=0;
    bool source_moving_jump_=false;
    bool source_air_dash_=false;
    bool source_relaunch_=false;
    bool source_dash_jump_=false;
    bool source_landing_interrupt_=false;
    bool source_surface_=false;
    bool source_steep_=false;
    bool source_fall_surface_=false;
    bool source_recovery_surface_=false;
    bool source_windup_surface_=false;
    bool source_active_surface_=false;
    bool source_relaunch_surface_=false;
    bool source_relaunch_entry_surface_=false;
    bool source_relaunch_floor_=false;
    bool source_air_dash_floor_=false;
    bool source_windup_entry_floor_=false;
    bool source_windup_positive_floor_=false;
    bool source_windup_entry_push_=false;
    bool source_walk_ledge_=false;
    bool source_ordered_ground_=false;
    bool source_dash_ledge_=false;
    bool source_ground_ledge_=false;
    bool source_ground_jump_=false;
    bool source_ground_landing_=false;
    bool source_ground_air_=false;
    bool source_ground_air_floor_=false;
    bool source_ground_relaunch_=false;
    bool source_ground_relaunch_floor_=false;
    bool source_ground_windup_entry_floor_=false;
    bool source_ground_surface_=false;
    bool source_ground_loss_surface_=false;
    bool source_ground_loss_jump_=false;
    bool source_ground_loss_landing_=false;
    bool source_ground_loss_air_=false;
    bool source_ground_loss_relaunch_=false;
    bool source_ground_fall_push_=false;
    bool source_ground_air_fall_push_=false;
    bool source_ground_relaunch_fall_push_=false;
    bool source_ground_recovery_push_=false;
    bool source_ground_windup_push_=false;
    bool source_ground_active_push_=false;
    bool source_ground_relaunch_push_=false;
    bool source_ground_relaunch_entry_push_=false;
    bool source_ground_windup_entry_push_=false;
    bool source_practice_=false;
    bool source_practice_close_camera_=false;
    std::weak_ptr<const NativeBorgSourcePoseScope> source_practice_frame_;
    PracticeBackScreen source_practice_back_=PracticeBackScreen::Close;
    bool source_practice_package_=false;
    u64 source_practice_owned_instance_=0;
    std::array<unsigned,2> source_practice_force_slot_{};
    std::string source_practice_start_label_;
    std::optional<StagePackageManifest> stage_manifest_;
    StihGrid floor_;HsdRecordAnimationBank stage_bank_;std::array<AttachmentModelScene,2> stage_;std::array<NativeBattleMvpBorg,kBattleMvpMaxActors> borgs_;std::vector<Vertex> geometry_;TextRenderer text_;
    struct StageViewObstacle {
        StageViewFadeProfile profile;
        std::array<StageViewFadeState,2> views{};
        AttachmentModelScene normal;
        AnimatedModelScene faded;
    };
    std::vector<StageViewObstacle> stage_fades_;
    static constexpr u32 ink_texture_=30000;
    static constexpr u32 field_texture(unsigned owner,unsigned move){return 30200+owner*32+move;}
    bool ink_ready_=false;
    static constexpr u32 source_floor_hash_=0x0d913901u,source_stage_hash_=0xbdf8e542u;
    static constexpr std::array<u32,2> source_grid_hashes_{0x01859c01u,0xfe083e8fu};
    std::array<StihGrid,2> projectile_grids_;
    StageRayParameters projectile_bounds_;
    bool projectile_source_stage_=false;
    bool projectile_stage_ready_=false;
    std::array<std::array<MovementPulse,kBorgBattleMovementFx>,kBattleMvpMaxActors> movement_pulses_{};
};
}
