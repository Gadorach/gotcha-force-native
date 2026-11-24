#pragma once

#include "platform/pc/portable_story_launch.hpp"
#include "platform/pc/portable_story_map.hpp"

namespace gf::pc {

enum class StoryCampaignCommand { Inspect, PlayCurrent, Close };

// Release-to-confirm keeps Enter (battle pause) and A (attack) from leaking
// from the map into the shared duel. Focus is presentation-only: inspecting a
// different node never changes which encounter the launch can admit.
class StoryCampaignInput {
public:
    const StoryMapFocus& focus() const { return focus_; }

    StoryCampaignCommand step(const InputSnapshot& input, const GamePadState& pad, const StoryMapLayout& layout,
                              const StoryMapProjection& map) {
        using Command = StoryCampaignCommand;
        const bool confirm = pad.a || pad.start;
        const bool back = pad.quit_requested || pad.b;
        const bool click = input.pointer.left_pressed || (input.pointer.left && !previous_left_);
        previous_left_ = input.pointer.left;
        if(input.quit) return Command::Close;
        if(wait_release_) {
            wait_release_ = back || confirm;
            return Command::Inspect;
        }
        if(back) return Command::Close;

        const bool backwards = pad.dpad_up || pad.dpad_left, forwards = pad.dpad_down || pad.dpad_right;
        if(backwards && !previous_backwards_) focus_.step(map, true);
        else if(forwards && !previous_forwards_) focus_.step(map, false);
        previous_backwards_ = backwards;
        previous_forwards_ = forwards;
        if(click && input.pointer.within_window)
            if(const auto hit = layout.hit_node(input.pointer.x, input.pointer.y, map, focus_))
                focus_.select(map, *hit);

        const auto i = focus_.index(map);
        const bool playable = layout.ready && i < map.nodes.size() && map.nodes[i].selected && map.nodes[i].available;
        if(!playable) armed_id_.clear();
        if(confirm && !previous_confirm_ && playable) armed_id_ = map.nodes[i].id;
        previous_confirm_ = confirm;
        if(!confirm && playable && armed_id_ == map.nodes[i].id) {
            armed_id_.clear();
            return Command::PlayCurrent;
        }
        return Command::Inspect;
    }

    void return_to_current() {
        focus_ = {};
        armed_id_.clear();
        previous_confirm_ = previous_backwards_ = previous_forwards_ = false;
        wait_release_ = true;
    }

private:
    StoryMapFocus focus_;
    std::string armed_id_;
    bool previous_confirm_ = false, previous_backwards_ = false, previous_forwards_ = false, wait_release_ = false;
    bool previous_left_ = false;
};

struct StoryDuelBudget {
    unsigned width, height, frames, seconds;
};

// The callback is the existing hosted package duel: it owns shutdown of the
// supplied battle renderer whether it returns a duel report or an error.
using StoryDuelRunner = std::function<int(RenderEngine&, PortableBattleHost&, StoryDuelBudget, PortableBattleHostReport*)>;

inline int run_portable_story_campaign(Context& context, RenderEngine& map_renderer, PortableStoryLaunch& story,
                                       PortableStoryMapOptions options, const StoryDuelRunner& run_duel) {
    if(!run_duel) { map_renderer.shutdown(); return 2; }
    const auto canvas = map_renderer.canvas_size();
    const auto started = context.clock().monotonic_ns();
    const auto start_frame = context.frame_index();
    const u64 duration_ns = u64(options.seconds)*1000000000ull;
    auto expired = [&](u64 elapsed, unsigned frames) {
        return (duration_ns && elapsed >= duration_ns) || (options.frames && frames >= options.frames);
    };

    StoryCampaignInput controls;
    StoryMapView view;
    InputRouter router;
    auto map = story.map_projection();
    TplImage cpu_art;
    std::string error, status;
    if(!map.map_art.empty() && !story.load_map_art(&cpu_art, &error))
        log_warn(context, "portable-story", "optional map art unavailable: %s", error.c_str());
    auto art = StoryMapArtTexture::upload(map_renderer, cpu_art);
    log_info(context, "portable-story", "map story=%s current=%s; in-memory progress",
        map.story_id.c_str(), story.current().id.c_str());
    bool map_live = true;
    unsigned map_frames = 0;
    int code = 0;
    while(context.window().pump_events() && !context.window().should_close()) {
        const auto elapsed = context.clock().monotonic_ns()-started;
        const auto frames = context.frame_index()-start_frame;
        if(expired(elapsed, frames)) break;
        const auto layout = desktop_story_map_layout(context, map_renderer, options.headless, art);
        context.input().pump();
        const auto& input = context.input().snapshot();
        const auto command = controls.step(input, router.route(input, 0), layout, map);
        if(command == StoryCampaignCommand::Close) break;
        if(command == StoryCampaignCommand::PlayCurrent) {
            auto [width, height] = canvas;
            if(!options.headless) context.window().logical_size(&width, &height);
            if(width < 320 || height < 240 || width > 4096 || height > 4096) {
                status = "RESIZE WINDOW TO PLAY";
                continue;
            }

            const auto from = story.current().id;
            log_info(context, "portable-story", "prepare node=%s", from.c_str());
            map_renderer.shutdown();
            map_live = false;
            {
                // One initialized renderer at a time. The map texture is
                // rebuilt from pinned CPU art; battle assets never cross a
                // stage or leak into the restored map renderer.
                RenderEngine battle_renderer(context.render());
                context.window().set_title("Story Battle");
                if(!battle_renderer.initialize({width, height, !options.headless, true, "Story Battle"},
                                               context.backends().window, &error)) {
                    battle_renderer.shutdown();
                    log_error(context, "portable-story", "battle renderer unavailable: %s", error.c_str());
                    status = "BATTLE DISPLAY UNAVAILABLE";
                } else {
                    std::unique_ptr<PortableStoryBattle> attempt;
                    if(const auto admission = story.prepare(battle_renderer, &attempt); !admission) {
                        log_warn(context, "portable-story", "node=%s refused: %s", from.c_str(), admission.reason.c_str());
                        status = "BATTLE REFUSED";
                        battle_renderer.shutdown();
                    } else if(const auto admitted_elapsed = context.clock().monotonic_ns()-started;
                              expired(admitted_elapsed, frames)) {
                        attempt->cancel();
                        battle_renderer.shutdown();
                    } else {
                        auto& host = *attempt->admitted_host();
                        const auto& request = host.accepted()->request;
                        log_info(context, "portable-story", "node=%s encounter=%s/%s force=%u", from.c_str(),
                            request.content_id.c_str(), request.encounter_id.c_str(),
                            std::get<PortableBattleSavedForce>(request.player).index+1);
                        PortableBattleHostReport report;
                        const auto remaining_ns = duration_ns ? duration_ns-admitted_elapsed : 0;
                        const StoryDuelBudget limit{width, height, options.frames ? options.frames-frames : 0,
                            unsigned((remaining_ns+999999999ull)/1000000000ull)};
                        const auto battle_code = run_duel(battle_renderer, host, limit, &report);
                        const auto returned = attempt->finish(battle_code ? PortableBattleHostReport{} : report);
                        if(!returned) log_warn(context, "portable-story", "return refused: %s", returned.reason.c_str());
                        else if(!battle_code) log_info(context, "portable-story", "return node=%s result=%s ticks=%u current=%s",
                            from.c_str(), portable_duel_end_name(report.end), report.ticks, story.current().id.c_str());
                        status = !returned ? "BATTLE RETURN REFUSED" : battle_code ? "BATTLE FAILED" :
                            std::string("LAST BATTLE: ") + portable_duel_end_name(report.end);
                    }
                }
            }
            if(context.window().should_close() ||
               expired(context.clock().monotonic_ns()-started, context.frame_index()-start_frame)) break;
            context.window().set_title("Story Map");
            map_renderer.state() = {};
            map_renderer.clear_shared_texture_upload_error();
            if(!map_renderer.initialize({canvas[0], canvas[1], !options.headless, true, "Story Map"},
                                        context.backends().window, &error)) {
                map_renderer.shutdown();
                log_error(context, "portable-story", "map renderer could not be restored: %s", error.c_str());
                return 2;
            }
            map_live = true;
            art = StoryMapArtTexture::upload(map_renderer, cpu_art);
            map = story.map_projection();
            controls.return_to_current();
            log_info(context, "portable-story", "map restored current=%s", story.current().id.c_str());
            continue;
        }

        map_renderer.begin_frame({context.frame_index(), .055f, .065f, .085f, true});
        map_renderer.reset_arena();
        const auto draws = view.draw(map_renderer, layout, map, controls.focus(), art.handle, true, status);
        if(!map_renderer.end_frame() || (layout.ready && !draws)) {
            log_error(context, "portable-story", "could not draw Story map");
            code = 2;
            break;
        }
        map_renderer.present();
        if(draws) ++map_frames;
        context.advance_frame();
    }
    if(map_live) map_renderer.shutdown();
    if(!code) log_info(context, "portable-story", "closed current=%s map-drawn-frames=%u; no progress saved",
        story.current().id.c_str(), map_frames);
    return code;
}

} // namespace gf::pc
