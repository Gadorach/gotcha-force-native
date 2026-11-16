#pragma once
#include "engine/gameplay/native_battle_mvp_borg.hpp"
#include "engine/gameplay/attachment_model_scene.hpp"
#include "engine/gameplay/stage_ground.hpp"
#include "engine/graphics/scene_draw.hpp"
#include "engine/graphics/text.hpp"
#include "engine/core/backends.hpp"
namespace gf {
class NativeBattleMvpView final:public IBattleMvpTerrain {
public:
    void source_ground_controls(bool interactive){source_general_=true;source_interactive_=interactive;}
    void source_air_dash_floor_controls(){source_air_dash_floor_=true;}
    void source_windup_entry_floor_controls(){source_windup_entry_floor_=true;}
    void source_vertical_jump_controls(s16 resource,bool moving=false,bool air_dash=false,bool relaunch=false,bool dash_jump=false,bool landing_interrupt=false,bool surface=false,bool steep=false,bool fall_surface=false,bool recovery_surface=false,bool source_windup=false,bool active_surface=false,bool relaunch_surface=false,bool entry_surface=false,bool relaunch_floor=false){source_vertical_jump_=true;source_moving_jump_=moving;source_air_dash_=air_dash;source_relaunch_=relaunch;source_dash_jump_=dash_jump;source_landing_interrupt_=landing_interrupt;source_surface_=surface;source_steep_=steep;source_fall_surface_=fall_surface;source_recovery_surface_=recovery_surface;source_windup_surface_=source_windup;source_active_surface_=active_surface;source_relaunch_surface_=relaunch_surface;source_relaunch_entry_surface_=entry_surface;source_relaunch_floor_=relaunch_floor;source_resource_=resource;}
    bool load(IFileSystem& files,const std::string& directory,RenderEngine& render,std::string* error){
        ready_=false;std::vector<u8> stage,floor_bytes;
        auto read=[&](const std::string& name,std::vector<u8>* out){u64 size=0;const auto path=directory+"/"+name;if(!files.file_size(path,&size)||!size||size>(32u<<20)||!files.read(path,*out)||out->size()!=size){if(error)*error="native MVP: missing/oversized asset "+path;return false;}return true;};
        if(!read("stage.hsd",&stage)||!read("floor.stih",&floor_bytes)||!floor_.load({floor_bytes.data(),floor_bytes.size()},error)||!stage_bank_.load({stage.data(),stage.size()},error,true))return false;
        floor_hash_=2166136261u;for(u8 byte:floor_bytes)floor_hash_=(floor_hash_^byte)*16777619u;
        AttachmentModelBank geometry;if(!geometry.load_prepared(stage_bank_,error))return false;
        AttachmentBoneTransform root;root.valid=true;
        for(unsigned i=0;i<2;++i){if(!stage_[i].load_shared(geometry,u16(i*2),error)||!stage_[i].set_world_static(root,error))return false;stage_[i].upload_textures_shared(render,1000);}
        for(unsigned i=0;i<2;++i){
            const std::string name=i?"sasuke":"g-red";std::vector<u8> model,prefix,pose,motions;
            if(!read(name+".hsd",&model)||!read(name+".prefix",&prefix)||!read(name+".pose",&pose)||!read(name+".motions",&motions)||!borgs_[i].load(i?10:0x615,{model.data(),model.size()},{prefix.data(),prefix.size()},{pose.data(),pose.size()},{motions.data(),motions.size()},error))return false;
            borgs_[i].upload(render,10000+i*2000);
        }
        if(render.shared_texture_upload_failed())return false;
        ready_=true;return true;
    }
    bool height(f32 x,f32 z,f32 current_y,f32* out)const override{
        NativeBorgStaticSurface surface;
        if(!out||!static_surface(x,z,current_y,&surface))return false;
        *out=surface.height;return true;
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
        if(!ready_||!world.motion_receipt_current(source.receipt)){borgs_[0].refuse_source_pose();if(error)*error="source view: stale/foreign native contact receipt";return false;}
        if(!borgs_[0].update_source_pose(world,source,error)||!borgs_[1].update(world.actors()[1],world.tick(),error))return false;
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
    static NativeBorgMovementFrame movement_frame(const NativeBattleMvp& world,s32 width,s32 height){
        const auto camera=camera_for(world,width,height);NativeBorgMovementFrame frame;native_borg_movement_frame({camera.target[0]-camera.eye[0],camera.target[2]-camera.eye[2]},&frame);return frame;
    }
    unsigned draw(RenderEngine& render,const NativeBattleMvp& world,s32 width,s32 height,std::string* error){
        if(!ready_||!world.ready()||width<=0||height<=0)return 0;
        const auto camera=camera_for(world,width,height);unsigned draws=0;
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
        const auto base=render.append_vertices(geometry_.data(),geometry_.size());draws+=render.draw(Primitive::Triangles,base,u32(geometry_.size()))?1:0;
        TextRenderer::Style style;style.scale=width>=1000?2:1;
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
        if(world.result()!=BattleMvpResult::Fighting)status+="\n"+std::string(world.result()==BattleMvpResult::GRedWins?"G-RED WINS":world.result()==BattleMvpResult::SasukeWins?"SASUKE WINS":"DRAW")+" - Q TO RESTART";
        else if(world.paused())status+="\nPAUSED";
        draws+=text_.draw(render,status,24,f32(height)-80,style);render.state()=state;return draws;
    }
private:
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
    StihGrid floor_;HsdRecordAnimationBank stage_bank_;std::array<AttachmentModelScene,2> stage_;std::array<NativeBattleMvpBorg,2> borgs_;std::vector<Vertex> geometry_;TextRenderer text_;
};
}
