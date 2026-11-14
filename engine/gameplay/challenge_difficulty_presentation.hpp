#pragma once
#include "engine/gameplay/challenge_difficulty_flow.hpp"
#include "engine/gameplay/challenge_difficulty_scene.hpp"
#include "engine/graphics/screen_fade.hpp"
namespace gf {
//Native resource/input/page/visual/fade owner. No source process, guest memory,
//card loading, music/SFX device, main-menu or battle acknowledgement lives here.
class ChallengeDifficultyPresentation {
public:
    bool open(ByteView bank,const ChallengeDifficultyArt& art,const AttachmentAlternateProfile& style,
              const ChallengeEntryProfile& entry,const ChallengeDifficultyProfile& difficulty,u8 participants,std::string* error){
        *this={};if(!bank.data||bank.size<32||bank.size>(64u<<20)||!art.valid()||art.limits!=difficulty.energy||!attachment_alternate_profile_valid(style)||!flow_.open(entry,difficulty,participants))return fail(error,"entry/profile/bank domain");
        bytes_.assign(bank.data,bank.data+bank.size);art_=art;style_=style;opened_=true;return true;
    }
    bool step(const std::array<ChallengeSetupPad,4>& pads,ChallengeEntryEvents* out,std::string* error){
        if(!out)return false;
        *out={};if(!opened_||failed_||flow_.terminal())return false;
        ChallengeEntryEvents event;if(!flow_.step(resources_ready_,pads,&event))return fail(error,"source flow callback");
        if(event.request_resources){if(!resources_.load({bytes_.data(),bytes_.size()},error,true)||resources_.hash()!=art_.bank_hash)return fail(error,"native resources");resources_ready_=true;bytes_.clear();}
        const bool creating=event.build_widgets;
        if(creating){if(!scene_.load_prepared(resources_,art_,style_,error))return fail(error,"native widget construction");widgets_=true;}
        if(event.retire_widgets){widgets_=false;scene_={};resources_={};resources_ready_=false;}
        //Newly constructed source widgets have their explicit constructor
        //updates already applied; never give them an extra update in that callback.
        if(widgets_&&!creating&&!scene_.step(flow_.visual_input(),error))return fail(error,"widget update");
        *out=event;return true;
    }
    bool draw(RenderEngine& render,std::string* error){
        if(!opened_||failed_)return false;
        if(widgets_&&!scene_.draw(render,error))return fail(error,"widget draw");
        if(!draw_screen_fade(render,flow_.fade(),640,480))return fail(error,"fade draw");
        return true;
    }
    const ChallengeDifficultyFlow& flow()const{return flow_;}
    const ChallengeDifficultyScene* scene()const{return widgets_&&!failed_?&scene_:nullptr;}
    bool resources_ready()const{return resources_ready_;}bool failed()const{return failed_;}
private:
    bool fail(std::string* error,const char* why){failed_=true;widgets_=false;if(error&&error->empty())*error=std::string("Challenge presentation: ")+why;return false;}
    ChallengeDifficultyFlow flow_;ChallengeDifficultyArt art_;AttachmentAlternateProfile style_;HsdRecordAnimationBank resources_;ChallengeDifficultyScene scene_;
    std::vector<u8> bytes_;bool opened_=false,resources_ready_=false,widgets_=false,failed_=false;
};
} // namespace gf
