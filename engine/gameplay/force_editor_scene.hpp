#pragma once
#include <map>
#include "engine/assets/force_editor_art.hpp"
#include "engine/gameplay/animated_model_scene.hpp"
#include "engine/gameplay/force_editor_grid.hpp"
#include "engine/graphics/scene_draw.hpp"
#include "engine/graphics/text.hpp"
#include "engine/assets/force_grid_hud_art.hpp"
#include "engine/graphics/source_text.hpp"
#include "engine/graphics/depth_clear.hpp"
#include "engine/gameplay/animated_shape_model_scene.hpp"
#include "engine/gameplay/force_budget_band.hpp"
#include "engine/gameplay/force_tooltip.hpp"
#include "engine/gameplay/force_wheel_motion.hpp"
#include "engine/gameplay/editor_pointer_plane.hpp"
#include "engine/gameplay/static_record_scene.hpp"
namespace gf {
// Native editor presentation through AnimatedModelScene -> MenuScene -> shared
// RenderEngine. GFED supplies source camera, grid and wheel placement operands.
class ForceEditorScene {
public:
    bool load(ByteView deck,ByteView icons,const ForceEditorArt& art,std::string* error){
        hud_ready_=band_ready_=tooltip_ready_=motion_ready_=mouse_hand_=false;band_scene_={};tooltip_={};motion_={};turn_=0;folded_=false;hud_models_.clear();deck_models_.clear();icon_models_.clear();
        art_=art;camera_=art.camera;scroll_=art.scroll;cursor_=art.cursor;
        if(!deck_.load(deck,error,true)||!icons_.load(icons,error,true))return false;
        return camera_.basis(nullptr);
    }
    const Camera& camera()const{return camera_;}
    bool configure_hud(const ForceGridHudArt& hud,const ForceListArt& lists,const SourceFont& font,RenderEngine& render){
        hud_ready_=band_ready_=tooltip_ready_=motion_ready_=mouse_hand_=false;band_scene_={};tooltip_={};motion_={};hud_models_.clear();if(hud.bank_hash!=deck_.hash()||lists.bank_hash!=deck_.hash()||!hud_text_.load(font,render,6000))return false;
        hud_=hud;list_art_=lists;hud_ready_=true;return true;
    }
    bool retail_hud()const{return hud_ready_;}
    bool configure_band(const ForceBudgetBandArt& art,std::string* error){
        band_ready_=false;band_scene_={};if(!hud_ready_||art.bank_hash!=deck_.hash()||art.parts.size()!=11||!band_scene_.load(deck_,art.record,error))return false;band_art_=art;band_ready_=true;return true;
    }
    bool configure_tooltip(const ForceTooltipArt& art,const ForceInventory& owner){
        tooltip_ready_=false;tooltip_={};if(!hud_ready_||art.bank_hash!=deck_.hash()||!tooltip_.bind_bottom(owner,art,camera_.viewport_width))return false;tooltip_art_=art;tooltip_ready_=true;return true;
    }
    bool configure_motion(const ForceWheelMotionProfile& profile){
        motion_ready_=false;motion_={};if(!hud_ready_||profile.bank_hash!=deck_.hash()||(band_ready_&&!band_art_.has_following)||!motion_.configure(profile,turn_,folded_))return false;motion_profile_=profile;motion_ready_=true;return true;
    }
    bool wheel_turning()const{return motion_ready_&&motion_.turning();}
    bool wheel_folded()const{return folded_;}
    const ForceWheelMotion* wheel_motion()const{return motion_ready_?&motion_:nullptr;}
    bool update_ui(const ForceInventory& owner,unsigned force,s16 held,bool grid_active){
        if(motion_ready_&&grid_active&&!motion_.step())return false;
        if(!tooltip_ready_)return true;
        const f32 world[3]={cursor_[0],0,cursor_[1]};std::array<f32,2> screen{};if(!camera_.project(world,&screen[0],&screen[1],nullptr))return false;
        return tooltip_.step(owner,force,held,hit(),screen,grid_active);
    }
    const std::array<f32,2>& cursor_position()const{return cursor_;}
    std::array<f32,3> member_position(unsigned slot)const{
        if(motion_ready_){ForceWheelPose pose;if(!motion_.member(art_,slot,&pose))return {NAN,NAN,NAN};const auto angle=pose.angle*motion_profile_.angle_unit;return {art_.wheel[0]+motion_.wheel_shift()+art_.member_base+pose.offset+art_.member_radius*std::cos(angle),0,art_.wheel[2]-art_.member_radius*std::sin(angle)};}
        const unsigned index=(slot+30-turn_)%30;const float angle=art_.angles[index]*0.000095873801969f;
        return {art_.wheel[0]+(folded_?300.f:0.f)+art_.member_base+art_.offsets[index]+art_.member_radius*std::cos(angle),0,art_.wheel[2]-art_.member_radius*std::sin(angle)};
    }
    std::array<f32,3> box_position(unsigned slot)const{return {art_.icon_origin[0]+art_.cell*(slot%5+(slot/40)*5)+scroll_[0],0,art_.icon_origin[1]+art_.cell*((slot%40)/5)+scroll_[1]};}
    bool rotate(int direction){if(motion_ready_){if(!motion_.request_turn(direction))return false;turn_=motion_.turn();return true;}turn_=unsigned((int(turn_)+direction%30+30)%30);return true;}
    void toggle_wheel(){folded_=!folded_;if(motion_ready_)motion_.toggle_fold();}
    void move_cursor(int horizontal,int vertical){
        // Keyboard axes supply +/-72 source-style stick units; BF144 divides by9.
        cursor_[0]+=std::clamp(horizontal,-1,1)*8.f;cursor_[1]-=std::clamp(vertical,-1,1)*8.f;
        auto edge=[&](unsigned axis,f32 border,f32 limit,bool high,f32 clip){
            const f32 delta=cursor_[axis]-border;if(high?delta<=0:delta>=0)return;
            const auto move=std::clamp(delta,-8.f,8.f);scroll_[axis]-=move;cursor_[axis]-=move;
            if(high?scroll_[axis]<limit:scroll_[axis]>limit){cursor_[axis]-=scroll_[axis]-limit;scroll_[axis]=limit;cursor_[axis]=high?std::min(cursor_[axis],clip):std::max(cursor_[axis],clip);}
        };
        edge(1,-145,-120,true,-145);edge(1,-360,-25,false,-360);edge(0,folded_?130.f:170.f,-800,true,170);edge(0,-130,-15,false,-170);
    }
    void scroll(int x,int y){scroll_[0]=std::clamp(scroll_[0]+x*art_.cell,-800.f,-15.f);scroll_[1]=std::clamp(scroll_[1]+y*art_.cell,-120.f,-25.f);}
    bool pointer(f32 x,f32 y){
        std::array<f32,3> point;if(!editor_pointer_plane(camera_,x,y,&point))return false;cursor_={point[0],point[2]};return true;
    }
    bool set_mouse_hand(bool enabled,f32 x=0,f32 y=0){mouse_hand_=false;if(!enabled)return true;if(!hud_ready_||x<0||y<0||x>=camera_.viewport_width||y>=camera_.viewport_height||!editor_pointer_plane(camera_,x,y,&mouse_hand_position_))return false;mouse_hand_=true;return true;}
    bool mouse_hand_active()const{return mouse_hand_;}
    const std::array<f32,3>& mouse_hand_position()const{return mouse_hand_position_;}
    bool draw_mouse_hand(RenderEngine& render,s16 held,u32 tick,std::string* error){
        if(!mouse_hand_||!hud_ready_||held< -1||held>=s16(kBoxBorgs)||render.model_pass())return false;
        const auto phase=std::bit_cast<s16>(u16((tick+1)*hud_.pulse_step));const auto pulse=std::clamp(std::fma(std::sin(phase*hud_.angle_unit),.5f,.5f),0.f,1.f);
        for(u8 pass:{u8(1),u8(4),u8(2)}){if(!render.begin_model_pass(pass))return false;const bool ok=hud_model(render,held>=0?hud_.held_hand:hud_.hand,pulse,mat4_translation(mouse_hand_position_.data()),camera_,error,true);if(!ok)render.reject_model_pass();const bool ended=render.end_model_pass();if(!ok||!ended)return false;}return true;
    }
    ForceEditorHit hit()const{
        ForceEditorHit result;const auto cx=art_.wheel[0]+wheel_shift()+art_.member_base,cz=art_.wheel[2];
        if(std::hypot(cursor_[0]-cx,cursor_[1]-cz)<=127.7539978f){
            if(wheel_turning())return result;
            for(unsigned i=0;i<30;++i){const auto p=member_position(i);if(p[0]<=190&&std::hypot(cursor_[0]-p[0],cursor_[1]-p[2])<=20){result.member=s8(i);break;}}
        }else{
            const auto x=cursor_[0]-scroll_[0]-art_.grid_origin[0],y=cursor_[1]-scroll_[1]-art_.grid_origin[1];
            if(x>=0&&x<25*art_.cell&&y>=0&&y<8*art_.cell){const auto column=unsigned(x/art_.cell),row=unsigned(y/art_.cell);result.box=s16((column/5)*40+column%5+row*5);}
        }
        return result;
    }
    bool draw(RenderEngine& render,const ForceInventory& owner,unsigned force,s16 held,std::string* error,s32 limit=-1,u32 tick=0,ForceCostMode cost_mode=ForceCostMode::Full){
        if(hud_ready_)return draw_retail(render,owner,force,held,limit,tick,cost_mode,error);
        if(!owner.ready()||force>=20)return false;
        const float grid[3]={scroll_[0],0,scroll_[1]},wheel[3]={art_.wheel[0]+(folded_?300.f:0.f),art_.wheel[1],art_.wheel[2]};
        if(!model(render,deck_,deck_models_,0,mat4_translation(grid),1,error)||!model(render,deck_,deck_models_,2,mat4_translation(grid),1,error)||!model(render,deck_,deck_models_,6,mat4_translation(wheel),1,error))return false;
        for(unsigned slot=0;slot<30;++slot){auto p=member_position(slot);p[1]=7.5f;if(!model(render,deck_,deck_models_,7,mat4_translation(p.data()),1,error))return false;}
        const auto& state=*owner.state();
        for(unsigned i=0;i<200;++i){const auto& b=state.box[i];if(!b.value.present()||s16(i)==held||(b.memberships&(u32(1)<<force)))continue;auto p=box_position(i);if(std::abs(p[0])>300)continue;if(!borg(render,b.value,p,art_.icon_scale,error))return false;}
        for(unsigned slot=0;slot<30;++slot){const auto item=state.forces[force].slots[slot];if(item<0)continue;auto p=member_position(slot);p[1]=7.5f;if(!borg(render,state.box[unsigned(item)].value,p,.5f,error))return false;}
        if(held>=0){const std::array<f32,3> p={cursor_[0],25,cursor_[1]};if(!borg(render,state.box[unsigned(held)].value,p,.5f,error))return false;}
        const auto hovered=hit();
        for(unsigned slot=0;slot<30;++slot){auto p=member_position(slot);p[1]=5;float x=0,y=0,z=0;if(camera_.project(p.data(),&x,&y,&z)&&x>0&&x<640&&y>50&&y<430){text(render,std::to_string(slot+1),int(x)-5,int(y)+10,hovered.member==s8(slot)?Tint{255,230,80,255}:Tint{255,255,255,255},1);}}
        float p[3]={cursor_[0],8,cursor_[1]},x=0,y=0,z=0;
        if(camera_.project(p,&x,&y,&z)){rect(render,x-9,y-1,x+9,y+1,{255,240,100,255});rect(render,x-1,y-9,x+1,y+9,{255,240,100,255});}
        return true;
    }
    bool draw_borg(RenderEngine& render,const ForceBorg& value,f32 x,f32 y,f32 scale,std::string* error){
        // Same model path in a camera-facing native modal preview.
        Camera::Basis basis;if(!camera_.basis(&basis))return false;const float depth=450;
        std::array<f32,3> p;for(unsigned i=0;i<3;++i)p[i]=basis.eye[i]+depth*basis.forward[i]+(x/640*2-1)*depth*basis.tan_half_fov*basis.aspect*basis.right[i]+(1-y/480*2)*depth*basis.tan_half_fov*basis.up[i];
        return borg(render,value,p,scale,error);
    }
    static std::string display_name(std::string_view encoded){
        std::string out;for(std::size_t i=0;i<encoded.size()&&encoded[i];++i){const auto c=u8(encoded[i]);if(c<128){out+=char(c);continue;}if(i+1>=encoded.size())break;const auto b=u8(encoded[++i]);if(c==0x82&&b>=0x4f&&b<=0x58)out+=char('0'+b-0x4f);else if(c==0x82&&b>=0x60&&b<=0x79)out+=char('A'+b-0x60);else if(c==0x82&&b>=0x81&&b<=0x9a)out+=char('a'+b-0x81);else if(c==0x81&&b==0x40)out+=' ';else if(c==0x81&&b==0x7c)out+='-';else out+='?';}return out;
    }
    static void rect(RenderEngine& render,f32 x0,f32 y0,f32 x1,f32 y1,Tint tint){
        std::vector<Vertex> vertices;ScreenDraw::textured_quad(vertices,x0,y0,x1,y1,0,0,0,1,1,tint);const auto base=render.append_vertices(vertices.data(),vertices.size());const auto saved=render.state();render.state()={};render.state().depth_test=false;render.state().depth_write=false;render.state().blend=true;render.state().blend_src=5;render.state().blend_dst=6;render.state().cull_backface=false;Mat4 identity;render.upload_matrix(0,identity.m);render.state().position_matrix=0;render.draw(Primitive::Triangles,base,u32(vertices.size()));render.state()=saved;
    }
    static void text(RenderEngine& render,const std::string& value,int x,int y,Tint colour={255,255,255,255},int scale=2){TextRenderer font;TextRenderer::Style style;style.r=colour.r;style.g=colour.g;style.b=colour.b;style.a=colour.a;style.scale=scale;const auto saved=render.state();render.state()={};render.state().blend_src=5;render.state().blend_dst=6;font.draw(render,value,x,y,style);render.state()=saved;}
private:
    f32 wheel_shift()const{return motion_ready_?motion_.wheel_shift():(folded_?300.f:0.f);}
    struct HudModel {AnimatedModelScene scene;f32 frame=0;};
    Mat4 facing(const std::array<f32,3>& position,f32 scale=1)const{
        Camera::Basis b;camera_.basis(&b);Mat4 world;
        for(unsigned i=0;i<3;++i){world.m[i*4]=b.right[i]*scale;world.m[i*4+1]=b.up[i]*scale;world.m[i*4+2]=-b.forward[i]*scale;world.m[i*4+3]=position[i];}return world;
    }
    bool hud_model(RenderEngine& render,u16 id,f32 frame,const Mat4& world,const Camera& camera,std::string* error,bool always=false){
        auto found=hud_models_.find(id);if(found==hud_models_.end()){HudModel m;if(!m.scene.load_compact_textures(deck_,id,2,0,0,error)||!m.scene.request(frame,0,error)){if(error)*error="grid HUD record "+std::to_string(id)+": "+*error;return false;}m.frame=frame;m.scene.set_enabled(true);m.scene.set_untextured_billboards(true);m.scene.upload_textures_shared(render,1);found=hud_models_.emplace(id,std::move(m)).first;}
        auto& m=found->second;if(m.frame!=frame){if(!m.scene.request_again(frame,0,error))return false;m.frame=frame;}
        if(always){if(!m.scene.set_depth_compare(7))return false;}else m.scene.clear_depth_compare();
        if(!m.scene.set_root_world(world,error))return false;
        m.scene.draw(render,camera);return true;
    }
    bool hud_widget(RenderEngine& render,const ForceListWidget& w,const Mat4& parent,f32 frame,const Camera& camera,std::string* error,bool billboard=false){
        if(w.record<0)return true;
        auto world=mat4_multiply(parent,mat4_translation(w.position.data()));
        if(billboard)world=facing({world.m[3],world.m[7],world.m[11]});
        const f32 scale[3]={w.scale,w.scale,w.scale};return hud_model(render,u16(w.record),frame,mat4_multiply(world,mat4_scale(scale)),camera,error,billboard);
    }
    bool draw_retail(RenderEngine& render,const ForceInventory& owner,unsigned force,s16 held,s32 limit,u32 tick,ForceCostMode cost_mode,std::string* error){
        if(!owner.ready()||force>=kForceCount||held<-1||held>=s16(kBoxBorgs)||(held>=0&&!owner.state()->box[unsigned(held)].value.present())||(cost_mode!=ForceCostMode::Full&&cost_mode!=ForceCostMode::TwoThirds))return false;
        //801BA86C selects policy6 ->8008D35C: separate object82 groups1..4,
        // each rendered through the common passes then8002A888 depth-only clear.
        for(unsigned layer=0;layer<4;++layer){
            for(u8 pass:{u8(1),u8(4),u8(2)}){if(!render.begin_model_pass(pass))return false;const bool ok=draw_retail_world(render,owner,force,held,limit,tick,layer,cost_mode,error);if(!ok)render.reject_model_pass();const bool end=render.end_model_pass();if(!ok||!end)return false;}
            if(!draw_depth_clear(render,camera_.viewport_width,camera_.viewport_height))return false;
        }
        // Source front-facing mode tab and button bar use camera1.
        for(u8 pass:{u8(1),u8(4),u8(2)}){if(!render.begin_model_pass(pass))return false;bool ok=true;
            //801C16CC/801C1770: original full-width legend backing, same camera1
            //layer as the controls, in front of the lower wheel edge.
            if(pass==1)ok=hud_widget(render,hud_.footer_background[0],Mat4{},0,list_art_.camera,error);
            auto tab=list_art_.tab_origin;if(motion_ready_)tab[1]+=motion_.tab_shift();const auto root=mat4_translation(tab.data());
            //801C52DC: wide background0 omitted for EDIT state0; arrows blink72.
            for(unsigned i=1;i<list_art_.tab.size()&&ok;++i){if(i>=5&&(tick+1)%72>=49)continue;auto w=list_art_.tab[i];if(i==2)w.record=list_art_.tab_labels[0];auto parent=root;if(i==6){parent=mat4_multiply(root,mat4_multiply(mat4_translation(w.position.data()),mat4_rotation_z(3.14159265f)));w.position={};}ok=hud_widget(render,w,parent,0,list_art_.camera,error);}
            for(unsigned f=0;f<list_art_.footer.size()&&ok;++f){const auto& foot=list_art_.footer[f];if(foot.domain!=3)continue;
                for(unsigned i=0;i<4&&ok;++i){auto id=foot.records[i];if(id<0)continue;if(i==0&&((tick+1)&16))++id;if(f==4&&i==2&&held>=0)++id;auto position=foot.position;for(unsigned k=0;k<3;++k)position[k]+=list_art_.footer_offsets[i][k];ok=hud_model(render,u16(id),0,mat4_translation(position.data()),list_art_.camera,error);}
            }
            if(!ok)render.reject_model_pass();
            const bool end=render.end_model_pass();if(!ok||!end)return false;
        }
        return true;
    }
    bool draw_retail_world(RenderEngine& render,const ForceInventory& owner,unsigned force,s16 held,s32 limit,u32 tick,unsigned layer,ForceCostMode cost_mode,std::string* error){
        const auto& state=*owner.state();const auto& force_state=state.forces[force];const auto hovered=hit();
        const f32 grid_p[3]={scroll_[0],0,scroll_[1]},wheel_p[3]={art_.wheel[0]+wheel_shift(),art_.wheel[1],art_.wheel[2]};const auto grid=mat4_translation(grid_p),wheel=mat4_translation(wheel_p);
        if(layer==0){for(unsigned i=0;i<hud_.grid.size();++i){const auto& w=hud_.grid[i];const auto pass=render.model_pass();if((pass==1&&!(w.pass&1))||(pass==4&&!(w.pass&2))||(pass==2&&!(w.pass&0x7c)))continue;const f32 mask[3]={0,0,hud_.grid_mask_z};if(!model(render,deck_,deck_models_,u16(w.record),i==1?mat4_translation(mask):grid,1,error))return false;}return true;}
        if(layer==2){
        if(!model(render,deck_,deck_models_,u16(hud_.header[0].record),wheel,1,error))return false;
        s32 prefix=-1;if(limit>=0&&!owner.budget_prefix(force,limit,cost_mode,&prefix))return false;
        if(band_ready_&&limit>=0&&render.model_pass()==1){const auto centre=mat4_multiply(wheel,mat4_translation(band_art_.origin.data()));
            if(motion_ready_){std::vector<ForceWheelBandPose> parts;if(!motion_.band(band_art_,prefix,&parts))return false;const auto phase=std::bit_cast<s16>(u16((tick+1)*band_art_.pulse_step));const auto pulse=std::clamp(std::fma(std::sin(phase*band_art_.angle_unit),.5f,.5f),0.f,1.f);for(const auto& part:parts){const auto world=mat4_multiply(centre,mat4_rotation_y(part.angle*band_art_.angle_unit));if(!band_scene_.sample_fractional(part.frame,pulse,world,error))return false;band_scene_.draw(render,camera_);}}
            else{std::vector<ForceBudgetBandSample> parts;if(!force_budget_band_samples(band_art_,prefix,turn_,tick,&parts))return false;for(const auto& part:parts){const auto world=mat4_multiply(centre,mat4_rotation_y(part.angle*band_art_.angle_unit));if(!band_scene_.sample(part.frame,part.material,world,error))return false;band_scene_.draw(render,camera_);}}
        }
        const auto force_digits=std::to_string(force+1);
        for(unsigned i=0;i<force_digits.size();++i){auto w=hud_.header[i+1];if(force_digits.size()==1)w.position[0]+=hud_.header_one;if(!hud_widget(render,w,wheel,f32(force_digits[i]-'0'),camera_,error))return false;}
        const auto zero=std::find(force_state.name.begin(),force_state.name.end(),u8(0));std::string name(force_state.name.begin(),zero);
        if(!name.empty()&&std::all_of(name.begin(),name.end(),[](unsigned char c){return c<128;})){std::string encoded;if(!hud_text_.font().encode_ascii(name,&encoded))return false;name=std::move(encoded);}
        const auto text_world=mat4_multiply(wheel,mat4_translation(hud_.name_origin.data()));
        if(!hud_text_.draw(render,camera_,text_world,name,hud_.name_extent,hud_.name_advance,{255,255,255,255},true,hud_.name_colours[1],hud_.name_colours[0]))return false;
        //801C28C4 resets the child basis to translation AFTER polar placement.
        for(unsigned slot=0;slot<kForceSlots;++slot){const auto p=member_position(slot);const auto base=mat4_translation(p.data());
            if(force_budget_marker_visible(prefix,slot)&&render.model_pass()==1){ForceWheelPose marker;if(motion_ready_){if(!motion_.marker(art_,slot,&marker))return false;}else{ForceBudgetMarkerPose fixed;if(!force_budget_marker_pose(art_,slot,turn_,&fixed))return false;marker={fixed.offset,fixed.angle};}const f32 position[3]={art_.member_base+marker.offset,0,0};const auto world=mat4_multiply(wheel,mat4_multiply(mat4_translation(position),mat4_rotation_y(marker.angle*hud_.angle_unit)));if(!hud_model(render,u16(hud_.blade[2].record),0,world,camera_,error))return false;}
            if(!hud_widget(render,hud_.blade[0],base,hovered.member==s8(slot)?10.f:0.f,camera_,error))return false;
            if(hovered.member==s8(slot)&&!hud_widget(render,hud_.blade[1],base,f32((tick+1)%hud_.period),camera_,error))return false;
            const auto member=force_state.slots[slot];if(member>=0){f32 position[3];mat4_transform_point(base,hud_.blade[3].position.data(),position);if(!borg(render,state.box[unsigned(member)].value,{position[0],position[1],position[2]},hud_.blade[3].scale,error,true))return false;}
            if(!hud_widget(render,hud_.blade[4],base,0,camera_,error,true))return false;
            const auto digits=std::to_string(slot+1);for(unsigned i=0;i<digits.size();++i){auto w=hud_.blade[5+i];if(digits.size()==1)w.position[0]+=hud_.blade_one;if(!hud_widget(render,w,base,f32(digits[i]-'0'),camera_,error,true))return false;}
        }
        return true;}
        if(layer==1){
        for(unsigned i=0;i<kBoxBorgs;++i){const auto& item=state.box[i];if(!item.value.present()||s16(i)==held)continue;const auto p=box_position(i);if(std::abs(p[0])>300)continue;
            if(!(item.memberships&(1u<<force))&&!borg(render,item.value,p,art_.icon_scale,error,true))return false;
            if(item.value.fresh){auto pos=hud_.new_origin;pos[0]+=scroll_[0]+art_.cell*(i%5+(i/40)*5);pos[2]+=scroll_[1]+art_.cell*((i%40)/5);if(!hud_model(render,hud_.fresh,f32((tick+1)%hud_.period),mat4_translation(pos.data()),camera_,error))return false;}
        }
        for(unsigned slot=0;slot<kForceSlots;++slot){const auto item=force_state.slots[slot];if(item<0)continue;auto p=hud_.membership_origin;
            p[0]+=scroll_[0]+art_.cell*(unsigned(item)%5+(unsigned(item)/40)*5);p[2]+=scroll_[1]+art_.cell*((unsigned(item)%40)/5);const auto parent=mat4_translation(p.data());
            if(!hud_widget(render,hud_.membership[0],parent,0,camera_,error))return false;
            const auto digits=std::to_string(slot+1);
            for(unsigned i=0;i<digits.size();++i){auto w=hud_.membership[i+1];if(digits.size()==1)w.position[0]+=hud_.membership_one;if(!hud_widget(render,w,parent,f32(digits[i]-'0'),camera_,error))return false;}
        }
        if(hovered.box>=0){const auto slot=unsigned(hovered.box);auto p=hud_.highlight_origin;p[0]+=scroll_[0]+art_.cell*(slot%5+(slot/40)*5);p[2]+=scroll_[1]+art_.cell*((slot%40)/5);if(!hud_model(render,hud_.highlight,f32((tick+1)%hud_.period),mat4_translation(p.data()),camera_,error))return false;}
        return true;}
        const auto signed_phase=std::bit_cast<s16>(u16((tick+1)*hud_.pulse_step));const auto pulse=std::clamp(std::fma(std::sin(signed_phase*hud_.angle_unit),.5f,.5f),0.f,1.f);const std::array<f32,3> hand={cursor_[0],hud_.hand_y+(held>=0?hud_.held_lift:0),cursor_[1]};
        if(!mouse_hand_&&!hud_model(render,held>=0?hud_.held_hand:hud_.hand,pulse,mat4_translation(hand.data()),camera_,error))return false;
        //Preserve the authored camera-facing/scaled hand-to-art grip in both
        //input modes; only the mouse hand anchor moves to the pointer.
        if(held>=0){auto world=mat4_multiply(mat4_multiply(facing(mouse_hand_?mouse_hand_position_:hand),mat4_scale(std::array<f32,3>{hud_.held_scale,hud_.held_scale,hud_.held_scale}.data())),mat4_translation(hud_.held_offset.data()));const auto id=art_.icon(u16(state.box[unsigned(held)].value.model));if(id<0||!model(render,icons_,icon_models_,u16(id),world,4096,error,false,true))return false;}
        Camera::Basis basis;if(!camera_.basis(&basis))return false;const auto inverse_view=facing({basis.eye[0],basis.eye[1],basis.eye[2]});
        s32 total=0;if(!owner.total_cost(force,cost_mode,&total))return false;
        for(unsigned kind=0;kind<(limit<0?1u:3u);++kind){auto origin=hud_.cost_origin[kind];if(limit<0)origin[1]-=hud_.unlimited_y;origin[0]+=motion_ready_?motion_.cost_shift():(folded_?hud_.fold_cost_x:0.f);const auto parent=mat4_multiply(inverse_view,mat4_translation(origin.data()));const s64 value=kind==0?s64(total):kind==1?s64(limit):s64(limit)-total;const bool alert=kind==0?limit>=0&&total>limit:kind==2&&value<0;
            for(unsigned i=0;i<2;++i)if(!hud_widget(render,hud_.cost[kind][i],parent,(i==1||kind==2)&&alert?1.f:0.f,camera_,error))return false;
            const auto digits=std::to_string(std::min<s64>(value<0?-value:value,999999));for(unsigned i=0;i<digits.size();++i)if(!hud_widget(render,hud_.cost[kind][i+2],parent,f32(digits[digits.size()-1-i]-'0'),camera_,error))return false;
        }
        if(tooltip_ready_){if(!tooltip_.bound(owner))return false;if(const auto* tip=tooltip_.visible(owner)){
            const auto parent=mat4_multiply(inverse_view,mat4_translation(tooltip_art_.corners[tip->corner].data()));
            for(unsigned i=0;i<2;++i){auto w=tooltip_art_.widgets[i];if(i==1)w.record=tooltip_art_.variants[tip->variant];if(!hud_widget(render,w,parent,0,camera_,error))return false;}
            const auto text=mat4_multiply(parent,mat4_translation(tooltip_art_.text_origin.data()));if(!hud_text_.draw(render,camera_,text,tip->name,tooltip_art_.extent,tooltip_art_.advance,{255,255,255,255},false,tooltip_art_.colours[1],tooltip_art_.colours[0]))return false;
            const std::array<s32,2> values={tip->cost,tip->level};for(unsigned field=0;field<2;++field){if(values[field]<0)continue;const auto digits=std::to_string(std::min(values[field],s32(999999)));for(unsigned i=0;i<digits.size();++i){auto position=tooltip_art_.numbers[field];position[0]+=(tooltip_art_.digits-1-i)*tooltip_art_.digit_advance;const f32 scale[3]={tooltip_art_.digit_scale,tooltip_art_.digit_scale,1};const auto world=mat4_multiply(parent,mat4_multiply(mat4_translation(position.data()),mat4_scale(scale)));if(!hud_model(render,tooltip_art_.digit_record,f32(digits[digits.size()-1-i]-'0'),world,camera_,error))return false;}}
        }}
        return true;
    }
    bool model(RenderEngine& render,const HsdRecordAnimationBank& bank,std::map<u16,AnimatedModelScene>& cache,u16 id,const Mat4& world,u32 texture_base,std::string* error,bool always=false,bool compact=true){
        return draw_static_record(render,bank,cache,id,world,camera_,texture_base,error,always,compact);
    }
    bool borg(RenderEngine& render,const ForceBorg& value,const std::array<f32,3>& p,f32 scale,std::string* error,bool always=false){
        const auto id=art_.icon(u16(value.model));if(id<0)return false;
        Camera::Basis basis;if(!camera_.basis(&basis))return false;Mat4 world;
        for(unsigned i=0;i<3;++i){world.m[i*4]=basis.right[i]*scale;world.m[i*4+1]=basis.up[i]*scale;world.m[i*4+2]=-basis.forward[i]*scale;world.m[i*4+3]=p[i];}
        return model(render,icons_,icon_models_,u16(id),world,4096,error,always,true);
    }
    ForceEditorArt art_;Camera camera_;std::array<f32,2> scroll_{},cursor_{};unsigned turn_=0;bool folded_=false;
    HsdRecordAnimationBank deck_,icons_;std::map<u16,AnimatedModelScene> deck_models_,icon_models_;
    bool hud_ready_=false;ForceGridHudArt hud_;ForceListArt list_art_;SourceTextRenderer hud_text_;std::map<u16,HudModel> hud_models_;
    bool band_ready_=false;ForceBudgetBandArt band_art_;AnimatedShapeModelScene band_scene_;
    bool tooltip_ready_=false;ForceTooltipArt tooltip_art_;ForceTooltip tooltip_;
    bool motion_ready_=false;ForceWheelMotionProfile motion_profile_;ForceWheelMotion motion_;
    bool mouse_hand_=false;std::array<f32,3> mouse_hand_position_{};
};
} // namespace gf
