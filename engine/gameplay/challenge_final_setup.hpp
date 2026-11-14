#pragma once
#include "engine/gameplay/challenge_setup_presentation.hpp"
#include "engine/gameplay/challenge_initial_party.hpp"
#include "engine/gameplay/challenge_stage_data.hpp"
namespace gf {
enum class ChallengeFinalPhase {Closed,Init,Delay,CoverWait,ResourceWait,Ai,PartyRequested,Taken,Failed};
struct ChallengeFinalEvent {bool stage_committed=false,request_data=false,request_bgm_fade=false,reset_input=false,retire_widgets=false,party_requested=false;u16 bgm_fade_updates=0;u8 bgm_player=2,bgm_fade_type=2;f32 bgm_target=0;ChallengeStageDataRequest data;};
//Explicit continuation AFTER all human choices finish. Old presentation/flow
//terminal boundaries stay unchanged. A54 wait16→A74 predecrement17→cover60→DA8
//input reset→EBC native-data readiness/cleanup→NEXT callback AI. No actor ACK.
class ChallengeFinalSetup {
public:
    ChallengeFinalSetup()=default;ChallengeFinalSetup(const ChallengeFinalSetup&)=delete;ChallengeFinalSetup& operator=(const ChallengeFinalSetup&)=delete;ChallengeFinalSetup(ChallengeFinalSetup&&)=default;ChallengeFinalSetup& operator=(ChallengeFinalSetup&&)=default;
    bool open(ChallengeSetupPresentation& p,const ChallengeDifficultyProfile& difficulty,unsigned selected,const ChallengeRandomForceProfile& random,const ChallengeStageRngProfile& stages,const ChallengeAiForceProfile& ai,const ChallengeFinalizationProfile& profile,SourceByteRngOwner& rng,std::string* error){
        if(phase_!=ChallengeFinalPhase::Closed)return false;
        if(p.failed()||!p.second_enabled_||p.flow().phase()!=ChallengeSetupPhase::ForceFinished||!profile.ready()||!stages.valid()||!difficulty.valid()||selected>=3||!rng.ready())return fail(error,"fresh completed selections/profiles/current explicit stream required");
        for(const auto& row:stages.rows){if(row.field_1e_count>17)return fail(error,"stage-resource field domain");for(unsigned a=0;a<3;++a)for(unsigned b=0;b<row.field_1e_count;++b)if(!profile.find(row.stage,u8(a),u8(b)))return fail(error,"missing stage-resource row");}
        count_=p.flow().team_up()?2:1;for(unsigned i=0;i<count_;++i)if(!p.take_choice(i,&choices_[i])||choices_[i].energy()!=difficulty.energy[selected])return fail(error,"one-shot selection/energy transfer");
        selection_=p.choice_batch_;rng_owner_=rng.snapshot();difficulty_=difficulty;selected_=selected;random_=random;stages_=stages;ai_=ai;profile_=profile;phase_=ChallengeFinalPhase::Init;if(error)error->clear();return true;
    }
    bool step(ChallengeSetupPresentation& p,SourceByteRngOwner& rng,const ChallengeStageDataReceipt* receipt,ChallengeFinalEvent* out,std::string* error){
        if(out)*out={};
        if(!out||terminal()||phase_==ChallengeFinalPhase::Closed)return false;
        const auto selection=selection_.lock();if(!selection||!selection->live||selection!=p.choice_batch_||p.failed()||!rng.same_owner(rng_owner_)||updates_>=8192)return fail(error,"selection/RNG owner/callback budget");
        const std::array<const ForceInventory*,2> owners{p.player_inventory(0),p.player_inventory(1)};ChallengeFinalEvent event;
        if(receipt&&receipt->failed()){if(receipt->request_.lock()!=request_)return fail(error,"foreign failed data receipt");return fail(error,"claimed native data read failed; no replay");}
        switch(phase_){
        case ChallengeFinalPhase::Init:remaining_=profile_.wait();phase_=ChallengeFinalPhase::Delay;break;
        case ChallengeFinalPhase::Delay:
            --remaining_;
            if(remaining_<0){if(!assembly_.prepare_stage(choices_,count_,owners,difficulty_,selected_,random_,stages_,ai_,rng,error))return fail(error,"human/stage candidate");const auto* stage=assembly_.pending_stage();if(!stage)return fail(error,"stage plan missing");const auto* row=profile_.find(stage->stage,stage->field_1d,stage->field_1e);if(!row)return fail(error,"stage-resource row missing");request_=std::make_shared<challenge_stage_data_detail::Request>();request_->row=*row;ChallengeStageRngPlan committed;if(!assembly_.take_stage(owners,rng,&committed))return fail(error,"stage stream commit");event.stage_committed=event.request_data=event.request_bgm_fade=true;event.bgm_fade_updates=profile_.cover();event.data.scope_=request_;if(!fade_.request(0,profile_.cover(),ScreenFadeDirection::Cover))return fail(error,"source final cover");phase_=ChallengeFinalPhase::CoverWait;}break;
        case ChallengeFinalPhase::CoverWait:if(!fade_.remaining()){event.reset_input=true;phase_=ChallengeFinalPhase::ResourceWait;}break;
        case ChallengeFinalPhase::ResourceWait:
            if(receipt){if(!receipt->valid()||receipt->request_.lock()!=request_)return fail(error,"foreign/stale native data receipt");data_=receipt->data_.lock();if(!data_)return fail(error,"native data owner expired");request_->live=false;fade_.clear();event.retire_widgets=true;phase_=ChallengeFinalPhase::Ai;}break;
        case ChallengeFinalPhase::Ai:
            if(!assembly_.complete_ai(owners,rng,error)||!assembly_.take(owners,rng,&party_))return fail(error,"delayed AI/current-stream publication");
            event.party_requested=true;phase_=ChallengeFinalPhase::PartyRequested;break;
        default:return fail(error,"unexpected final phase");
        }
        if(!p.retained_final_callback(event.reset_input,event.retire_widgets,error))return fail(error,"retained setup callback");
        fade_.advance();++updates_;*out=event;return true;
    }
    bool draw(ChallengeSetupPresentation& p,RenderEngine& render,std::string* error){return phase_!=ChallengeFinalPhase::Failed&&phase_!=ChallengeFinalPhase::Closed&&p.choice_batch_==selection_.lock()&&p.draw(render,error)&&draw_screen_fade(render,fade_,640,480);}
    bool take(const std::array<const ForceInventory*,2>& owners,ChallengeInitialPartyCandidate* out){if(out)*out={};if(!out||phase_!=ChallengeFinalPhase::PartyRequested||!party_.current(owners))return false;*out=party_;if(const auto entry=party_.preparation_entry_.lock();entry&&entry->live)entry->handoff_ready=true;phase_=ChallengeFinalPhase::Taken;return true;}
    ChallengeFinalPhase phase()const{return phase_;}bool terminal()const{return phase_==ChallengeFinalPhase::PartyRequested||phase_==ChallengeFinalPhase::Taken||phase_==ChallengeFinalPhase::Failed;}
    const ScreenFade& fade()const{return fade_;}s16 remaining()const{return remaining_;}u32 updates()const{return updates_;}
    std::shared_ptr<const ChallengeNativeStageData> data()const{return data_;}
private:
    bool fail(std::string* error,const char* why){if(request_)request_->live=false;phase_=ChallengeFinalPhase::Failed;party_={};if(error&&error->empty())*error=std::string("Challenge final setup: ")+why;return false;}
    ChallengeFinalPhase phase_=ChallengeFinalPhase::Closed;std::array<ChallengeSetupChoice,2> choices_;unsigned count_=0,selected_=0;ChallengeDifficultyProfile difficulty_;ChallengeRandomForceProfile random_;ChallengeStageRngProfile stages_;ChallengeAiForceProfile ai_;ChallengeFinalizationProfile profile_;
    std::weak_ptr<challenge_choice_detail::Batch> selection_;SourceByteRngSnapshot rng_owner_;ChallengeInitialPartyAssembly assembly_;ChallengeInitialPartyCandidate party_;std::shared_ptr<challenge_stage_data_detail::Request> request_;std::shared_ptr<const ChallengeNativeStageData> data_;ScreenFade fade_;s16 remaining_=0;u32 updates_=0;
};
} // namespace gf
