#pragma once

#include "engine/assets/battle_roster_preset.hpp"
#include "engine/gameplay/native_battle_mvp.hpp"

namespace gf {

inline constexpr u32 kBattleRosterTicksPerSecond=60;

// Standalone session rules only. Native KOs take precedence over the
// optional timeout. No session result can affect inventory or progression.
class BattleRosterSeries {
public:
    explicit BattleRosterSeries(u8 goal,u16 round_seconds=0):goal_(goal),round_seconds_(round_seconds){}

    bool enabled()const{
        return (goal_||round_seconds_)&&goal_<=kBattleRosterSeriesMaxWins&&round_seconds_<=kBattleRosterMaxRoundSeconds;
    }
    bool keeps_score()const{return enabled()&&goal_!=0;}
    u8 goal()const{return goal_;}
    const auto& wins()const{return wins_;}
    u32 tick_limit()const{return u32(round_seconds_)*kBattleRosterTicksPerSecond;}
    u32 remaining_ticks(const NativeBattleMvp& world)const{
        const u32 limit=tick_limit();
        return owns_round(world)&&world.tick()<limit?limit-world.tick():0;
    }
    BattleMvpResult outcome()const{return outcome_;}
    bool ended()const{return outcome_!=BattleMvpResult::Fighting;}
    bool timed_out()const{return timed_out_;}
    u8 winner()const{
        if(keeps_score())for(u8 i=0;i<2;++i)if(wins_[i]>=goal_)return i;
        return 255;
    }
    bool owns_round(const NativeBattleMvp& world)const{
        return enabled()&&owner_==&world&&world.owns_motion_scope(round_);
    }
    bool open_round(const NativeBattleMvp& world){
        if(!enabled()||!world.ready()||world.actor_count()<3||!world.config().local_aim_cycle||
           world.tick()||world.result()!=BattleMvpResult::Fighting||(owner_&&owner_!=&world)||owns_round(world))
            return false;
        if(winner()!=255)wins_={}; // reset only when starting a rematch after the series
        owner_=&world;round_=world.presentation_scope();
        outcome_=BattleMvpResult::Fighting;timed_out_=false;
        return true;
    }
    bool observe(const NativeBattleMvp& world){
        if(!owns_round(world)||!world.tick()||ended())return false;
        outcome_=world.result();
        if(outcome_==BattleMvpResult::Fighting){
            const auto limit=tick_limit();
            if(!limit||world.tick()<limit)return false;
            timed_out_=true;outcome_=vitality_result(world);
        }
        if(keeps_score()){
            if(outcome_==BattleMvpResult::GRedWins)++wins_[0];
            if(outcome_==BattleMvpResult::SasukeWins)++wins_[1];
        }
        return true; // a draw is a completed round without a point
    }
private:
    static BattleMvpResult vitality_result(const NativeBattleMvp& world){
        std::array<u64,2> hp{},capacity{};
        for(unsigned i=0;i<world.actor_count();++i){
            const auto& actor=world.actors()[i];
            hp[actor.team]+=actor.health;
            capacity[actor.team]+=world.maximum_health(i);
        }
        // Compare exact fractions, retaining defeated fighters in the
        // denominator. Raw HP and roster size cannot break a tie.
        const u64 a=hp[0]*capacity[1],b=hp[1]*capacity[0];
        return a==b?BattleMvpResult::Draw:a>b?BattleMvpResult::GRedWins:BattleMvpResult::SasukeWins;
    }

    u8 goal_=0;
    u16 round_seconds_=0;
    std::array<u8,2> wins_{};
    const NativeBattleMvp* owner_=nullptr;
    std::weak_ptr<const NativeBorgMotionScope> round_;
    BattleMvpResult outcome_=BattleMvpResult::Fighting;
    bool timed_out_=false;
};

} // namespace gf
