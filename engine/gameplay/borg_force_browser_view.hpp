#pragma once
#include <cstdio>
#include "engine/gameplay/borg_force_browser.hpp"
#include "engine/graphics/scene_draw.hpp"
#include "engine/graphics/text.hpp"
#include "engine/graphics/transform.hpp"

namespace gf {
class BorgForceBrowserView {
public:
    unsigned draw(RenderEngine& render,s32 width,s32 height,const BorgForceBrowser& cursor,
                  const MixedBorgForceView* force,std::string_view save_name,std::optional<std::string_view> practice={}){
        if(width<520||height<360)return 0;
        const auto saved=render.state();
        render.state()={};render.state().depth_test=render.state().depth_write=false;
        render.state().cull_backface=false;render.state().blend=true;render.state().blend_src=5;render.state().blend_dst=6;
        const Mat4 identity;render.upload_matrix(0,identity.m);
        TextRenderer::Style title;title.scale=2;
        TextRenderer::Style small;small.scale=1;
        TextRenderer::Style muted=small,warning=small;
        muted.r=193;muted.g=221;muted.b=239;warning.r=255;warning.g=197;warning.b=112;
        unsigned draws=0;
        auto fill=[&](s32 x,s32 y,s32 w,s32 h,Tint tint){
            quads_.clear();
            ScreenDraw::textured_quad(quads_,f32(x),f32(y),f32(x+w),f32(y+h),0,0,0,1,1,tint);
            const auto first=render.append_vertices(quads_.data(),quads_.size());
            draws+=render.draw(Primitive::Triangles,first,u32(quads_.size()))?1:0;
        };
        auto text=[&](std::string_view value,s32 x,s32 y,s32 room,const TextRenderer::Style& style){
            auto label=printable(value);
            label.resize(std::min(label.size(),std::size_t(std::max(0,room)/(style.scale*6))));
            draws+=text_.draw(render,label,x,y,style);
        };
        text("SAVED FORCE BROWSER",16,12,width-32,title);
        const auto name=printable(save_name);
        text(name.empty()?"SELECTED SAVE / READ ONLY":"SAVE: "+name+" / READ ONLY",16,38,width-32,muted);
        if(!force){
            fill(12,75,width-24,height-114,{31,43,56,255});
            text("REOPEN THE SELECTED SAVE",22,94,width-44,title);
            text("THE SAVED FORCE CHANGED OR COULD NOT BE READ",22,128,width-44,warning);
            text("ESC CLOSE",16,height-24,width-32,small);
            render.state()=saved;return draws;
        }
        const auto cost=force->declared_cost?std::to_string(*force->declared_cost):"UNAVAILABLE";
        text("FORCE "+std::to_string(force->force+1)+"/"+std::to_string(kForceCount)+
            "  OVERLAPS "+std::to_string(force->overlaps)+"  DECLARED COST "+cost,16,54,width-32,muted);
        const s32 split=width*37/100,right=split+16,right_room=width-right-22;
        fill(12,74,split-16,height-114,{24,36,48,255});
        fill(split,74,width-split-12,height-114,{31,43,56,255});
        text("SLOTS",22,87,split-36,title);
        const unsigned room=unsigned(std::max(1,(height-(practice?175:153))/22));
        const unsigned start=cursor.slot()>=room?cursor.slot()-room+1:0;
        for(unsigned slot=start;slot<std::min(kForceSlots,start+room);++slot){
            const auto* row=force->find_slot(slot);
            const s32 y=115+s32(slot-start)*22;
            if(slot==cursor.slot())fill(18,y-4,split-27,20,{39,102,126,255});
            const auto number=(slot<9?"0":"")+std::to_string(slot+1);
            const auto kind=!row||(!row->retail&&!row->packaged)?"EMPTY":row->overlap()?"OVERLAP":
                row->retail?"RETAIL":row->packaged->saved.disabled?"PACKAGE DISABLED":
                row->packaged->selectable_metadata()?"PACKAGE":"PACKAGE UNAVAILABLE";
            text(number+"  "+kind,23,y,split-43,row&&row->overlap()?warning:small);
        }
        const auto* selected=force->find_slot(cursor.slot());
        text("SLOT "+std::to_string(cursor.slot()+1),right,87,right_room,title);
        if(selected&&selected->overlap())text("BOTH CLAIMS / NO WINNER",right,109,right_room,warning);
        else text(force->retail_record_present?"VERIFIED RETAIL RECORD":"RETAIL RECORD ABSENT",right,109,right_room,muted);
        text("RETAIL CLAIM",right,134,right_room,muted);
        if(selected&&selected->retail){
            const auto& r=*selected->retail;
            char model[16];std::snprintf(model,sizeof(model),"MODEL %04X",unsigned(u16(r.value.model)));
            const auto label=printable(r.source_name);
            text(label.empty()?model:label,right,151,right_room,small);
            text(std::string(model)+" / BOX "+std::to_string(r.box_slot+1)+" / VARIANT "+std::to_string(r.value.variant),
                right,167,right_room,small);
            text("COST "+std::to_string(r.declared_cost)+(r.value.edits.modified?" / EDITED":""),right,183,right_room,small);
        }else text("NONE",right,151,right_room,small);
        text("PACKAGE CLAIM",right,209,right_room,muted);
        if(selected&&selected->packaged){
            const auto& p=*selected->packaged;
            const auto label=printable(p.saved.identity.name);
            text(label.empty()?p.saved.identity.id:label,right,226,right_room,small);
            text("INSTANCE "+std::to_string(p.saved.instance)+" / "+p.saved.identity.id,right,242,right_room,small);
            if(p.saved.variant){
                const auto variant=printable(p.saved.variant->label);
                text(variant.empty()?p.saved.variant->id:p.saved.variant->id+" / "+variant,right,258,right_room,small);
            }else text("NO EXACT SAVED VARIANT",right,258,right_room,small);
            text(std::string(slot_status(p.status))+(p.saved.disabled?" / DISABLED":""),
                right,274,right_room,p.selectable_metadata()?muted:warning);
        }else text("NONE",right,226,right_room,small);
        if(practice){
            text("ROUTE: "+std::string(cursor.practice_route_name()),22,height-61,split-42,muted);
            if(!practice->empty())text(*practice,right,height-61,right_room,muted);
            text("UP/DOWN SLOT  LEFT/RIGHT FORCE  L/R ROUTE",16,height-35,width-32,small);
            text("Q/Y PRACTICE  ESC CLOSE",16,height-22,width-32,small);
        }else text("UP/DOWN SLOT  LEFT/RIGHT FORCE  ESC CLOSE",16,height-24,width-32,small);
        render.state()=saved;return draws;
    }
private:
    static std::string printable(std::string_view value){
        std::string text(value);
        for(auto& c:text){
            if(c>='a'&&c<='z')c=char(c-'a'+'A');
            if(c=='_')c='-';
            if(!TextRenderer::covers(c))return {};
        }
        return text;
    }
    static std::string_view slot_status(PackageForceSlotStatus status){
        using Status=PackageForceSlotStatus;
        switch(status){
        case Status::MetadataReady:return "METADATA RESOLVED";
        case Status::ScanUnavailable:return "PACKAGE SCAN UNAVAILABLE";
        case Status::MissingPackage:return "PACKAGE MISSING";
        case Status::IncompletePackage:return "PACKAGE INCOMPLETE";
        case Status::Conflict:return "CONFLICTING PACKAGE IDS";
        case Status::VariantUnspecified:return "VARIANT UNSPECIFIED";
        case Status::CatalogueUnavailable:return "CATALOGUE UNAVAILABLE";
        case Status::MissingVariant:return "SAVED VARIANT MISSING";
        }
        return "UNAVAILABLE";
    }
    TextRenderer text_;
    std::vector<Vertex> quads_;
};
}
