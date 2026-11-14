#pragma once
#include "engine/gameplay/challenge_force_visual.hpp"
#include "engine/gameplay/challenge_setup_scene.hpp"
#include "engine/assets/force_editor_art.hpp"
#include "engine/graphics/source_text.hpp"
namespace gf {
//Force widgets and icons participate in the SAME source global passes as chrome.
//All setup-bank records reuse the caller's retained ChallengeSetupScene cache;
//unitall icons reuse the editor's compact static-record helper and source slots.
class ChallengeForceScene {
public:
    bool load_prepared(const HsdRecordAnimationBank& icons,const ForceEditorArt& mapping,const ChallengeDifficultyArt& common,const SourceFont& font,RenderEngine& render,std::string* error){
        *this={};if(!icons.records()||!common.valid()||mapping.icons.empty()||!text_.load(font,render,60000))return fail(error,"prepared bank/map/camera/font");
        icons_=icons;mapping_=mapping;camera_=common.camera;bank_hash_=common.bank_hash;ready_=true;return true;
    }
    bool draw(RenderEngine& render,ChallengeSetupScene& setup,std::span<const ChallengeSetupDraw> chrome,const ChallengeForceVisual& visual,const ForceInventory& owner,std::string* error,std::span<const ChallengeSetupDraw> notices={}){
        if(!ready_||!visual.current(owner)||setup.bank_hash()!=bank_hash_||visual.art().bank_hash!=bank_hash_||chrome.size()>80||notices.size()>4)return fail(error,"owner/revision/setup binding");
        for(u8 pass:{u8(1),u8(4),u8(2)}){
            if(!render.begin_model_pass(pass))return fail(error,"global pass");
            bool ok=true;
            for(auto it=notices.rbegin();it!=notices.rend()&&ok;++it)ok=setup.draw_in_pass(render,*it,error);
            for(auto it=visual.draws().rbegin();it!=visual.draws().rend()&&ok;++it){
                ok=setup.draw_in_pass(render,it->pose,error);
                if(ok&&it->model>=0&&pass!=4){const int id=mapping_.icon(u16(it->model));if(id<0){ok=false;break;}
                    ok=draw_static_record(render,icons_,icon_models_,u16(id),it->icon_world,camera_,50000,error,true,true,&it->icon_appearance)&&!render.shared_texture_upload_failed();}
            }
            if(ok&&!visual.name().empty()){
                auto name=visual.name();if(std::all_of(name.begin(),name.end(),[](unsigned char c){return c<128;})){std::string encoded;if(!text_.font().encode_ascii(name,&encoded))ok=false;else name=std::move(encoded);}
                if(ok)ok=text_.draw(render,camera_,visual.name_world(),name,visual.art().name_extent,visual.art().name_advance,{255,255,255,u8(visual.name_alpha()*255.f)});
            }
            for(auto it=chrome.rbegin();it!=chrome.rend()&&ok;++it)ok=setup.draw_in_pass(render,*it,error);
            if(!ok)render.reject_model_pass();
            const bool ended=render.end_model_pass();if(!ok||!ended)return fail(error,"source widget/icon/text submission");
        }
        return true;
    }
    bool ready()const{return ready_;}std::size_t icon_models()const{return icon_models_.size();}
private:
    bool fail(std::string* error,const char* why){ready_=false;if(error&&error->empty())*error=std::string("Challenge force scene: ")+why;return false;}
    HsdRecordAnimationBank icons_;ForceEditorArt mapping_;Camera camera_;SourceTextRenderer text_;std::map<u16,AnimatedModelScene> icon_models_;u32 bank_hash_=0;bool ready_=false;
};
} // namespace gf
