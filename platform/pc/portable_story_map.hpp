#pragma once
#include <functional>
#include "engine/gameplay/story_map_view.hpp"
#include "engine/input/input_router.hpp"
#include "engine/log.hpp"
#include "platform/pc/portable_story_scan.hpp"

namespace gf::pc {

inline int list_portable_stories(const std::filesystem::path& root) {
    const auto catalog = scan_portable_stories(root);
    using Status = PortableStoryScanStatus;
    if(catalog.status != Status::Complete && catalog.status != Status::NoDirectory) {
        std::fputs(catalog.status == Status::EntryBudget ? "Story discovery budget exceeded\n" :
            "Story discovery unavailable\n", stderr);
        return 2;
    }
    for(const auto& entry : catalog.discovered)
        std::printf("%s\t%s\tnodes=%zu\n", entry.manifest.id.c_str(), entry.manifest.name.c_str(), entry.manifest.nodes.size());
    for(const auto& issue : catalog.issues)
        std::fprintf(stderr, "Story rejected%s%s: %s\n", issue.id.empty() ? "" : " ", issue.id.c_str(), issue.reason.c_str());
    if(catalog.discovered.empty()) std::puts("No portable Stories discovered.");
    return 0;
}

struct PortableStoryMapOptions {
    bool headless = false;
    unsigned frames = 0, seconds = 0;
};

struct StoryMapArtTexture {
    u32 handle = 0, width = 4, height = 3;

    static StoryMapArtTexture upload(RenderEngine& render, const TplImage& image) {
        constexpr u32 handle = 0x53544d50u; // STMP, local to the map render scope.
        if(!image.rgba.empty() && !render.shared_texture_reserved(handle) &&
           render.upload_texture(handle, {image.width, image.height, image.rgba.data(), 0, 0, true}))
            return {handle, image.width, image.height};
        return {};
    }
};

inline StoryMapLayout desktop_story_map_layout(Context& context, RenderEngine& render, bool headless,
                                               StoryMapArtTexture art) {
    auto [width, height] = render.canvas_size();
    unsigned logical_w = width, logical_h = height;
    if(!headless) {
        context.window().size(&width, &height);
        context.window().logical_size(&logical_w, &logical_h);
    }
    return StoryMapLayout::fit(width, height, logical_w, logical_h, art.width, art.height);
}

// Standalone inspection: borrows the initialized render/window context, but
// owns only one optional UI texture for its lifetime. No selected save or
// battle launcher is consulted. Captures use the caller's existing seam.
inline int run_portable_story_map(Context& context, RenderEngine& render, const std::filesystem::path& stories,
                                 std::string_view id, PortableStoryMapOptions options,
                                 const std::function<void(unsigned)>& capture = {}) {
    const auto fail = [&](const char* reason) {
        log_error(context, "story-map", "%s", reason);
        return 2;
    };
    if(!borg_package_id_valid(id)) return fail("invalid Story ID");
    const auto catalog = scan_portable_stories(stories);
    if(catalog.status == PortableStoryScanStatus::EntryBudget) return fail("Story discovery budget exceeded");
    if(catalog.status == PortableStoryScanStatus::Unavailable) return fail("Story discovery unavailable");
    const auto* entry = catalog.find(id);
    if(!entry) {
        for(const auto& issue : catalog.issues)
            if(issue.id == id) return fail(issue.reason.c_str());
        return fail("Story ID not found");
    }
    const auto map = story_map_preview(entry->manifest);

    StoryMapArtTexture art;
    if(!entry->manifest.map_art.empty()) {
        std::string error;
        TplImage image;
        if(load_portable_story_map_art(std::span(&stories, 1), *entry, &image, &error))
            art = StoryMapArtTexture::upload(render, image);
        if(!art.handle) {
            log_warn(context, "story-map", "optional art unavailable for %s: %s", entry->manifest.id.c_str(),
                error.empty() ? "texture upload refused" : error.c_str());
        }
    }
    context.window().set_title("Story Map");
    log_info(context, "story-map", "story=%s start=%s nodes=%zu read-only",
        entry->manifest.id.c_str(), entry->manifest.start.c_str(), map.nodes.size());
    StoryMapFocus focus;
    StoryMapView view;
    InputRouter router;
    bool previous_backwards = false, previous_forwards = false, previous_left = false;
    const auto start = context.clock().monotonic_ns();
    unsigned drawn_frames = 0;
    int status = 0;
    for(unsigned frame = 0; context.window().pump_events() && !context.window().should_close(); ++frame) {
        if((options.frames && frame >= options.frames) ||
           (options.seconds && context.clock().monotonic_ns() - start >= u64(options.seconds)*1000000000ull)) break;
        const auto layout = desktop_story_map_layout(context, render, options.headless, art);
        context.input().pump();
        const auto& input = context.input().snapshot();
        const auto pad = router.route(input, 0);
        if(input.quit || pad.quit_requested || pad.b) break;
        const bool backwards = pad.dpad_up || pad.dpad_left, forwards = pad.dpad_down || pad.dpad_right;
        if(backwards && !previous_backwards) focus.step(map, true);
        else if(forwards && !previous_forwards) focus.step(map, false);
        previous_backwards = backwards;
        previous_forwards = forwards;
        const bool click = input.pointer.left_pressed || (input.pointer.left && !previous_left);
        previous_left = input.pointer.left;
        if(click && input.pointer.within_window)
            if(const auto hit = layout.hit_node(input.pointer.x, input.pointer.y, map, focus))
                focus.select(map, *hit);

        render.begin_frame({frame, .055f, .065f, .085f, true});
        render.reset_arena();
        const auto draws = view.draw(render, layout, map, focus, art.handle);
        if(!render.end_frame() || (layout.ready && !draws)) {
            status = fail("could not draw Story map");
            break;
        }
        if(draws) ++drawn_frames;
        if(capture && layout.ready) capture(frame);
        render.present();
        context.advance_frame();
    }
    if(art.handle) render.release_texture(art.handle);
    if(!status) log_info(context, "story-map", "read-only drawn-frames=%u", drawn_frames);
    return status;
}

} // namespace gf::pc
