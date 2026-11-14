#pragma once
#include "engine/gameplay/challenge_difficulty_visual.hpp"
#include "engine/gameplay/challenge_setup_widgets.hpp"
namespace gf {
struct ChallengeSetupVisualInput {u8 visible=8,interactive=0,closing=0,difficulty=0,players=0,context=0;};
//One common clock, retained difficulty widgets, retained player/box widgets.
//Initial/re-entry callbacks do not recreate the bank or any widget owner.
class ChallengeSetupVisual {
public:
    bool open(const ChallengeDifficultyArt& common,const ChallengeSetupWidgetsArt& art){
        *this={};if(!chrome_.open(common,true)||!difficulty_.open_widgets(common)||!pages_.open(common,art))return false;
        ready_=true;return publish();
    }
    bool step(const ChallengeSetupVisualInput& input){
        if(!ready_||input.difficulty>2||input.players>1||input.context>2||((input.visible|input.interactive|input.closing)&~15u))return fail();
        auto next=*this;
        if(!next.chrome_.step(input.visible,input.interactive)||!next.difficulty_.step_widgets({u8(input.visible&8),u8(input.interactive&8),u8(input.closing&8),input.difficulty},input.context)||!next.pages_.step({input.visible,input.interactive,input.closing,input.players,input.context})||!next.publish())return fail();
        *this=std::move(next);return true;
    }
    bool ready()const{return ready_;}const std::vector<ChallengeSetupDraw>& draws()const{return draws_;}const ChallengeSetupChrome& chrome()const{return chrome_;}
    const ChallengeSetupWidgets& pages()const{return pages_;}
private:
    bool publish(){draws_=chrome_.draws();draws_.insert(draws_.end(),difficulty_.draws().begin(),difficulty_.draws().end());draws_.insert(draws_.end(),pages_.draws().begin(),pages_.draws().end());return draws_.size()<=80;}
    bool fail(){ready_=false;draws_.clear();return false;}
    ChallengeSetupChrome chrome_;ChallengeDifficultyVisual difficulty_;ChallengeSetupWidgets pages_;std::vector<ChallengeSetupDraw> draws_;bool ready_=false;
};
} // namespace gf
