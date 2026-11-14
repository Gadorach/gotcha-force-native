#pragma once
#include "engine/gameplay/challenge_difficulty_visual.hpp"
#include "engine/gameplay/challenge_setup_scene.hpp"
namespace gf {
//Difficulty-only original UI component. Source plans go through the existing
//record/material/UV/world renderer; no input clock, controller or backdrop fork.
class ChallengeDifficultyScene {
public:
    bool load(ByteView bank,const ChallengeDifficultyArt& art,const AttachmentAlternateProfile& style,std::string* error){
        *this={};HsdRecordAnimationBank prepared;if(!prepared.load(bank,error,true))return fail(error,"bank decode");
        return load_prepared(prepared,art,style,error);
    }
    //Own an immutable prepared resource without decoding the UI archive again.
    //Readiness here is native CPU resource/scene readiness, not source DMA/GPU ACK.
    bool load_prepared(const HsdRecordAnimationBank& bank,const ChallengeDifficultyArt& art,const AttachmentAlternateProfile& style,std::string* error){
        *this={};if(!visual_.open(art)||!scene_.load_prepared(bank,art,style,error))return fail(error,"art/bank/style binding");
        ready_=true;return true;
    }
    bool step(const ChallengeDifficultyVisualInput& input,std::string* error){
        if(!ready_||!visual_.step(input))return fail(error,"visual callback domain");
        return true;
    }
    bool draw(RenderEngine& render,std::string* error){
        if(!ready_)return false;
        if(!scene_.draw(render,visual_.draws(),error))return fail(error,"shared submission refused");
        return true;
    }
    const ChallengeDifficultyVisual& visual()const{return visual_;}
    bool ready()const{return ready_;}
    std::size_t models()const{return scene_.models();}
private:
    bool fail(std::string* error,const char* why){ready_=false;if(error&&error->empty())*error=std::string("Challenge scene: ")+why;return false;}
    ChallengeSetupScene scene_;ChallengeDifficultyVisual visual_;bool ready_=false;
};
} // namespace gf
