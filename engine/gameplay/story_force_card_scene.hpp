#pragma once
#include "engine/gameplay/story_force_cards.hpp"
#include "engine/assets/force_editor_art.hpp"
namespace gf {
//Isolated Story cards only, NOT full SELECT A FORCE chrome/cost/name/footer.
//The host must join all widgets in the same source global queues later. Every
//model uses the existing static-record/AnimatedModelScene/MenuScene path.
class StoryForceCardScene {
public:
    bool load_prepared(const StoryForceCardArt& art,const HsdRecordAnimationBank& setup,const HsdRecordAnimationBank& icons,const ForceEditorArt& mapping,std::string* error){
        *this={};if(!art.ready()||setup.hash()!=art.bank_hash()||art.shell()>=setup.records()||!icons.records()||mapping.icons.empty())return fail(error,"prepared art/banks/map");
        for(unsigned i=0;i<3;++i)if(art.layout().formats[i].record>=setup.records())return fail(error,"source card format record");
        for(const auto& pair:mapping.icons)if(pair.second>=icons.records())return fail(error,"icon record");
        art_=art;setup_=setup;icons_=icons;mapping_=mapping;ready_=true;return true;
    }
    bool draw(RenderEngine& render,const StoryForceCardPlan& plan,const ForceInventory& owner,std::string* error){
        if(!ready_||!plan.current(owner)||plan.art_tag()!=art_.tag()||plan.draws().size()>31)return fail(error,"inventory revision/art binding");
        //Reject unknown icons before submitting any packet. Source callbacks
        //cannot turn an unresolved record into a silent invisible Borg.
        for(const auto& draw:plan.draws())if(draw.record>=setup_.records()||(draw.member&&mapping_.icon(u16(draw.model))<0))return fail(error,"model/icon binding");
        for(u8 pass:{u8(1),u8(4),u8(2)}){
            if(!render.begin_model_pass(pass))return fail(error,"global pass");
            bool ok=true;
            for(auto it=plan.draws().rbegin();it!=plan.draws().rend()&&ok;++it){
                const auto& draw=*it;const bool queued=pass==1?(draw.mask&1):pass==4?(draw.mask&2):(draw.mask&0x7c);
                if(!queued)continue;
                const StaticRecordAppearance shell{draw.member?draw.icon_appearance.alpha:1,{255,255,255}};
                ok=draw_static_record(render,setup_,setup_models_,draw.record,draw.world,art_.camera(),40000,error,draw.member,true,draw.member?&shell:nullptr);
                if(ok&&draw.member){const int record=mapping_.icon(u16(draw.model));ok=draw_static_record(render,icons_,icon_models_,u16(record),draw.icon_world,art_.camera(),50000,error,true,true,&draw.icon_appearance);}
                ok=ok&&!render.shared_texture_upload_failed();
            }
            if(!ok)render.reject_model_pass();
            const bool ended=render.end_model_pass();if(!ok||!ended)return fail(error,"shared static-card submission");
        }
        return true;
    }
    bool ready()const{return ready_;}std::size_t models()const{return setup_models_.size()+icon_models_.size();}
    //Opt-in page composition: append a single card producer packet inside the
    //caller's already-open global pass. Existing card-only draw stays unchanged.
    bool matches(const StoryForceCardPlan& plan,const ForceInventory& owner)const{
        if(!ready_||!plan.current(owner)||plan.art_tag()!=art_.tag()||plan.draws().size()>31)return false;
        for(const auto& draw:plan.draws())if(draw.record>=setup_.records()||(draw.member&&mapping_.icon(u16(draw.model))<0))return false;
        return true;
    }
    bool draw_packet_in_pass(RenderEngine& render,const StoryForceCardPlan& plan,const ForceInventory& owner,unsigned index,std::string* error){
        const auto pass=render.model_pass();if((pass!=1&&pass!=4&&pass!=2)||!matches(plan,owner)||index>=plan.draws().size())return fail(error,"composed pass/packet binding");
        const auto& draw=plan.draws()[index];const bool queued=pass==1?(draw.mask&1):pass==4?(draw.mask&2):(draw.mask&0x7c);if(!queued)return true;
        const StaticRecordAppearance shell{draw.member?draw.icon_appearance.alpha:1,{255,255,255}};
        if(!draw_static_record(render,setup_,setup_models_,draw.record,draw.world,art_.camera(),40000,error,draw.member,true,draw.member?&shell:nullptr))return fail(error,"composed shell");
        if(draw.member){const int record=mapping_.icon(u16(draw.model));if(!draw_static_record(render,icons_,icon_models_,u16(record),draw.icon_world,art_.camera(),50000,error,true,true,&draw.icon_appearance))return fail(error,"composed icon");}
        return !render.shared_texture_upload_failed()||fail(error,"composed shared texture");
    }
private:
    bool fail(std::string* error,const char* why){ready_=false;if(error&&error->empty())*error=std::string("Story card scene: ")+why;return false;}
    StoryForceCardArt art_;HsdRecordAnimationBank setup_,icons_;ForceEditorArt mapping_;std::map<u16,AnimatedModelScene> setup_models_,icon_models_;bool ready_=false;
};
} //namespace gf
