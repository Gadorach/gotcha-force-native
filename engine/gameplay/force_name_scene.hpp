#pragma once
#include "engine/gameplay/force_name_keyboard.hpp"
#include "engine/gameplay/animated_model_scene.hpp"
#include "engine/graphics/source_text.hpp"
#include "engine/graphics/depth_clear.hpp"
namespace gf {
struct ForceNameWidgetPose {Mat4 world;f32 frame=0,alpha=1;u8 tint=255;bool visible=false;};
// Source801C70EC/type callbacks. Corner phases are explicit caller-supplied
// native phases; they are not presented as recovered source RNG outcomes.
inline ForceNameWidgetPose force_name_widget_pose(const ForceNameArt& art,const ForceNameKeyboard& keyboard,unsigned index,const std::array<u16,4>& corners){
    ForceNameWidgetPose out;if(index>=art.widgets.size()||!keyboard.visible())return out;
    const auto& w=art.widgets[index];auto position=w.position;f32 sx=1,sy=1,angle=0;out.visible=true;position[1]+=keyboard.offset();
    if(w.type==1){if(index<1||index>4){out.visible=false;return out;}angle=std::bit_cast<s16>(u16(corners[index-1]-u16(keyboard.age()*256)))*art.angle_unit;}
    else if(w.type==2){out.visible=keyboard.title_visible();const auto tick=keyboard.title_tick();
        if(tick<24){const auto sine=std::sin(f32((tick*16384)/24)*art.angle_unit);sx=std::fma(art.title_delta,sine,art.title_start);sy=art.title_y*sine;}
        else{const auto cosine=std::cos(std::bit_cast<s16>(u16((tick-23)*1024))*art.angle_unit);sx=1-art.title_pulse*cosine;sy=1+art.title_pulse*cosine;}
        // The first authored title sample has exactly zero height. It contributes
        // no triangles; omit that instance rather than submit a singular matrix.
        if(sy==0)out.visible=false;
    }else if(w.type==3){out.visible=keyboard.text_visible();out.frame=f32(keyboard.age()%150)/art.underline_divisor;out.alpha=keyboard.underline_alpha();out.tint=keyboard.underline_tint();}
    else if(w.type==4){position[0]=art.x[keyboard.column()];position[1]=art.y[keyboard.row()]+keyboard.offset();angle=std::bit_cast<s16>(u16(0-u16(keyboard.age()*256)))*art.angle_unit;}
    else if(w.type>=5&&w.type<=7)out.visible=w.type==5;
    else if(w.type>=8)out.visible=(w.type&1)==unsigned(keyboard.age()%60>29);
    const f32 scale[3]={sx,sy,1};out.world=mat4_multiply(mat4_translation(position.data()),mat4_multiply(mat4_rotation_z(angle),mat4_scale(scale)));return out;
}
class ForceNameScene {
public:
    bool load(ByteView archive,const ForceNameArt& art,const SourceFont& font,const AttachmentAlternateProfile& style,const Camera& camera,RenderEngine& render,std::array<u16,4> corners,std::string* error){
        ready_=false;models_.clear();empty_.clear();art_=art;camera_=camera;corners_=corners;
        if(!art.valid_for(font)||!bank_.load(archive,error,true)||bank_.hash()!=art.bank_hash||!text_.load(font,render,12000))return false;
        for(const auto& widget:art.widgets){AnimatedModelScene model;const u8 channels=widget.type==3?2:0;
            const bool empty=bank_.is_empty_static_record(widget.record);empty_.push_back(empty);
            if(empty){models_.push_back(std::move(model));continue;}
            if(!model.load_compact_alternate_textures(bank_,widget.record,channels,0,0,style,error)||!model.request(0,0,error)||!model.replace_unanimated_alpha(1,error))return false;
            model.set_enabled(true);const auto first=12001+u32(models_.size())*128;model.upload_textures_shared(render,first);models_.push_back(std::move(model));
        }
        if(render.shared_texture_upload_failed())return false;
        ready_=true;return true;
    }
    bool draw(RenderEngine& render,const ForceNameKeyboard& keyboard,std::string* error){
        if(!ready_||render.model_pass())return false;
        if(!keyboard.visible())return true;
        const auto saved=render.state();render.state()={};render.state().blend=true;render.state().blend_src=5;render.state().blend_dst=6;render.state().depth_test=render.state().depth_write=false;
        Mat4 identity;if(!render.upload_matrix(0,identity.m)){render.state()=saved;return false;}
        std::vector<Vertex> dim;ScreenDraw::textured_quad(dim,0,0,camera_.viewport_width,camera_.viewport_height,0,0,0,1,1,{u8(art_.dim_colour>>24),u8(art_.dim_colour>>16),u8(art_.dim_colour>>8),keyboard.dim()});
        const auto first=render.append_vertices(dim.data(),dim.size());const bool dim_ok=render.draw(Primitive::Triangles,first,u32(dim.size()));render.state()=saved;if(!dim_ok||!draw_depth_clear(render,camera_.viewport_width,camera_.viewport_height))return false;
        std::vector<ForceNameWidgetPose> poses;for(unsigned i=0;i<models_.size();++i){auto pose=force_name_widget_pose(art_,keyboard,i,corners_);auto& model=models_[i];
            if(empty_[i])pose.visible=false;
            if(pose.visible){if(!model.request_again(pose.frame,0,error)||!model.replace_unanimated_alpha(pose.alpha,error)||!model.set_root_world(pose.world,error))return false;model.set_post_rgb_multiply({pose.tint,pose.tint,pose.tint});}poses.push_back(pose);
        }
        for(u8 pass:{u8(1),u8(4),u8(2)}){if(!render.begin_model_pass(pass))return false;for(unsigned i=0;i<models_.size();++i)if(poses[i].visible)models_[i].draw(render,camera_);if(!render.end_model_pass())return false;}
        if(!keyboard.screen_text_visible())return true;
        const f32 sx=camera_.viewport_width/art_.screen_width,sy=camera_.viewport_height/art_.screen_height;
        auto pair=[](u16 code){return std::string{char(code>>8),char(code)};};const auto& colours=art_.colours;
        //8005116C loads style+32 into KONST and style+36 into TEV0:
        // (TEV0*(1-TEXC) + KONST*TEXC). The profile retains source order;
        // draw_screen expects low,high, as do the existing list/HUD callers.
        if(!text_.draw_screen(render,pair(keyboard.selected()),art_.screen_x[keyboard.column()]+art_.preview_x,art_.screen_y[keyboard.row()]+art_.preview_y,art_.cell*art_.preview_scale,0,sx,sy,colours[1][1],colours[1][0]))return false;
        if(!text_.draw_screen(render,keyboard.text(),art_.name_x,art_.name_y,art_.cell*art_.name_scale,art_.name_advance,sx,sy,colours[0][1],colours[0][0]))return false;
        return !keyboard.cursor_visible()||text_.draw_screen(render,pair(art_.cursor_code),art_.name_x+keyboard.cursor()*art_.name_advance,art_.name_y,art_.cell*art_.name_scale,0,sx,sy,colours[0][1],colours[0][0]);
    }
private:
    bool ready_=false;HsdRecordAnimationBank bank_;ForceNameArt art_;Camera camera_;SourceTextRenderer text_;std::array<u16,4> corners_{};std::vector<AnimatedModelScene> models_;std::vector<bool> empty_;
};
} // namespace gf
