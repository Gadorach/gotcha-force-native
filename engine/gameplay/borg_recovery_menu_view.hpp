#pragma once
#include "engine/gameplay/borg_recovery_menu.hpp"
#include "engine/graphics/scene_draw.hpp"
#include "engine/graphics/text.hpp"
#include "engine/graphics/transform.hpp"

namespace gf {
class BorgRecoveryMenuView {
public:
    unsigned draw(RenderEngine& render,s32 width,s32 height,const BorgPackageRecoverySession& session,
                  const BorgRecoveryMenu& menu,std::string_view save_name,std::string_view status,bool read_only){
        if(width<520||height<360)return 0;
        const auto saved=render.state();
        render.state()={};render.state().depth_test=render.state().depth_write=false;
        render.state().cull_backface=false;render.state().blend=true;render.state().blend_src=5;render.state().blend_dst=6;
        const Mat4 identity;render.upload_matrix(0,identity.m);
        TextRenderer::Style title;title.scale=2;TextRenderer::Style small;small.scale=1;
        TextRenderer::Style details=small;details.r=193;details.g=221;details.b=239;
        unsigned draws=0;
        auto fill=[&](s32 x,s32 y,s32 w,s32 h,Tint tint){
            quads_.clear();
            ScreenDraw::textured_quad(quads_,f32(x),f32(y),f32(x+w),f32(y+h),0,0,0,1,1,tint);
            const auto first=render.append_vertices(quads_.data(),quads_.size());
            draws+=render.draw(Primitive::Triangles,first,u32(quads_.size()))?1:0;
        };
        auto text=[&](std::string_view value,s32 x,s32 y,const TextRenderer::Style& style,std::string_view fallback={}){
            auto label=printable(value);
            if(label.empty())label=printable(fallback);
            label.resize(std::min(label.size(),std::size_t(std::max(0,width-x-18)/(style.scale*6))));
            draws+=text_.draw(render,label,x,y,style);
        };
        auto choice=[&](std::string_view label,unsigned index,s32 y,bool danger=false){
            if(menu.action_index()==index)fill(21,y-4,width-42,21,danger?Tint{120,46,55,255}:Tint{31,106,131,255});
            text(label,27,y,small);
        };
        fill(12,64,width-24,height-112,{29,42,57,255});
        text("MISSING BORG RECOVERY",16,12,title);
        text(save_name,16,40,details,"SELECTED SAVE");
        const auto phase=session.phase();
        if(phase==BorgRecoveryPhase::List){
            text("AFFECTED OWNED BORGS",22,80,title);
            const auto list=session.notices();
            if(list.empty())text("NO CONFIRMED MISSING BORGS OR VARIANTS",22,118,small);
            else{
                const auto visible=std::size_t(std::max(1,(height-181)/27));
                const auto cursor=std::min(menu.notice_index(),list.size()-1);
                const auto start=cursor>=visible?cursor-visible+1:0;
                for(std::size_t i=start;i<std::min(list.size(),start+visible);++i){
                    const auto& row=list[i];const s32 y=112+s32(i-start)*27;
                    if(i==cursor)fill(20,y-4,width-40,22,{31,106,131,255});
                    const auto name=printable(row.saved.identity.name);
                    const auto label=row.saved.variant?printable(row.saved.variant->label):std::string{};
                    text((name.empty()?row.saved.identity.id:name)+(label.empty()?"":" - "+label)+
                         (row.saved.disabled?" - DISABLED":""),27,y,small);
                }
            }
        }else if(phase==BorgRecoveryPhase::NeedsReload){
            text("REOPEN THE SELECTED SAVE",22,80,title);
            text("NO FURTHER WRITE CAN BE CONFIRMED HERE",22,118,small);
        }else if(const auto* notice=session.selected()){
            text(notice->saved.identity.name,22,80,title,notice->saved.identity.id);
            text(notice->saved.identity.id,22,105,details);
            if(notice->saved.variant)
                text(notice->saved.variant->label,22,123,small,notice->saved.variant->id);
            const char* reason=notice->reason==PackageBorgAvailability::MissingVariant?"SAVED VARIANT MISSING":
                notice->reason==PackageBorgAvailability::Conflict?"CONFLICTING PACKAGES - NO RECOVERY ACTION":
                notice->reason==PackageBorgAvailability::Incomplete?"PACKAGE FILES INCOMPLETE":"PACKAGE MISSING";
            text(reason,22,143,details);
            if(notice->affected.empty())text("NO AFFECTED FORCE SLOTS",22,162,small);
            else{
                text("AFFECTED FORCE NUMBERS",22,162,small);
                std::string line;
                s32 y=176;unsigned count=0;
                for(const auto& place:notice->affected){
                    if(!line.empty())line+=", ";
                    line+=std::to_string(unsigned(place.force)+1);
                    if(++count==10){text(line,22,y,details);y+=12;count=0;line.clear();}
                }
                if(!line.empty())text(line,22,y,details);
            }
            if(phase==BorgRecoveryPhase::Notice){
                choice("CANCEL",0,height-140);
                if(BorgRecoveryMenu::notice_actions(notice)>1){
                    choice("DISABLE IN ALL AFFECTED FORCES",1,height-117);
                    choice("DELETE FROM THIS SAVE",2,height-94,true);
                }
            }else{
                const bool deleting=phase==BorgRecoveryPhase::ConfirmDelete;
                text(deleting?"REMOVE THIS OWNED INSTANCE AND ITS FORCE SLOTS":"KEEP OWNERSHIP AND SLOTS BUT DISABLE USE",22,height-141,small);
                choice("CANCEL",0,height-117);
                choice(deleting?"CONFIRM DELETE FROM THIS SAVE":"CONFIRM DISABLE",1,height-94,deleting);
            }
        }
        if(!status.empty())text(status,22,height-64,details);
        text(read_only?"HEADLESS INSPECTION ONLY - NO SAVE WRITES":"UP/DOWN SELECT  SPACE/A ACCEPT  SHIFT/B CANCEL  ESC CLOSE",
            16,height-28,small);
        render.state()=saved;return draws;
    }
private:
    static std::string printable(std::string_view text){
        std::string label(text);
        for(char& c:label){
            if(c>='a'&&c<='z')c=char(c-'a'+'A');
            if(c=='_')c='-';
            if(!TextRenderer::covers(c))return {};
        }
        return label;
    }
    TextRenderer text_;
    std::vector<Vertex> quads_;
};
}
