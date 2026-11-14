#pragma once
#include <map>
#include "engine/assets/force_popup_art.hpp"
#include "engine/gameplay/shape_model_scene.hpp"
#include "engine/gameplay/animated_model_scene.hpp"
#include "engine/graphics/source_text.hpp"
#include "engine/gameplay/editor_list_mouse.hpp"
namespace gf {
struct ForcePopupPresentation {ForcePopupKind kind=ForcePopupKind::Operations;unsigned selected=0,first=0;u32 disabled=0,frame=25,age=0;bool components=false;std::vector<std::string> labels_ascii{};};
class ForcePopupScene {
public:
    void enable_mouse(bool enabled){mouse_enabled_=enabled;mouse_geometry_.begin();}
    bool mouse_hit(f32 x,f32 y,EditorListMouseTarget* out)const{return mouse_enabled_&&mouse_geometry_.hit(x,y,out);}
    const EditorListMouseGeometry& mouse_geometry()const{return mouse_geometry_;}
    bool load(const HsdRecordAnimationBank& bank,const ForcePopupArt& art,const Camera& camera){
        ready_=text_ready_=mouse_enabled_=false;mouse_geometry_.begin();models_.clear();shapes_.clear();if(bank.hash()!=art.bank_hash||!camera.basis(nullptr))return false;bank_=bank;art_=art;camera_=camera;ready_=true;return true;
    }
    bool configure_text(const SourceFont& font,const ForceListArt& layout,RenderEngine& render){
        text_ready_=false;if(!ready_||layout.bank_hash!=bank_.hash()||!std::isfinite(layout.extent)||!std::isfinite(layout.advance)||layout.extent<=0||layout.extent>10000||layout.advance<=0||layout.advance>10000||!text_.load(font,render,7200))return false;
        std::array<f32,6> left{};for(unsigned kind=0;kind<left.size();++kind){if(art_.menus[kind].labels.empty())return false;AnimatedModelScene label;std::string error;f32 lo[3],hi[3];if(!label.load_compact_textures(bank_,art_.menus[kind].labels[0],2,0,0,&error)||!label.request(0,0,&error)||!label.set_root_world(Mat4{},&error)||!label.bounds(lo,hi))return false;left[kind]=lo[0]+layout.extent*.5f;}
        text_left_=left;text_extent_=layout.extent;text_advance_=layout.advance;text_ready_=true;return true;
    }
    bool draw(RenderEngine& render,const ForcePopupPresentation& p,std::string* error){
        return draw_many(render,{&p,1},error);
    }
    bool draw_many(RenderEngine& render,std::span<const ForcePopupPresentation> states,std::string* error){
        if(mouse_enabled_)mouse_geometry_.begin();
        if(!ready_||states.size()>4)return false;
        for(const auto& p:states){if(unsigned(p.kind)>6||p.frame>u32(art_.open_limit))return false;
            if(!p.labels_ascii.empty()){if(!text_ready_||p.kind==ForcePopupKind::Erase||p.labels_ascii.size()>32||(p.kind!=ForcePopupKind::Tribe&&p.labels_ascii.size()!=art_.menus[unsigned(p.kind)].labels.size()))return false;for(const auto& label:p.labels_ascii){std::string encoded;if(label.empty()||label.size()>32||!text_.font().encode_ascii(label,&encoded))return false;}}
            if(p.kind==ForcePopupKind::Erase){if(p.selected>1||p.first)return false;}
            else{const auto count=p.labels_ascii.empty()?art_.menus[unsigned(p.kind)].labels.size():p.labels_ascii.size();if(p.selected>=count||p.first>p.selected||p.selected>=p.first+8||p.first>=count||(count<=8&&p.first))return false;}}
        for(u8 pass:{u8(1),u8(4),u8(2)}){if(!render.begin_model_pass(pass))return false;bool ok=true;for(unsigned i=0;i<states.size();++i){capture_top_=i+1==states.size();if(!draw_pass(render,states[i],error)){ok=false;break;}}if(!ok)render.reject_model_pass();const bool end=render.end_model_pass();if(!ok||!end)return false;}return !mouse_enabled_||mouse_geometry_.finish();
    }
private:
    struct Model {AnimatedModelScene scene;float frame=0;};
    bool model(RenderEngine& render,u16 id,float frame,const std::array<f32,3>& position,float angle,std::string* error){
        auto found=models_.find(id);if(found==models_.end()){Model m;if(!m.scene.load_compact_textures(bank_,id,2,0,0,error)||!m.scene.request(frame,0,error))return false;m.frame=frame;m.scene.set_enabled(true);m.scene.set_untextured_billboards(true);m.scene.upload_textures_shared(render,1);found=models_.emplace(id,std::move(m)).first;}
        auto& m=found->second;if(m.frame!=frame){if(!m.scene.request_again(frame,0,error))return false;m.frame=frame;}const auto world=mat4_multiply(mat4_translation(position.data()),mat4_rotation_z(angle));if(!m.scene.set_root_world(world,error))return false;PointerTriangles triangles;m.scene.draw(render,camera_,mouse_enabled_&&capture_top_&&mouse_target_?&triangles:nullptr);return !mouse_enabled_||!capture_top_||!mouse_target_||mouse_geometry_.add(std::move(triangles),*mouse_target_);
    }
    bool panel(RenderEngine& render,u16 id,u32 frame,const std::array<f32,3>& position,std::string* error){
        auto found=shapes_.find(id);if(found==shapes_.end()){ShapeModelScene shape;if(!shape.load(bank_,id,render,1,error,true))return false;found=shapes_.emplace(id,std::move(shape)).first;}
        if(!found->second.sample(frame,mat4_translation(position.data()),error))return false;
        found->second.draw(render,camera_);return true;
    }
    bool draw_pass(RenderEngine& render,const ForcePopupPresentation& p,std::string* error){
        const bool confirm=p.kind==ForcePopupKind::Erase;const float z=confirm?art_.confirm_z:art_.menu_z+(p.kind==ForcePopupKind::Tribe?1.f:0.f);
        const auto angle=std::bit_cast<s16>(u16(0u-u32(p.age)*256u))*art_.angle_unit;
        if(confirm){
            for(unsigned i=0;i<6;++i){if(i&&p.frame<=art_.visible_after)continue;auto w=art_.confirmation[i];w.position[2]+=z;if(i==1||i==2)w.position[1]+=art_.row_step*p.selected;if(i==3&&!p.components)++w.record;
                if(i==0){if(!panel(render,u16(w.record),p.frame,w.position,error))return false;}
                else{if(i>=4&&mouse_enabled_)mouse_target_=EditorListMouseTarget{s16(i-4),0x100,0};if(!model(render,u16(w.record),i==1?1.f:0.f,w.position,i==2?angle:0,error))return false;mouse_target_.reset();}
            }
        }else{
            const auto& layout=art_.menus[unsigned(p.kind)];const auto count=p.labels_ascii.empty()?layout.labels.size():p.labels_ascii.size();const bool scrolling=count>8;
            for(unsigned i=0;i<(scrolling?5u:3u);++i){if(i&&(p.frame<=art_.visible_after||(i>=3&&p.age%72>=49)))continue;auto w=layout.widgets[i];w.position[2]+=z;if(i==1||i==2)w.position[1]+=art_.row_step*(p.selected-p.first);
                if(i==0){if(!panel(render,u16(w.record),p.frame,w.position,error))return false;}
                else{if(i>=3&&mouse_enabled_)mouse_target_=EditorListMouseTarget{-1,0,i==3?8u:4u};if(!model(render,u16(w.record),0,w.position,i==2?angle:i==4?3.14159265f:0,error))return false;mouse_target_.reset();}
            }
            if(p.frame>art_.visible_after)for(unsigned i=0;i<8&&p.first+i<count;++i){const unsigned row=p.first+i;auto position=layout.label_origin;position[1]+=art_.row_step*i;position[2]+=z;
                if(mouse_enabled_)mouse_target_=EditorListMouseTarget{s16(row),0x100,0};
                if(!p.labels_ascii.empty()){std::string encoded;if(!text_.font().encode_ascii(p.labels_ascii[row],&encoded))return false;position[0]+=text_left_[unsigned(p.kind)];const u8 intensity=p.disabled&(1u<<row)?128:255;PointerTriangles triangles;if(!text_.draw(render,camera_,mat4_translation(position.data()),encoded,text_extent_,text_advance_,{intensity,intensity,intensity,255},false,0x000000ff,0xffffffffu,mouse_enabled_&&capture_top_?&triangles:nullptr)|| (mouse_enabled_&&capture_top_&&!mouse_geometry_.add(std::move(triangles),*mouse_target_)))return false;}
                else if(!model(render,layout.labels[row],(p.disabled&(1u<<row))?art_.disabled_frame:0,position,0,error))return false;
                mouse_target_.reset();}
        }
        return true;
    }
    bool ready_=false,text_ready_=false;ForcePopupArt art_;Camera camera_;HsdRecordAnimationBank bank_;std::map<u16,Model> models_;std::map<u16,ShapeModelScene> shapes_;SourceTextRenderer text_;f32 text_extent_=0,text_advance_=0;std::array<f32,6> text_left_{};
    bool mouse_enabled_=false,capture_top_=false;EditorListMouseGeometry mouse_geometry_;std::optional<EditorListMouseTarget> mouse_target_;
};
} // namespace gf
