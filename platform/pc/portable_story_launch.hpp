#pragma once

#include "engine/gameplay/portable_story.hpp"
#include "platform/pc/portable_battle_host.hpp"
#include "platform/pc/portable_challenge_scan.hpp"
#include "platform/pc/portable_story_scan.hpp"

namespace gf::pc {

struct PortableStorySelection {
    std::string id, save_root;
    AccountSaveKey save;
    PortableBattleSavedForce force;
};

struct PortableStoryLaunchRoots {
    std::filesystem::path stories = "stories", challenges = "challenges", borgs = "borgs", stages = "stages";
};

namespace portable_story_launch_detail {
struct State {
    std::unique_ptr<PortableStoryRun> run;
    BorgPackageCatalog borgs;
    std::unique_ptr<BorgForceBrowseSnapshot> selected_save;
    PortableStoryEntry entry;
    std::filesystem::path story_root, borg_root, stage_root;
};
}

class PortableStoryLaunch;

// One attempt: the shared host validates and owns the accepted receipt, this
// object correlates that receipt with its Story and map return. Keep the
// owning renderer alive through the duel; no GL resources are re-bound here.
class PortableStoryBattle {
public:
    PortableStoryBattle(const PortableStoryBattle&) = delete;
    PortableStoryBattle& operator=(const PortableStoryBattle&) = delete;
    ~PortableStoryBattle() { cancel(); }

    PortableBattleHost* admitted_host() { return active_ ? &host_ : nullptr; }

    // Only the trusted shared duel runner supplies a report. A malformed
    // report leaves the valid attempt active so it can be cancelled cleanly.
    PortableBattleCheck finish(const PortableBattleHostReport& report) {
        using Status = PortableBattleStatus;
        if(!active_) return {Status::Stale, "Story battle attempt is no longer active"};
        if(!state_->selected_save->current()) {
            cancel();
            return {Status::Stale, "selected save changed during the Story battle"};
        }
        if(!host_.finish(*state_->run, report))
            return {Status::Malformed, "invalid or uncorrelated Story battle report"};
        active_ = false;
        return {Status::Ready, {}};
    }

    bool cancel() {
        if(!active_ || !host_.finish(*state_->run, {PortableDuelEnd::Cancelled, 0})) return false;
        active_ = false;
        return true;
    }

private:
    friend class PortableStoryLaunch;
    PortableStoryBattle(std::shared_ptr<portable_story_launch_detail::State> state, RenderEngine& render,
                        PortableForcePolicy policy):
        state_(std::move(state)), host_(state_->borgs, *state_->selected_save, view_, render, policy,
                                       state_->borg_root, state_->stage_root) {}

    std::shared_ptr<portable_story_launch_detail::State> state_;
    NativeBattleMvpView view_;
    PortableBattleHost host_;
    bool active_ = false;
};

class PortableStoryLaunch {
public:
    PortableStoryLaunch(const PortableStoryLaunch&) = delete;
    PortableStoryLaunch& operator=(const PortableStoryLaunch&) = delete;

    // Pin an exact Story and one explicit account/save/Force for the run.
    // Opening the selection does not publish actors, mutate progress or
    // require optional map art. The shared battle adapter admits each duel.
    static PortableBattleCheck open(const PortableStorySelection& choice, ForceCatalog retail,
                                    std::unique_ptr<PortableStoryLaunch>* out,
                                    const PortableStoryLaunchRoots& roots = {}) {
        using Status = PortableBattleStatus;
        if(!out || !borg_package_id_valid(choice.id) || choice.save_root.empty() || !choice.save.account ||
           !choice.save.save || choice.force.index >= kForceCount)
            return {Status::Malformed, "explicit Story, account, save and Force are required"};

        const auto stories = scan_portable_stories(roots.stories);
        if(stories.status == PortableStoryScanStatus::NoDirectory)
            return {Status::Missing, "stories directory is missing"};
        if(stories.status != PortableStoryScanStatus::Complete)
            return {Status::Unavailable, "Story discovery unavailable or over budget"};
        const auto* entry = stories.find(choice.id);
        if(!entry) {
            for(const auto& issue : stories.issues)
                if(issue.id == choice.id && issue.kind == PortableStoryIssueKind::DuplicateId)
                    return {Status::Malformed, "duplicate Story ID: " + choice.id};
            return {Status::Missing, "Story ID is missing: " + choice.id};
        }

        std::shared_ptr<const PortableChallengeCatalog> challenges;
        if(std::any_of(entry->manifest.nodes.begin(), entry->manifest.nodes.end(),
                       [](const auto& node) { return node.encounter.has_value(); }))
            challenges = std::make_shared<const PortableChallengeCatalog>(scan_portable_challenges(roots.challenges));
        auto state = std::make_shared<portable_story_launch_detail::State>();
        if(const auto check = PortableStoryRun::bind(entry->manifest, std::move(challenges), choice.force, &state->run);
           !check) return check;
        state->borgs = scan_borg_packages(roots.borgs);
        state->entry = *entry;
        state->story_root = roots.stories;
        state->borg_root = roots.borgs;
        state->stage_root = roots.stages;
        std::string error;
        state->selected_save = BorgForceBrowseSnapshot::open(choice.save_root, choice.save, state->borg_root,
                                                             state->borgs, std::move(retail), &error);
        if(!state->selected_save) return {Status::Unavailable, std::move(error)};
        *out = std::unique_ptr<PortableStoryLaunch>(new PortableStoryLaunch(std::move(state)));
        return {Status::Ready, {}};
    }

    const StoryNode& current() const { return state_->run->current(); }
    StoryMapProjection map_projection() const { return state_->run->map_projection(); }
    bool load_map_art(TplImage* out, std::string* error) const {
        return load_portable_story_map_art(std::span(&state_->story_root, 1), state_->entry, out, error);
    }

    PortableBattleCheck prepare(RenderEngine& battle_renderer, std::unique_ptr<PortableStoryBattle>* out,
                                PortableForcePolicy policy = {}) {
        using Status = PortableBattleStatus;
        if(!out) return {Status::Malformed, "missing Story battle destination"};
        if(*out && (*out)->admitted_host()) return {Status::Busy, "destination already holds an active battle"};
        auto attempt = std::unique_ptr<PortableStoryBattle>(new PortableStoryBattle(state_, battle_renderer, policy));
        if(const auto check = attempt->host_.admit(*state_->run); !check) return check;
        attempt->active_ = true;
        *out = std::move(attempt);
        return {Status::Ready, {}};
    }

private:
    explicit PortableStoryLaunch(std::shared_ptr<portable_story_launch_detail::State> state):
        state_(std::move(state)) {}

    std::shared_ptr<portable_story_launch_detail::State> state_;
};

} // namespace gf::pc
