#include <cstdio>
#include <cstdlib>
#include "engine/core/headless_backends.hpp"
#include "platform/pc/account_save_storage.hpp"
#include "platform/pc/portable_story_campaign.hpp"

namespace {
using namespace gf;
using namespace gf::pc;
using Status = PortableBattleStatus;
using End = PortableDuelEnd;

void check(bool ok, const char* reason) {
    if(!ok) {
        std::fprintf(stderr, "Story handoff: %s\n", reason);
        std::abort();
    }
}

struct TestRenderer {
    headless::Bundle backends;
    RenderEngine render{*backends.get().render};
    TestRenderer() {
        check(render.initialize({640, 480, false, true, "Story handoff test"}, backends.get().window, nullptr),
              "initialize shared renderer");
    }
    ~TestRenderer() { render.shutdown(); }
};

struct Origins final : IBorgBattleOrigins {
    explicit Origins(const NativeBattleMvpView& view) : view_(view) {}
    bool socket(unsigned owner, const BattleMvpActor& actor, u32 tick, u8 move, u16 frame,
                const BorgBattleSocket& spec, BattleMvpVec* out) const override {
        return view_.package_socket(owner, actor, tick, move, frame, spec, nullptr, out);
    }
private:
    const NativeBattleMvpView& view_;
};

// Exercise the actual world, not calculated or injected campaign outcomes.
PortableBattleHostReport run_duel(PortableBattleHost& host, RenderEngine& render) {
    NativeBattleMvp world;
    auto& view = host.view();
    std::string error;
    check(world.start(host.config(), &view) && view.update(world, &error), "start exact admitted duel");
    Origins origins(view);
    const auto limit = host.accepted()->request.tick_limit;
    auto end = End::Fighting;
    while(world.tick() < 300 && end == End::Fighting) {
        const auto ai = portable_duel_enemy_input(world);
        check(world.step({GamePadState{}, ai.movement}, false, &view, nullptr, nullptr, nullptr,
                         {{{}, ai.attack}}, &origins) && view.update(world, &error), "advance admitted actors");
        render.begin_frame({world.tick(), .15f, .21f, .3f});
        render.reset_arena();
        const auto draws = view.draw(render, world, 640, 480, &error);
        check(render.end_frame() && draws, "render through the admitted shared view");
        end = portable_duel_end(world, limit);
    }
    return {end, world.tick()};
}

struct CampaignRender final : IRenderBackend {
    bool live = false;
    unsigned scopes = 0, closed = 0, map_frames = 0, map_uploads = 0;
    bool initialize(const RenderConfig&, IWindowBackend*, std::string*) override {
        check(!live, "map and battle renderer scopes never overlap");
        live = true;
        ++scopes;
        return true;
    }
    void shutdown() override { check(live, "close each scope once"); live = false; ++closed; }
    RenderCapabilities capabilities() const override { return {4096, false, false, "campaign test"}; }
    bool upload_vertices(const Vertex*, std::size_t) override { return live; }
    bool upload_matrix(u32, const f32*) override { return live; }
    bool upload_texture(u32 handle, const TextureDesc& desc) override {
        if(handle == 0x53544d50u) {
            check(desc.width == 4 && desc.height == 3 && desc.pixels, "reload pinned CPU map art");
            ++map_uploads;
        }
        return live;
    }
    void begin_frame(const FrameDesc& desc) override { if(desc.drawable_coordinates) ++map_frames; }
    void submit(const DrawCall&) override {}
    void end_frame() override {}
    void present() override {}
};

struct CampaignInput final : IInputBackend {
    void pump() override {
        input_.clear();
        const auto keys = default_input_map();
        // Inspect the second node and try to launch it. Only after moving
        // focus back and releasing Enter can the current node play.
        switch(frame_++) {
        case 0: {
            const auto layout = StoryMapLayout::fit(640, 480, 640, 480);
            const auto [x, y] = layout.position({"retry", "text.retry", .42f, .54f});
            input_.pointer.x = s32(x);
            input_.pointer.y = s32(y);
            input_.pointer.left = input_.pointer.within_window = true;
            break;
        }
        case 1: input_.pad(0).buttons = keys.mask_start; break;
        case 3: input_.pad(0).buttons = keys.mask_dpad_left; break;
        case 4:
        case 5: input_.pad(0).buttons = keys.mask_start; break;
        case 7: input_.pad(0).quit = true; break; // Held Escape cannot close the restored map.
        case 9: input_.pad(0).buttons = keys.mask_a; break;
        case 12: input_.quit = true; break;
        default: break;
        }
    }
    const InputSnapshot& snapshot() const override { return input_; }
    unsigned frames() const { return frame_; }
private:
    InputSnapshot input_;
    unsigned frame_ = 0;
};

void campaign_fixture(const PortableStorySelection& selected) {
    std::unique_ptr<PortableStoryLaunch> story;
    check(bool(PortableStoryLaunch::open(selected, {}, &story)), "open campaign fixture");
    headless::Bundle bundle;
    auto io = bundle.get();
    CampaignRender backend;
    CampaignInput input;
    io.render = &backend;
    io.input = &input;
    Context context(io, 60);
    RenderEngine map_renderer(backend);
    check(map_renderer.initialize({}, io.window, nullptr), "open map renderer");
    unsigned duels = 0;
    const auto code = run_portable_story_campaign(context, map_renderer, *story, {true, 400, 0},
        [&](RenderEngine& battle, PortableBattleHost& host, StoryDuelBudget budget, PortableBattleHostReport* report) {
            check((duels == 0 && input.frames() == 7 && story->current().id == "arrival" &&
                   host.accepted()->request.stage.id == "example:workshop") ||
                  (duels == 1 && input.frames() == 11 && story->current().id == "retry" &&
                   host.accepted()->request.stage.id == "example:second-workshop"),
                  "only the released current-node command runs each exact stage; map resumes after held input");
            check(budget.width == 640 && budget.height == 480 && budget.frames < 400, "propagate remaining frame budget");
            *report = run_duel(host, battle);
            check(report->end == (duels == 0 ? End::Loss : End::Timeout), "actual duel reports drive map return");
            for(unsigned i = 0; i < report->ticks; ++i) context.advance_frame();
            ++duels;
            battle.shutdown();
            return 0;
        });
    check(!code && duels == 2 && story->current().id == "retry" && input.frames() == 13 &&
          backend.map_frames == 10 && backend.map_uploads == 3 && backend.scopes == 5 &&
          backend.closed == 5 && !backend.live,
          "two live duels restore map scopes with one in-memory transition and no automated relaunch");
}

int run_fixture(const std::string& save_root) {
    const PortableStorySelection selected{"example:story", save_root, {1, 1}, {0}};
    std::unique_ptr<PortableStoryLaunch> story;
    const auto loaded = PortableStoryLaunch::open(selected, {}, &story);
    check(bool(loaded), loaded.reason.c_str());
    const auto initial = story->map_projection();
    check(story->current().id == "arrival" && initial.nodes[0].selected && initial.nodes[0].available,
          "only the authored start offers a battle");

    auto invalid = selected;
    invalid.id = "example:missing-encounter";
    check(PortableStoryLaunch::open(invalid, {}, &story).status == Status::Missing &&
          story->current().id == "arrival", "missing named future encounter cannot change current run");
    invalid = selected;
    invalid.save = {1, 21};
    check(PortableStoryLaunch::open(invalid, {}, &story).status == Status::Unavailable,
          "never fall back to a different save");
    invalid = selected;
    invalid.force.index = 1;
    std::unique_ptr<PortableStoryLaunch> empty_force;
    check(bool(PortableStoryLaunch::open(invalid, {}, &empty_force)), "open the exact saved lease");
    TestRenderer renderer;
    std::unique_ptr<PortableStoryBattle> attempt;
    check(empty_force->prepare(renderer.render, &attempt).status == Status::Denied && !attempt,
          "empty specified Force is refused, not replaced with the first Force");

    const auto admitted = story->prepare(renderer.render, &attempt);
    check(bool(admitted), admitted.reason.c_str());
    auto* host = attempt->admitted_host();
    const auto* accepted = host ? host->accepted() : nullptr;
    const auto exact_borg=[](const auto& roster,std::string_view id){
        return roster.size()==1&&roster.front().package_id==id&&roster.front().variant_id=="normal";
    };
    check(accepted && accepted->request.content_id == "example:trials" &&
          accepted->request.encounter_id == "detour" && accepted->request.stage.id == "example:workshop" &&
          accepted->request.player_spawns[0].id == "west" && accepted->request.enemy_spawns[0].id == "east" &&
          std::get<PortableBattleSavedForce>(accepted->request.player).index == 0 &&
          exact_borg(accepted->players(),"example:flight-stick") &&
          exact_borg(accepted->request.enemies,"example:jump-stick"),
          "pin exact selected Force, named non-first encounter, and stage markers");
    std::unique_ptr<PortableStoryBattle> concurrent;
    check(story->prepare(renderer.render, &concurrent).status == Status::Busy &&
          !concurrent && !story->map_projection().nodes[0].available, "one active receipt and no reopening the map node");
    check(attempt->finish({End::Win, 0}).status == Status::Malformed && story->current().id == "arrival" &&
          attempt->admitted_host() == host, "invalid report neither advances nor consumes the correct attempt");
    const auto loss = run_duel(*host, renderer.render);
    const auto returned = attempt->finish(loss);
    check(loss.end == End::Loss && bool(returned) && story->current().id == "retry" &&
          story->map_projection().nodes[1].selected && story->map_projection().nodes[1].available &&
          initial.nodes[0].selected, "real defeat applies only the Story loss edge and returns to the new map");
    check(attempt->finish(loss).status == Status::Stale, "returned result cannot replay");

    TestRenderer next_renderer;
    check(bool(story->prepare(next_renderer.render, &concurrent)) && concurrent->admitted_host()->accepted() &&
          concurrent->admitted_host()->accepted()->request.encounter_id == "one-tick",
          "next transition uses its own freshly admitted request");
    check(concurrent->finish({End::Timeout, 2}).status == Status::Malformed, "wrong timeout tick refuses");
    const auto timeout = run_duel(*concurrent->admitted_host(), next_renderer.render);
    check(timeout.end == End::Timeout && timeout.ticks == 1 && bool(concurrent->finish(timeout)) &&
          story->current().id == "retry", "timeout returns without taking the loss edge");

    TestRenderer cancelled_renderer;
    check(bool(story->prepare(cancelled_renderer.render, &concurrent)), "prepare another accepted attempt");
    concurrent.reset();
    check(story->current().id == "retry" && story->map_projection().nodes[1].available,
          "dropping an unexecuted attempt cancels its receipt without moving progress");

    campaign_fixture(selected);
    return 0;
}
}

int main(int argc, char** argv) {
    if(argc == 2) return run_fixture(argv[1]);
    if(argc != 1) return 2;
    std::unique_ptr<PortableStoryLaunch> launch;
    check(PortableStoryLaunch::open({}, {}, &launch).status == Status::Malformed && !launch,
          "no implicit Story, account or Force");
    check(PortableStoryLaunch::open({"example:story", "unused", {1, 1}, {}}, {}, &launch).status == Status::Malformed,
          "default Force selector refuses before reading content");
}
