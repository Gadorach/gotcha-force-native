#pragma once
#include <memory>

#include "engine/gameplay/portable_challenge.hpp"
#include "engine/gameplay/story_map_projection.hpp"

namespace gf {

// Engine-owned progression. A run is constructed only after graph and named
// dependency validation. It shares the engine's immutable Challenge snapshot;
// merely constructing it does not preflight an encounter or start a battle.
class PortableStoryRun {
public:
    PortableStoryRun(const PortableStoryRun&) = delete;
    PortableStoryRun& operator=(const PortableStoryRun&) = delete;

    static PortableBattleCheck bind(StoryManifest manifest,
                                   std::shared_ptr<const PortableChallengeCatalog> challenges,
                                   PortableBattleSavedForce selected_force,
                                   std::unique_ptr<PortableStoryRun>* out) {
        if(!out) return {PortableBattleStatus::Malformed, "missing Story run destination"};
        story_detail::GraphLinks links;
        std::string error;
        if(!story_detail::link_graph(manifest, &links, &error))
            return {PortableBattleStatus::Malformed, std::move(error)};
        for(const auto& node : manifest.nodes) {
            if(!node.encounter) continue;
            if(!challenges || (challenges->status != PortableChallengeScanStatus::Complete &&
                               challenges->status != PortableChallengeScanStatus::NoDirectory))
                return {PortableBattleStatus::Unavailable, "Challenge catalogue is unavailable"};
            const auto* challenge = challenges->find(node.encounter->challenge_id);
            if(!challenge || !challenge->manifest.find(node.encounter->encounter_id))
                return {PortableBattleStatus::Missing, "Story node " + node.id + ": missing encounter " +
                    node.encounter->challenge_id + "/" + node.encounter->encounter_id};
        }
        *out = std::unique_ptr<PortableStoryRun>(
            new PortableStoryRun(std::move(manifest), links, std::move(challenges), selected_force));
        return {PortableBattleStatus::Ready, {}};
    }

    const StoryNode& current() const { return manifest_.nodes[current_]; }

    StoryMapProjection map_projection() const {
        // Available means this node is open for preflight, not that the
        // current Force, stage and runtime have been admitted.
        return story_detail::map_projection(manifest_, current().id, !active_.accepted());
    }

    PortableBattleCheck prepare(const PortableBattleResolver& resolver) {
        if(active_.accepted()) return {PortableBattleStatus::Busy, "a Story battle is already accepted"};
        // Replacing a prepare must not leave an earlier pending request
        // acceptable if the new request cannot be resolved.
        session_.cancel();
        const auto& reference = current().encounter;
        if(!reference) return {PortableBattleStatus::Denied, "Story node is presentation only"};
        const auto* challenge = challenges_->find(reference->challenge_id);
        if(!challenge) return {PortableBattleStatus::Missing, "Story Challenge dependency is unavailable"};
        const auto request = portable_challenge_battle_request(
            challenge->manifest, reference->encounter_id, selected_force_, PortableBattleUse::Campaign);
        if(!request) return {PortableBattleStatus::Missing, "Story encounter dependency is unavailable"};
        return session_.prepare(*request, resolver);
    }

    PortableBattleCheck accept(const PortableBattleResolver& resolver, PortableBattleReceipt* out) {
        if(!out) return {PortableBattleStatus::Denied, "missing Story battle receipt destination"};
        const auto check = session_.accept(resolver, out);
        if(check) active_ = *out;
        return check;
    }

    // Only the trusted battle runner may report outcomes. A matching receipt
    // is single-use; an absent edge still consumes it without moving.
    bool complete(const PortableBattleReceipt& receipt, PortableBattleOutcome outcome) {
        if(!session_.complete(receipt, outcome)) return false;
        active_ = {};
        const auto next = links_.next[current_][outcome == PortableBattleOutcome::Win ? 0 : 1];
        if(next != story_detail::no_node) current_ = next;
        return true;
    }

    // An explicit command for this Story run clears either a prepared or an
    // accepted battle. Asynchronous accepted-battle cancellation must supply
    // its receipt so it cannot clear a newer acceptance.
    void cancel() {
        session_.cancel();
        active_ = {};
    }

    bool cancel(const PortableBattleReceipt& receipt) {
        if(!active_.accepted() || receipt.accepted() != active_.accepted()) return false;
        cancel();
        return true;
    }

private:
    PortableStoryRun(StoryManifest manifest, story_detail::GraphLinks links,
                     std::shared_ptr<const PortableChallengeCatalog> challenges, PortableBattleSavedForce selected_force):
        manifest_(std::move(manifest)), links_(links), challenges_(std::move(challenges)),
        selected_force_(selected_force), current_(links_.start) {}

    const StoryManifest manifest_;
    const story_detail::GraphLinks links_;
    const std::shared_ptr<const PortableChallengeCatalog> challenges_;
    const PortableBattleSavedForce selected_force_;
    std::size_t current_;
    PortableBattleSession session_;
    PortableBattleReceipt active_;
};

} // namespace gf
