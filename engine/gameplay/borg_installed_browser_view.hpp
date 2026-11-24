#pragma once
#include "engine/gameplay/borg_installed_browser.hpp"
#include "engine/graphics/scene_draw.hpp"
#include "engine/graphics/portable_text.hpp"
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
    std::optional<std::string> force_name{};
    u32 before_cost=0,after_cost=0;
    std::string_view target_claim{};
    static bool review_fits(const BorgCollectionLayout& layout){return layout.width>=280&&layout.height>=180;}
};
struct BorgBrowserPracticePanel {std::string_view status;};

class BorgInstalledBrowserView {
public:
    unsigned draw(RenderEngine& render,s32 width,s32 height,const BorgInstalledBrowser& browser,
                  const BorgBrowserTile& tile,bool confirmed,bool preview_unavailable=false,std::string_view save_name={},
                  const BorgPlacementPanel* placement=nullptr,const BorgBrowserPracticePanel* practice=nullptr,
                  const BorgCollectionLayout* layout=nullptr,bool cheat_picker=false,const PortableFont* collection_font=nullptr){
        const auto* collection=browser.collection();
        const auto* font_metrics=collection&&!placement&&!practice&&!cheat_picker&&collection_font?collection_font->metrics(render):nullptr;
        if(!collection&&(width<440||height<320))return 0;
        const auto fitted=BorgCollectionLayout::fit(unsigned(std::max(0,width)),unsigned(std::max(0,height)),
            unsigned(std::max(0,width)),unsigned(std::max(0,height)));
        if(collection&&!layout)layout=&fitted;
        const s32 scale=layout?layout->scale:1;
        const auto saved=render.state();
        render.state()={};render.state().depth_test=render.state().depth_write=false;
        render.state().cull_backface=false;render.state().blend=true;render.state().blend_src=5;render.state().blend_dst=6;
        const Mat4 identity;render.upload_matrix(0,identity.m);
        TextRenderer::Style title;title.scale=2;TextRenderer::Style small;small.scale=1;
        TextRenderer::Style body=small;body.r=200;body.g=224;body.b=242;
        unsigned draws=0;
        auto fill=[&](s32 x,s32 y,s32 w,s32 h,Tint tint,u32 texture=0){
            if(w<=0||h<=0)return;
            quads_.clear();render.state().texture=texture;
            ScreenDraw::textured_quad(quads_,f32(x*scale),f32(y*scale),f32((x+w)*scale),f32((y+h)*scale),0,0,0,1,1,tint);
            const auto first=render.append_vertices(quads_.data(),quads_.size());
            draws+=render.draw(Primitive::Triangles,first,u32(quads_.size()))?1:0;
            render.state().texture=0;
        };
        auto text=[&](std::string_view value,s32 x,s32 y,s32 space,const TextRenderer::Style& style){
            if(font_metrics){
                // Fit an authored atlas's full ink height into the current
                // one-line text slot, including unusual bearings, without
                // stretching either axis or changing the hit-test layout.
                const auto fit=std::min(16.f,f32(style.scale*scale*TextRenderer::kGlyphRows)/f32(font_metrics->ink_height()));
                const PortableTextStyle font_style{{style.r,style.g,style.b,style.a},fit,f32(std::max(0,space)*scale)};
                draws+=portable_text_.draw(render,collection_font,value,f32(x*scale),f32(y*scale)-font_metrics->ink_top()*fit,font_style);
                return;
            }
            auto label=collection?compatible(value):printable(value);
            if(label.empty())return;
            const auto cells=std::size_t(std::max(0,space)/(style.scale*6));
            if(label.size()>cells){
                label.resize(cells);
                if(collection&&cells>=4)label.replace(cells-3,3,"...");
            }
            auto scaled=style;scaled.scale*=scale;
            draws+=text_.draw(render,label,x*scale,y*scale,scaled);
        };
        if(collection){
            if(placement&&(placement->offered||placement->locked))
                draw_placement(*layout,*placement,save_name,fill,text,title,small,body);
            else draw_collection(*layout,browser,*collection,tile,confirmed,preview_unavailable,save_name,placement,
                cheat_picker,fill,text,title,small,body);
            render.state()=saved;
            return draws;
        }
        const auto* owned=browser.owned();
        text(owned?"OWNED BORG COLLECTION":"INSTALLED BORG PICKER",16,12,width-32,title);
        const auto saved_label=printable(save_name);
        if(owned)text(saved_label.empty()?"SELECTED SAVE - READ ONLY":"SAVE: "+saved_label+" - READ ONLY",16,40,width-32,small);
        else text("READ ONLY - NO OWNERSHIP OR BATTLE ADMISSION",16,40,width-32,small);
        const s32 split=width*45/100,right=split+16,right_space=width-right-24;
        constexpr s32 top=66;
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
        if(practice&&browser.owned_entry())text("ROUTE: "+std::string(browser.practice_route_name()),22,height-61,split-40,body);
        text(practice?"UP/DOWN OWNED COPY  LEFT/RIGHT PRACTICE ROUTE":
            owned?"UP/DOWN OWNED COPY  SHIFT/B RETURN FROM PREVIEW":"UP/DOWN PACKAGE  LEFT/RIGHT VARIANT",
            16,height-32,width-32,small);
        text(practice?"SPACE/A INSPECT  E/X PREVIEW  Q/Y PRACTICE  ESC CLOSE":"SPACE/A INSPECT  E/X BODY PREVIEW  ESC CLOSE",
            16,height-19,width-32,small);
        render.state()=saved;
        return draws;
    }
private:
    template<class Fill,class Text>
    static void draw_placement(const BorgCollectionLayout& layout,const BorgPlacementPanel& panel,std::string_view save_name,
                               Fill&& fill,Text&& text,TextRenderer::Style title,
                               const TextRenderer::Style& small,const TextRenderer::Style& body){
        const auto x=layout.list_left,w=layout.width-2*x,h=layout.height;
        if(h<22||w<10)return;
        if(w<380)title=body;
        text("ARRANGE SAVED FORCE",x,5,w,title);
        if(h>=35)text(save_name.empty()?"SELECTED SAVE":"SAVE: "+std::string(save_name),x,25,w,body);
        const auto target="FORCE "+std::to_string(panel.force+1)+" / SLOT "+std::to_string(panel.slot+1);
        if(h>=50)text(target,x,41,w,body);
        if(panel.locked){
            if(h>=70)text(panel.status,x,57,w,body);
            if(h>=88)text("REOPEN THE SAVE TO VIEW CURRENT FORCE SLOTS",x,73,w,small);
            return;
        }
        if(!BorgPlacementPanel::review_fits(layout)){
            if(h>=70)text("ENLARGE WINDOW TO REVIEW EXACT PLACEMENT",x,57,w,body);
            return;
        }
        const auto& borg=*panel.offered;
        text(panel.force_name.value_or("PACKAGE FORCE"),x,52,w,small);
        text("OWNED INSTANCE "+std::to_string(borg.instance),x,64,w,body);
        text(borg.identity.name,x,75,w,small);
        if(borg.variant){
            text("VARIANT "+borg.variant->id,x,86,w,body);
            text(borg.variant->label,x,97,w,small);
        }
        text("COPY COST "+std::to_string(panel.declared_cost)+" / FORCE "+
            std::to_string(panel.before_cost)+" TO "+std::to_string(panel.after_cost),x,109,w,body);
        const s32 controls=h-58;
        fill(x,controls+(panel.confirm_selected?17:0),w,16,{35,105,130,255});
        text("CANCEL",x+5,controls+4,w-10,small);
        text("CONFIRM PLACEMENT",x+5,controls+21,w-10,small);
        text(panel.headless?"HEADLESS - NO SAVE WRITES":"UP/DOWN CHOOSE  SPACE/A ACCEPT  SHIFT/B CANCEL",
            x,h-15,w,small);
    }
    template<class Fill,class Text>
    static void draw_collection(const BorgCollectionLayout& layout,const BorgInstalledBrowser& browser,
                                const BorgCollection& collection,const BorgBrowserTile& tile,bool confirmed,bool preview_unavailable,
                                std::string_view save_name,const BorgPlacementPanel* placement,bool cheat_picker,
                                Fill&& fill,Text&& text,TextRenderer::Style title,
                                const TextRenderer::Style& small,const TextRenderer::Style& body){
        const auto w=layout.width,h=layout.height,x=layout.list_left;
        const auto list_w=layout.list_width;
        if(w<10||h<4)return;
        if(w<370)title=body;
        if(h>=27)text(placement?"ARRANGE SAVED FORCE":cheat_picker?"CHEAT PICKER / OFFER ONLY":
            collection.is_owned()?"OWNED BORG COLLECTION":"INSTALLED BORG COLLECTION",x,5,w-2*x,title);
        if(h>=116){
            const auto detail=placement?"FORCE "+std::to_string(placement->force+1)+" / SLOT "+std::to_string(placement->slot+1)+
                    " / "+std::string(placement->target_claim):
                cheat_picker?(confirmed?"EXACT VARIANT OFFER READY - NO GRANTS":"NO OWNERSHIP OR SAVE CHANGES"):
                save_name.empty()?"READ ONLY":"SAVE: "+std::string(save_name)+" - READ ONLY";
            text(detail,x,27,w-2*x,body);
            text(collection.sort()==BorgCollectionSort::Identity?"ORDER: ID":"ORDER: NAME",x,43,list_w,small);
            if(w>=250)text(collection.filter()==BorgCollectionFilter::All?"FILTER: ALL":"FILTER: EXACT",
                x+std::min(116,w/2),43,w-x-std::min(116,w/2),small);
        }
        const auto [first,last]=collection.visible_range(layout.rows);
        fill(x,layout.panel_top,list_w,std::max(0,layout.list_y-layout.panel_top)+s32(layout.rows)*layout.row_height,{25,36,48,255});
        if(h>=116){
            const auto shown=collection.size()?std::to_string(first+1)+"-"+std::to_string(last)+" / "+std::to_string(collection.size()):"0";
            text(shown,x+6,layout.panel_top+6,list_w-12,body);
        }
        if(!collection.size()){
            const auto status=browser.catalog().status;
            const auto empty=collection.filter()==BorgCollectionFilter::Exact?"NO MATCHING ROWS":
                collection.is_owned()?"NO OWNED PACKAGE BORGS":
                status==BorgPackageScanStatus::Unavailable||status==BorgPackageScanStatus::EntryBudget?
                    "PACKAGE SCAN UNAVAILABLE":"NO PACKAGES FOUND";
            if(layout.rows&&layout.row_height>=7)text(empty,x+6,layout.list_y+std::min(5,layout.row_height-7),list_w-12,small);
        }
        for(auto i=first;i<last;++i){
            const auto y=layout.list_y+s32(i-first)*layout.row_height;
            if(i==collection.selected())fill(x+2,y,list_w-4,layout.row_height-1,{35,105,130,255});
            const auto* owned=collection.owned_row(i);
            const auto* row=collection.installed_row(i);
            const auto& id=owned?owned->saved.identity:row->identity;
            auto label=owned?std::to_string(owned->saved.instance)+"  ":"";
            label+=id.name;
            if(owned&&owned->saved.variant)label+=" / "+owned->saved.variant->label;
            if(!owned&&row->key)label+=" / "+row->variant_label;
            if(owned&&owned->saved.disabled)label+=" - DISABLED";
            if(layout.row_height>=7)text(label,x+6,y+std::min(8,layout.row_height-7),list_w-12,small);
        }
        if(layout.show_details&&collection.size()){
            const auto* owned=collection.owned_row(collection.selected());
            const auto* row=collection.installed_row(collection.selected());
            const auto& id=owned?owned->saved.identity:row->identity;
            const auto state=owned?owned->state:row->state;
            const std::string label=owned?(owned->saved.variant?owned->saved.variant->label:std::string{}):row->variant_label;
            const std::string variant=owned?(owned->saved.variant?owned->saved.variant->id:std::string{}):
                row->key?row->key->variant_id:std::string{};
            const auto d=layout.detail_left,y=layout.detail_y,space=layout.detail_width-12;
            fill(d,y-3,layout.detail_width,layout.footer_y-y,{31,43,56,255});
            text(id.name,d+6,y+4,space,title);
            text(id.id,d+6,y+22,space,body);
            text(label.empty()?std::string{}:label+" - "+variant,d+6,y+33,space,small);
            auto status=std::string(borg_library_state_label(state));
            if(owned&&owned->saved.disabled)status="DISABLED - "+status;
            text(status,d+6,y+44,space,body);
            if(row&&row->declared_cost){
                text("COST "+std::to_string(*row->declared_cost)+" / TRIBE "+row->tribe,d+6,y+55,space,small);
                text(row->gameplay_declared?"GAMEPLAY DECLARED - NOT ADMITTED":"GAMEPLAY NOT DECLARED",d+6,y+66,space,small);
            }
            if(row&&row->key){
                const auto message=preview_unavailable?"BODY PREVIEW UNAVAILABLE":confirmed?
                    (cheat_picker?"EXACT VARIANT OFFER READY":"EXACT SELECTION INSPECTED"):
                    row->preview_declared?"BODY PREVIEW DECLARED":"BODY PREVIEW NOT DECLARED";
                text(message,d+6,y+77,space,body);
            }
            if(owned&&y+95<layout.footer_y){
                const auto slots=std::count_if(browser.owned()->placements.begin(),browser.owned()->placements.end(),
                    [&](const auto& placement){return placement.instance==owned->saved.instance;});
                text("COPY "+std::to_string(owned->saved.instance)+" / FORCE SLOTS "+std::to_string(slots),d+6,y+88,space,small);
            }
            const auto icon_y=y+(owned?108:99);
            const auto room=std::min({96,space,layout.footer_y-(icon_y+6)});
            if(room>=16&&tile.status==BorgBrowserTileStatus::Image&&tile.width&&tile.height&&tile.texture){
                const auto ratio=f32(room)/f32(std::max(tile.width,tile.height));
                fill(d+6,icon_y,std::max(1,s32(f32(tile.width)*ratio)),std::max(1,s32(f32(tile.height)*ratio)),{},tile.texture);
            }else if(room>=1&&row&&row->key&&tile.status!=BorgBrowserTileStatus::Image){
                text(tile.status==BorgBrowserTileStatus::Placeholder?"EDITOR ICON PLACEHOLDER":
                    tile.status==BorgBrowserTileStatus::Unresolved?"EDITOR ICON UNRESOLVED":"EDITOR ICON UNAVAILABLE",d+6,icon_y,space,small);
            }
        }
        if(layout.show_footer){
            text(placement&&!placement->status.empty()?placement->status:"UP/DOWN ROW  LEFT/RIGHT PAGE  L SORT  R FILTER",x,h-25,w-2*x,small);
            text(placement?"SPACE/A STAGE POSITION  E/X PREVIEW  ESC CLOSE":
                cheat_picker?"SPACE/A MAKE OFFER  E/X PREVIEW  ESC CLOSE":"SPACE/A INSPECT  E/X PREVIEW  ESC CLOSE",x,h-12,w-2*x,small);
        }
    }
    // One visible replacement per unsupported character; the stored UTF-8
    // names are never rewritten to satisfy the compatibility font.
    static std::string compatible(std::string_view text){
        std::string label;
        for(char c:text){
            if((u8(c)&0xc0u)==0x80u)continue;
            if(c>='a'&&c<='z')c=char(c-'a'+'A');
            if(c=='_')c='-';
            label+=TextRenderer::covers(c)?c:'?';
        }
        return label;
    }
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
    PortableTextRenderer portable_text_;
    std::vector<Vertex> quads_;
};
}
