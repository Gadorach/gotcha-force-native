#pragma once
#include "engine/gameplay/borg_installed_browser.hpp"
#include "engine/graphics/scene_draw.hpp"
#include "engine/graphics/text.hpp"
#include "engine/graphics/transform.hpp"

namespace gf {
enum class BorgBrowserTileStatus {Unavailable,Placeholder,Unresolved,Image};
struct BorgBrowserTile {
    BorgBrowserTileStatus status=BorgBrowserTileStatus::Unavailable;
    u32 texture=0,width=0,height=0;
};
struct BorgPlacementPanel {
    unsigned force=0,slot=0;
    std::string_view status;
    const OwnedPackageBorg* offered=nullptr;
    u16 declared_cost=0;
    bool confirm_selected=false,locked=false,headless=false;
};
struct BorgBrowserPracticePanel {std::string_view status;};

class BorgInstalledBrowserView {
public:
    unsigned draw(RenderEngine& render,s32 width,s32 height,const BorgInstalledBrowser& browser,
                  const BorgBrowserTile& tile,bool confirmed,bool preview_unavailable=false,std::string_view save_name={},
                  const BorgPlacementPanel* placement=nullptr,const BorgBrowserPracticePanel* practice=nullptr){
        if(width<440||height<320)return 0;
        const auto saved=render.state();
        render.state()={};render.state().depth_test=render.state().depth_write=false;
        render.state().cull_backface=false;render.state().blend=true;render.state().blend_src=5;render.state().blend_dst=6;
        const Mat4 identity;render.upload_matrix(0,identity.m);
        TextRenderer::Style title;title.scale=2;TextRenderer::Style small;small.scale=1;
        TextRenderer::Style body=small;body.r=200;body.g=224;body.b=242;
        unsigned draws=0;
        auto fill=[&](s32 x,s32 y,s32 w,s32 h,Tint tint,u32 texture=0){
            quads_.clear();render.state().texture=texture;
            ScreenDraw::textured_quad(quads_,f32(x),f32(y),f32(x+w),f32(y+h),0,0,0,1,1,tint);
            const auto first=render.append_vertices(quads_.data(),quads_.size());
            draws+=render.draw(Primitive::Triangles,first,u32(quads_.size()))?1:0;
            render.state().texture=0;
        };
        auto text=[&](std::string_view value,s32 x,s32 y,s32 space,const TextRenderer::Style& style){
            auto label=printable(value);
            if(label.empty())return;
            label.resize(std::min(label.size(),std::size_t(std::max(0,space)/(style.scale*6))));
            draws+=text_.draw(render,label,x,y,style);
        };
        const auto* owned=browser.owned();
        text(placement?"PLACE OWNED BORG":owned?"OWNED BORG COLLECTION":"INSTALLED BORG PICKER",16,12,width-32,title);
        const auto saved_label=printable(save_name);
        if(placement){
            const auto target="FORCE "+std::to_string(placement->force+1)+" - SLOT "+std::to_string(placement->slot+1);
            text(target,16,40,width-32,body);
            text(saved_label.empty()?"SELECTED SAVE":"SAVE: "+saved_label,16,54,width-32,small);
            if(placement->offered){
                const auto& borg=*placement->offered;
                const auto name=printable(borg.identity.name);
                fill(12,74,width-24,height-112,{31,43,56,255});
                text("CONFIRM PLACEMENT",22,88,width-44,title);
                text(name.empty()?borg.identity.id:name,22,124,width-44,title);
                if(borg.variant){
                    const auto label=printable(borg.variant->label);
                    text(label.empty()?borg.variant->id:label+" - "+borg.variant->id,22,149,width-44,body);
                }
                text("OWNED INSTANCE "+std::to_string(borg.instance)+" - DECLARED COST "+std::to_string(placement->declared_cost),
                    22,170,width-44,small);
                text("ADD TO THIS EMPTY POSITION ONLY",22,192,width-44,small);
                const s32 first=height-115,second=height-87;
                fill(18,(placement->confirm_selected?second:first)-5,width-36,22,{35,105,130,255});
                text("CANCEL",22,first,width-44,small);
                text("CONFIRM PLACEMENT",22,second,width-44,small);
                text(placement->headless?"HEADLESS - NO SAVE WRITES":"UP/DOWN CHOOSE  SPACE/A ACCEPT  SHIFT/B CANCEL  ESC CLOSE",
                    16,height-24,width-32,small);
                render.state()=saved;
                return draws;
            }
            if(placement->locked){
                fill(12,74,width-24,height-112,{31,43,56,255});
                text(placement->status,22,96,width-44,title);
                text("REOPEN THE SAVE TO VIEW CURRENT FORCE SLOTS",22,127,width-44,body);
                text("ESC CLOSE",16,height-24,width-32,small);
                render.state()=saved;
                return draws;
            }
        }else if(owned)text(saved_label.empty()?"SELECTED SAVE - READ ONLY":"SAVE: "+saved_label+" - READ ONLY",16,40,width-32,small);
        else text("READ ONLY - NO OWNERSHIP OR BATTLE ADMISSION",16,40,width-32,small);
        const s32 split=width*45/100,right=split+16,right_space=width-right-24;
        const s32 top=placement?74:66;
        fill(12,top,split-18,height-top-39,{25,36,48,255});
        fill(split,top,width-split-12,height-top-39,{31,43,56,255});
        text(owned?"OWNED COPIES":"PACKAGES",22,80,split-35,title);
        text("VARIANT DETAILS",right,80,right_space,title);
        if(!browser.size()){
            const auto status=browser.catalog().status;
            text(owned?"NO OWNED PACKAGE BORGS":status==BorgPackageScanStatus::Unavailable||status==BorgPackageScanStatus::EntryBudget?
                "PACKAGE SCAN UNAVAILABLE":"NO PACKAGES FOUND",22,118,split-35,small);
        }else{
            const auto visible=std::size_t(std::min(12,std::max(1,(height-163)/27)));
            const auto selected=browser.package_index();
            const auto start=selected>=visible?selected-visible+1:0;
            for(std::size_t i=start;i<std::min(browser.size(),start+visible);++i){
                const s32 y=111+s32(i-start)*27;
                if(i==selected)fill(18,y-3,split-30,22,{35,105,130,255});
                const auto& id=owned?owned->owned[i].identity:browser.catalog().discovered[i].manifest.identity;
                const auto name=printable(id.name);
                const auto prefix=owned?std::to_string(owned->owned[i].instance)+" ":"";
                text(prefix+(name.empty()?id.id:name),24,y+3,split-49,small);
            }
            const auto* saved=browser.owned_entry();
            const auto& id=saved?saved->identity:browser.entry()->manifest.identity;
            const auto name=printable(id.name);
            text(name.empty()?std::string_view(id.id):std::string_view(name),right,113,right_space,title);
            text(id.id,right,135,right_space,body);
            if(saved){
                text("OWNED INSTANCE "+std::to_string(saved->instance),right,151,right_space,small);
                if(saved->variant){
                    const auto label=printable(saved->variant->label);
                    text(label.empty()?saved->variant->id:label,right,170,right_space,title);
                    text("VARIANT ID: "+saved->variant->id,right,193,right_space,small);
                }
                text(browser.owned_status(),right,209,right_space,body);
                const auto placements=std::count_if(owned->placements.begin(),owned->placements.end(),
                    [&](const auto& place){return place.instance==saved->instance;});
                text("FORCE SLOTS: "+std::to_string(placements)+(saved->disabled?" - DISABLED":""),right,226,right_space,small);
                if(const auto* variant=browser.variant()){
                    text("DECLARED COST "+std::to_string(variant->force_cost),right,242,right_space,body);
                    const s32 room=std::min(100,height-343);
                    if(room>=24&&tile.status==BorgBrowserTileStatus::Image&&tile.width&&tile.height&&tile.texture){
                        const f32 scale=f32(room)/f32(std::max(tile.width,tile.height));
                        fill(right,267,std::max(1,s32(f32(tile.width)*scale)),std::max(1,s32(f32(tile.height)*scale)),{},tile.texture);
                    }
                }
                if(practice&&!practice->status.empty())text(practice->status,right,height-60,right_space,body);
                else if(preview_unavailable)text("BODY PREVIEW UNAVAILABLE",right,height-60,right_space,small);
                else if(confirmed)text("SELECTION INSPECTED",right,height-60,right_space,body);
            }else if(const auto* variant=browser.variant()){
                const auto label=printable(variant->label);
                text(label.empty()?std::string_view(variant->id):std::string_view(label),right,166,right_space,title);
                text("VARIANT "+std::to_string(browser.variant_index()+1)+"/"+
                     std::to_string(browser.metadata()->profile.variants.size())+" - "+variant->id,right,189,right_space,small);
                text("FORCE COST "+std::to_string(variant->force_cost),right,207,right_space,body);
                const s32 room=std::min(100,height-294);
                if(room>=24&&tile.status==BorgBrowserTileStatus::Image&&tile.width&&tile.height&&tile.texture){
                    const f32 scale=f32(room)/f32(std::max(tile.width,tile.height));
                    fill(right,225,std::max(1,s32(f32(tile.width)*scale)),std::max(1,s32(f32(tile.height)*scale)),{},tile.texture);
                }else{
                    const char* label=tile.status==BorgBrowserTileStatus::Placeholder?"EDITOR ICON PLACEHOLDER":
                        tile.status==BorgBrowserTileStatus::Unresolved?"EDITOR ICON UNRESOLVED":"EDITOR ICON UNAVAILABLE";
                    text(label,right,235,right_space,small);
                }
                if(confirmed)text("SELECTION INSPECTED - NO SAVE CHANGES",right,height-62,right_space,body);
                if(preview_unavailable)text("BODY PREVIEW UNAVAILABLE",right,height-79,right_space,small);
            }else text("CATALOGUE METADATA UNAVAILABLE",right,166,right_space,small);
        }
        if(placement){
            text(placement->status.empty()?"UP/DOWN SELECT AN OWNED COPY":placement->status,16,height-32,width-32,body);
            text(placement->headless?"HEADLESS - NO SAVE WRITES  ESC CLOSE":"SPACE/A STAGE PLACEMENT  E/X BODY PREVIEW  ESC CLOSE",
                16,height-19,width-32,small);
        }else{
            text(owned?"UP/DOWN OWNED COPY  SHIFT/B RETURN FROM PREVIEW":"UP/DOWN PACKAGE  LEFT/RIGHT VARIANT",
                16,height-32,width-32,small);
            text(practice?"SPACE/A INSPECT  E/X PREVIEW  Q/Y PRACTICE  ESC CLOSE":"SPACE/A INSPECT  E/X BODY PREVIEW  ESC CLOSE",
                16,height-19,width-32,small);
        }
        render.state()=saved;
        return draws;
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
