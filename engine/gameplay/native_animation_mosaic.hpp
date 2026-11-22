#pragma once
//Numbered, in-place BODY animation previews, not gameplay actors or source
//callback execution. Uses the shared model/pose/skin/renderer systems.
#include <cstdio>
#include <memory>
#include "engine/assets/borg_preview_profile.hpp"
#include "engine/gameplay/character_model_scene.hpp"
#include "engine/gameplay/character_motion_driver.hpp"
#include "engine/gameplay/character_program_height_control.hpp"
#include "engine/gameplay/gred_ground_dash.hpp"
#include "engine/input/input_map.hpp"
#include "engine/graphics/scene_draw.hpp"
#include "engine/graphics/text.hpp"
#include "engine/core/backends.hpp"
namespace gf {
struct NativeAnimationEntry {
    unsigned id=0,archive=0;u8 group=0;u16 choice=0;bool common=false;
    std::vector<std::pair<u8,u16>> aliases;
};
struct NativeAnimationPreview {NativeAnimationEntry entry;f32 end_frame=0;bool supported=false;std::string error;bool source_program=false;};
inline std::vector<NativeAnimationEntry> native_animation_catalog(const CharacterMotionBank& bank,bool common=false){
    std::vector<NativeAnimationEntry> entries;std::vector<unsigned> lookup(bank.archives.size(),std::numeric_limits<unsigned>::max());
    for(unsigned group=0;group<16;++group)for(unsigned choice=0;choice<bank.groups[group].size();++choice){
        const auto archive=bank.groups[group][choice];if(archive==kCharacterNoMotion||archive>=bank.archives.size())continue;
        if(lookup[archive]==std::numeric_limits<unsigned>::max()){
            lookup[archive]=unsigned(entries.size());entries.push_back({(common?10000u:0u)+group*256+choice,archive,u8(group),u16(choice),common,{}});
        }
        entries[lookup[archive]].aliases.emplace_back(u8(group),u16(choice));
    }
    return entries;
}
class NativeAnimationMosaic {
public:
    bool load(IFileSystem& files,const std::string& directory,RenderEngine& render,unsigned columns,unsigned rows,std::string* error){
        if(!close_package_preview(render)){if(error)*error="package preview textures still owned";return false;}
        ready_=source_programs_=source_handlers_=package_preview_=false;cells_.clear();selected_=page_=0;
        if(!columns||!rows||columns>6||rows>4||columns*rows>24){if(error)*error="animation mosaic: grid must have1..24 cells";return false;}
        columns_=columns;rows_=rows;
        std::vector<u8> shared_bytes;CharacterMotionBank shared;
        const auto shared_path=directory+"/common.motions";
        if(files.exists(shared_path)&&(!read(files,shared_path,&shared_bytes,error)||!character_motion_bank_load(view(shared_bytes),&shared,error)))return false;
        for(unsigned i=0;i<2;++i){
            auto& source=sources_[i];source.name=i?"SASUKE":"G-RED";const std::string stem=i?"sasuke":"g-red";
            std::vector<u8> motions;
            if(!read(files,directory+"/"+stem+".hsd",&source.model_bytes,error)||!read(files,directory+"/"+stem+".prefix",&source.prefix,error)||!read(files,directory+"/"+stem+".pose",&source.pose,error)||!read(files,directory+"/"+stem+".motions",&motions,error)||!source.bank.load(view(source.model_bytes),error,true)||!character_motion_bank_load(view(motions),&source.motions,error))return false;
            if(source.bank.hash()!=(i?0xeab3632au:0xe2b4439au)){if(error)*error="animation mosaic: original normal model required";return false;}
            source.entries=native_animation_catalog(source.motions);
            if(shared.loaded){source.common=shared;auto extra=native_animation_catalog(source.common,true);source.entries.insert(source.entries.end(),extra.begin(),extra.end());}
        }
        ready_=true;return prepare_page(render,error);
    }
    // One package body, with explicitly mapped in-place clips. Changing clips
    // keeps the same decoded rig and texture owner. No gameplay actor.
    bool load_package_preview(const BorgIdentity& identity,const BorgPreviewProfile& profile,
                              ByteView model,ByteView prefix,ByteView pose,ByteView motions,
                              RenderEngine& render,std::string* error,bool returnable=false){
        if(render.frame_in_progress()){if(error)*error="package preview can only load between frames";return false;}
        if(!close_package_preview(render)){if(error)*error="package preview textures still owned";return false;}
        ready_=source_programs_=source_handlers_=false;package_preview_=true;
        package_static_=profile.static_body;
        package_returnable_=returnable;
        cells_.clear();sources_={};selected_=page_=0;columns_=rows_=1;
        clock_=yaw_=0;paused_=false;speed_=.5f;previous_={};
        auto fail=[&](const char* message){
            if(message&&error)*error=message;
            close_package_preview(render);return false;
        };
        if(!borg_package_id_valid(identity.id)||!display_name_valid(identity.name,96)||!borg_catalog_variant_id_valid(profile.variant)||
           !borg_preview_clips_valid(profile)||
           !prefix.data||!prefix.size||
           prefix.size>(1u<<16)||pose.size>(1u<<16)||
           (profile.whole_body?pose.size!=0:!pose.data||!pose.size)||
           (profile.static_body?motions.size!=0:!motions.data||!motions.size))
            return fail("package preview: invalid identity, clip table or rig assets");
        auto& source=sources_[0];source.name=preview_label(identity.name);
        if(source.name.empty())source.name=preview_label(identity.id);
        package_variant_=preview_label(profile.variant);
        source.prefix.assign(prefix.data,prefix.data+prefix.size);
        if(!source.bank.load(model,error,true))return fail(nullptr);
        if(profile.whole_body){
            if(!whole_body_profile(model,prefix,&source.pose,error))return fail(nullptr);
        }else source.pose.assign(pose.data,pose.data+pose.size);
        if(!profile.static_body&&!character_motion_bank_load(motions,&source.motions,error))return fail(nullptr);
        auto add=[&](const BorgPreviewClip& clip){
            CharacterMotionSelection selected;
            if(!character_motion_select(source.motions,clip.group,clip.choice,&selected,error)||selected.fallback){
                if(error)*error="package preview: declared "+clip.id+" clip missing; fallback is not allowed";
                return false;
            }
            source.entries.push_back({unsigned(source.entries.size()),selected.index,clip.group,clip.choice,false,{}});
            package_clip_labels_.push_back(clip.id);
            return true;
        };
        if(profile.static_body){
            source.entries.push_back({});
            package_clip_labels_.push_back("static-body");
        }else if(!add({"idle",profile.idle_group,profile.idle_choice}))return fail(nullptr);
        for(const auto& clip:profile.clips)if(!add(clip))return fail(nullptr);
        ready_=prepare_page(render,error);
        if(!ready_)close_package_preview(render);
        return ready_;
    }
    bool close_package_preview(RenderEngine& render){
        if(!package_preview_)return true;
        if(render.frame_in_progress()||(package_texture_owner_&&package_texture_owner_!=&render))return false;
        ready_=false;
        for(unsigned i=0;i<package_texture_count_;++i)
            if(!render.release_texture(package_texture_base_+i))return false;
        package_texture_count_=0;package_texture_owner_=nullptr;
        cells_.clear();sources_={};package_clips_.clear();package_clip_labels_.clear();package_preview_=package_static_=package_returnable_=false;
        return true;
    }
    //Explicit source-timeline inspection only. No input admission, root-motion
    //integration, attack effects or gameplay actor is produced by this view.
    bool load_source_programs(IFileSystem& files,const std::string& programs,const std::string& height,RenderEngine& render,std::string* error){
        if(!ready_||package_preview_){if(error)*error="source program mosaic: normal model resources required";return false;}
        ready_=source_handlers_=false;cells_.clear();
        std::vector<u8> encoded,height_bytes;
        if(!read(files,programs,&encoded,error,65536)||!read(files,height,&height_bytes,error,28)||
           !character_program_load_control_flow(view(encoded),0xe2b4439au,&program_asset_,error)||
           !character_pose_height_profile_load(view(height_bytes),&program_height_,error))return false;
        if(character_program_model_hash(view(encoded))!=0x638570ffu){if(error)*error="source program mosaic: unaltered normal G-Red locomotion profile required";return false;}
        //Only the reviewed base-program namespace; never treat attack programs
        //or another Borg's controller as supported through this replay entry.
        if(program_asset_.programs.size()!=32||program_asset_.lane_mask!=3){if(error)*error="source program mosaic: G-Red base32 profile required";return false;}
        for(unsigned i=0;i<32;++i)if(program_asset_.programs[i].group!=1||program_asset_.programs[i].choice!=i){if(error)*error="source program mosaic: base-program identity";return false;}
        auto& source=sources_[0];CharacterModelScene reference;
        if(!reference.load_with_record_bank(source.bank,view(source.prefix),view(source.pose),error))return false;
        std::array<std::array<bool,128>,16> needed{};
        for(const auto& descriptor:program_asset_.descriptors)for(unsigned lane=0;lane<2;++lane){
            const unsigned at=4+lane*8,group=descriptor.bytes[at],choice=descriptor.bytes[at+1];
            if(group>=16||choice>=128){if(error)*error="source program mosaic: common/invalid clip not admitted";return false;}needed[group][choice]=true;
        }
        std::vector<CharacterMotionClipBinding> clips;
        for(unsigned group=0;group<16;++group)for(unsigned choice=0;choice<128;++choice)if(needed[group][choice]){
            CharacterMotionSelection selected;CharacterMotionClipBinding binding;binding.group=u8(group);binding.index=u8(choice);
            if(!character_motion_select(source.motions,group,choice,&selected,error)||selected.fallback||!hsd_bone_motion_load(selected.archive,reference.rig().binding,&binding.clip,error))return false;
            clips.push_back(std::move(binding));
        }
        CharacterLocalPose initial;
        if(!character_pose_initial_local(reference.rig(),&initial,error)||
           !program_driver_.load_frozen_target_transitions(view(source.model_bytes),reference.rig(),program_asset_,clips,initial,error))return false;
        source.entries.clear();
        //A short essentials view, not another bulk classification task.
        for(u8 choice:{u8(27),u8(28),u8(29),u8(30),u8(2),u8(4),u8(6),u8(8),u8(10),u8(11),u8(12),u8(14),u8(0),u8(1)})
            source.entries.push_back({choice,std::numeric_limits<unsigned>::max(),1,choice,false,{{1,choice}}});
        source_programs_=true;selected_=page_=0;ready_=true;return prepare_page(render,error);
    }
    bool load_source_motion(IFileSystem& files,const std::string& path,RenderEngine& render,std::string* error){
        if(!ready_||!source_programs_){if(error)*error="source motion replay: source-program scope required";return false;}
        ready_=false;cells_.clear();std::vector<u8> bytes;
        if(!read(files,path,&bytes,error,376)||!character_movement_load(view(bytes),&program_motion_,error)||
           program_motion_.model_hash!=program_asset_.model_hash)return false;
        source_handlers_=true;ready_=true;return prepare_page(render,error);
    }
    void sync_input(const GamePadState& pad){previous_=pad;}
    bool step(const GamePadState& pad,f32 seconds,RenderEngine& render,std::string* error){
        if(!ready_||!std::isfinite(seconds)||seconds<0)return false;
        bool changed=false;
        unsigned borg=selected_,page=page_;
        if(!package_preview_&&!source_programs_&&((pad.dpad_up&&!previous_.dpad_up)||(pad.dpad_down&&!previous_.dpad_down))){
            borg=1-selected_;page=0;changed=true;
        }
        const auto count=std::max(1u,(unsigned(sources_[borg].entries.size())+columns_*rows_-1)/(columns_*rows_));
        if(pad.dpad_right&&!previous_.dpad_right){page=(page+1)%count;changed|=page!=page_;}
        if(pad.dpad_left&&!previous_.dpad_left){page=(page+count-1)%count;changed|=page!=page_;}
        if(pad.a&&!previous_.a)paused_=!paused_;
        if(pad.x&&!previous_.x)speed_=std::min(4.f,speed_*2);
        if(pad.y&&!previous_.y)speed_=std::max(.125f,speed_*.5f);
        if(pad.z&&!previous_.z)yaw_=std::fmod(yaw_+1.57079632679f,6.28318530718f);
        previous_=pad;
        if(changed&&!select(borg,page,render,error))return false;
        if(!paused_)clock_+=std::min(seconds,.25f)*60*speed_;
        for(auto& cell:cells_)if(cell->valid&&!evaluate(*cell,clock_,&cell->error)){
            cell->valid=false;if(source_programs_||package_preview_){if(error)*error=(package_preview_?"package preview: ":"source program mosaic: ")+cell->error;ready_=false;return false;}
        }
        return true;
    }
    unsigned draw(RenderEngine& render,s32 width,s32 height){
        if(!ready_||width<200||height<200)return 0;
        const auto saved=render.state();const auto* old=render.scene_lighting();const bool had=old!=nullptr;const auto old_light=had?*old:SceneLighting{};
        SceneLighting light;light.declared=true;SceneLight ambient;ambient.colour[0]=ambient.colour[1]=ambient.colour[2]=210;ambient.colour[3]=255;light.lights.push_back(ambient);
        SceneLight sun;sun.flags=5;sun.colour[0]=sun.colour[1]=sun.colour[2]=140;sun.colour[3]=255;sun.position[0]=-1;sun.position[1]=2;sun.position[2]=2;light.lights.push_back(sun);
        if(!render.set_scene_lighting(light))return 0;
        const s32 top=76,bottom=36,tile_width=width/s32(columns_),tile_height=(height-top-bottom)/s32(rows_);
        if(tile_width<80||tile_height<100){if(had)render.set_scene_lighting(old_light);else render.clear_scene_lighting();return 0;}
        render.state()={};render.state().depth_test=render.state().depth_write=false;render.state().cull_backface=false;render.state().blend_src=5;render.state().blend_dst=6;Mat4 identity;render.upload_matrix(0,identity.m);panels_.clear();
        for(unsigned i=0;i<cells_.size();++i){const f32 x=f32((i%columns_)*tile_width),y=f32(top+(i/columns_)*tile_height);ScreenDraw::textured_quad(panels_,x+3,y+3,x+tile_width-3,y+tile_height-3,0,0,0,1,1,{33,40,51,255});}
        unsigned draws=0;if(!panels_.empty()){const auto start=render.append_vertices(panels_.data(),panels_.size());draws+=render.draw(Primitive::Triangles,start,u32(panels_.size()))?1:0;}
        for(u8 pass:{u8(1),u8(4),u8(2)}){
            if(!render.begin_model_pass(pass))return 0;
            for(unsigned i=0;i<cells_.size();++i)if(cells_[i]->valid){
                const auto& cell=*cells_[i];const auto bounds=preview_bounds(width,height,i);Camera camera;camera.viewport_x=bounds[0];camera.viewport_y=bounds[1];camera.viewport_width=bounds[2];camera.viewport_height=bounds[3];
                camera.target[0]=cell.centre[0];camera.target[1]=cell.centre[1];camera.target[2]=cell.centre[2];
                camera.eye[0]=cell.centre[0]+cell.radius*1.3f;camera.eye[1]=cell.centre[1]+cell.radius*.7f;camera.eye[2]=cell.centre[2]+cell.radius*3;
                camera.fov_y_degrees=42;camera.near_plane=1;camera.far_plane=5000;
                draws+=cells_[i]->model.draw(render,camera);
            }
            if(!render.end_model_pass())return 0;
        }
        if(had)render.set_scene_lighting(old_light);else render.clear_scene_lighting();
        render.state()={};render.state().depth_test=render.state().depth_write=false;render.state().blend=true;render.state().blend_src=5;render.state().blend_dst=6;render.state().cull_backface=false;
        TextRenderer::Style title;title.scale=2;char heading[192];
        if(package_preview_)std::snprintf(heading,sizeof heading,"PACKAGE BODY PREVIEW - %s",sources_[0].name.c_str());
        else std::snprintf(heading,sizeof heading,"%s - %s - PAGE %u/%u - %u %s",source_handlers_?"VM + DASH HANDLER REPLAY":source_programs_?"SOURCE PROGRAM REPLAY":"ANIMATION MOSAIC",sources_[selected_].name.c_str(),page_+1,page_count(),unsigned(sources_[selected_].entries.size()),source_programs_?"PROGRAMS":"CLIPS");
        draws+=text_.draw(render,heading,12,10,title);
        TextRenderer::Style small;small.scale=1;
        draws+=text_.draw(render,package_static_?(package_returnable_?"SHIFT/B BACK  Z ROTATE":"Z ROTATE"):
            package_preview_?(package_returnable_?"SHIFT/B BACK  LEFT/RIGHT CLIP  SPACE PAUSE  Q/E SPEED  Z ROTATE":
            "LEFT/RIGHT CLIP  SPACE PAUSE  Q/E SPEED  Z ROTATE"):source_programs_?"LEFT/RIGHT PAGE  SPACE PAUSE  Q/E SPEED  Z ROTATE":"LEFT/RIGHT PAGE  UP/DOWN BORG  SPACE PAUSE  Q/E SPEED  Z ROTATE",12,34,small);
        char mode[160];
        if(package_static_)std::snprintf(mode,sizeof mode,"VARIANT %s - STATIC BODY - NO CLIP DECLARED",package_variant_.c_str());
        else if(package_preview_)std::snprintf(mode,sizeof mode,"VARIANT %s - CLIP %u/%u - %.3gX %s",
            package_variant_.c_str(),page_+1,page_count(),double(speed_),paused_?"PAUSED":"LOOPING");
        else std::snprintf(mode,sizeof mode,"%s - %.3gX %s",
            source_handlers_?"HELD-INPUT DASH HOLDS/TIMERS - 96-TICK WINDOW - FINISH POSE HELD":
            source_programs_?"VM WAITS/CONTROLS IN PLACE - 96-TICK REPLAY WINDOW":"BODY CLIPS IN PLACE - 10000+ IDS ARE SHARED CLIPS",
            double(speed_),paused_?"PAUSED":"LOOPING");draws+=text_.draw(render,mode,12,50,small);
        for(unsigned i=0;i<cells_.size();++i){
            const auto& cell=*cells_[i];const s32 x=s32((i%columns_)*tile_width)+10,y=top+s32((i/columns_)*tile_height)+tile_height-34;char label[128];
            if(source_handlers_&&cell.entry.choice>=27&&cell.entry.choice<=30)
                std::snprintf(label,sizeof label,"P%u T%u F%.0f C%u %s",cell.entry.choice,cell.program_tick,double(cell.vm.state().lanes[1].motion.frame),cell.vm.state().actor.control1[0],cell.dash.selector==0?"DONE":cell.dash.selector==13?"ACTIVE":"RECOVER");
            else if(source_programs_)std::snprintf(label,sizeof label,"P%u:%u T%u CONTROL%u",cell.entry.group,cell.entry.choice,cell.program_tick,cell.vm.state().actor.control1[0]);
            else if(package_static_)std::snprintf(label,sizeof label,"STATIC BODY");
            else if(package_preview_)std::snprintf(label,sizeof label,"%s G%u C%u",preview_label(package_clip_labels_[page_]).c_str(),cell.entry.group,cell.entry.choice);
            else std::snprintf(label,sizeof label,"%04u  G%u C%u%s",cell.entry.id,cell.entry.group,cell.entry.choice,cell.entry.common?" SHARED":"");
            draws+=text_.draw(render,label,x,y,title);
            if(!cell.valid)draws+=text_.draw(render,source_programs_?"UNSUPPORTED PROGRAM - NOT IDLE FALLBACK":"UNSUPPORTED CLIP - NOT IDLE FALLBACK",x,y-14,small);
        }
        draws+=text_.draw(render,package_preview_?"PACKAGE BODY ONLY - NO ACTOR, GAMEPLAY RULES OR EFFECTS.":source_programs_?"SOURCE PROGRAM TIMELINES ONLY - NO GAMEPLAY INPUT, PHYSICS, ROOT INTEGRATION OR ATTACK EFFECTS.":"CLASSIFY BY BORG + NUMBER. IDS KEEP RAW GROUP/CLIP METADATA; NO UNIVERSAL LOCOMOTION MAP ASSUMED.",12,height-24,small);
        render.state()=saved;return draws;
    }
    unsigned page()const{return page_;}unsigned borg()const{return selected_;}
    bool select(unsigned borg,unsigned page,RenderEngine& render,std::string* error){
        if(!ready_||borg>=sources_.size()||(source_programs_&&borg)||page>=(sources_[borg].entries.size()+columns_*rows_-1)/(columns_*rows_)){if(error)*error="animation mosaic: Borg/page out of range";return false;}
        if(package_preview_){
            if(render.frame_in_progress()||cells_.size()!=1||(!package_static_&&page>=package_clips_.size())||
               (package_texture_owner_&&package_texture_owner_!=&render)){
                if(error)*error="package preview: current renderer between frames required";
                return false;
            }
            if(page==page_)return true;
            auto& cell=*cells_.front();
            // Clip bindings were all verified at load. Keep textures and rig.
            page_=page;clock_=0;cell.entry=sources_[0].entries[page];
            if(!evaluate(cell,0,error)){ready_=cell.valid=false;return false;}
            return true;
        }
        selected_=borg;page_=page;return prepare_page(render,error);
    }
    unsigned page_count()const{return std::max(1u,(unsigned(sources_[selected_].entries.size())+columns_*rows_-1)/(columns_*rows_));}
    const std::vector<NativeAnimationEntry>& catalog(unsigned borg)const{return sources_[borg].entries;}
    std::array<s32,4> preview_bounds(s32 width,s32 height,unsigned cell)const{
        const s32 tile_width=width/s32(columns_),tile_height=(height-76-36)/s32(rows_);
        return {s32(cell%columns_)*tile_width+8,76+s32(cell/columns_)*tile_height+8,tile_width-16,tile_height-48};
    }
    std::vector<NativeAnimationPreview> page_previews()const{
        std::vector<NativeAnimationPreview> result;for(const auto& cell:cells_)result.push_back({cell->entry,source_programs_?95.f:package_static_?0.f:raw_clip(*cell).end_frame,cell->valid,cell->error,source_programs_});return result;
    }
    void sample_phase(f32 phase){
        for(auto& cell:cells_)if(cell->valid&&!evaluate(*cell,std::clamp(phase,0.f,.999999f)*(source_programs_?96.f:package_static_?1.f:std::max(1.f,std::floor(raw_clip(*cell).end_frame)+1)),&cell->error))cell->valid=false;
    }
private:
    struct Source {std::string name;std::vector<u8> prefix,pose,model_bytes;HsdRecordAnimationBank bank;CharacterMotionBank motions,common;std::vector<NativeAnimationEntry> entries;};
    struct Cell {NativeAnimationEntry entry;CharacterModelScene model;HsdBoneMotionClip clip;CharacterProgramVm vm;CharacterProgramHeightControl height;GRedGroundDashState dash;f32 speed=0,change=0;unsigned program_tick=0;std::string error;bool valid=false,program_started=false;std::array<f32,3> centre{0,90,0};f32 radius=120;};
    const HsdBoneMotionClip& raw_clip(const Cell& cell)const{return package_preview_?package_clips_[page_]:cell.clip;}
    static ByteView view(const std::vector<u8>& bytes){return {bytes.data(),bytes.size()};}
    static bool whole_body_profile(ByteView model,ByteView prefix,std::vector<u8>* out,std::string* error){
        HsdModelBones bones;
        if(!hsd_bones_load(prefix,model,&bones,error))return false;
        // Presentation-only: one lane animates the complete exact logical rig
        // without mirroring. This is NOT an inferred gameplay lane layout.
        const unsigned n=unsigned(bones.bones.size()),order=32,mirror=order+n,identities=(mirror+n+3)&~3u;
        if(n<2||n>kHsdMaxCharacterBones){if(error)*error="whole-body preview: rig size unsupported";return false;}
        out->assign(identities+n*4,0);auto* p=out->data();
        store_be32(p,0x47464350);store_be16(p+4,1);store_be16(p+6,u16(n));
        p[8]=1;p[9]=p[10]=p[11]=u8(n);
        store_be32(p+12,order);store_be32(p+16,mirror);store_be32(p+20,identities);
        store_be32(p+24,u32(out->size()));store_be32(p+28,bones.root_offset);
        for(unsigned i=0;i<n;++i){p[order+i]=p[mirror+i]=u8(i);store_be32(p+identities+i*4,bones.bones[i].joint_offset);}
        return true;
    }
    // The small diagnostic font is ASCII/uppercase only. This changes the
    // drawn label, not the catalog name or the variant/identity key.
    static std::string preview_label(std::string_view text){
        std::string label(text);
        for(char& c:label){
            if(c>='a'&&c<='z')c=char(c-'a'+'A');
            if(c=='_')c='-';
            if(!TextRenderer::covers(c))return {};
        }
        return label;
    }
    static bool read(IFileSystem& files,const std::string& path,std::vector<u8>* out,std::string* error,u64 maximum=32u<<20){u64 size=0;if(!files.file_size(path,&size)||!size||size>maximum||!files.read(path,*out)||out->size()!=size){if(error)*error="animation mosaic: missing/oversized "+path;return false;}return true;}
    bool evaluate(Cell& cell,f64 clock,std::string* error){
        if(source_programs_){
            const unsigned target=unsigned(std::fmod(std::max(0.,clock),96.));
            const bool handlers=source_handlers_&&cell.entry.choice>=27&&cell.entry.choice<=30;
            std::vector<CharacterProgramOperation> operations;
            if(!cell.program_started||target<cell.program_tick){
                CharacterPoseHeightFields fields;fields.model_group3e8=6;
                if(!cell.vm.load_fresh_actor_height_controls(program_asset_,{},error)||
                   !cell.height.bind(cell.vm,program_height_,fields,{},CharacterProgramHeightControlMode::DeclaredFields,error))return false;
                if(handlers){
                    cell.dash={};cell.dash.direction_class=cell.dash.coarse57d=u8(cell.entry.choice-27);
                    GRedGroundDashPlan plan;
                    if(!gred_ground_dash_plan(program_motion_,cell.dash,0,0,0,&plan)||
                       !cell.vm.request_with_actor_controls(1,plan.program,2,-1,1,program_driver_,cell.height,&operations,error)||
                       !cell.vm.request_with_actor_controls(1,plan.program,1,-1,1,program_driver_,cell.height,&operations,error))return false;
                    cell.dash=plan.state;cell.speed=plan.speed44;cell.change=plan.change4c;
                }else if(!cell.vm.request_with_actor_controls(cell.entry.group,u8(cell.entry.choice),3,-1,1,program_driver_,cell.height,&operations,error))return false;
                cell.program_started=true;cell.program_tick=0;
            }
            while(cell.program_tick<target){
                if(handlers){
                    if(cell.dash.selector){
                        GRedGroundDashPlan plan;
                        if(!gred_ground_dash_plan(program_motion_,cell.dash,cell.speed,cell.change,cell.vm.state().actor.control1[0],&plan))return false;
                        if(plan.advance_program&&(!cell.vm.update_with_actor_controls(2,1,program_driver_,cell.height,&operations,error)||
                                                  !cell.vm.update_with_actor_controls(1,1,program_driver_,cell.height,&operations,error)))return false;
                        //Only the scalar speed step is inspected here; no world
                        //position, target steering, collision or actor is produced.
                        cell.speed=std::max(0.f,std::fma(plan.change4c,1.f,plan.speed44));cell.change=plan.change4c;
                        if(!gred_ground_dash_finish(program_motion_,plan,cell.vm.state().actor.control1[0],cell.vm.state().lanes[1].done,
                                                   program_motion_.held_dash_masks[cell.dash.coarse57d],1,&cell.dash))return false;
                    }
                }else if(!cell.vm.update_with_actor_controls(3,1,program_driver_,cell.height,&operations,error))return false;
                ++cell.program_tick;
            }
            std::array<CharacterProgramMotion,2> motions{cell.vm.state().lanes[0].motion,cell.vm.state().lanes[1].motion};
            CharacterLocalPose local;CharacterPoseFrame pose;CharacterWorldPose world;
            return program_driver_.compose_local_pose(motions,&local,error)&&
                character_pose_initial_frame(cell.model.rig(),mat4_rotation_y(yaw_),&pose,error)&&
                character_pose_direct_world(cell.model.rig(),local,pose,&world,error)&&cell.model.set_world_pose(world,{},error);
        }
        if(package_static_){
            CharacterLocalPose local;CharacterPoseFrame pose;CharacterWorldPose world;
            const auto& rig=cell.model.rig();
            return character_pose_initial_local(rig,&local,error)&&
                character_pose_initial_frame(rig,mat4_rotation_y(yaw_),&pose,error)&&
                character_pose_direct_world(rig,local,pose,&world,error)&&cell.model.set_world_pose(world,{},error);
        }
        const auto& clip=raw_clip(cell);
        const u32 length=std::max(1u,u32(clip.end_frame)+1);const f64 frame=std::fmod(clock,length);const u32 time=u32(frame);const f32 fraction=f32(frame-time);
        HsdBoneMotionSample a,b;if(!hsd_bone_motion_sample(clip,time,&a,error))return false;
        if(fraction>0&&!hsd_bone_motion_sample(clip,(time+1)%length,&b,error))return false;
        std::array<CharacterPoseLane,4> lanes;for(auto& lane:lanes){lane.current=fraction>0?&b:&a;if(fraction>0){lane.previous=&a;lane.previous_weight=1-fraction;}}
        CharacterLocalPose local;CharacterPoseFrame pose;CharacterWorldPose world;
        return character_pose_sample(cell.model.rig(),lanes,&local,error)&&character_pose_initial_frame(cell.model.rig(),mat4_rotation_y(yaw_),&pose,error)&&character_pose_direct_world(cell.model.rig(),local,pose,&world,error)&&cell.model.set_world_pose(world,{},error);
    }
    bool prepare_page(RenderEngine& render,std::string* error){
        cells_.clear();clock_=0;const auto& source=sources_[selected_];const unsigned first=page_*columns_*rows_,last=std::min(unsigned(source.entries.size()),first+columns_*rows_);
        for(unsigned i=first;i<last;++i){
            auto cell=std::make_unique<Cell>();cell->entry=source.entries[i];const auto& bank=cell->entry.common?source.common:source.motions;
            cell->valid=cell->model.load_with_record_bank(source.bank,view(source.prefix),view(source.pose),&cell->error);
            if(cell->valid&&package_preview_&&!package_static_){
                for(const auto& entry:source.entries){
                    HsdBoneMotionClip clip;
                    if(!hsd_bone_motion_load(view(bank.archives[entry.archive]),cell->model.rig().binding,&clip,&cell->error)){
                        cell->error="invalid "+package_clip_labels_[entry.id]+" clip: "+cell->error;
                        cell->valid=false;break;
                    }
                    package_clips_.push_back(std::move(clip));
                }
            }else if(cell->valid&&!source_programs_&&!package_static_){const auto& bytes=bank.archives[cell->entry.archive];cell->valid=hsd_bone_motion_load(view(bytes),cell->model.rig().binding,&cell->clip,&cell->error);}
            if(cell->valid)cell->valid=evaluate(*cell,0,&cell->error);
            if((source_programs_||package_preview_)&&!cell->valid){if(error)*error=(package_preview_?"package preview: ":"source program mosaic: ")+cell->error;ready_=false;return false;}
            if(cell->valid){
                std::array<f32,3> low{1e7f,1e7f,1e7f},high{-1e7f,-1e7f,-1e7f};
                for(const auto& mesh:cell->model.world_geometry().world_vertices)for(const auto& v:mesh)for(unsigned k=0;k<3;++k){low[k]=std::min(low[k],v.position[k]);high[k]=std::max(high[k],v.position[k]);}
                f32 extent=1;for(unsigned k=0;k<3;++k){cell->centre[k]=(low[k]+high[k])*.5f;extent=std::max(extent,high[k]-low[k]);}
                //Program poses can extend beyond their initial stance. Extra
                //inspection-camera margin is not a source movement parameter.
                cell->radius=std::max(50.f,extent*(source_programs_?.85f:.65f));
                if(package_preview_){
                    const auto count=cell->model.asset().scene.textures.size();
                    if(count>1024){if(error)*error="package preview: texture count budget";return false;}
                    for(unsigned i=0;i<count;++i)if(render.texture_is_shared(package_texture_base_+i)){
                        if(error)*error="package preview: texture range conflicts with shared art";
                        return false;
                    }
                    if(count&&!render.release_texture(package_texture_base_)){
                        if(error)*error="package preview: backend cannot release preview textures";
                        return false;
                    }
                    package_texture_count_=unsigned(count);package_texture_owner_=&render;
                    if(cell->model.upload_textures(render,package_texture_base_)!=count){
                        if(error)*error="package preview: texture upload failed";
                        return false;
                    }
                }else cell->model.upload_textures_shared(render,selected_?70000:60000);
            }
            cells_.push_back(std::move(cell));
        }
        if(render.shared_texture_upload_failed()){if(error)*error="animation mosaic texture upload failed";return false;}
        return true;
    }
    std::array<Source,2> sources_;std::vector<std::unique_ptr<Cell>> cells_;TextRenderer text_;std::vector<Vertex> panels_;
    CharacterProgramAsset program_asset_;CharacterPoseHeightProfile program_height_;CharacterMotionDriver program_driver_;bool source_programs_=false;
    CharacterMovementProfile program_motion_;bool source_handlers_=false,package_preview_=false,package_static_=false;
    std::string package_variant_;
    std::vector<HsdBoneMotionClip> package_clips_;
    std::vector<std::string> package_clip_labels_;
    static constexpr u32 package_texture_base_=65000;
    unsigned package_texture_count_=0;
    RenderEngine* package_texture_owner_=nullptr;
    bool package_returnable_=false;
    unsigned selected_=0,page_=0,columns_=4,rows_=3;f64 clock_=0;f32 speed_=.5f,yaw_=0;bool ready_=false,paused_=false;GamePadState previous_{};
};
}
