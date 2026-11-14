#pragma once
#include "engine/gameplay/borg_stats_editor.hpp"
#include "engine/gameplay/force_spec_scene.hpp"
#include "engine/gameplay/force_popup_scene.hpp"
namespace gf {
//Native inline edit focus on the original stats widgets, not a popup/form.
//The prior preview has retired/released, or this stats entry was provisional.
class BorgStatsEditScene {
public:
    bool load(ByteView bank,const ForceSpecArt& art,const SourceMessageBank& messages,const SourceFont& font,RenderEngine& render,std::string* error){ready_=bound_=false;if(!ui_.load_with_horizon_fog(bank,art,messages,font,render,error)||!text_.load(font,render,7300))return false;ready_=true;return true;}
    bool bind(const ForceInventory& owner,const BorgStatsEditor& editor,ByteView plate,RenderEngine& render,std::string* error){reset();ForceSpecificationSnapshot snapshot;if(!ready_||!editor.snapshot(owner,&snapshot)||!snapshot.value(owner)||!ui_.bind_nameplate(u16(snapshot.value(owner)->model),plate,render,error))return false;model_=snapshot.value(owner)->model;binding_=editor.binding_;callbacks_=1;bound_=true;return true;}
    bool update(const ForceInventory& owner,const BorgStatsEditor& editor){if(!current(owner,editor)||callbacks_==~u32(0))return false;++callbacks_;return true;}
    bool draw(const ForceInventory& owner,const BorgStatsEditor& editor,ForcePopupScene&,RenderEngine& render,std::string* error){
        ForceSpecificationSnapshot snapshot;if(!current(owner,editor)||!editor.snapshot(owner,&snapshot)||!snapshot.value(owner)||snapshot.value(owner)->model!=model_||render.model_pass())return false;
        const auto* old=render.scene_lighting();const bool had=old!=nullptr;const SceneLighting saved=had?*old:SceneLighting{};if(!ui_.lighting()||!render.set_scene_lighting(*ui_.lighting()))return false;
        ForceSpecWidgetInput input;input.borg=*snapshot.view(owner);input.experience=snapshot.value(owner)->experience;input.edited=editor.committed_modified(owner);input.callbacks=callbacks_;input.page=ForceSpecPage::Statistics;
        const auto* fog=ui_.horizon_fog();bool accepted=fog&&ForceSpecScene::draw_backdrop(render,{fog->colour[0],fog->colour[1],fog->colour[2],255})&&ui_.draw_background(render,input,error)&&ui_.draw_borg_edit_foreground(render,input,error);
        //Only fields without an original Statistics widget need addon text.
        if(accepted){static constexpr std::array<const char*,6> variants={"NORMAL","ALT","GOLD","SILVER","CRYSTAL","BLACK"};for(unsigned row=0;row<3;++row){const unsigned field=row?row+7:0;const auto value=editor.field_value(owner,field);const std::string label=row?(row==1?"B AMMO ":"X AMMO ")+std::string(value<0?"DEFAULT":std::to_string(value)):"VARIANT "+std::string(variants[unsigned(value)]);std::string encoded;if(!text_.font().encode_ascii(label,&encoded)||!text_.draw_screen(render,encoded,36,180+36.f*row,20,10,1,1,0x000000ff,0xffffffffu)){accepted=false;break;}}}
        std::array<f32,4> bounds;if(accepted)accepted=selection_bounds(owner,editor,&bounds)&&outline(render,bounds);
        if(accepted){std::string hint;accepted=text_.font().encode_ascii("UP / DOWN SELECT - LEFT / RIGHT CHANGE - A SAVE - B CANCEL",&hint)&&text_.draw_screen(render,hint,118,456,14,7,1,1,0x000000ff,0xffffffffu);}
        if(had){if(!render.set_scene_lighting(saved))accepted=false;}else render.clear_scene_lighting();return accepted;
    }
    bool hint(RenderEngine& render,bool provisional){std::string encoded;return ready_&&text_.font().encode_ascii("E EDIT BORG",&encoded)&&text_.draw_screen(render,encoded,provisional?65.f:370.f,provisional?410.f:468.f,12,6,1,1,0x000000ff,0xffffffffu);}
    bool selection_bounds(const ForceInventory& owner,const BorgStatsEditor& editor,std::array<f32,4>* out){
        if(!out||!current(owner,editor))return false;
        const auto field=editor.cursor();
        if(field==0||field>=8){const unsigned row=field==0?0:field-7;*out={32,176+36.f*row,292,204+36.f*row};return true;}
        ForceSpecificationSnapshot snapshot;if(!editor.snapshot(owner,&snapshot))return false;ForceSpecWidgetInput input;input.borg=*snapshot.view(owner);input.experience=snapshot.value(owner)->experience;input.callbacks=callbacks_;return ui_.borg_edit_bounds(input,field,out);
    }
    void reset(){bound_=false;callbacks_=0;binding_.reset();}
private:
    static bool outline(RenderEngine& render,const std::array<f32,4>& bounds){
        const f32 x=std::max(1.f,bounds[0]-3),y=std::max(1.f,bounds[1]-3),right=std::min(639.f,bounds[2]+3),bottom=std::min(479.f,bounds[3]+3);if(!(x<right&&y<bottom))return false;
        const auto saved=render.state();render.state()={};render.state().depth_test=render.state().depth_write=false;render.state().cull_backface=false;Mat4 identity;if(!render.upload_matrix(0,identity.m)){render.state()=saved;return false;}std::vector<Vertex> vertices;const Tint orange{255,128,0,255};for(const auto& r:std::array<std::array<f32,4>,4>{{{x,y,right,y+2},{x,bottom-2,right,bottom},{x,y,x+2,bottom},{right-2,y,right,bottom}}})ScreenDraw::textured_quad(vertices,r[0],r[1],r[2],r[3],0,0,0,1,1,orange);const auto at=render.append_vertices(vertices.data(),vertices.size());const bool ok=render.draw(Primitive::Triangles,at,u32(vertices.size()));render.state()=saved;return ok;
    }
    bool current(const ForceInventory& owner,const BorgStatsEditor& editor)const{const auto binding=binding_.lock();return bound_&&binding&&binding==editor.binding_&&editor.current(owner);}
    std::weak_ptr<const BorgStatsEditBinding> binding_;
    ForceSpecScene ui_;SourceTextRenderer text_;s16 model_=-1;bool ready_=false,bound_=false;u32 callbacks_=0;
};
} //namespace gf
