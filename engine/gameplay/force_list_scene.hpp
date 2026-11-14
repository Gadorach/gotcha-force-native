#pragma once
#include <map>
#include <type_traits>
#include "engine/assets/force_list_art.hpp"
#include "engine/gameplay/force_editor_list.hpp"
#include "engine/gameplay/animated_model_scene.hpp"
#include "engine/graphics/source_text.hpp"
#include "engine/gameplay/warehouse_list_view.hpp"
#include "engine/gameplay/force_list_title.hpp"
#include "engine/gameplay/editor_list_mouse.hpp"
namespace gf {
// Retail list geometry/materials, source positions and encoded font, through the
// shared renderer. No screenshot textures, fitted rectangles or parallel engine.
class ForceListScene {
public:
    void enable_mouse(bool enabled){mouse_enabled_=enabled;mouse_geometry_.begin();}
    bool mouse_hit(f32 x,f32 y,EditorListMouseTarget* out)const{return mouse_enabled_&&mouse_geometry_.hit(x,y,out);}
    const EditorListMouseGeometry& mouse_geometry()const{return mouse_geometry_;}
    const HsdRecordAnimationBank& model_bank()const{return bank_;}
    const Camera& camera()const{return art_.camera;}
    bool load(ByteView deck,const ForceListArt& art,const SourceFont& font,RenderEngine& render,std::string* error){
        ready_=mouse_enabled_=false;mouse_geometry_.begin();title_record_=-1;models_.clear();art_=art;
        if(!font.ready()||!bank_.load(deck,error,true)||bank_.hash()!=art.bank_hash||!text_.load(font,render,6000)){if(error&&error->empty())*error="force list scene: bank/font binding";return false;}
        ready_=true;return true;
    }
    bool draw(RenderEngine& render,const ForceInventory& owner,const ForceEditorList& list,const ForceSupplement& parts,unsigned force,u32 tick,std::string* error){
        return draw_list(render,owner,list,parts,force,tick,error);
    }
    bool draw_tools(RenderEngine& render,const ForceInventory& owner,const WarehouseTools& tools,const ForceSupplement& parts,unsigned force,u32 tick,std::string* error){
        WarehouseListView view;if(!WarehouseListView::build(owner,tools,&view,error))return false;
        return draw_list(render,owner,view,parts,force,tick,error);
    }
private:
    template<class List>bool draw_list(RenderEngine& render,const ForceInventory& owner,const List& list,const ForceSupplement& parts,unsigned force,u32 tick,std::string* error){
        if(mouse_enabled_)mouse_geometry_.begin();
        for(u8 pass:{u8(1),u8(4),u8(2)}){
            if(!render.begin_model_pass(pass))return false;
            const bool accepted=draw_pass(render,owner,list,parts,force,tick,error);if(!accepted)render.reject_model_pass();
            const bool ended=render.end_model_pass();if(!accepted||!ended)return false;
        }
        return !mouse_enabled_||mouse_geometry_.finish();
    }
    template<class List>bool draw_pass(RenderEngine& render,const ForceInventory& owner,const List& list,const ForceSupplement& parts,unsigned force,u32 tick,std::string* error){
        if(!ready_||!list.current(owner)||!parts.for_catalog(*owner.catalog())||force>=20)return false;
        const unsigned domain=unsigned(list.view().domain);const auto mode=f32(domain);
        // Background and column labels.
        for(unsigned i=0;i<art_.body.size();++i){auto w=art_.body[i];if(i==2&&domain==2)++w.record;if(i==3){if constexpr(std::is_same_v<List,WarehouseListView>){if(list.plain_fields())continue;}w.record=art_.columns[list.display_flags()];}if(!widget(render,w,mode,error))return false;}
        if(domain==0){for(unsigned i=0;i<art_.header.size();++i){auto w=art_.header[i];if(i==2)w.record=s16(w.record+s16(domain));if(!widget(render,w,i<2?mode:0,error,i>2?-f32(tick%128)*.0490873852f:0))return false;}}
        else for(unsigned i=0;i<art_.tab.size();++i){auto w=art_.tab[i];if(i==2)w.record=art_.tab_labels[domain];for(unsigned k=0;k<3;++k)w.position[k]+=art_.tab_origin[k];
            if constexpr(std::is_same_v<List,WarehouseListView>){if(i==2){if(!tool_title(render,w,list.title(),error))return false;continue;}}
            if(mouse_enabled_&&i>=3)mouse_target_=EditorListMouseTarget{-1,(i==3||i==5)?0x40u:0x20u,0};
            if(!widget(render,w,i<3?mode:0,error,i==6?3.14159265f:0))return false;
            mouse_target_.reset();}
        for(unsigned row=0;row<list.visible_rows();++row){auto w=art_.row[0];w.position[1]+=art_.row_step*row;w.scale*=list.first_row()+row==list.cursor()?art_.selected_scale:1;
            if(mouse_enabled_&&list.first_row()+row<list.view().count)mouse_target_=EditorListMouseTarget{s16(list.first_row()+row),0x100,0};
            if(!widget(render,w,mode,error))return false;
            mouse_target_.reset();}
        if(list.view().count){auto p=art_.highlight;p[1]+=art_.row_step*(list.cursor()-list.first_row());if(!model(render,85,0,p,1,error))return false;}
        for(unsigned row=0;row<list.visible_rows();++row){
            const unsigned index=list.first_row()+row;const bool selected=index==list.cursor();const f32 scale=selected?art_.selected_scale:1;const f32 dy=art_.row_step*row;
            if(index>=list.view().count)continue;
            ForceListRowValue value;if(!read_row(owner,list,parts,force,index,&value))return false;
            const auto& borg=value.borg;const bool fresh=value.fresh,member=value.member;std::string name=value.name;
            if(value.ascii&&!text_.font().encode_ascii(value.name,&name))return false;
            const auto* metadata=value.has_borg?owner.catalog()->find(u16(borg.model)):nullptr;if(value.has_borg&&!metadata)return false;
            for(unsigned item=1;item<6;++item){auto w=art_.row[item];bool show=true;float frame=0;
                if(item==1)frame=selected?4:mode;
                else if(!value.has_borg)continue;
                else if(item==2)show=fresh;
                else if(item==3){w.record=art_.variants[borg.variant];if(domain==2)w.position[0]=art_.part_variant_x;}
                else if(item==4){const auto rarity=metadata->rarity[borg.variant];if(rarity<0||rarity>=s16(art_.rarity.size()))return false;w.record=art_.rarity[unsigned(rarity)];show=list.display_flags()!=0;if(domain==2)w.position[0]=art_.part_rarity_x;}
                else {if(metadata->tribe>=art_.tribes.size())return false;w.record=art_.tribes[metadata->tribe];frame=selected?4:mode;show=list.display_flags()==0;}
                if(!show)continue;
                w.position[1]+=dy;if(selected)w.position[2]+=art_.selected_z;w.scale*=scale;if(!widget(render,w,frame,error))return false;
            }
            auto point=art_.name_origin;point[1]+=dy;if(selected)point[2]+=art_.selected_z;const unsigned colour=member?4:selected?3:2-domain; // source slots39/38/37/40/41 -> stored37..42
            if(!font(render,point,name,scale,art_.advance,colour,error,value.disabled))return false;
            if(!number(render,art_.numbers[0],index+1,domain,dy,scale,selected,error))return false;
            if(!value.value_ascii.empty()){std::string encoded;if(!text_.font().encode_ascii(value.value_ascii,&encoded))return false;auto position=art_.numbers[1].position;position[1]+=dy;if(selected)position[2]+=art_.selected_z;if(!font(render,position,encoded,scale,art_.advance,colour,error))return false;}
            if(domain!=2&&list.display_flags()==1){if(!number(render,art_.numbers[1],u32(force_borg_cost(*owner.catalog(),borg)),domain,dy,scale,selected,error)||!number(render,art_.numbers[2],u32(borg.level)+1,domain,dy,scale,selected,error))return false;}
        }
        unsigned filter=0;if(list.tribes()!=~u32(0))for(unsigned i=0;i<20;++i)if(list.tribes()&(1u<<i)){filter=i+1;break;}
        std::string count=std::to_string(list.view().count);count.insert(count.begin(),4-count.size(),' ');
        for(unsigned i=0;i<6;++i){auto w=art_.summary[i];float frame=mode;if(i==0){if constexpr(std::is_same_v<List,WarehouseListView>){if(list.plain_fields())continue;}w.record=art_.filters[filter];}else if(i>=2){if(count[i-2]==' ')continue;frame=mode*10+count[i-2]-'0';}if(!widget(render,w,frame,error))return false;}
        for(unsigned i=0;i<2;++i){const bool visible=i==0?list.first_row()>0:list.first_row()+list.visible_rows()<list.view().count;if(visible){if(mouse_enabled_)mouse_target_=EditorListMouseTarget{-1,0,i?2u:1u};if(!widget(render,art_.arrows[i],0,error,i?3.14159265f:0))return false;mouse_target_.reset();}}
        // C1 source footer instances remain active over the full-screen lists.
        if constexpr(std::is_same_v<List,WarehouseListView>){if(!tool_footer(render,list.confirm_footer(),tick,error,domain==1,list.stored_footer()))return false;}
        else for(unsigned i=0;i<art_.footer.size();++i)if(art_.footer[i].domain==domain){const u32 action=(i==7||i==12||i==17)?0x800u:(i==8||i==13||i==18)?0x100u:(i==9||i==14||i==19)?0x400u:(i==11||i==16)?0x80u:0u;if(!draw_footer(render,art_.footer[i],tick,error,action))return false;}
        if(domain==2||std::is_same_v<List,WarehouseListView>){
            //The Items-only plate also contains orange/brown lower chrome.
            //Warehouse keeps its original green rows/footer, with informational
            //text in the same unused lower slots; Picker retains the Items plate.
            if(domain==2&&!model(render,163,0,art_.part_origin,1,error))return false;
            if constexpr(std::is_same_v<List,WarehouseListView>){for(unsigned i=0;i<2;++i){const auto& detail=list.details()[i];if(detail.text.empty())continue;std::string text=detail.text;if(detail.ascii&&!text_.font().encode_ascii(detail.text,&text))return false;auto point=art_.part_origin;for(unsigned k=0;k<3;++k)point[k]+=art_.part_text[i][k];if(!font(render,point,text,1,art_.part_advance,domain==2?5:1,error))return false;}}
            else if(list.selected()>=0){const auto& p=owner.state()->parts[unsigned(list.selected())];const auto* metadata=owner.catalog()->find(p.model());std::string digit;if(!text_.font().encode_ascii(std::to_string(parts.required(p.model())),&digit))return false;std::array<std::string,2> lines;
                if(!format(art_.part_strings[1],digit,&lines[0])||!format(art_.part_strings[p.variant()?3:2],metadata->source_name,&lines[1]))return false;
                for(unsigned i=0;i<2;++i){auto point=art_.part_origin;for(unsigned k=0;k<3;++k)point[k]+=art_.part_text[i][k];if(!font(render,point,lines[i],1,art_.part_advance,5,error))return false;}
            }
        }
        return true;
    }
private:
    bool tool_title(RenderEngine& render,const ForceListWidget& w,const std::string& title,std::string* error){
        if(title_record_!=w.record){AnimatedModelScene reference;if(w.record<0||!reference.load_compact_textures(bank_,u16(w.record),2,0,0,error)||!reference.request(2,0,error))return false;
            const auto* asset=reference.asset();const auto* pose=reference.pose();if(!asset||!pose||asset->joints.size()!=1||asset->meshes.size()!=1||pose->materials.size()!=1||asset->meshes[0].texture>=asset->textures.size()||!force_list_title_slot(asset->meshes[0],asset->textures[asset->meshes[0].texture],pose->materials[0],&title_slot_)){if(error)*error="force list title: original word coverage refused";return false;}title_record_=w.record;}
        std::string encoded;std::array<f32,3> position{};f32 scale=0;const f32 size[3]={w.scale,w.scale,1};const auto world=mat4_multiply(mat4_translation(w.position.data()),mat4_scale(size));
        if(!text_.font().encode_ascii(title,&encoded)||!force_list_title_text(text_.font(),encoded,art_.extent,art_.advance,title_slot_,world,&position,&scale)){if(error)*error="force list title: text/slot layout refused";return false;}return font(render,position,encoded,scale,art_.advance,0,error);
    }
    static bool read_row(const ForceInventory& owner,const ForceEditorList& list,const ForceSupplement& parts,unsigned force,unsigned index,ForceListRowValue* out){
        if(!out||index>=list.view().count)return false;
        ForceListRowValue row;const unsigned domain=unsigned(list.view().domain),slot=unsigned(list.view().slots[index]);row.has_borg=true;
        if(domain==2){const auto& part=owner.state()->parts[slot];row.borg.model=s16(part.model());row.borg.variant=part.variant();row.fresh=part.fresh;if(!parts.part_name(*owner.catalog(),part.model(),part.variant(),part.component(),&row.name))return false;}
        else{row.borg=domain==0?owner.state()->box[slot].value:owner.state()->warehouse[slot];row.fresh=row.borg.fresh;if(domain==0)row.member=(owner.state()->box[slot].memberships&(1u<<force))!=0;const auto* meta=owner.catalog()->find(u16(row.borg.model));if(!meta)return false;row.name=meta->source_name;}
        *out=std::move(row);return true;
    }
    static bool read_row(const ForceInventory&,const WarehouseListView& list,const ForceSupplement&,unsigned,unsigned index,ForceListRowValue* out){const auto* row=list.row(index);if(!out||!row)return false;*out=*row;return true;}
    bool draw_footer(RenderEngine& render,const ForceListFooter& footer,u32 tick,std::string* error,u32 action=0){for(unsigned i=0;i<4;++i){const auto id=footer.records[i];if(id<0)continue;auto p=footer.position;for(unsigned k=0;k<3;++k)p[k]+=art_.footer_offsets[i][k];const auto blink=i==0&&((tick+1)&16)?1:0;if(mouse_enabled_&&action)mouse_target_=EditorListMouseTarget{-1,action,0};if(!model(render,u16(id+blink),0,p,1,error))return false;mouse_target_.reset();}return true;}
    bool tool_footer(RenderEngine& render,bool confirm,u32 tick,std::string* error,bool warehouse=false,bool stored=false){
        if(warehouse){
            //Original green Warehouse choose/sort/confirm/spec graphics. The
            //unused Start/quit slot holds native B-back; groups have no X slot.
            if(!draw_footer(render,art_.footer[10],tick,error)||!draw_footer(render,art_.footer[12],tick,error,0x800)||(confirm&&!draw_footer(render,art_.footer[13],tick,error,0x100))||(stored&&!draw_footer(render,art_.footer[14],tick,error,0x400)))return false;
            //No Start icon is drawn here: begin at its full source slot, without
            //the word offset that would reserve an icon and crowd the Y graphic.
            auto position=art_.footer[11].position;position[0]+=art_.extent*.5f;std::string text;if(mouse_enabled_)mouse_target_=EditorListMouseTarget{-1,0x200,0};const bool ok=text_.font().encode_ascii("B BACK",&text)&&font(render,position,text,1,art_.advance,1,error);mouse_target_.reset();return ok;
        }
        //Source Items choose/confirm plus encoded B-back text in an existing
        //Items footer slot. The source Editor's TURN control is NOT B-back.
        //Do not advertise unsupported
        //Start-quit, Y-sort or X-spec actions on the addon controller pages.
        if(art_.footer[15].domain!=2||art_.footer[18].domain!=2||art_.footer[19].domain!=2)return false;
        if(!draw_footer(render,art_.footer[15],tick,error))return false;
        if(confirm){auto row=art_.footer[18];row.position=art_.footer[17].position;if(!draw_footer(render,row,tick,error,0x100))return false;}
        auto position=art_.footer[19].position;for(unsigned k=0;k<3;++k)position[k]+=art_.footer_offsets[2][k];position[0]+=art_.extent*.5f;
        std::string text;if(!text_.font().encode_ascii("B BACK",&text))return false;if(mouse_enabled_)mouse_target_=EditorListMouseTarget{-1,0x200,0};const bool ok=font(render,position,text,1,art_.advance,2,error);mouse_target_.reset();return ok;
    }
    struct Model {AnimatedModelScene scene;f32 frame=0;};
    bool model(RenderEngine& render,u16 id,f32 frame,const std::array<f32,3>& position,f32 scale,std::string* error,f32 rotation=0){
        auto found=models_.find(id);if(found==models_.end()){Model entry;if(!entry.scene.load_compact_textures(bank_,id,2,0,0,error)||!entry.scene.request(frame,0,error)){if(error)*error="force list record "+std::to_string(id)+": "+*error;return false;}entry.frame=frame;entry.scene.set_enabled(true);entry.scene.set_untextured_billboards(true);entry.scene.upload_textures_shared(render,1);found=models_.emplace(id,std::move(entry)).first;}
        auto& entry=found->second;if(frame!=entry.frame){if(!entry.scene.request_again(frame,0,error))return false;entry.frame=frame;}
        const f32 size[3]={scale,scale,1};const auto world=mat4_multiply(mat4_translation(position.data()),mat4_multiply(mat4_rotation_z(rotation),mat4_scale(size)));
        if(!entry.scene.set_root_world(world,error))return false;
        PointerTriangles triangles;entry.scene.draw(render,art_.camera,mouse_enabled_&&mouse_target_?&triangles:nullptr);return !mouse_enabled_||!mouse_target_||mouse_geometry_.add(std::move(triangles),*mouse_target_);
    }
    bool widget(RenderEngine& render,const ForceListWidget& w,f32 frame,std::string* error,f32 rotation=0){return w.record<0||model(render,u16(w.record),frame,w.position,w.scale,error,rotation);}
    bool font(RenderEngine& render,const std::array<f32,3>& position,const std::string& text,f32 scale,f32 advance,unsigned style,std::string* error,bool disabled=false){
        const f32 size[3]={scale,scale,scale};const auto world=mat4_multiply(mat4_translation(position.data()),mat4_scale(size));const auto colours=art_.font_colours[style];
        const u8 intensity=disabled?128:255;
        PointerTriangles triangles;if(!text_.draw(render,art_.camera,world,text,art_.extent,advance,{intensity,intensity,intensity,255},false,colours[1],colours[0],mouse_enabled_&&mouse_target_?&triangles:nullptr)){if(error)*error="force list: encoded font draw refused";return false;}return !mouse_enabled_||!mouse_target_||mouse_geometry_.add(std::move(triangles),*mouse_target_);
    }
    bool number(RenderEngine& render,const ForceListNumber& layout,u32 value,unsigned domain,f32 row_y,f32 scale,bool selected,std::string* error){
        std::string digits=std::to_string(value);if(digits.size()>layout.digits)return false;
        if(!(layout.flags&0xf0))digits.insert(digits.begin(),layout.digits-digits.size(),(layout.flags&15)?'0':' ');
        for(unsigned i=0;i<digits.size();++i){if(digits[i]==' ')continue;auto p=layout.position;p[1]+=row_y;if(selected)p[2]+=art_.selected_z;const f32 local=(layout.flags&0xf0)?(f32(2+2*i)-f32(digits.size()))*art_.digit_advance*.5f:i*art_.digit_advance;p[0]+=local;if(!model(render,u16(layout.record),f32(domain*10+unsigned(digits[i]-'0')),p,scale,error))return false;}return true;
    }
    static bool format(const std::string& pattern,const std::string& argument,std::string* out){const auto at=pattern.find("%s");if(at==std::string::npos||pattern.find("%s",at+2)!=std::string::npos||pattern.size()+argument.size()>1024)return false;*out=pattern.substr(0,at)+argument+pattern.substr(at+2);return true;}
    bool ready_=false;s16 title_record_=-1;ForceListTitleSlot title_slot_;ForceListArt art_;HsdRecordAnimationBank bank_;SourceTextRenderer text_;std::map<u16,Model> models_;
    bool mouse_enabled_=false;EditorListMouseGeometry mouse_geometry_;std::optional<EditorListMouseTarget> mouse_target_;
};
} // namespace gf
