#include <cmath>
#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/portable_story.hpp"

namespace {
void check(bool ok, const char* why) {
    if(!ok) {
        std::fprintf(stderr, "Portable Story: %s\n", why);
        std::abort();
    }
}

constexpr std::string_view story_text =
    "GFSTORY 1\nid=example:story\nname=Story\nstart=arrival\nmap-art=art/map.tpl\n"
    "node=ending|text.ending|1000|0|none|none\n"
    "node=arrival|text.arrival|240|370|example:trials|round-one\n"
    "node=retry|text.retry|0|1000|example:trials|round-one\n"
    "edge=arrival|win|retry\n"
    "edge=retry|win|ending\n"
    "edge=retry|loss|arrival\n";
constexpr std::string_view challenge_text =
    "GFCHALLENGE 1\nid=example:trials\nname=Trials\n"
    "encounter=other|example:workshop|east|west|duel-ai-v1|defeat-opponent|0\n"
    "enemy=other|example:other|normal\n"
    "encounter=round-one|example:workshop|west|east|duel-ai-v1|defeat-opponent|3600\n"
    "enemy=round-one|example:enemy|night\n";
}

int main() {
    using namespace gf;
    using Status = PortableBattleStatus;
    using Outcome = PortableBattleOutcome;
    const auto ready = PortableBattleCheck{Status::Ready, {}};
    StoryManifest story;
    PortableChallengeManifest challenge;
    check(story_read_manifest(story_text, &story) && portable_challenge_read_manifest(challenge_text, &challenge),
          "parse authored fixtures");
    auto challenges = std::make_shared<const PortableChallengeCatalog>(PortableChallengeCatalog{
        PortableChallengeScanStatus::Complete, {{"unrelated-folder", challenge}}, {}});
    const PortableBattleSavedForce selected_force{3};

    std::unique_ptr<PortableStoryRun> run, other;
    check(bool(PortableStoryRun::bind(story, challenges, selected_force, &run)) &&
          bool(PortableStoryRun::bind(story, challenges, selected_force, &other)), "bind separate Story sessions");
    auto* original = run.get();
    auto invalid = story;
    invalid.edges[1].to = "not-a-node";
    check(PortableStoryRun::bind(invalid, challenges, selected_force, &run).status == Status::Malformed &&
          run.get() == original, "bind validates programmatic graph and leaves existing run alone");
    invalid = story;
    invalid.nodes[2].encounter->encounter_id = "missing";
    check(PortableStoryRun::bind(invalid, challenges, selected_force, &run).status == Status::Missing &&
          run.get() == original, "resolve even a future node, with no fallback to another encounter");
    invalid.nodes[2].encounter->challenge_id = "example:absent";
    check(PortableStoryRun::bind(invalid, challenges, selected_force, &run).status == Status::Missing &&
          run.get() == original, "missing Challenge ID does not choose a folder");
    check(PortableStoryRun::bind(story, nullptr, selected_force, &run).status == Status::Unavailable &&
          run.get() == original, "unavailable Challenge catalogue");
    auto unsupported = challenge;
    unsupported.encounters.back().enemies.push_back({"example:second", "normal"});
    const auto unsupported_catalog = std::make_shared<const PortableChallengeCatalog>(PortableChallengeCatalog{
        PortableChallengeScanStatus::Complete, {{"unsupported", unsupported}}, {}});
    std::unique_ptr<PortableStoryRun> unsupported_run;
    check(bool(PortableStoryRun::bind(story, unsupported_catalog, selected_force, &unsupported_run)),
          "resolved graph is not a promise of battle admission");
    check(unsupported_run->prepare({}).status == Status::Unsupported &&
          unsupported_run->current().id == "arrival", "unsupported shared roster size stays on current node");

    const auto initial_map = run->map_projection();
    check(initial_map.story_id == story.id && initial_map.map_art == "art/map.tpl" && initial_map.nodes.size() == 3 &&
          initial_map.nodes[0].id == "ending" && initial_map.nodes[0].x == 1.0f && initial_map.nodes[0].y == 0.0f &&
          initial_map.nodes[1].label_key == "text.arrival" && std::abs(initial_map.nodes[1].x - 0.24f) < 1e-6f &&
          std::abs(initial_map.nodes[1].y - 0.37f) < 1e-6f &&
          !initial_map.nodes[0].selected && !initial_map.nodes[2].available &&
          initial_map.nodes[1].selected && initial_map.nodes[1].available, "map preserves order, normalized coordinates and status");

    u64 revision = 1;
    unsigned checks = 0;
    auto resolver = PortableBattleResolver{
        [&](const PortableBattleStage& stage, auto players, auto enemies) {
            ++checks;
            check(stage.id == "example:workshop" && players.size() == 1 && players[0].id == "west" &&
                  enemies.size() == 1 && enemies[0].id == "east", "preflight receives shared stage and spawn markers");
            return ready;
        },
        [&](const PortableBattleBorg& borg, PortableBattleSide side) {
            if(side == PortableBattleSide::Player)
                check(borg == PortableBattleBorg{"example:player", "normal"}, "preflight exact selected Borg");
            else
                check(borg == PortableBattleBorg{"example:enemy", "night"}, "preflight exact authored enemy");
            return ready;
        },
        [&](PortableBattleSavedForce force, PortableBattleUse, PortableBattleForceSnapshot* out) {
            check(force.index == selected_force.index, "preflight rechecks engine-selected Force");
            *out = {revision, {{"example:player", "normal"}}};
            return ready;
        },
        {}
    };
    PortableBattleReceipt receipt;
    check(run->accept(resolver, &receipt).status == Status::Denied &&
          !run->complete(receipt, Outcome::Win) && !run->cancel(receipt) && checks == 0,
          "browse and absent receipts never preflight, accept or apply progress");
    check(bool(run->prepare(resolver)) && run->current().id == "arrival" && checks == 1,
          "prepare does not advance or accept the node");
    run->cancel();
    check(run->accept(resolver, &receipt).status == Status::Denied && run->map_projection().nodes[1].available,
          "cancel pending prepare and leave node available");
    check(bool(run->prepare(resolver)), "prepare again");
    ++revision;
    check(run->accept(resolver, &receipt).status == Status::Stale && !receipt.accepted() &&
          !run->complete(receipt, Outcome::Win) && run->current().id == "arrival",
          "Force revision changes refuse acceptance and progression");

    auto begin = [&](PortableStoryRun& launch) {
        PortableBattleReceipt accepted;
        check(bool(launch.prepare(resolver)) && bool(launch.accept(resolver, &accepted)) &&
              accepted.accepted(), "accept a Story request");
        return accepted;
    };
    const auto expected = portable_challenge_battle_request(challenge, "round-one", selected_force);
    const auto foreign = begin(*other);
    receipt = begin(*run);
    check(expected && receipt.accepted()->request == *expected, "Story reused the exact Challenge request");
    check(!run->complete(foreign, Outcome::Win) && !run->cancel(foreign) &&
          !run->complete(receipt, static_cast<Outcome>(255)) &&
          run->prepare(resolver).status == Status::Busy && run->accept(resolver, nullptr).status == Status::Denied &&
          run->map_projection().nodes[1].selected && !run->map_projection().nodes[1].available,
          "wrong session and invalid outcomes leave the active receipt intact");
    const auto first = receipt;
    check(run->complete(first, Outcome::Loss) && run->current().id == "arrival" &&
          !run->complete(first, Outcome::Win), "missing edge still consumes the accepted receipt");
    receipt = begin(*run);
    check(run->cancel(receipt) && !run->complete(receipt, Outcome::Win) &&
          run->current().id == "arrival" && run->map_projection().nodes[1].available,
          "matching cancellation invalidates the accepted receipt without progress");

    PortableBattleSession practice;
    auto practice_request = *expected;
    practice_request.use = PortableBattleUse::Practice;
    PortableBattleReceipt practice_receipt;
    check(bool(practice.prepare(practice_request, resolver)) && bool(practice.accept(resolver, &practice_receipt)),
          "accept separate practice request");
    const auto win = begin(*run);
    check(!run->complete(practice_receipt, Outcome::Win) && !run->cancel(receipt) &&
          run->complete(win, Outcome::Win) && run->current().id == "retry" &&
          !run->complete(win, Outcome::Win), "practice and stale events cannot advance; one result takes one edge");
    check(run->map_projection().nodes[2].selected && run->map_projection().nodes[2].available,
          "projection follows the accepted transition");
    const auto loss = begin(*run);
    check(!run->complete(win, Outcome::Win) && run->complete(loss, Outcome::Loss) &&
          run->current().id == "arrival", "same encounter at another node needs its own receipt; loss edge may cycle");
    check(run->complete(begin(*run), Outcome::Win) && run->complete(begin(*run), Outcome::Win) &&
          run->current().id == "ending", "separate accepted battles are needed to reach terminal presentation");
    const auto terminal_map = run->map_projection();
    check(terminal_map.nodes[0].selected && !terminal_map.nodes[0].available &&
          !terminal_map.nodes[1].selected && !terminal_map.nodes[2].available &&
          initial_map.nodes[1].selected && initial_map.nodes[1].available &&
          run->prepare(resolver).status == Status::Denied && run->accept(resolver, &receipt).status == Status::Denied,
          "terminal remains selectable for presentation only; old projection remains a snapshot");
    check(other->current().id == "arrival" && other->complete(foreign, Outcome::Win) &&
          other->current().id == "retry", "other Story session remained independent");

    StoryManifest splash;
    std::unique_ptr<PortableStoryRun> splash_run;
    check(story_read_manifest("GFSTORY 1\nid=example:splash\nname=Splash\nstart=end\nnode=end|text.end|0|0|none|none",
          &splash) && bool(PortableStoryRun::bind(splash, nullptr, selected_force, &splash_run)) &&
          splash_run->current().id == "end", "artless terminal Story needs no Challenge dependency");
}
