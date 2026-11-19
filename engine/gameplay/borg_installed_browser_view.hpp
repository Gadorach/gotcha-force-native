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

class BorgInstalledBrowserView {
public:
    unsigned draw(RenderEngine& render,s32 width,s32 height,const BorgInstalledBrowser& browser,
                  const BorgBrowserTile& tile,bool confirmed){
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
        const s32 split=width*45/100,right=split+16,right_space=width-right-24;
        fill(12,66,split-18,height-105,{25,36,48,255});
        fill(split,66,width-split-12,height-105,{31,43,56,255});
        text("INSTALLED BORG PICKER",16,12,width-32,title);
        text("READ ONLY - NO OWNERSHIP OR BATTLE ADMISSION",16,40,width-32,small);
        text("PACKAGES",22,80,split-35,title);
        text("VARIANT DETAILS",right,80,right_space,title);
        const auto& entries=browser.catalog().discovered;
        if(entries.empty()){
            const auto status=browser.catalog().status;
            text(status==BorgPackageScanStatus::Unavailable||status==BorgPackageScanStatus::EntryBudget?
                "PACKAGE SCAN UNAVAILABLE":"NO PACKAGES FOUND",22,118,split-35,small);
        }else{
            const auto visible=std::size_t(std::min(12,std::max(1,(height-163)/27)));
            const auto selected=browser.package_index();
            const auto start=selected>=visible?selected-visible+1:0;
            for(std::size_t i=start;i<std::min(entries.size(),start+visible);++i){
                const s32 y=111+s32(i-start)*27;
                if(i==selected)fill(18,y-3,split-30,22,{35,105,130,255});
                const auto& id=entries[i].manifest.identity;
                const auto name=printable(id.name);
                text(name.empty()?std::string_view(id.id):std::string_view(name),24,y+3,split-49,small);
            }
            const auto& id=browser.entry()->manifest.identity;
            const auto name=printable(id.name);
            text(name.empty()?std::string_view(id.id):std::string_view(name),right,113,right_space,title);
            text(id.id,right,135,right_space,body);
            if(const auto* variant=browser.variant()){
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
            }else text("CATALOGUE METADATA UNAVAILABLE",right,166,right_space,small);
        }
        text("UP/DOWN PACKAGE  LEFT/RIGHT VARIANT  SPACE/A INSPECT  ESC CLOSE",16,height-27,width-32,small);
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
