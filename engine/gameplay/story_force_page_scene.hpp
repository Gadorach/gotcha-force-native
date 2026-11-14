#pragma once
#include "engine/gameplay/story_force_widgets.hpp"
#include "engine/gameplay/story_force_card_scene.hpp"
#include "engine/graphics/source_text.hpp"
namespace gf {
//Prepared whole force-page widget/card submission, not MAIN/resource/fade/storage
//integration. Producer ordering is explicit; all widgets share global passes.
class StoryForcePageScene {
public:
    bool load_prepared(const StoryForceWidgetsArt& art,const StoryForceCardArt& card,const HsdRecordAnimationBank& setup,const HsdRecordAnimationBank& icons,const ForceEditorArt& mapping,const SourceFont& font,RenderEngine& render,std::string* error){
        *this={};if(!art.ready()||art.card_tag()!=card.tag()||setup.hash()!=art.bank_hash()||!cards_.load_prepared(card,setup,icons,mapping,error)||!text_.load(font,render,90000))return fail(error,"art/banks/font");
        art_=art;card_=card;setup_=setup;ready_=true;return true;
    }
    bool draw(RenderEngine& render,const StoryForceWidgets& widgets,const StoryForceCardPlan& cards,const ForceInventory& owner,std::string* error){
        if(!ready_||render.model_pass()||!widgets.current(owner)||widgets.art_tag()!=art_.tag()||widgets.force()!=cards.force()||!cards_.matches(cards,owner)||widgets.draws().size()>32)return fail(error,"live coherent visual/card owner");
        std::string name=widgets.name();
        //Existing native ASCII fixture compatibility only, not general locale or
        //a change to encoded Story names. Unsupported encoded glyphs refuse.
        if(std::all_of(name.begin(),name.end(),[](unsigned char c){return c<128;})){std::string encoded;if(!text_.font().encode_ascii(name,&encoded))return fail(error,"native ASCII name encoding");name=std::move(encoded);}
        if(name.size()%2||name.size()>80)return fail(error,"name extent");
        for(unsigned i=0;i<name.size();i+=2){const u16 code=u16(u16(u8(name[i]))<<8|u8(name[i+1]));if(code!=text_.font().blank()&&text_.font().glyph(code)<0)return fail(error,"unknown name glyph");}
        for(const auto& draw:widgets.draws())if(!draw.text&&(draw.record>=setup_.records()||draw.joint_time<0||draw.joint_time>8192||draw.material_time<0||draw.material_time>8192||!std::isfinite(draw.joint_time)||!std::isfinite(draw.material_time)))return fail(error,"widget packet");
        struct Item {bool card=false;unsigned index=0;};std::vector<Item> items;
        //Constructor table0/1/2, four physical card bands3, then4/5/6. Each
        //callback emits its own source order (cost digits are already reversed).
        const auto poses=widgets.draws();unsigned at=0;while(at<poses.size()&&poses[at].type<3)items.push_back({false,at++});
        for(unsigned i=0;i<cards.draws().size();++i)items.push_back({true,i});
        while(at<poses.size())items.push_back({false,at++});
        for(u8 pass:{u8(1),u8(4),u8(2)}){
            if(!render.begin_model_pass(pass))return fail(error,"global pass");
            bool ok=true;
            for(auto it=items.rbegin();it!=items.rend()&&ok;++it){
                if(it->card){ok=cards_.draw_packet_in_pass(render,cards,owner,it->index,error);continue;}
                const auto& draw=poses[it->index];const bool queued=pass==1?(draw.mask&1):pass==4?(draw.mask&2):(draw.mask&0x7c);if(!queued)continue;
                if(draw.text){ok=text_.draw(render,card_.camera(),draw.world,name,art_.value(StoryForceWidgetsArt::TextExtent),art_.value(StoryForceWidgetsArt::TextAdvance),{255,255,255,255},false,art_.low(),art_.high());continue;}
                ok=draw_widget(render,draw,error);
            }
            if(!ok)render.reject_model_pass();
            const bool ended=render.end_model_pass();if(!ok||!ended)return fail(error,"source page submission");
        }
        return true;
    }
    bool ready()const{return ready_;}std::size_t widget_models()const{return models_.size();}
private:
    struct Cached {AnimatedModelScene scene;f32 joint=0,material=0;};
    bool draw_widget(RenderEngine& render,const StoryForceWidgetDraw& draw,std::string* error){
        auto found=models_.find(draw.record);if(found==models_.end()){
            Cached model;if(!model.scene.load_compact_textures(setup_,draw.record,3,0,0,error)||!model.scene.request_selectors(draw.joint_time,draw.material_time,error))return false;
            model.scene.set_enabled(true);model.scene.set_untextured_billboards(true);model.scene.upload_textures_shared(render,40000);if(render.shared_texture_upload_failed())return false;model.joint=draw.joint_time;model.material=draw.material_time;found=models_.emplace(draw.record,std::move(model)).first;
        }
        auto& model=found->second;if(model.joint!=draw.joint_time||model.material!=draw.material_time){if(!model.scene.request_again_selectors(draw.joint_time,draw.material_time,error))return false;model.joint=draw.joint_time;model.material=draw.material_time;}
        if(!draw.authored_world&&!model.scene.set_root_world(draw.world,error))return false;
        model.scene.draw(render,card_.camera());return true;
    }
    bool fail(std::string* error,const char* why){ready_=false;if(error&&error->empty())*error=std::string("Story force page: ")+why;return false;}
    StoryForceWidgetsArt art_;StoryForceCardArt card_;HsdRecordAnimationBank setup_;StoryForceCardScene cards_;SourceTextRenderer text_;std::map<u16,Cached> models_;bool ready_=false;
};
} //namespace gf
