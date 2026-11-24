#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <variant>
#include "engine/assets/portable_battle_keys.hpp"
#include "engine/gameplay/force_inventory.hpp"

namespace gf {

struct PortableBattleAuthoredRoster {
    std::vector<PortableBattleBorg> members;
    bool operator==(const PortableBattleAuthoredRoster&)const=default;
};
// A selector only: it confers no authority and cannot supply members.
struct PortableBattleSavedForce {
    u32 index=kForceCount; // Default construction is not a selection.
    bool operator==(const PortableBattleSavedForce&)const=default;
};
enum class PortableBattleUse {Campaign,Practice,Preview};
enum class PortableBattleSide {Player,Enemy};

struct PortableBattleRequest {
    // Identifies the authored encounter, even when a Story node reuses it.
    std::string content_id,encounter_id;
    PortableBattleStage stage;
    std::vector<PortableBattleSpawn> player_spawns,enemy_spawns;
    std::variant<PortableBattleAuthoredRoster,PortableBattleSavedForce> player;
    std::vector<PortableBattleBorg> enemies;
    PortableBattleAI ai;
    PortableBattleRules rules;
    u32 tick_limit=0; // Zero is untimed.
    PortableBattleUse use=PortableBattleUse::Campaign;
    bool operator==(const PortableBattleRequest&)const=default;
};

enum class PortableBattleStatus {Ready,Malformed,Missing,Unsupported,Denied,Unavailable,Busy,Stale};
struct PortableBattleCheck {
    PortableBattleStatus status=PortableBattleStatus::Unavailable;
    std::string reason;
    explicit operator bool()const{return status==PortableBattleStatus::Ready;}
};

struct PortableBattleForceSnapshot {
    // Opaque trusted selection version. It must change on replacement of a
    // saved copy or selection, even if the visible package/variant is unchanged.
    u64 revision=0;
    std::vector<PortableBattleBorg> members;
    bool operator==(const PortableBattleForceSnapshot&)const=default;
};

// Supplied by the engine/host, not the manifest or UI. stage validates the
// stage and its marker IDs, floor support and spacing together. borg validates
// gameplay admission of that exact variant (no default lookup). saved_force
// supplies the entire ordered Force only after checking current selection,
// permissions, budget and every owned/edited copy's mode eligibility.
// The engine/host must keep the resolved stage and Borg content pinned for its
// session. These checks must not publish actors or write a save.
struct PortableBattleResolver {
    std::function<PortableBattleCheck(const PortableBattleStage&,std::span<const PortableBattleSpawn>,
                                      std::span<const PortableBattleSpawn>)> stage;
    std::function<PortableBattleCheck(const PortableBattleBorg&,PortableBattleSide)> borg;
    std::function<PortableBattleCheck(PortableBattleSavedForce,PortableBattleUse,PortableBattleForceSnapshot*)> saved_force;
    // Optional further restriction for a particular backend (e.g. a host
    // without timed battles). Cannot expand the adapter's capabilities.
    std::function<PortableBattleCheck(const PortableBattleRequest&)> capability;
};

struct PortableBattlePrepared {
    PortableBattleRequest request;
    std::optional<PortableBattleForceSnapshot> saved_force;
    const std::vector<PortableBattleBorg>& players()const{
        return saved_force?saved_force->members:std::get<PortableBattleAuthoredRoster>(request.player).members;
    }
    bool operator==(const PortableBattlePrepared&)const=default;
};

// This only validates and pins request data; it never creates actors, starts
// a battle, grants a copy or writes a save. A refusal leaves *out untouched.
inline PortableBattleCheck preflight_portable_battle_request(const PortableBattleRequest& request,
                                                              const PortableBattleResolver& resolver,
                                                              PortableBattlePrepared* out){
    using Status=PortableBattleStatus;
    const auto valid_spawns=[](const auto& spawns){
        return !spawns.empty()&&spawns.size()<=kPortableBattleRosterLimit&&
            std::all_of(spawns.begin(),spawns.end(),[](const auto& spawn){return borg_catalog_variant_id_valid(spawn.id);});
    };
    const auto valid_roster=[](const auto& roster){
        return !roster.empty()&&roster.size()<=kPortableBattleRosterLimit&&
            std::all_of(roster.begin(),roster.end(),portable_battle_borg_valid);
    };
    if(!out||!borg_package_id_valid(request.content_id)||!borg_catalog_variant_id_valid(request.encounter_id)||
       !borg_package_id_valid(request.stage.id)||!valid_spawns(request.player_spawns)||!valid_spawns(request.enemy_spawns)||
       !borg_catalog_variant_id_valid(request.ai.key)||!borg_catalog_variant_id_valid(request.rules.key)||
       request.tick_limit>kPortableBattleTickLimit||!valid_roster(request.enemies)||
       (request.use!=PortableBattleUse::Campaign&&request.use!=PortableBattleUse::Practice&&request.use!=PortableBattleUse::Preview))
        return {Status::Malformed,"invalid portable battle request"};
    const auto* authored=std::get_if<PortableBattleAuthoredRoster>(&request.player);
    const auto* saved=std::get_if<PortableBattleSavedForce>(&request.player);
    if((authored&&!valid_roster(authored->members))||(saved&&saved->index>=kForceCount)||(!authored&&!saved))
        return {Status::Malformed,"invalid player roster source"};
    if(request.ai.key!=kPortableBattleDuelAI||request.rules.key!=kPortableBattleDefeatOpponent)
        return {Status::Unsupported,"unsupported battle AI or rule"};
    if(request.enemies.size()!=1||(authored&&authored->members.size()!=1))
        return {Status::Unsupported,"duel adapter requires exactly one actor per side"};
    if(request.player_spawns.size()!=1||request.enemy_spawns.size()!=1)
        return {Status::Unsupported,"duel adapter requires exactly one spawn marker per side"};
    if(request.player_spawns.front()==request.enemy_spawns.front())
        return {Status::Unsupported,"duel spawn markers must be distinct"};
    if(!resolver.stage||!resolver.borg||(saved&&!resolver.saved_force))
        return {Status::Unavailable,"portable battle resolver is incomplete"};
    PortableBattlePrepared next{request,{}};
    if(saved){
        auto& force=next.saved_force.emplace();
        if(const auto check=resolver.saved_force(*saved,request.use,&force);!check)return check;
        if(force.members.empty())return {Status::Denied,"selected Force is empty"};
        if(force.members.size()!=1)return {Status::Unsupported,"duel adapter cannot run the full selected Force"};
        if(!valid_roster(force.members))return {Status::Malformed,"selected Force has no exact Borg variant"};
    }
    if(const auto check=resolver.stage(request.stage,request.player_spawns,request.enemy_spawns);!check)return check;
    if(const auto check=resolver.borg(next.players().front(),PortableBattleSide::Player);!check)return check;
    if(const auto check=resolver.borg(request.enemies.front(),PortableBattleSide::Enemy);!check)return check;
    if(resolver.capability)
        if(const auto check=resolver.capability(request);!check)return check;
    *out=std::move(next);
    return {Status::Ready,{}};
}

enum class PortableBattleOutcome {Win,Loss};

// An opaque in-memory correlation receipt, not evidence a battle ran. Each
// acceptance gets a new identity, even across identical requests or sessions.
class PortableBattleReceipt {
public:
    // The pinned request snapshot can still be inspected after cancellation
    // or completion. Only its session can decide whether it is still active.
    const PortableBattlePrepared* accepted()const{return accepted_.get();}

private:
    friend class PortableBattleSession;
    std::shared_ptr<const PortableBattlePrepared> accepted_;
};

// For an engine-owned launcher. UI callbacks can select/preview/cancel; only
// the eventual runner should report an outcome for its receipt.
class PortableBattleSession {
public:
    PortableBattleSession()=default;
    PortableBattleSession(const PortableBattleSession&)=delete;
    PortableBattleSession& operator=(const PortableBattleSession&)=delete;

    PortableBattleCheck prepare(const PortableBattleRequest& request,const PortableBattleResolver& resolver){
        if(active_)return {PortableBattleStatus::Busy,"a request is already accepted"};
        pending_.reset();
        PortableBattlePrepared next;
        const auto check=preflight_portable_battle_request(request,resolver,&next);
        if(check)pending_=std::move(next);
        return check;
    }

    PortableBattleCheck accept(const PortableBattleResolver& resolver,PortableBattleReceipt* out){
        if(active_)return {PortableBattleStatus::Busy,"a request is already accepted"};
        if(!pending_||!out)return {PortableBattleStatus::Denied,"no prepared request or receipt destination"};
        auto prepared=std::move(*pending_);
        pending_.reset();
        PortableBattlePrepared current;
        const auto check=preflight_portable_battle_request(prepared.request,resolver,&current);
        if(!check)return check;
        if(current!=prepared)return {PortableBattleStatus::Stale,"selected Force changed after preparation"};
        active_=std::make_shared<const PortableBattlePrepared>(std::move(current));
        out->accepted_=active_;
        return {PortableBattleStatus::Ready,{}};
    }

    bool complete(const PortableBattleReceipt& receipt,PortableBattleOutcome outcome){
        if(!active_||receipt.accepted_!=active_||active_->request.use!=PortableBattleUse::Campaign||
           (outcome!=PortableBattleOutcome::Win&&outcome!=PortableBattleOutcome::Loss))return false;
        active_.reset();
        return true;
    }

    void cancel(){pending_.reset();active_.reset();}

    bool cancel(const PortableBattleReceipt& receipt){
        if(!active_||receipt.accepted_!=active_)return false;
        cancel();
        return true;
    }

private:
    std::optional<PortableBattlePrepared> pending_;
    std::shared_ptr<const PortableBattlePrepared> active_;
};

} // namespace gf
