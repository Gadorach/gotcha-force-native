#pragma once
#include "engine/assets/borg_practice_profile.hpp"
#include "engine/gameplay/stage_inclined_ledge.hpp"
#include "engine/gameplay/native_battle_mvp_borg.hpp"
#include "engine/gameplay/attachment_model_scene.hpp"
#include "engine/gameplay/stage_ground_surface.hpp"
#include "engine/graphics/scene_draw.hpp"
#include "engine/graphics/text.hpp"
#include "engine/core/backends.hpp"
namespace gf {
enum class PracticeBackScreen {Close,OwnedCollection,ForceBrowser};
class NativeBattleMvpView final:public IBattleMvpTerrain {
public:
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
    static constexpr const char* windup_from_relaunch_snap_hint="AIRBORNE / RELAUNCH TO WIND-UP";
    static constexpr const char* relaunch_from_active_snap_hint="AIRBORNE / ACTIVE DASH TO RELAUNCH";
    static const char* practice_timed_release_push_cue(const NativeBorgSourceReadout& frame){
        if(!frame.timed_release_push)return nullptr;
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
        ready_=false;source_practice_package_=false;source_practice_owned_instance_=0;source_practice_force_slot_={};
        source_practice_frame_.reset();std::vector<u8> stage,floor_bytes;
        if(practice&&!practice->qualified()){if(error)*error="unqualified packaged practice view inputs";return false;}
        auto read=[&](const std::string& name,std::vector<u8>* out){u64 size=0;const auto path=directory+"/"+name;if(!files.file_size(path,&size)||!size||size>(32u<<20)||!files.read(path,*out)||out->size()!=size){if(error)*error="native MVP: missing/oversized asset "+path;return false;}return true;};
        if(!read("stage.hsd",&stage)||!read("floor.stih",&floor_bytes)||!floor_.load({floor_bytes.data(),floor_bytes.size()},error)||!stage_bank_.load({stage.data(),stage.size()},error,true))return false;
        floor_hash_=2166136261u;for(u8 byte:floor_bytes)floor_hash_=(floor_hash_^byte)*16777619u;
        AttachmentModelBank geometry;if(!geometry.load_prepared(stage_bank_,error))return false;
        AttachmentBoneTransform root;root.valid=true;
        for(unsigned i=0;i<2;++i){if(!stage_[i].load_shared(geometry,u16(i*2),error)||!stage_[i].set_world_static(root,error))return false;stage_[i].upload_textures_shared(render,1000);}
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
    bool static_surface(f32 x,f32 z,f32 current_y,NativeBorgStaticSurface* out)const override{
        if(!ready_||!out)return false;
        const StageGroundParameters p{-100000,100000,500,0,-.5f,20000,-20000,true};std::span<const u32> refs;
        const std::array<f32,3> point{x,current_y,z};if(!stage_ground_detail::cell(floor_,p,point,&refs))return false;
        bool found=false,below=false;f32 best=-20000,lowest=20000;
        NativeBorgStaticSurface selected{-20000},low{20000};
        for(u32 index:refs){const auto& triangle=floor_.triangles()[index];std::array<f32,2> delta;f32 y=0;
            if(!stage_ground_detail::projected(triangle,point,false,&delta))continue;
            if(!stage_ground_detail::height(triangle,delta,&y))return false;
            const NativeBorgStaticSurface candidate{y,{triangle.plane[0],triangle.plane[1],triangle.plane[2]},index};
            //Tie metadata is last-wins, like the source paired query; height's
            //numeric max/min behavior remains unchanged for ordinary MVP users.
            found=true;if(y<=lowest){lowest=y;low=candidate;}
            if(y<=current_y+45&&(!below||y>=best)){below=true;best=y;selected=candidate;}}
        if(!found)return false;
        *out=below?selected:low;return true;
    }
    bool update(const NativeBattleMvp& world,std::string* error){
        source_practice_frame_.reset();
        if(!ready_||!world.ready())return false;
        for(unsigned i=0;i<2;++i)if(!borgs_[i].update(world.actors()[i],world.tick(),error))return false;
        rendered_scope_=world.presentation_scope();rendered_tick_=world.tick();
        return true;
    }
    bool source_seed(const NativeBattleMvp& world,NativeBorgAnimationSeed* out)const{
        if(!ready_||!world.ready()||!out||rendered_tick_!=world.tick()||!world.owns_motion_scope(rendered_scope_)||!borgs_[0].animation_seed(world.actors()[0],world.tick(),out))return false;
        out->stage_hash=stage_bank_.hash();out->floor_hash=floor_hash_;out->world_scope=rendered_scope_;return true;
    }
    std::weak_ptr<const CharacterModelBinding> source_binding()const{return borgs_[0].render_binding();}
    bool update_source(const NativeBattleMvp& world,const NativeBorgSourcePose& source,std::string* error){
        source_practice_frame_.reset();
        if(!ready_||!world.motion_receipt_current(source.receipt)){borgs_[0].refuse_source_pose();if(error)*error="source view: stale/foreign native contact receipt";return false;}
        if(!borgs_[0].update_source_pose(world,source,error)||!borgs_[1].update(world.actors()[1],world.tick(),error))return false;
        if(source_practice_)source_practice_frame_=source.frame_scope;
        rendered_scope_=world.presentation_scope();rendered_tick_=world.tick();return true;
    }
    bool actor_bounds(unsigned actor,BattleMvpVec* low,BattleMvpVec* high)const{return actor<2&&borgs_[actor].bounds(low,high);}
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
    static Camera camera_for(const NativeBattleMvp& world,s32 width,s32 height){return camera_for_actors(world.actors()[0],world.actors()[1],width,height);}
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
    static NativeBorgMovementFrame movement_frame(const NativeBattleMvp& world,s32 width,s32 height){
        const auto camera=camera_for(world,width,height);NativeBorgMovementFrame frame;native_borg_movement_frame({camera.target[0]-camera.eye[0],camera.target[2]-camera.eye[2]},&frame);return frame;
    }
    unsigned draw(RenderEngine& render,const NativeBattleMvp& world,s32 width,s32 height,std::string* error,bool practice_input_refused=false){
        if(!ready_||!world.ready()||width<=0||height<=0)return 0;
        const auto camera=source_practice_&&source_practice_close_camera_?close_practice_camera(world,width,height):camera_for(world,width,height);
        unsigned draws=0;
        const SceneLighting* old=render.scene_lighting();const bool had=old!=nullptr;const SceneLighting saved=had?*old:SceneLighting{};
        SceneLighting light;light.declared=true;SceneLight ambient;ambient.colour[0]=ambient.colour[1]=ambient.colour[2]=180;ambient.colour[3]=255;light.lights.push_back(ambient);
        SceneLight sun;sun.flags=5;sun.colour[0]=sun.colour[1]=sun.colour[2]=190;sun.colour[3]=255;sun.position[0]=-1;sun.position[1]=2;sun.position[2]=1;light.lights.push_back(sun);
        if(!render.set_scene_lighting(light))return 0;
        for(u8 pass:{u8(1),u8(4),u8(2)}){
            if(!render.begin_model_pass(pass))return 0;
            for(auto& model:stage_)draws+=model.draw(render,camera);
            for(unsigned i=0;i<2;++i)if(world.actors()[i].health)draws+=borgs_[i].draw(render,camera);
            if(!render.end_model_pass()){if(error)*error="native MVP model pass failed";return 0;}
        }
        if(had)render.set_scene_lighting(saved);else render.clear_scene_lighting();
        const auto state=render.state();render.state()={};render.state().depth_test=true;render.state().depth_write=false;render.state().cull_backface=false;render.state().blend=true;render.state().blend_src=5;render.state().blend_dst=6;Mat4 identity;render.upload_matrix(0,identity.m);geometry_.clear();
        for(const auto& shot:world.projectiles())if(shot.active){
            const Tint colour=shot.owner?Tint{120,180,255,255}:Tint{255,100,65,255};
            const f32 size=shot.weapon==BattleMvpWeapon::Bomb?28:shot.weapon==BattleMvpWeapon::EnergyBeam?18:12;
            std::size_t first=0,count=0;ScreenDraw::billboard(camera,shot.position.data(),size,colour,geometry_,&first,&count);
            if(shot.weapon==BattleMvpWeapon::EnergyBeam){BattleMvpVec trail=shot.position;for(unsigned i=0;i<3;++i)trail[i]-=shot.velocity[i]*.7f;ScreenDraw::billboard(camera,trail.data(),size*.65f,colour,geometry_,&first,&count);}
        }
        for(const auto& impact:world.impacts())if(impact.ticks){std::size_t first=0,count=0;ScreenDraw::ring(camera,impact.position.data(),impact.explosion?140:28,impact.explosion?12:5,16,{255,210,70,u8(impact.ticks*14)},geometry_,&first,&count);}
        if(!geometry_.empty()){const auto base=render.append_vertices(geometry_.data(),geometry_.size());draws+=render.draw(Primitive::Triangles,base,u32(geometry_.size()))?1:0;}
        render.state()={};render.state().depth_test=render.state().depth_write=false;render.state().blend=true;render.state().blend_src=5;render.state().blend_dst=6;render.state().cull_backface=false;render.upload_matrix(0,identity.m);geometry_.clear();
        for(unsigned i=0;i<2;++i){const f32 x=i?f32(width)-310:24;ScreenDraw::textured_quad(geometry_,x,24,x+286,42,0,0,0,1,1,{35,35,45,230});ScreenDraw::textured_quad(geometry_,x,24,x+286*f32(world.actors()[i].health)/world.config().health,42,0,0,0,1,1,i?Tint{100,170,255,255}:Tint{255,100,70,255});}
        const auto practice=practice_readout(world);
        if(practice&&width>=200&&height>=190){
            const auto meter=practice->meter;
            const f32 width_px=f32(std::min(286,width-48));
            ScreenDraw::textured_quad(geometry_,24,52,24+width_px,62,0,0,0,1,1,{35,35,45,230});
            if(meter.value)ScreenDraw::textured_quad(geometry_,24,52,24+width_px*f32(meter.value)/meter.limit,62,0,0,0,1,1,{255,210,80,255});
        }
        const auto base=render.append_vertices(geometry_.data(),geometry_.size());draws+=render.draw(Primitive::Triangles,base,u32(geometry_.size()))?1:0;
        TextRenderer::Style style;style.scale=width>=1000?2:1;
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
        if(world.result()!=BattleMvpResult::Fighting)status+="\n"+std::string(world.result()==BattleMvpResult::GRedWins?"G-RED WINS":world.result()==BattleMvpResult::SasukeWins?"SASUKE WINS":"DRAW")+" - Q TO RESTART";
        else if(world.paused())status+=source_practice_?"\nPAUSED AT TICK "+std::to_string(world.tick()):"\nPAUSED";
        const s32 status_y=source_practice_?std::max(0,height-16-
            s32(1+std::count(status.begin(),status.end(),'\n'))*text_.line_height(style)):height-80;
        draws+=text_.draw(render,status,24,status_y,style);render.state()=state;return draws;
    }
private:
    const char* practice_exit_hint()const{
        switch(source_practice_back_){
        case PracticeBackScreen::OwnedCollection:return "ESC COLLECTION";
        case PracticeBackScreen::ForceBrowser:return "ESC FORCE BROWSER";
        default:return "ESC CLOSE";
        }
    }
    bool ready_=false,source_general_=false,source_interactive_=false,source_vertical_jump_=false;u32 floor_hash_=0,rendered_tick_=0;std::weak_ptr<const NativeBorgMotionScope> rendered_scope_;
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
    StihGrid floor_;HsdRecordAnimationBank stage_bank_;std::array<AttachmentModelScene,2> stage_;std::array<NativeBattleMvpBorg,2> borgs_;std::vector<Vertex> geometry_;TextRenderer text_;
};
}
