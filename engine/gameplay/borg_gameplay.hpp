#pragma once
#include <optional>
#include <span>
#include <type_traits>
#include "engine/assets/borg_gameplay_profile.hpp"
#include "engine/gameplay/weapon_stock_mode_zero.hpp"

namespace gf {
enum class BorgBattleEventKind:u8 {Started,Marker,StockSpent,StockRefused,StockRefilled,Emitted,EmitRefused,Interrupted,Completed,Hit,Retired,
    ModifierApplied,ModifierRefused,ModifierExpired,PullQueued,PullApplied,PullRefused,PullCancelled,ButtonRelease,Repeated,RepeatStockRefused,
    MeleeArmed,MeleeRefused,MeleeRetired,HeldFireStarted,HeldFireCadence,HeldFireStopped,BlastTriggered,BlastHit,ProjectileContact,
    ProjectileLanded,Detonation,StockReturned,GuidanceBound,GuidanceEnded,GuardArmed,GuardBlocked,GuardBroken,GuardRetired,
    StageImpact,EmitterDeployed,EmitterDeployRefused,EmitterFired,EmitterFireRefused,EmitterRetired,
    AnchorPlaced,AnchorRefused,RewindQueued,RewindReached,RewindCompleted,RewindStopped,AnchorPathSealed,RewindRefused,AnchorPlanted,
    Tap,DoubleTap,AnchorFallback,SocketLaunch,SocketRefused,AimCommitted,BombProximity,BombFuseCommitted,
    BlastChildren,BlastChildrenRefused,SmokeCreated,SmokeRefused,InkApplied,PowerFieldPlaced,PowerFieldRefused,ShotPowered,
    SlowFieldPlaced,SlowFieldRefused,FieldGuidanceBroken,FieldPlaced,FieldRefused,FieldAnchorLost,
    AccuracyApplied,AccuracyAffected,FlightChildren,FlightChildrenRefused,ImpactChildren,ImpactChildrenRefused,
    ImpactTrailStarted,ImpactTrailRefused,ImpactHazardStarted,ImpactHazardPlaced,ImpactHazardRefused,
    ExtraEmitted,ExtraEmitRefused,WalkPulseCreated,WalkPulseRefused,FieldShotPulled,FieldActorPulled,FieldActorPullRefused,
    FenceRecoilApplied,FenceRecoilRefused,EntryStockRefused,FieldReplaced,LaunchSound,ReleaseSound,ImpactSound,
    ActionSound,ContactSound};
enum class BorgBattlePullRefusal:u16 {AlreadyQueued=1,OwnerBusy,NoTravel,SupportUnavailable,TerrainOrBody,RoundEnded};
inline const char* borg_battle_pull_reason(u16 value){
    switch(BorgBattlePullRefusal(value)){
    case BorgBattlePullRefusal::AlreadyQueued:return "already-queued";
    case BorgBattlePullRefusal::OwnerBusy:return "owner-busy";
    case BorgBattlePullRefusal::NoTravel:return "no-travel";
    case BorgBattlePullRefusal::SupportUnavailable:return "support-unavailable";
    case BorgBattlePullRefusal::TerrainOrBody:return "terrain-or-body";
    case BorgBattlePullRefusal::RoundEnded:return "round-ended";
    }
    return "unknown";
}
struct BorgBattleEvent {
    BorgBattleEventKind kind{};
    u32 tick=0,object=0;
    u8 actor=0,move=255,recipient=255;
    u16 frame=0,value=0;
};
struct BorgBattleInputs {
    // Button bits for gameplay B/X, independent of native pad assignments.
    static constexpr u8 b=1,x=2,mask=b|x;
    static constexpr u8 button(BorgBattleButton n){return u8(1u<<unsigned(n));}
    u8 pressed=0,held=0,released=0;
    bool valid()const{return ((pressed|held|released)&~mask)==0&&(held&released)==0;}
    bool operator==(const BorgBattleInputs&)const=default;
};
struct BorgBattleActionState {
    static constexpr u8 none=255;
    u8 move=none;u16 frame=0;
    u8 cycle=0,armed=0;
    std::array<u16,2> hold_ticks{};
    std::array<u8,2> tap_remaining{};
    std::array<u32,kBorgBattleMoves> spin{}; // per move, so switching weapons cannot transfer spin
    u16 fire_gap=0;
    u16 release_wait=0;
    bool release_pending=false;
    std::array<WeaponStockModeZeroState,kBorgBattleStocks> stocks{};
    void reset(const BorgGameplayProfile& p){
        *this={};
        for(unsigned i=0;i<p.stocks.size();++i){
            const auto& s=p.stocks[i];
            stocks[i]={s16(s.capacity),s16(s.capacity),s16(s.refill_ticks),0};
        }
    }
    void cancel_action(){move=none;frame=0;cycle=0;spin={};fire_gap=release_wait=0;release_pending=false;discard_input();}
    void discard_input(){armed=0;hold_ticks={};tap_remaining={};}
    static WeaponStockRefillMode stock_mode(const BorgBattleStockDefinition& spec){
        if(spec.add_on_debit)return WeaponStockRefillMode::AddOnDebit;
        return spec.refill_ticks?WeaponStockRefillMode::SeedOnDepletion:WeaponStockRefillMode::ClearClockOnDebit;
    }
    // Read-only admission probe. Only the scheduler advances clocks or
    // commits a debit at a release marker.
    bool stock_ready(const BorgGameplayProfile& profile,u8 channel,u8 amount)const{
        if(!amount)return true;
        if(channel>=profile.stocks.size()||channel>=stocks.size())return false;
        WeaponStockPlan check;
        return weapon_stock_plan(stocks[channel],stock_mode(profile.stocks[channel]),
                                 WeaponStockOperation::CheckOnly,amount,&check)&&check.accepted;
    }
    // Called after movement, before contact. Prior accepted damage interrupts
    // before this visit's marker; new contact is delivered by the world later.
    // A paid release is not rolled back on pool refusal.
    template<class Emit,class Trace,class Pair=std::nullptr_t>
    void visit(const BorgGameplayProfile& p,const BorgBattleInputs& input,bool grounded,bool interrupted,Emit emit,Trace trace,
               std::optional<f32> entry_distance={},u8 available_anchors=0,Pair pair=nullptr,
               std::optional<f32> aim_error={}){
        for(unsigned i=0;i<p.stocks.size();++i){
            if(!p.stocks[i].refill_ticks)continue;
            auto& s=stocks[i];const auto old=s.current;
            if(p.stocks[i].add_on_debit)weapon_stock_mode_one_refill(s,s16(p.stocks[i].capacity),1,false,&s);
            else weapon_stock_mode_zero_refill(s,s16(p.stocks[i].capacity),1,false,&s);
            if(s.current!=old)trace(BorgBattleEventKind::StockRefilled,none,0,0,u16(s.current));
        }
        std::array<std::optional<u16>,2> released;
        if(interrupted)discard_input();
        else for(unsigned i=0;i<released.size();++i){
            const auto bit=u8(1u<<i);
            if(input.pressed&bit){hold_ticks[i]=0;armed|=bit;}
            if((input.released&armed&bit)!=0)released[i]=hold_ticks[i];
            if(!(input.held&bit)){hold_ticks[i]=0;armed&=u8(~bit);}
            else if((armed&bit)&&hold_ticks[i]<=kBorgBattleHoldTicks)++hold_ticks[i];
        }
        std::array<const BorgBattleMove*,2> gesture_spec{};
        for(const auto& candidate:p.moves)
            if(candidate.activation==BorgBattleActivation::Tap||candidate.activation==BorgBattleActivation::DoubleTap)
                gesture_spec[unsigned(candidate.button)]=&candidate;
        std::array<BorgBattleActivation,2> gestures{};
        if(!interrupted)for(unsigned i=0;i<gestures.size();++i)if(const auto* spec=gesture_spec[i]){
            auto& remaining=tap_remaining[i];
            if(released[i]&&*released[i]<=spec->maximum_hold){
                if(remaining){remaining=0;gestures[i]=BorgBattleActivation::DoubleTap;}
                else remaining=u8(spec->tap_window);
            }else if(remaining&&!--remaining)gestures[i]=BorgBattleActivation::Tap;
        }
        auto context_matches=[&](BorgBattleContext context){
            return context==BorgBattleContext::Any||(context==BorgBattleContext::Grounded)==grounded;
        };
        auto range_matches=[&](const BorgBattleMove& action){
            return !action.entry_range||(entry_distance&&*entry_distance<=action.entry_range);
        };
        auto launch=[&](u8 stock,u8 cost,u8 attack,std::span<const u8> extras={},bool return_pair=false,bool debit_sound=false){
            if constexpr(std::is_same_v<Pair,std::nullptr_t>){
                if(return_pair){trace(BorgBattleEventKind::EmitRefused,move,frame,0,0);return;}
            }
            if(const auto& link=p.projectiles[attack].rewind_hit;link&&!(available_anchors&(1u<<link->set))){
                if(link->fallback==255){trace(BorgBattleEventKind::AnchorRefused,move,frame,0,link->set);return;}
                attack=link->fallback;trace(BorgBattleEventKind::AnchorFallback,move,frame,0,attack);
            }
            const auto& projectile=p.projectiles[attack];
            if(cost){
                auto& s=stocks[stock];WeaponStockPlan spend;
                const auto mode=stock_mode(p.stocks[stock]);
                if(!weapon_stock_plan(s,mode,WeaponStockOperation::Debit,cost,&spend)||!spend.accepted){
                    trace(BorgBattleEventKind::StockRefused,move,frame,0,u16(s.current));return;
                }
                s=spend.state;trace(BorgBattleEventKind::StockSpent,move,frame,0,u16(s.current));
                if(debit_sound)trace(BorgBattleEventKind::ReleaseSound,move,frame,0,0);
            }
            const u8 return_stock=return_pair||projectile.stock_return==BorgProjectileReturn::None?255:stock;
            const u32 first=emit(projectile,move,return_stock);
            trace(first?BorgBattleEventKind::Emitted:BorgBattleEventKind::EmitRefused,move,frame,first,first?projectile.count:0);
            for(auto extra:extras){
                const auto& followup=p.projectiles[extra];
                const u32 second=emit(followup,move,255);
                trace(second?BorgBattleEventKind::ExtraEmitted:BorgBattleEventKind::ExtraEmitRefused,
                      move,frame,second,second?followup.count:0);
                if constexpr(!std::is_same_v<Pair,std::nullptr_t>){
                    if(return_pair)pair(move,stock,first,second);
                }
            }
        };
        const auto eligible_fire=[&](const BorgBattleMove& action){
            return !interrupted&&context_matches(action.context)&&range_matches(action)&&
                   (input.held&BorgBattleInputs::button(action.button));
        };
        const auto has_entry_stock=[&](const BorgBattleMove& action,u8 index){
            if(!action.entry_stock)return true;
            const auto& gate=*action.entry_stock;
            if(stock_ready(p,gate.stock,gate.minimum))return true;
            trace(BorgBattleEventKind::EntryStockRefused,index,0,0,u16(stocks[gate.stock].current));
            return false;
        };
        const auto max_spin=[](const BorgBattleHeldFire& spec){return u32(spec.warmup)*std::max<u32>(1,spec.cooldown);};
        const auto spin_readout=[&](unsigned i){return u16(1000*spin[i]/max_spin(*p.moves[i].held_fire));};
        for(unsigned i=0;i<p.moves.size();++i){
            const auto& action=p.moves[i];
            if(!action.held_fire||!spin[i])continue;
            if(i==move&&eligible_fire(action)&&!(input.pressed&BorgBattleInputs::button(action.button)))continue;
            const auto& spec=*action.held_fire;
            spin[i]=spec.cooldown&&spin[i]>spec.warmup?spin[i]-spec.warmup:0;
        }
        if(move!=none&&p.moves[move].held_fire){
            const auto& action=p.moves[move];const auto& spec=*action.held_fire;
            const bool fresh=(input.pressed&BorgBattleInputs::button(action.button))!=0;
            if(eligible_fire(action)&&!fresh){
                frame=u16((frame+1)%action.duration); // pose clock only; not a gameplay release cue
                const auto maximum=max_spin(spec);
                spin[move]=std::min(maximum,spin[move]+std::max<u32>(1,spec.cooldown));
                const auto interval=u16(spec.slow_interval-u32(spec.slow_interval-spec.fast_interval)*spin[move]/maximum);
                if(++fire_gap>=interval){
                    fire_gap=0; // refuse or emit, never accumulate a later burst
                    trace(BorgBattleEventKind::HeldFireCadence,move,frame,0,interval);
                    launch(spec.stock,spec.cost,spec.projectile);
                }
                return;
            }
            trace(BorgBattleEventKind::HeldFireStopped,move,frame,0,spin_readout(move));
            if(interrupted)trace(BorgBattleEventKind::Interrupted,move,frame,0,0);
            move=none;frame=0;cycle=0;fire_gap=0;
            // A new press can arrive without a separate simulation visit
            // for its preceding release. End the old cadence before reselecting.
            if(!fresh||interrupted)return;
        }
        if(move!=none){
            const auto& action=p.moves[move];
            const auto aims=[&](){
                return aim_error&&std::isfinite(*aim_error)&&std::abs(*aim_error)<=action.release_gate->half_angle;
            };
            const auto pay_release=[&](const BorgBattleRelease& release){
                launch(release.stock,release.cost,release.projectile,
                       {release.followups.data(),release.count},release.return_pair,!release.debit_sound.empty());
            };
            if(release_pending){
                if(interrupted){
                    trace(BorgBattleEventKind::Interrupted,move,frame,0,0);cancel_action();
                }else if(aims()||++release_wait>=action.release_gate->timeout){
                    release_pending=false;release_wait=0;pay_release(action.releases.front());
                }
                return;
            }
            if(interrupted||frame+1>=action.duration){
                if(!interrupted&&action.hold_cycles>cycle&&context_matches(action.context)&&range_matches(action)&&
                   (input.held&BorgBattleInputs::button(action.button))&&has_entry_stock(action,move)){
                    // Availability is a read-only repeat gate. The later
                    // marker still owns the attempt to debit and allocate.
                    const auto& release=action.releases.front();WeaponStockPlan available;
                    if(weapon_stock_plan(stocks[release.stock],stock_mode(p.stocks[release.stock]),
                        WeaponStockOperation::CheckOnly,release.cost,&available)&&available.accepted){
                        frame=0;++cycle;trace(BorgBattleEventKind::Repeated,move,frame,0,cycle);
                        if(action.aim_lock==frame)trace(BorgBattleEventKind::AimCommitted,move,frame,0,0);
                        return;
                    }
                    trace(BorgBattleEventKind::RepeatStockRefused,move,frame,0,u16(stocks[release.stock].current));
                }
                trace(interrupted?BorgBattleEventKind::Interrupted:BorgBattleEventKind::Completed,move,frame,0,0);
                move=none;frame=0;cycle=0;
            }else{
                ++frame;
                if(action.aim_lock==frame)trace(BorgBattleEventKind::AimCommitted,move,frame,0,0);
                for(const auto& release:action.releases)if(release.frame==frame){
                    trace(BorgBattleEventKind::Marker,move,frame,0,0);
                    if(action.release_gate&&!aims())release_pending=true;
                    else pay_release(release);
                }
            }
            return;
        }
        if(interrupted)return;
        for(unsigned i=0;i<p.moves.size();++i){
            const auto& m=p.moves[i];
            const auto charge=released[unsigned(m.button)];
            const bool activated=m.activation==BorgBattleActivation::Press?bool(input.pressed&BorgBattleInputs::button(m.button)):
                m.activation==BorgBattleActivation::ReleaseWindow?charge&&*charge>=m.minimum_hold&&*charge<=m.maximum_hold:
                gestures[unsigned(m.button)]==m.activation;
            if(!activated||!context_matches(m.context)||!range_matches(m)||(m.held_fire&&!eligible_fire(m)))continue;
            if(!has_entry_stock(m,u8(i)))continue;
            if(move==none||m.priority>p.moves[move].priority)move=u8(i);
        }
        if(move!=none){
            const auto& chosen=p.moves[move];
            if(chosen.activation==BorgBattleActivation::ReleaseWindow)
                trace(BorgBattleEventKind::ButtonRelease,move,0,0,*released[unsigned(chosen.button)]);
            else if(chosen.activation==BorgBattleActivation::Tap||chosen.activation==BorgBattleActivation::DoubleTap)
                trace(chosen.activation==BorgBattleActivation::Tap?BorgBattleEventKind::Tap:BorgBattleEventKind::DoubleTap,
                    move,0,0,chosen.tap_window);
            cycle=1;trace(BorgBattleEventKind::Started,move,frame,0,0);
            if(chosen.aim_lock==frame)trace(BorgBattleEventKind::AimCommitted,move,frame,0,0);
            if(chosen.held_fire)trace(BorgBattleEventKind::HeldFireStarted,move,0,0,spin_readout(move));
        }
    }
};
}
