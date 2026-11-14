#pragma once
#include <memory>
#include "engine/gameplay/story_force_page_scene.hpp"
namespace gf {
//Installed native assets only. No runtime DOL/AFS/PZZ/card or guessed filename.
//The owner supplies its installation manifest; FNV is binding/corruption scope,
//NOT authentication or proof of a source campaign/resource operation.
struct StoryForceNativeSpec {
    enum File {SetupBank,IconBank,IconMapping,FontProfile,FontAtlas,CardLayout,CardPresentation,WidgetArt,Count};
    std::array<std::string,Count> paths;std::array<u32,Count> fingerprints{};u32 catalog_fingerprint=0;
};
class StoryForceNativeAssets {
public:
    StoryForceNativeAssets()=default;StoryForceNativeAssets(const StoryForceNativeAssets&)=delete;StoryForceNativeAssets& operator=(const StoryForceNativeAssets&)=delete;
    StoryForceNativeAssets(StoryForceNativeAssets&&)=default;StoryForceNativeAssets& operator=(StoryForceNativeAssets&&)=default;
    bool load(IFileSystem& files,RenderEngine& render,const ForceInventory& owner,const StoryForceNativeSpec& spec,std::string* error){
        release();auto fail=[&](const char* why){if(error)*error=std::string("Story native force assets: ")+why;return false;};
        if(!owner.ready()||!spec.catalog_fingerprint||spec.catalog_fingerprint!=owner.catalog()->fingerprint())return fail("inventory/catalog manifest");
        const auto binding=owner.instance_binding().lock();const auto revision=owner.revision();const ForceCatalog catalog=*owner.catalog();
        std::array<std::vector<u8>,StoryForceNativeSpec::Count> bytes;
        for(unsigned i=0;i<StoryForceNativeSpec::Count;++i){
            if(spec.paths[i].empty()||spec.paths[i].size()>4096||!spec.fingerprints[i])return fail("explicit manifest entry required");
            u64 size=0;const u64 cap=i<=StoryForceNativeSpec::IconBank?64u<<20:i==StoryForceNativeSpec::FontAtlas?16u<<20:1u<<20;
            if(!files.file_size(spec.paths[i],&size)||!size||size>cap)return fail("bounded native file extent");
            if(!files.read_range(spec.paths[i],0,std::size_t(size),bytes[i])||bytes[i].size()!=size)return fail("exact native range read");
            u32 hash=2166136261u;for(u8 b:bytes[i])hash=(hash^b)*16777619u;if(hash!=spec.fingerprints[i])return fail("native manifest fingerprint");
        }
        auto view=[&](unsigned i){return ByteView{bytes[i].data(),bytes[i].size()};};auto next=std::make_unique<Data>();
        if(!next->card.load(view(StoryForceNativeSpec::CardLayout),view(StoryForceNativeSpec::CardPresentation),error)||!next->widgets.load(view(StoryForceNativeSpec::WidgetArt),next->card,error)||!next->setup.load(view(StoryForceNativeSpec::SetupBank),error,true)||!next->icons.load(view(StoryForceNativeSpec::IconBank),error,true)||!next->mapping.load(view(StoryForceNativeSpec::IconMapping),catalog,error)||!next->font.load(view(StoryForceNativeSpec::FontProfile),view(StoryForceNativeSpec::FontAtlas),error))return false;
        if(next->setup.hash()!=next->card.bank_hash()||next->mapping.icons.size()!=catalog.entries().size())return fail("complete native bank/map binding");
        std::vector<u16> widgets{next->widgets.background(),next->widgets.title(),next->widgets.arrows()};
        for(const auto& row:next->widgets.footers())for(unsigned i=0;i<4;++i)if(row.records[i]>=0){widgets.push_back(u16(row.records[i]));if(i==2)widgets.push_back(u16(row.records[i]+1));}
        for(const auto& p:next->widgets.name_parts())widgets.push_back(p.record);
        for(const auto& p:next->widgets.cost_parts())widgets.push_back(p.record);
        std::sort(widgets.begin(),widgets.end());widgets.erase(std::unique(widgets.begin(),widgets.end()),widgets.end());
        //Real native binding and checked upload operations, not a caller-ready
        //boolean. An accepted upload prefix is not GPU atomicity/playback proof.
        for(u16 record:widgets){AnimatedModelScene model;if(!model.load_compact_textures(next->setup,record,3,0,0,error)||!model.request_selectors(record==next->widgets.background()?1.f:0.f,record==next->widgets.background()?3.f:0.f,error))return false;model.upload_textures_shared(render,40000);if(render.shared_texture_upload_failed())return fail("widget upload refused");}
        for(unsigned i=0;i<4;++i){const auto record=i<3?next->card.layout().formats[i].record:next->card.shell();AnimatedModelScene model;if(!model.load_compact_static(next->setup,record,error)||!model.request(0,0,error))return false;model.upload_textures_shared(render,40000);if(render.shared_texture_upload_failed())return fail("card upload refused");}
        for(const auto& entry:catalog.entries()){
            const int record=next->mapping.icon(entry.model);if(record<0||unsigned(record)>=next->icons.records())return fail("catalog icon coverage");AnimatedModelScene model;if(!model.load_compact_static(next->icons,u16(record),error)||!model.request(0,0,error))return false;model.upload_textures_shared(render,50000);if(render.shared_texture_upload_failed())return fail("icon upload refused");
        }
        if(!next->page.load_prepared(next->widgets,next->card,next->setup,next->icons,next->mapping,next->font,render,error)||render.shared_texture_upload_failed())return false;
        if(!owner.ready()||owner.instance_binding().lock()!=binding||owner.revision()!=revision)return fail("owner changed during native preparation");
        next->owner=binding;next->catalog=catalog.fingerprint();next->render=&render;data_=std::move(next);if(error)error->clear();return true;
    }
    bool ready()const{return data_&&data_->page.ready();}
    bool bound(const ForceInventory& owner,const RenderEngine& render)const{const auto scope=data_?data_->owner.lock():nullptr;return ready()&&scope&&owner.ready()&&scope==owner.instance_binding().lock()&&data_->catalog==owner.catalog()->fingerprint()&&data_->render==&render;}
    bool open_widgets(const ForceInventory& owner,unsigned force,StoryForceWidgets* out)const{return out&&inventory_bound(owner)&&out->open(data_->widgets,owner,force);}
    bool card_plan(const ForceInventory& owner,unsigned force,s32 energy,StoryForceCardPlan* out)const{return inventory_bound(owner)&&story_force_card_plan(data_->card,owner,force,energy,out);}
    bool draw(RenderEngine& render,const StoryForceWidgets& widgets,const StoryForceCardPlan& cards,const ForceInventory& owner,std::string* error){return bound(owner,render)&&data_->page.draw(render,widgets,cards,owner,error);}
    void release(){data_.reset();}
private:
    bool inventory_bound(const ForceInventory& owner)const{const auto binding=data_?data_->owner.lock():nullptr;return ready()&&binding&&owner.ready()&&owner.instance_binding().lock()==binding&&owner.catalog()->fingerprint()==data_->catalog;}
    struct Data {StoryForceCardArt card;StoryForceWidgetsArt widgets;HsdRecordAnimationBank setup,icons;ForceEditorArt mapping;SourceFont font;StoryForcePageScene page;std::weak_ptr<const ForceInventoryScope> owner;u32 catalog=0;RenderEngine* render=nullptr;};
    std::unique_ptr<Data> data_;
};
} //namespace gf
