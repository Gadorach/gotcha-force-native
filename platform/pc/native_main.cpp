// Native rewrite entry point.
//
// This is the Linux/desktop host: it assembles the platform backends, hands the
// engine a Context, and runs frames. It contains no game logic; swapping a
// renderer or moving to another OS changes this file and platform/, nothing else.
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <filesystem>
#include <fstream>

#include "engine/core/context.hpp"
#include "engine/core/headless_backends.hpp"
#include "engine/assets/tpl.hpp"
#include "engine/assets/menu_layout.hpp"
#include "engine/assets/menu_motion.hpp"
#include "engine/assets/menu_navigation.hpp"
#include "engine/assets/option_camera_profile.hpp"
#include "engine/content/archive_summary.hpp"
#include <vector>
#include "engine/gameplay/menu_overlay.hpp"
#include "engine/gameplay/menu_scene.hpp"
#include "engine/gameplay/menu_world.hpp"
#include "engine/gameplay/option_scene.hpp"
#include "engine/gameplay/option_menu.hpp"
#include "engine/gameplay/option_output.hpp"
#include "engine/gameplay/credits_scene.hpp"
#include "engine/assets/menu_music.hpp"
#include "engine/gameplay/menu_music.hpp"
#include "engine/assets/credits_music.hpp"
#include "engine/gameplay/credits_music.hpp"
#include "engine/gameplay/session.hpp"
#include "engine/assets/battle_test_config.hpp"
#include "engine/gameplay/battle_test_view.hpp"
#include "engine/gameplay/native_battle_mvp_view.hpp"
#include "engine/gameplay/native_gred_ground_dash_demo.hpp"
#include "engine/gameplay/native_animation_mosaic.hpp"
#include "engine/graphics/render_engine.hpp"
#include "engine/graphics/scene_draw.hpp"
#include "engine/input/input_router.hpp"
#include "engine/log.hpp"
#include "platform/gl/gl_renderer.hpp"
#include "platform/pc/sdl_platform.hpp"

namespace {

constexpr gf::f32 kProbeIdentity[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};

struct Options {
    bool headless = false;
    bool menu = false;
    bool demo_input = false;   // scripted pad for unattended demos
    bool demo_option = false;
    bool demo_option_values = false;
    bool demo_credits = false;
    bool demo_credits_fast_back = false;
    bool no_audio = false;
    bool test_battle = false,demo_battle=false;
    bool battle_mvp=false,demo_battle_mvp=false,demo_locomotion=false,demo_source_ground=false;
    bool source_ground_walk=false,source_ground_interactive=false;
    bool source_vertical_jump=false;
    bool source_moving_jump=false;
    bool source_air_dash=false;
    bool source_relaunch=false;
    bool source_dash_jump=false;
    bool source_landing_interrupt=false;
    bool source_surface=false;
    bool source_steep=false;
    bool source_fall_surface=false;
    bool source_recovery_surface=false;
    bool source_windup_surface=false;
    bool source_active_surface=false;
    bool source_relaunch_surface=false;
    bool source_relaunch_entry_surface=false;
    bool source_relaunch_floor=false;
    bool source_air_dash_floor=false;
    bool source_windup_entry_floor=false;
    bool source_windup_positive_floor=false;
    bool source_windup_entry_push=false;
    bool source_walk_ledge=false;
    bool source_ordered_ground=false;
    bool source_dash_ledge=false;
    bool source_ground_ledge=false;
    bool source_ground_jump=false;
    bool source_ground_landing=false;
    bool source_ground_air=false;
    bool source_ground_air_floor=false;
    bool source_ground_relaunch=false;
    bool source_ground_relaunch_floor=false;
    bool source_ground_windup_entry_floor=false;
    bool source_ground_surface=false;
    std::string source_ground_assets;
    std::string battle_mvp_assets="local/content/native-battle-mvp-v1";
    bool animation_mosaic=false;unsigned animation_columns=4,animation_rows=3,animation_page=0,animation_borg=0;
    std::string animation_export;unsigned animation_export_frames=36;bool animation_export_page_only=false;
    std::string animation_programs,animation_height_profile,animation_motion_profile;
    std::string battle_config="content/test-battle/battle.ini";
    int probe_texture = -1;    // AFS index to decode as a TPL and draw over the menu
    bool placeholder_menu = false;   // force the box set instead of the real room
    bool no_cull = false;            // diagnostics for the real room
    gf::f32 clear[3] = {0.10f, 0.12f, 0.16f};
    bool no_alpha_test = false;
    bool no_depth = false;
    bool show_source_markers = false;
    int only_mesh = -1;
    bool explicit_camera = false;
    bool file_pose = false;
    bool scene_only = false;
    gf::f32 animation_frame = -1.0f; // default: end-of-clip preview, not proven menu state
    std::string scene_asset = "stff_mdl.arc";
    std::string menu_layout = "local/content/main-menu.gfml";
    std::string menu_motion = "local/content/main-menu.gfmv";
    std::string menu_navigation = "local/content/main-menu.gfmn";
    std::string option_camera = "local/content/main-menu.gfop";
    std::string option_widgets = "local/content/option-widgets.gfow";
    std::string option_output = "local/content/option-output.gfos";
    std::string credits = "local/content/credits.gfcr";
    std::string music_profile = "local/content/main-menu.gfmb";
    std::string music_transitions = "local/content/credits-music.gfmt";
    std::string audio_archive = "original/extracted/files/poq_adx_usa.afs";
    gf::f32 camera_eye[3] = {0.0f, 0.0f, 0.0f};
    gf::f32 camera_target[3] = {0.0f, 0.0f, 0.0f};
    gf::f32 camera_fov = 42.0f;
    unsigned frames = 0;
    unsigned seconds = 0;     // run for a wall-clock duration instead of a frame count
    unsigned width = 640;
    unsigned height = 480;
    std::string screenshot;   // optional PPM capture path
    unsigned capture_frame = 0;
};

bool parse(int argc, char** argv, Options* options) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--headless") options->headless = true;
        else if (arg == "--menu") options->menu = true;
        else if (arg == "--demo-input") options->demo_input = true;
        else if (arg == "--demo-option") options->demo_option = true;
        else if (arg == "--demo-option-values") options->demo_option=options->demo_option_values=true;
        else if (arg == "--demo-credits") options->demo_credits=true;
        else if (arg == "--demo-credits-fast-back") options->demo_credits=options->demo_credits_fast_back=true;
        else if (arg == "--no-audio") options->no_audio=true;
        else if (arg == "--test-battle") options->test_battle=true;
        else if (arg == "--demo-battle") options->test_battle=options->demo_battle=true;
        else if (arg == "--battle-mvp") options->battle_mvp=true;
        else if (arg == "--demo-battle-mvp") options->battle_mvp=options->demo_battle_mvp=true;
        else if (arg == "--demo-locomotion") options->battle_mvp=options->demo_locomotion=true;
        else if (arg == "--demo-source-ground-dash" || arg == "--demo-source-ground" || arg == "--source-ground" ||
                 arg == "--source-vertical-jump" || arg == "--demo-source-vertical-jump" ||
                 arg == "--source-jump" || arg == "--demo-source-jump" ||
                 arg == "--source-air-dash" || arg == "--demo-source-air-dash" ||
                 arg == "--source-relaunch" || arg == "--demo-source-relaunch" ||
                 arg == "--source-dash-jump" || arg == "--demo-source-dash-jump" ||
                 arg == "--source-landing" || arg == "--demo-source-landing" ||
                 arg == "--source-surface" || arg == "--demo-source-surface" ||
                 arg == "--source-steep" || arg == "--demo-source-steep" ||
                 arg == "--source-fall-surface" || arg == "--demo-source-fall-surface" ||
                 arg == "--source-recovery-surface" || arg == "--demo-source-recovery-surface" ||
                 arg == "--source-windup-surface" || arg == "--demo-source-windup-surface" ||
                 arg == "--source-active-surface" || arg == "--demo-source-active-surface" ||
                 arg == "--source-relaunch-surface" || arg == "--demo-source-relaunch-surface" ||
                 arg == "--source-relaunch-entry-surface" || arg == "--demo-source-relaunch-entry-surface" ||
                 arg == "--source-relaunch-floor" || arg == "--demo-source-relaunch-floor" ||
                 arg == "--source-air-dash-floor" || arg == "--demo-source-air-dash-floor" ||
                 arg == "--source-windup-entry-floor" || arg == "--demo-source-windup-entry-floor" ||
                 arg == "--source-windup-positive-floor" || arg == "--demo-source-windup-positive-floor" ||
                 arg == "--source-windup-entry-push" || arg == "--demo-source-windup-entry-push" ||
                 arg == "--source-walk-ledge" || arg == "--demo-source-walk-ledge" ||
                 arg == "--source-ordered-ground" || arg == "--demo-source-ordered-ground" ||
                 arg == "--source-dash-ledge" || arg == "--demo-source-dash-ledge" ||
                 arg == "--source-ground-ledge" || arg == "--demo-source-ground-ledge" ||
                 arg == "--source-ground-jump" || arg == "--demo-source-ground-jump" ||
                 arg == "--source-ground-landing" || arg == "--demo-source-ground-landing" ||
                 arg == "--source-ground-air" || arg == "--demo-source-ground-air" ||
                 arg == "--source-ground-air-floor" || arg == "--demo-source-ground-air-floor" ||
                 arg == "--source-ground-relaunch" || arg == "--demo-source-ground-relaunch" ||
                 arg == "--source-ground-relaunch-floor" || arg == "--demo-source-ground-relaunch-floor" ||
                 arg == "--source-ground-windup-entry-floor" || arg == "--demo-source-ground-windup-entry-floor" ||
                 arg == "--source-ground-surface" || arg == "--demo-source-ground-surface"){
            if(options->demo_source_ground)return false;
            options->battle_mvp=options->demo_source_ground=true;
            options->source_ground_walk=arg!="--demo-source-ground-dash";
            options->source_ground_interactive=arg=="--source-ground"||arg=="--source-vertical-jump"||arg=="--source-jump"||arg=="--source-air-dash"||arg=="--source-relaunch"||arg=="--source-dash-jump"||arg=="--source-landing"||arg=="--source-surface"||arg=="--source-steep"||arg=="--source-fall-surface"||arg=="--source-recovery-surface"||arg=="--source-windup-surface"||arg=="--source-active-surface"||arg=="--source-relaunch-surface"||arg=="--source-relaunch-entry-surface"||arg=="--source-relaunch-floor"||arg=="--source-air-dash-floor"||arg=="--source-windup-entry-floor"||arg=="--source-windup-positive-floor"||arg=="--source-windup-entry-push"||arg=="--source-walk-ledge";
            options->source_walk_ledge=arg=="--source-walk-ledge"||arg=="--demo-source-walk-ledge";
            options->source_dash_ledge=arg=="--source-dash-ledge"||arg=="--demo-source-dash-ledge";
            options->source_ground_ledge=arg=="--source-ground-ledge"||arg=="--demo-source-ground-ledge";
            options->source_ground_jump=arg=="--source-ground-jump"||arg=="--demo-source-ground-jump";
            options->source_ground_landing=arg=="--source-ground-landing"||arg=="--demo-source-ground-landing";
            options->source_ground_air=arg=="--source-ground-air"||arg=="--demo-source-ground-air";
            options->source_ground_air_floor=arg=="--source-ground-air-floor"||arg=="--demo-source-ground-air-floor";
            options->source_ground_relaunch=arg=="--source-ground-relaunch"||arg=="--demo-source-ground-relaunch";
            options->source_ground_relaunch_floor=arg=="--source-ground-relaunch-floor"||arg=="--demo-source-ground-relaunch-floor";
            options->source_ground_windup_entry_floor=arg=="--source-ground-windup-entry-floor"||arg=="--demo-source-ground-windup-entry-floor";
            options->source_ground_surface=arg=="--source-ground-surface"||arg=="--demo-source-ground-surface";
            options->source_ordered_ground=arg=="--source-ordered-ground"||arg=="--demo-source-ordered-ground"||options->source_dash_ledge||options->source_ground_ledge||options->source_ground_jump||options->source_ground_landing||options->source_ground_air||options->source_ground_air_floor||options->source_ground_relaunch||options->source_ground_relaunch_floor||options->source_ground_windup_entry_floor||options->source_ground_surface;
            if(options->source_ordered_ground)options->source_ground_interactive=arg=="--source-ordered-ground"||arg=="--source-dash-ledge"||arg=="--source-ground-ledge"||arg=="--source-ground-jump"||arg=="--source-ground-landing"||arg=="--source-ground-air"||arg=="--source-ground-air-floor"||arg=="--source-ground-relaunch"||arg=="--source-ground-relaunch-floor"||arg=="--source-ground-windup-entry-floor"||arg=="--source-ground-surface";
            options->source_windup_entry_push=arg=="--source-windup-entry-push"||arg=="--demo-source-windup-entry-push";
            options->source_windup_positive_floor=arg=="--source-windup-positive-floor"||arg=="--demo-source-windup-positive-floor"||options->source_windup_entry_push;
            options->source_windup_entry_floor=arg=="--source-windup-entry-floor"||arg=="--demo-source-windup-entry-floor"||options->source_windup_positive_floor;
            options->source_air_dash_floor=arg=="--source-air-dash-floor"||arg=="--demo-source-air-dash-floor"||options->source_windup_entry_floor;
            options->source_relaunch_floor=arg=="--source-relaunch-floor"||arg=="--demo-source-relaunch-floor"||options->source_air_dash_floor;
            options->source_relaunch_entry_surface=arg=="--source-relaunch-entry-surface"||arg=="--demo-source-relaunch-entry-surface"||options->source_relaunch_floor;
            options->source_relaunch_surface=arg=="--source-relaunch-surface"||arg=="--demo-source-relaunch-surface"||options->source_relaunch_entry_surface;
            options->source_active_surface=arg=="--source-active-surface"||arg=="--demo-source-active-surface"||options->source_relaunch_surface;
            options->source_windup_surface=arg=="--source-windup-surface"||arg=="--demo-source-windup-surface"||options->source_active_surface;
            options->source_recovery_surface=arg=="--source-recovery-surface"||arg=="--demo-source-recovery-surface"||options->source_windup_surface;
            options->source_fall_surface=arg=="--source-fall-surface"||arg=="--demo-source-fall-surface"||options->source_recovery_surface;
            options->source_steep=arg=="--source-steep"||arg=="--demo-source-steep"||options->source_fall_surface;
            options->source_surface=arg=="--source-surface"||arg=="--demo-source-surface"||options->source_steep;
            options->source_landing_interrupt=arg=="--source-landing"||arg=="--demo-source-landing"||options->source_surface;
            options->source_dash_jump=arg=="--source-dash-jump"||arg=="--demo-source-dash-jump"||options->source_landing_interrupt;
            options->source_relaunch=arg=="--source-relaunch"||arg=="--demo-source-relaunch"||options->source_dash_jump;
            options->source_air_dash=arg=="--source-air-dash"||arg=="--demo-source-air-dash"||options->source_relaunch;
            options->source_moving_jump=arg=="--source-jump"||arg=="--demo-source-jump"||options->source_air_dash||options->source_walk_ledge;
            options->source_vertical_jump=arg=="--source-vertical-jump"||arg=="--demo-source-vertical-jump"||options->source_moving_jump;
        }
        else if (arg == "--source-ground-assets" && i+1<argc) options->source_ground_assets=argv[++i];
        else if (arg == "--battle-mvp-assets" && i+1<argc) options->battle_mvp_assets=argv[++i];
        else if (arg == "--animation-mosaic") options->animation_mosaic=true;
        else if (arg == "--animation-grid" && i+2<argc){options->animation_columns=std::strtoul(argv[++i],nullptr,10);options->animation_rows=std::strtoul(argv[++i],nullptr,10);}
        else if (arg == "--animation-page" && i+1<argc){const auto page=std::strtoul(argv[++i],nullptr,10);if(!page)return false;options->animation_page=unsigned(page-1);}
        else if (arg == "--animation-borg" && i+1<argc){const std::string borg=argv[++i];if(borg!="g-red"&&borg!="sasuke")return false;options->animation_borg=borg=="sasuke";}
        else if (arg == "--animation-export" && i+1<argc) options->animation_export=argv[++i];
        else if (arg == "--animation-export-frames" && i+1<argc) options->animation_export_frames=std::strtoul(argv[++i],nullptr,10);
        else if (arg == "--animation-export-page-only") options->animation_export_page_only=true;
        else if (arg == "--animation-programs" && i+1<argc) options->animation_programs=argv[++i];
        else if (arg == "--animation-height-profile" && i+1<argc) options->animation_height_profile=argv[++i];
        else if (arg == "--animation-motion-profile" && i+1<argc) options->animation_motion_profile=argv[++i];
        else if (arg == "--battle-config" && i+1<argc) options->battle_config=argv[++i];
        else if (arg == "--placeholder-menu") options->placeholder_menu = true;
        else if (arg == "--file-pose") options->file_pose = true;
        else if (arg == "--scene-only") options->scene_only = true;
        else if (arg == "--scene-asset" && i + 1 < argc) options->scene_asset=argv[++i];
        else if (arg == "--menu-layout" && i + 1 < argc) options->menu_layout=argv[++i];
        else if (arg == "--menu-motion" && i + 1 < argc) options->menu_motion=argv[++i];
        else if (arg == "--menu-navigation" && i + 1 < argc) options->menu_navigation=argv[++i];
        else if (arg == "--option-camera" && i + 1 < argc) options->option_camera=argv[++i];
        else if (arg == "--option-widgets" && i + 1 < argc) options->option_widgets=argv[++i];
        else if (arg == "--option-output" && i + 1 < argc) options->option_output=argv[++i];
        else if (arg == "--credits" && i + 1 < argc) options->credits=argv[++i];
        else if (arg == "--menu-music" && i + 1 < argc) options->music_profile=argv[++i];
        else if (arg == "--music-transitions" && i + 1 < argc) options->music_transitions=argv[++i];
        else if (arg == "--audio-archive" && i + 1 < argc) options->audio_archive=argv[++i];
        else if (arg == "--animation-frame" && i + 1 < argc)
            options->animation_frame = std::strtof(argv[++i],nullptr);
        else if (arg == "--no-cull") options->no_cull = true;
        else if (arg == "--no-depth") options->no_depth = true;
        else if (arg == "--show-source-markers") options->show_source_markers = true;
        else if (arg == "--only-mesh" && i + 1 < argc)
            options->only_mesh = static_cast<int>(std::strtol(argv[++i], nullptr, 10));
        else if (arg == "--clear" && i + 3 < argc) {
            for (unsigned k = 0; k < 3; ++k) options->clear[k] = std::strtof(argv[++i], nullptr);
        }
        else if (arg == "--no-alpha-test") options->no_alpha_test = true;
        else if (arg == "--camera" && i + 7 < argc) {
            // eye xyz, target xyz, vertical fov in degrees
            options->explicit_camera = true;
            for (unsigned k = 0; k < 3; ++k) options->camera_eye[k] = std::strtof(argv[++i], nullptr);
            for (unsigned k = 0; k < 3; ++k) options->camera_target[k] = std::strtof(argv[++i], nullptr);
            options->camera_fov = std::strtof(argv[++i], nullptr);
        }
        else if (arg == "--probe-texture" && i + 1 < argc)
            options->probe_texture = static_cast<int>(std::strtol(argv[++i], nullptr, 10));
        else if (arg == "--frames" && i + 1 < argc) options->frames = std::strtoul(argv[++i], nullptr, 10);
        else if (arg == "--capture-frame" && i + 1 < argc) options->capture_frame=std::strtoul(argv[++i],nullptr,10);
        else if (arg == "--seconds" && i + 1 < argc) options->seconds = std::strtoul(argv[++i], nullptr, 10);
        else if (arg == "--size" && i + 2 < argc) {
            options->width = std::strtoul(argv[++i], nullptr, 10);
            options->height = std::strtoul(argv[++i], nullptr, 10);
        } else if (arg == "--screenshot" && i + 1 < argc) {
            options->screenshot = argv[++i];
        } else {
            std::fprintf(stderr,
                         "usage: gf_native [--headless] [--menu] [--frames N] [--seconds N] [--size W H] "
                         "[--screenshot PATH.ppm]\n");
            return false;
        }
    }
    if(unsigned(options->demo_input)+unsigned(options->demo_option)+unsigned(options->demo_credits)>1)return false;
    if(options->test_battle&&(options->menu||options->demo_input||options->demo_option||options->demo_credits||options->probe_texture>=0))return false;
    if(options->battle_mvp&&(options->test_battle||options->menu||options->demo_input||options->demo_option||options->demo_credits||options->probe_texture>=0))return false;
    if(options->animation_mosaic&&(options->battle_mvp||options->test_battle||options->menu||options->demo_input||options->demo_option||options->demo_credits||options->probe_texture>=0))return false;
    if(!options->animation_export.empty()&&(!options->animation_mosaic||options->headless||options->animation_export_frames<2||options->animation_export_frames>120))return false;
    if(!options->animation_export.empty()&&(options->width>2048||options->height>2048||options->width<320||options->height<240))return false;
    if(options->animation_programs.empty()!=options->animation_height_profile.empty())return false;
    if(!options->animation_programs.empty()&&(!options->animation_mosaic||options->animation_borg||!options->animation_export.empty()))return false;
    if(!options->animation_motion_profile.empty()&&options->animation_programs.empty())return false;
    if(options->demo_source_ground&&(options->demo_battle_mvp||options->demo_locomotion))return false;
    if(!options->source_ground_assets.empty()&&!options->demo_source_ground)return false;
    if (options->headless && options->frames == 0) options->frames = 3;
    return true;
}

// The back buffer is undefined after SwapWindow, so a screenshot must be taken
// between end_frame and present, on the last frame only.
bool capture_if_requested(gf::Context& context, const Options& options, unsigned frame) {
    if (options.screenshot.empty()) return false;
    if(frame<options.capture_frame)return false;
    // Only the final frame is written: the run is deterministic, and reading a
    // 1 MB framebuffer every frame would dominate a 30 second demo.
    // A frame-count run captures on its last frame; the caller stops after the
    // first successful capture on a duration run, so a 30 second demo does not
    // pay a framebuffer read every frame.
    if (options.seconds == 0 && options.frames != 0 && frame + 1 < options.frames) return false;
    std::vector<gf::u8> rgba;
    gf::s32 captured_width = 0, captured_height = 0;
    if (gf::gl::capture_framebuffer(rgba, &captured_width, &captured_height) &&
        gf::gl::write_ppm(options.screenshot.c_str(), rgba, captured_width, captured_height)) {
        gf::log_info(context, "host", "screenshot %s (%dx%d)", options.screenshot.c_str(),
                     captured_width, captured_height);
        return true;
    }
    gf::log_warn(context, "host", "framebuffer capture unavailable");
    return false;
}

// Draws a decoded, uploaded texture as an aspect-fitted screen-space quad. This
// is a pipeline probe: it proves decode -> upload -> textured draw on the real
// backend, and is never part of the menu itself.
unsigned draw_probe_texture(gf::RenderEngine& render, gf::u32 texture, gf::u32 texture_width,
                            gf::u32 texture_height, unsigned surface_width,
                            unsigned surface_height) {
    if (!texture || !texture_width || !texture_height || !surface_width || !surface_height) return 0;
    const gf::f32 max_width = static_cast<gf::f32>(surface_width) * 0.45f;
    const gf::f32 max_height = static_cast<gf::f32>(surface_height) * 0.45f;
    gf::f32 width = static_cast<gf::f32>(texture_width);
    gf::f32 height = static_cast<gf::f32>(texture_height);
    const gf::f32 scale = std::min(max_width / width, max_height / height);
    width *= scale;
    height *= scale;
    const gf::f32 x0 = (static_cast<gf::f32>(surface_width) - width) * 0.5f;
    const gf::f32 y0 = static_cast<gf::f32>(surface_height) - height - 24.0f;

    std::vector<gf::Vertex> quad;
    gf::ScreenDraw::textured_quad(quad, x0, y0, x0 + width, y0 + height, 1.0f, 0.0f, 0.0f, 1.0f,
                                  1.0f, gf::Tint{255, 255, 255, 255});
    const gf::u32 base = render.append_vertices(quad.data(), quad.size());

    const gf::RenderState saved = render.state();
    render.state().blend = true;
    render.state().blend_src = 5;
    render.state().blend_dst = 6;
    render.state().depth_test = false;    // UI pass: always on top of the scene
    render.state().depth_write = false;
    render.state().cull_backface = false;
    // Cut-out: decoded art frequently carries transparency (I4/I8 intensity in
    // alpha, CMPR's transparent index), and blending alone would still write
    // depth for those texels.
    render.state().alpha_test = true;
    render.state().alpha_ref = 8;
    render.state().texture = texture;
    render.upload_matrix(0, kProbeIdentity);
    render.state().position_matrix = 0;
    const bool submitted = render.draw(gf::Primitive::Triangles, base,
                                       static_cast<gf::u32>(quad.size()));
    render.state() = saved;
    return submitted ? 1u : 0u;
}

// Scripted pad for unattended demos: step the selection once per 180 native
// updates, not an inferred original wall-clock cadence. This only synthesises
// input; navigation, selection and drawing are the normal engine path.
gf::GamePadState demo_pad(unsigned frame,bool spatial) {
    gf::GamePadState pad;
    // --menu already starts in MainMenu; confirming here would enter a
    // destination and freeze the navigation-only demonstration.
    if (frame >= 60) {
        const unsigned phase = (frame - 60) % 180,step=(frame-60)/180;
        if(phase<6){
            if(!spatial)pad.dpad_down=true;
            else { // Reference-observed cycle, starting at authored STORY selection.
                switch(step%7){
                case 0:case 6:pad.dpad_right=true;break;
                case 1:pad.dpad_up=true;break;
                case 2:case 3:case 4:pad.dpad_left=true;break;
                case 5:pad.dpad_down=true;break;
                }
            }
        }
    }
    return pad;
}
gf::GamePadState option_demo_pad(unsigned frame,bool values,bool widgets){
    gf::GamePadState pad;
    if(frame>=60&&frame<66)pad.dpad_right=true;
    if(frame>=120&&frame<126)pad.dpad_up=true;
    if(frame>=180&&frame<186)pad.a=true;
    if(frame>=420&&frame<426)pad.b=true;
    if(widgets&&frame>=480&&frame<486)pad.a=true; // acknowledge an actual native save
    if(values){
        if(frame>=250&&frame<256)pad.dpad_right=true;
        for(unsigned at:{280u,320u,360u})if(frame>=at&&frame<at+6)pad.dpad_down=true;
        for(unsigned at:{300u,340u,380u})if(frame>=at&&frame<at+6)pad.dpad_left=true;
    }
    return pad;
}
gf::GamePadState credits_demo_pad(unsigned frame,bool fast_back){
    gf::GamePadState pad;
    if(frame>=60&&frame<66)pad.dpad_right=true;
    if(frame>=120&&frame<126)pad.dpad_up=true;
    if(frame>=180&&frame<186)pad.a=true;
    if((frame>=260&&frame<266)||(frame>=300&&frame<306))pad.dpad_up=true;
    if(frame>=340&&frame<346)pad.a=true;
    if(!fast_back){
        if(frame>=800&&frame<866)pad.b=true; // held return must not become save/exit
        if(frame>=900&&frame<906)pad.b=true;
        if(frame>=960&&frame<966)pad.a=true;
    }else{
        if(frame>=410&&frame<416)pad.b=true;
        if(frame>=500&&frame<506)pad.b=true;
        if(frame>=560&&frame<566)pad.a=true;
    }
    return pad;
}

int run_battle_test(gf::Context& context,gf::RenderEngine& render,const Options& options){
    std::vector<gf::u8> bytes;gf::BattleTestConfig config;std::string error;
    if(!context.files().read(options.battle_config,bytes)||!gf::battle_test_config_load({bytes.data(),bytes.size()},&config,&error)){
        gf::log_error(context,"battle-test","loose configuration refused: %s",error.c_str());render.shutdown();return 2;}
    gf::BattleTest battle;if(!battle.configure(config)){render.shutdown();return 2;}
    gf::BattleTestView view;gf::InputRouter router;unsigned frame=0,commands=0;gf::u64 phase=0,dropped_ticks=0;
    bool captured=false;const auto started=context.clock().monotonic_ns();auto previous=started;
    gf::log_info(context,"battle-test","DIAGNOSTIC: proxy art/test rules, NOT original combat; native tick-policy=%u; loose=%s; no AFS/save/network/game audio",config.tick_rate,options.battle_config.c_str());
    while(context.window().pump_events()&&!context.window().should_close()){
        const auto now=context.clock().monotonic_ns();if(options.seconds&&now-started>=gf::u64(options.seconds)*1000000000ull)break;
        context.input().pump();const auto& snapshot=context.input().snapshot();auto pad=router.route(snapshot,0);
        if(snapshot.quit||pad.quit_requested)break;
        const unsigned width=options.width,height=options.height;unsigned logical_w=width,logical_h=height;context.window().logical_size(&logical_w,&logical_h);
        const auto px=logical_w?gf::s32(gf::s64(snapshot.pointer.x)*width/logical_w):-1;
        const auto py=logical_h?gf::s32(gf::s64(snapshot.pointer.y)*height/logical_h):-1;
        gf::BattlePoint aim;bool pointer=snapshot.pointer.within_window&&snapshot.pointer.left&&gf::BattleTestView::pointer_aim(battle,width,height,px,py,&aim);
        if(pointer)pad.a=true;
        unsigned steps=1;
        if(!options.headless){const auto elapsed=now>=previous?now-previous:0;const auto bounded=std::min<gf::u64>(elapsed,250000000);
            phase+=bounded*config.tick_rate;const auto due=phase/1000000000ull;phase%=1000000000ull;steps=unsigned(std::min<gf::u64>(due,8));
            dropped_ticks+=due-steps+(elapsed-bounded)*config.tick_rate/1000000000ull;}
        previous=now;
        for(unsigned i=0;i<steps;++i){auto input=pad;
            if(options.demo_battle){input={};const auto t=commands%720;input.a=t%18==3;input.z=t==1;input.x=t==719;
                if(t>=120&&t<180)input.stick_x=90;
                if(t>=210&&t<250)input.stick_y=-90;
                if(t==480||t==540)input.start=true;}
            if(!battle.step(input,pointer&&!options.demo_battle?&aim:nullptr)){gf::log_error(context,"battle-test","simulation refused");render.shutdown();return 2;}++commands;}
        render.begin_frame({frame,0.035f,0.045f,0.06f});render.reset_arena();const auto draws=view.draw(render,battle,width,height);render.end_frame();
        if(!captured)captured=capture_if_requested(context,options,frame);
        render.present();context.advance_frame();++frame;
        if(options.frames&&frame>=options.frames)break;
        if(!draws){gf::log_error(context,"battle-test","no diagnostic draws");render.shutdown();return 2;}
    }
    gf::log_info(context,"battle-test","frames=%u commands=%u ticks=%u shots=%u hits=%u down=%u resets=%u dropped-native-ticks=%llu; diagnostic counters, NOT game parity",
        frame,commands,battle.tick(),battle.shots(),battle.hits(),battle.destroyed(),battle.resets(),static_cast<unsigned long long>(dropped_ticks));
    render.shutdown();return 0;
}

int run_battle_mvp(gf::Context& context,gf::RenderEngine& render,const Options& options){
    gf::NativeBattleMvp world;gf::NativeBattleMvpView view;std::string error;
    gf::BattleMvpConfig config;
    //Explicit native demo placement at original stage473 floor triangle1297's
    //interior, not a recovered actor constructor/spawn or edited stage asset.
    if(options.source_surface&&!options.source_ground_interactive)config.spawn[0]={-619.2617f,169.4453f};
    if(options.source_steep&&!options.source_ground_interactive)config.spawn[0]={-606.6735f,-1373.0029f}; //native placement on steep original triangle1320
    if(options.source_fall_surface&&!options.source_ground_interactive)config.spawn[0]={-606.4511719f,-1373.5872803f}; //native interior point on the same original steep face
    //Bounded original-stage recovery route; BOTH placements are native demo
    //assumptions, not recovered actor constructor/spawn or edited stage data.
    if(options.source_recovery_surface&&!options.source_ground_interactive){
        config.spawn[0]={44.928558350f,-332.878112793f};config.spawn[1]={1200,1200};
    }
    //Unchanged original mountain triangle2536. Wider bounds and a static-floor
    //query reference are declared native DEMO assumptions only; normal play
    //keeps its old arena/spawn and no airborne position is fabricated.
    if(options.source_windup_surface&&!options.source_ground_interactive){
        config.arena_half=4500;config.spawn[0]={-2302.447265625f,528.269836426f};
        config.spawn_query_y[0]=591.912719727f;config.spawn[1]={240,-300};
    }
    //Native one-shot active-contact route. Both actors stand on real selected
    //floors; only placement/bounds are demo assumptions, not source spawns.
    if(options.source_active_surface&&!options.source_ground_interactive){
        config.arena_half=4500;config.spawn[0]={295.518066406f,1631.279907227f};
        config.spawn[1]={-2850,3000};config.spawn_query_y={0,0};
    }
    //This neutral relaunch demo needs only a native player placement on the
    //original small incline; reset inherited demo-only bounds/target/reference.
    if(options.source_relaunch_surface&&!options.source_ground_interactive){
        config={};config.spawn[0]={-606.4511719f,-1373.5872803f};
    }
    if(options.source_relaunch_floor&&!options.source_ground_interactive)config={}; //ordinary native MVP spawn, target, bounds and floor references
    if(options.source_windup_entry_push&&!options.source_ground_interactive){
        config={};config.arena_half=4500;config.spawn[0]={-2760.075195312f,-3528.048583984f};config.spawn_query_y[0]=385.716827393f;
    } //native-only placement/reference at original triangle145; interactive bounds/spawn unchanged
    if(options.source_walk_ledge&&!options.source_ground_interactive){
        config={};config.arena_half=4500;config.spawn[0]={1010.166015625f,2315.240722656f};config.spawn_query_y[0]=482.747558594f;
    } //native-only roof249 placement/reference; no actor teleport or edited stage
    if(options.source_dash_ledge&&!options.source_ground_interactive){
        config={};config.arena_half=4500;config.spawn[0]={1012.360657f,1403.599731f};config.spawn_query_y[0]=482.747589f;
    } //native ground spawn on the original roof edge; unchanged target/stage, no airborne teleport
    if(options.source_ground_ledge&&!options.source_ground_interactive){
        config={};config.arena_half=4500;config.spawn[0]={1010.166015625f,2315.240722656f};config.spawn_query_y[0]=482.747558594f;
    } //native ground spawn on original roof249; walk, neutral release, then genuine dash input in the same world
    if(!view.load(context.files(),options.battle_mvp_assets,render,&error)||!world.start(config,&view)||!view.update(world,&error)){
        gf::log_error(context,"battle-mvp","initialization failed: %s",error.c_str());render.shutdown();return 2;}
    gf::NativeGRedGroundDashDemo source;gf::GamePadState source_previous;
    const auto source_inputs=options.source_ground_assets.empty()?std::string(options.source_walk_ledge?"local/content/source-gred-ground-v8":options.source_relaunch_floor?"local/content/source-gred-ground-v7":options.source_steep?"local/content/source-gred-ground-v6":options.source_surface?"local/content/source-gred-ground-v5":options.source_air_dash?"local/content/source-gred-ground-v4":options.source_moving_jump?"local/content/source-gred-ground-v3":
        options.source_ordered_ground?"local/content/source-gred-ground-v9":options.source_ground_walk?"local/content/source-gred-ground-v2":"local/content/source-gred-ground-v1"):options.source_ground_assets;
    const auto source_mode=options.source_walk_ledge?gf::NativeGRedGroundDashDemo::Mode::WalkLedge:options.source_windup_entry_push?gf::NativeGRedGroundDashDemo::Mode::WindupEntryPush:options.source_windup_positive_floor?gf::NativeGRedGroundDashDemo::Mode::WindupPositiveFloor:options.source_windup_entry_floor?gf::NativeGRedGroundDashDemo::Mode::WindupEntryFloor:options.source_air_dash_floor?gf::NativeGRedGroundDashDemo::Mode::AirDashFloor:options.source_relaunch_floor?gf::NativeGRedGroundDashDemo::Mode::RelaunchFloor:options.source_relaunch_entry_surface?gf::NativeGRedGroundDashDemo::Mode::RelaunchEntrySurface:options.source_relaunch_surface?gf::NativeGRedGroundDashDemo::Mode::RelaunchSurface:options.source_active_surface?gf::NativeGRedGroundDashDemo::Mode::ActiveSurface:options.source_windup_surface?gf::NativeGRedGroundDashDemo::Mode::WindupSurface:options.source_recovery_surface?gf::NativeGRedGroundDashDemo::Mode::RecoverySurface:options.source_fall_surface?gf::NativeGRedGroundDashDemo::Mode::FallingSurface:options.source_steep?gf::NativeGRedGroundDashDemo::Mode::Steep:options.source_surface?gf::NativeGRedGroundDashDemo::Mode::Surface:options.source_landing_interrupt?gf::NativeGRedGroundDashDemo::Mode::LandingInterrupt:options.source_dash_jump?gf::NativeGRedGroundDashDemo::Mode::DashJump:options.source_relaunch?gf::NativeGRedGroundDashDemo::Mode::Relaunch:options.source_air_dash?gf::NativeGRedGroundDashDemo::Mode::AirDash:options.source_moving_jump?gf::NativeGRedGroundDashDemo::Mode::Jump:options.source_vertical_jump?gf::NativeGRedGroundDashDemo::Mode::VerticalJump:
        options.source_ground_surface?gf::NativeGRedGroundDashDemo::Mode::GroundSurface:options.source_ground_windup_entry_floor?gf::NativeGRedGroundDashDemo::Mode::GroundWindupEntryFloor:options.source_ground_relaunch_floor?gf::NativeGRedGroundDashDemo::Mode::GroundRelaunchFloor:options.source_ground_relaunch?gf::NativeGRedGroundDashDemo::Mode::GroundRelaunch:options.source_ground_air_floor?gf::NativeGRedGroundDashDemo::Mode::GroundAirFloor:options.source_ground_air?gf::NativeGRedGroundDashDemo::Mode::GroundAir:options.source_ground_landing?gf::NativeGRedGroundDashDemo::Mode::GroundLanding:options.source_ground_jump?gf::NativeGRedGroundDashDemo::Mode::GroundJump:options.source_ground_ledge?gf::NativeGRedGroundDashDemo::Mode::GroundLedge:options.source_dash_ledge?gf::NativeGRedGroundDashDemo::Mode::DashLedge:options.source_ordered_ground?gf::NativeGRedGroundDashDemo::Mode::OrderedGround:options.source_ground_walk?gf::NativeGRedGroundDashDemo::Mode::Ground:gf::NativeGRedGroundDashDemo::Mode::DashOnly;
    auto bind_source=[&](){gf::NativeBorgAnimationSeed seed;return view.source_seed(world,&seed)&&source.bind(context.files(),options.battle_mvp_assets,source_inputs,world,seed,&error,source_mode);};
    if(options.demo_source_ground&&!bind_source()){gf::log_error(context,"source-ground","bind failed: %s",error.c_str());render.shutdown();return 2;}
    gf::InputRouter router;unsigned frame=0,commands=0;gf::u64 phase=0;bool captured=false;const auto started=context.clock().monotonic_ns();auto previous=started;
    gf::log_info(context,"battle-mvp","G-Red vs Sasuke: original stage/models, provisional native gameplay rules; no source-frame equivalence or audio claim");
    if(options.demo_source_ground)gf::log_info(context,"source-ground","original movement programs/motion; native body/terrain proxies and target-center approximation; %s; no original whole-frame equivalence",options.source_ground_surface?"one combined controller with paired surface landing":options.source_ground_windup_entry_floor?"one ground/air/relaunch/entry-floor controller":options.source_ground_relaunch_floor?"one ground/air/relaunch-floor controller":options.source_ground_relaunch?"one ground/air/relaunch controller":options.source_ground_air_floor?"one ground/air controller with scoped floor response":options.source_ground_air?"one ground/ledge/jump/landing/air-dash controller":options.source_ground_landing?"one ground/ledge/jump/landing controller":options.source_ground_jump?"one ground/ledge/jump controller":options.source_walk_ledge?"walk-only ground loss and owned native frame history":options.source_moving_jump?"ground plus moving jump/air steering":options.source_vertical_jump?"ground plus neutral vertical jump":options.source_ground_walk?"ground walk/turn/stop/dash only":"scripted dash only, no source walk/jump/combat");
    if(options.source_ground_walk){
        view.source_ground_controls(options.source_ground_interactive);
        if(options.source_ordered_ground){
            view.source_ordered_ground_controls();
            if(options.source_ground_surface){
                view.source_ground_surface_controls();
                gf::log_info(context,"source-ground","ground-surface controller enabled: shared combined movement plus paired static-surface callback landing, zero698 source angle gate and exact pre-response jump/air integration proof; no guessed UP/equal-height requirement; grounded entry/cache/resource remain separate; push, nonflat ground-loss landing, positive698 overlap, raised-side/clipped/unsupported contacts and combat refused; older policies unchanged");
                if(!options.source_ground_interactive)gf::log_info(context,"source-ground","ground-surface demo only: unchanged native spawn/arena/stage/target; jump5-20, genuine backward air flick48-107, active callback lands on selected inclined plane108 then separate landing/idle; no teleport/rebind");
            }else if(options.source_ground_windup_entry_floor){
                view.source_ground_windup_entry_floor_controls();
                gf::log_info(context,"source-ground","ground-windup-entry-floor controller enabled: shared ground/air/relaunch/floor plus genuine negative/positive wind-up entry snaps; halve44/48 without translation/integration/cost, then ret1 zero48, retaining50/cache/timer; push, ground-loss relaunch/positive698 overlap, nonflat landing, clipping and combat refused; older policies unchanged");
                if(!options.source_ground_interactive)gf::log_info(context,"source-ground","ground-windup-entry-floor demo only: unchanged native spawn/arena/stage/target; jump5-30, relaunch78-86, genuine right flick86-124 after positive relaunch snap; separate wind-up/active/touchdown/landing; no teleport/rebind");
            }else if(options.source_ground_relaunch_floor){
                view.source_ground_relaunch_floor_controls();
                gf::log_info(context,"source-ground","ground-relaunch-floor controller enabled: shared ground/air/relaunch plus null-callback selected-floor snaps with negative-only .8 velocity tail; preserve50/latches/airborne cache and exact costs; wind-up entry snaps, push, ground-loss relaunch/positive698 overlap, nonflat landing, side-entry/clipping and combat refused; older policies unchanged");
                if(!options.source_ground_interactive)gf::log_info(context,"source-ground","ground-relaunch-floor demo only: unchanged native spawn/arena/stage/target; jump5-30, genuine relaunch press78-242 held through source depletion; eight snaps then lift, separate falling21/landing; no teleport/rebind");
            }else if(options.source_ground_relaunch){
                view.source_ground_relaunch_controls();
                gf::log_info(context,"source-ground","ground-relaunch controller enabled: shared ground/air/floor plus airborne relaunch with independent lower/upper14/15 latches, additive15 entry, exact scalar/gravity/resource proof and released-relaunch21-24 lineage; relaunch-floor, wind-up entry snaps, push, ground-loss relaunch/positive698 overlap, nonflat landing, clipping and combat refused; older policies unchanged");
                if(!options.source_ground_interactive)gf::log_info(context,"source-ground","ground-relaunch demo only: unchanged native spawn/arena/stage/target; jump5-30, genuine relaunch press60-99, then release/falling21/landing; no teleport/rebind");
            }else if(options.source_ground_air_floor){
                view.source_ground_air_floor_controls();
                gf::log_info(context,"source-ground","ground-air-floor controller enabled: shared ground/air plus wind-up-update zero48 and early-active retained48 snaps; null callback stays airborne until timer readiness; wind-up entry snaps, relaunch, push, positive698 overlap, nonflat landing, side-entry/clipping and combat refused; older policies unchanged");
                if(!options.source_ground_interactive)gf::log_info(context,"source-ground","ground-air-floor demo only: unchanged native spawn/arena/stage/target; jump5-20, genuine air flick60-96; no teleport/rebind");
            }else if(options.source_ground_air){
                view.source_ground_air_controls();
                gf::log_info(context,"source-ground","ground-air controller enabled: shared ground/ledge/jump/accepted landing input plus source air-dash wind-up/active/recovery; exact phase/scalar/gravity/timer/resource evidence; relaunch, floor-snap/push, positive698 overlap, nonflat landing, clipping and combat refused; older policies unchanged");
                if(!options.source_ground_interactive)gf::log_info(context,"source-ground","ground-air demo only: unchanged native spawn/arena/stage/target; jump5-349, genuine air flick45-86 and90-131, then recovery/falling/landing; one producer, no teleport/rebind");
            }else if(options.source_ground_landing){
                view.source_ground_landing_controls();
                gf::log_info(context,"source-ground","ground-landing controller enabled: new jump/owned ordered dash after accepted landing-entry update, never on touchdown; source buffered intent before old landing dispatch, explicit native target4/12 evidence; air dash/relaunch/combat, nonflat landing/clipping and unowned variants refused; older modes unchanged");
                if(!options.source_ground_interactive)gf::log_info(context,"source-ground","ground-landing demo only: unchanged native spawn/arena/stage/target; neutral jump5-34, accepted-entry-driven second jump and later backward landing dash, one producer, no teleport/rebind");
            }else if(options.source_ground_jump){
                view.source_ground_jump_controls();
                gf::log_info(context,"source-ground","ground-jump controller enabled: shared walk/stop, owned ordered dash, both ledges and source jump; buffered dash-to-takeoff admission before old root/motion, source resource and exact jump kinematics; air dash/relaunch/landing cancellation/combat, nonflat landing and clipping refused; native proxies, older policies unchanged");
                if(!options.source_ground_interactive)gf::log_info(context,"source-ground","ground-jump demo only: unchanged native spawn/arena/stage/target; walk5-24, neutral25-26, backward flick27-34, jump35-70 then release; one world/producer, no teleport/rebind");
            }else if(options.source_ground_ledge){
                view.source_ground_ledge_controls();
                gf::log_info(context,"source-ground","ground-ledge controller enabled: shared walk/stop and owned-root ordered dash can both depart; distinct walking tangent vs post-dash scalar checks, next priority program13, delayed drift and flat landing; jump/air-dash/flight/combat, nonflat dash/landing and clipping refused; native proxies, older policies unchanged");
                if(!options.source_ground_interactive)gf::log_info(context,"source-ground","ground-ledge demo only: native roof249 ground spawn/reference and arena4500; walk5-69, neutral70-71, genuine flick72 then one post-delay drift; same world/producer, no teleport/rebind or stage/target edit; interactive spawn/bounds unchanged");
            }else if(options.source_dash_ledge){
                view.source_dash_ledge_controls();
                gf::log_info(context,"source-ground","dash-ledge enabled: owned ordered dash departure retains post-motion scalar; next frame priority program13, timer698 drift delay and flat touchdown; walking ledges, jump/air-dash/flight/combat, nonflat dash/landing and clipping refused; native clocks/target/contact proxies, older policies unchanged");
                if(!options.source_ground_interactive)gf::log_info(context,"source-ground","dash-ledge demo only: native ground spawn1012.360657/482.747589/1403.599731, arena4500, original roof/stage and unchanged target; hold through one post-delay drift then release; interactive spawn/bounds unchanged, no airborne teleport");
            }else gf::log_info(context,"source-ground","ordered ground enabled: immediate dash entry motion, incoming-speed steering, sealed post-VM AObj root-speed reads, motion before done/reset; flat dash floor only, no ledge/jump/flight/combat/clipping or multiplier/null-root fallback; native clocks/target/contact proxies, older modes unchanged");
        }
        if(!options.source_vertical_jump&&!options.source_ground_jump&&!options.source_ground_landing&&!options.source_ground_air&&!options.source_ground_air_floor&&!options.source_ground_relaunch&&!options.source_ground_relaunch_floor&&!options.source_ground_windup_entry_floor&&!options.source_ground_surface)gf::log_info(context,"source-ground","%s source walk/turn/stop/dash; arrows/D-pad WALK, stick/WASD FLICK DASH; Enter pause, Q restart; jump/flight/combat refused before simulation; native rate664=1",options.source_ground_interactive?"interactive":"scripted");
    }
    if(options.source_vertical_jump){
        view.source_vertical_jump_controls(source.resource(),options.source_moving_jump,options.source_air_dash,options.source_relaunch,options.source_dash_jump,options.source_landing_interrupt,options.source_surface,options.source_steep,options.source_fall_surface,options.source_recovery_surface,options.source_windup_surface,options.source_active_surface,options.source_relaunch_surface,options.source_relaunch_entry_surface,options.source_relaunch_floor);
        if(options.source_walk_ledge){
            view.source_walk_ledge_controls();
            gf::log_info(context,"source-ground","source walk-ledge enabled: native-owned frame-start snapshot, delayed priority6/marker1/program13, scoped air/landing/idle flag writes, timer698 drift suppression and flat touchdown; jump/flick/dash/flight/combat, nonflat landings, clipping and unresolved geometry refused; native proxies only");
            if(!options.source_ground_interactive)gf::log_info(context,"source-ground","walk-ledge demo only: native arena-half4500, roof249 placement/reference482.7476; hold walk through one delayed drift update then release; target/interactive arena/spawn unchanged; not original spawn/input data");
        }else if(options.source_windup_entry_push){
            view.source_windup_entry_push_controls();
            gf::log_info(context,"source-ground","source wind-up entry push enabled: half old44/48 without displacement/acceleration, then steep ret0 .9 projection; NOT ret1 zero48 or landing/refill; exact entry receipt; positive-speed entry pushes/raised-side/attached/unknown contacts refused; native proxies only");
            if(!options.source_ground_interactive)gf::log_info(context,"source-ground","windup-entry-push demo only: native arena-half4500, player placement/reference at original triangle145; initial floor reference385.7168; flick after accepted falling push; target/interactive arena/spawn unchanged; not original input/spawn data");
        }else if(options.source_windup_positive_floor){
            view.source_air_dash_floor_controls();view.source_windup_entry_floor_controls();view.source_windup_positive_floor_controls();
            gf::log_info(context,"source-ground","source positive-speed wind-up floor enabled: entry halves positive48 then ret1 zeros48 at unchanged position; no translation/acceleration/spend/refill, retain50/history; separate positive receipt; rising-motion/entry-push/raised-side/attached/unknown geometry refused; native proxies only");
            if(!options.source_ground_interactive)gf::log_info(context,"source-ground","windup-positive-floor demo: ordinary native spawn/bounds/target/references; eight relaunch snaps then right flick86-112; hold jump until accepted air-dash entry; not original input/AI/spawn data");
        }else if(options.source_windup_entry_floor){
            view.source_air_dash_floor_controls();view.source_windup_entry_floor_controls();
            gf::log_info(context,"source-ground","source wind-up entry floor enabled: no translation or acceleration on entry, half old44/48 then ret1 zero48; retain50/history, no entry cost/refill; separate receipt; positive-speed/entry-push/raised-side/attached/unknown contacts refused; native proxies only");
            if(!options.source_ground_interactive)gf::log_info(context,"source-ground","windup-entry-floor demo: ordinary native spawn/bounds/target/references; two relaunch snaps then right flick80-106; hold jump until accepted air-dash entry; not original input/AI/spawn data");
        }else if(options.source_air_dash_floor){
            view.source_air_dash_floor_controls();
            gf::log_info(context,"source-ground","source air-dash null-callback floor enabled: wind-up clears48, early active preserves48, both retain50/history; real65E64 remains separate; wind-up-entry/raised-side/attached/unknown contacts refused; native proxies, no whole-frame equivalence");
            if(!options.source_ground_interactive)gf::log_info(context,"source-ground","air-dash-floor demo: ordinary native spawn/bounds/target/references; right flick78-104, separate callback touchdown after minimum active timer; not original input/AI/spawn data");
        }else if(options.source_relaunch_floor){
            gf::log_info(context,"source-ground","source null-callback relaunch floor response enabled: selecting8 clears754; static snap preserves50/87/latches, negative48 gets caller .8, NOT landing7/refill; distinct floor receipts and GFMM7; attachments/offsets/platforms/combat refused; native proxies, no whole-frame equivalence");
            if(!options.source_ground_interactive)gf::log_info(context,"source-ground","relaunch-floor demo: ordinary native spawn/bounds/target/references; holds through two accepted null-callback snaps then releases to separate falling touchdown; not original input/AI/spawn data");
        }else if(options.source_relaunch_entry_surface){
            gf::log_info(context,"source-ground","source descending relaunch-entry surface push enabled: add15 BEFORE displacement/positive acceleration/clamp/.9 projection; request14 and spend150 once, no repeated additive entry/refill; distinct native entry-push capability/receipt, GFMM6; rising entries, relaunch landing, platforms/combat refused; native proxies, no whole-frame equivalence");
            if(!options.source_ground_interactive)gf::log_info(context,"source-ground","entry-push demo only: native player placement on triangle1320, ordinary arena/target/references; releases jump and samples one soft left drift after accepted entry push, not a motion override or original input/spawn script");
        }else if(options.source_relaunch_surface){
            gf::log_info(context,"source-ground","source continued relaunch surface push enabled: programs14/15 retain separate lower/upper latches, continuation spend and positive neutral/directional acceleration; .9 projection stays airborne, no repeated additive entry; relaunch-only evidence/native receipt; entry pushes, landing, offsets/platforms/combat refused; GFMM6/native proxies, no whole-frame equivalence");
            if(!options.source_ground_interactive)gf::log_info(context,"source-ground","relaunch demo only: native player placement on original triangle1320; ordinary arena-half1800, target and floor references retained; releases jump after first accepted relaunch push, not an original input/spawn script");
        }else if(options.source_active_surface){
            gf::log_info(context,"source-ground","source continued active surface push enabled: selector16/program11 spending before steering/motion/contact; readiness timer is not forced recovery, depletion preserves readiness then enters separate17; -.07 acceleration/.9 push, GFMM6; entry pushes, active-relaunch pushes, early/null landing, platforms and combat refused; native proxies, no whole-frame equivalence");
            if(!options.source_ground_interactive)gf::log_info(context,"source-ground","active demo only: native arena-half=4500 and Borg placements; real floor references0; releases direction after first accepted active push, not an original AI/input script; interactive arena/spawn unchanged");
        }
        else if(options.source_windup_surface){
            gf::log_info(context,"source-ground","source continued windup surface push enabled: selector15/program10 old-velocity integration, -.07 acceleration and .9 push damping; timer/cost before contact, separate active-entry reset; entry-frame pushes, early/null-callback landing, active dash/relaunch pushes, platforms and combat refused; GFMM6/native proxies, no whole-frame equivalence");
            if(!options.source_ground_interactive)gf::log_info(context,"source-ground","windup demo only: native arena-half=4500 ground-query-reference=591.9127 at original triangle2536; native Borg placements, unchanged stage; interactive arena/spawn unchanged");
        }else if(options.source_recovery_surface)gf::log_info(context,"source-ground","source airborne dash recovery surface push enabled: selector17/program12 motion and selected-plane .9 projection precede lower completion; no landing/refill shortcut; GFMM6 and distinct native recovery-push receipt; windup/active dash/relaunch pushes, grounded recovery, unresolved seams, platforms and combat refused; native proxies, no whole-frame equivalence");
        else if(options.source_fall_surface)gf::log_info(context,"source-ground","source selector6 surface contact includes post-dash/relaunch falling: retains programs13/21-24, resets falling acceleration before shared motion/contact; selected-plane .9 push stays airborne without refill; GFMM6; windup/active dash/relaunch pushes, unresolved seams, platforms and combat refused; native proxies, no whole-frame equivalence");
        else if(options.source_steep)gf::log_info(context,"source-ground","source static steep push enabled for ordinary falling only: selected-plane projection, .9 speed damping, remains airborne with no landing/refill; GFMM6 and independent native terrain validation required; takeoff samples re-press without locomotion cancel; dash/relaunch steep variants/attachments/combat refused; native proxies, no original whole-frame equivalence");
        else if(options.source_surface)gf::log_info(context,"source-ground","source static surface landing enabled: paired selected height/normal, away-displacement gate and normal-G-Red angle limit; takeoff samples re-press without locomotion cancel; steep response/attachments/combat refused; native contact/camera/ceiling proxies, no original whole-frame equivalence");
        else if(options.source_landing_interrupt)gf::log_info(context,"source-ground","source landing interruption enabled after accepted landing entry: jump before ground dash; cached87 resource recovery, no completion/refill shortcut; touchdown cancellation/takeoff interruption/combat refused; native flat-floor/ceiling proxy, effects omitted");
        else if(options.source_dash_jump)gf::log_info(context,"source-ground","source dash-to-jump enabled: buffered/held jump admission before old dash dispatch; normal class12 clears dash speed; jump/air-dash/relaunch retained; landing/takeoff interruption/combat refused; native flat-floor/ceiling proxy, effects omitted");
        else if(options.source_relaunch)gf::log_info(context,"source-ground","source relaunch enabled: fresh airborne Shift/B through source readiness/resource predicate; additive launch velocity; original lower/upper windup-to-ascent handoff; dash-to-jump/landing interruption/combat refused; native flat-floor/ceiling proxy, effects omitted");
        else if(options.source_air_dash)gf::log_info(context,"source-ground","source air dash enabled: jump with Shift/B; arrows/D-pad steer, airborne WASD/stick flicks dash; source policy1 windup/active/recovery costs and timer; relaunch/dash-to-jump/combat refused; native flat-floor/ceiling proxy, effects omitted");
        else if(options.source_moving_jump)gf::log_info(context,"source-ground","moving source jump enabled: Shift/B from walk/idle; native jump directions capped32 below flick threshold; ascent body turn, falling camera-relative drift; relaunch/air dash/dash-to-jump/combat refused; native flat-floor/ceiling proxy, effects omitted");
        else gf::log_info(context,"source-ground","vertical source jump enabled: Shift/B from idle; neutral stick in air; held ascent spends source resource; release/depletion falls; moving takeoff/air steering/relaunch/dash/combat refused; native flat-floor/ceiling proxy, effects omitted");
    }
    bool input_refused=false;
    gf::u32 landing_jump_script_tick=0,landing_dash_script_tick=0;
    gf::u32 windup_entry_push_flick_tick=0;
    for(unsigned i=0;i<2;++i){gf::BattleMvpVec low,high;if(view.actor_bounds(i,&low,&high))gf::log_info(context,"battle-mvp","actor=%u ground-y=%.1f model-y=%.1f..%.1f",i,world.actors()[i].position[1],low[1],high[1]);}
    while(context.window().pump_events()&&!context.window().should_close()){
        const auto now=context.clock().monotonic_ns();if(options.seconds&&now-started>=gf::u64(options.seconds)*1000000000ull)break;
        context.input().pump();const auto& snapshot=context.input().snapshot();const auto pad=router.route(snapshot,0);if(snapshot.quit||pad.quit_requested)break;
        unsigned steps=1;
        if(!options.headless&&!((options.demo_battle_mvp||options.demo_locomotion||(options.demo_source_ground&&!options.source_ground_interactive))&&options.frames)){
            const auto elapsed=now>=previous?std::min<gf::u64>(now-previous,250000000):0;phase+=elapsed*60;steps=unsigned(std::min<gf::u64>(phase/1000000000ull,8));phase%=1000000000ull;}
        previous=now;
        for(unsigned i=0;i<steps;++i){
            auto input=pad;
            if(options.demo_battle_mvp){input={};const auto t=commands%600;input.a=t%90<30;input.x=t==120;input.b=t==180;input.y=t==599;input.stick_x=t>=60&&t<160?80:t>=220&&t<280?-80:0;}
            if(options.demo_locomotion){
                input={};const auto t=commands%360;input.y=t==359;input.b=t==160;
                if(t<42){input.stick_y=-90;input.l=t==30;}
                else if(t>=60&&t<72){input.stick_x=-127;input.l=t==60;}
                else if(t>=90&&t<102){input.stick_y=127;input.l=t==90;}
                else if(t>=120&&t<132){input.stick_x=127;input.l=t==120;}
                else if(t==180){input.stick_x=127;input.l=true;}
            }
            const auto movement=gf::NativeBattleMvpView::movement_frame(world,options.width,options.height);
            if(options.demo_source_ground){
                gf::GamePadState controls;controls.start=pad.start;controls.y=pad.y;
                gf::NativeGRedGroundInput mapped;
                if(options.source_ground_walk){
                    auto device=pad;
                    if(!options.source_ground_interactive){
                        device={};const unsigned t=world.tick()%(options.source_air_dash?480:options.source_moving_jump?480:options.source_vertical_jump?600:360);
                         if(options.source_ground_surface){
                             const auto tick=world.tick();device.b=tick>=5&&tick<21;
                             if(tick>=48&&tick<108)device.stick_y=127;
                         }else if(options.source_ground_windup_entry_floor){
                             const auto tick=world.tick();device.b=(tick>=5&&tick<31)||(tick>=78&&tick<87);
                             if(tick>=86&&tick<125)device.stick_x=127;
                         }else if(options.source_ground_relaunch_floor){
                             const auto tick=world.tick();device.b=(tick>=5&&tick<31)||(tick>=78&&tick<243);
                         }else if(options.source_ground_relaunch){
                             const auto tick=world.tick();device.b=(tick>=5&&tick<31)||(tick>=60&&tick<100);
                         }else if(options.source_ground_air_floor){
                             const auto tick=world.tick();device.b=tick>=5&&tick<21;
                             if(tick>=60&&tick<97)device.stick_y=-127;
                         }else if(options.source_ground_air){
                             const auto tick=world.tick();device.b=tick>=5&&tick<350;
                             if((tick>=45&&tick<87)||(tick>=90&&tick<132))device.stick_y=-127;
                         }else if(options.source_ground_landing){
                             const auto tick=world.tick();device.b=tick>=5&&tick<35;
                             if(!landing_jump_script_tick&&source.landing_entry_accepted()&&!source.landing_jumps())landing_jump_script_tick=tick;
                             if(landing_jump_script_tick&&tick<landing_jump_script_tick+20)device.b=true;
                             if(landing_jump_script_tick&&!landing_dash_script_tick&&source.landings()>=2&&source.landing_entry_accepted())landing_dash_script_tick=tick;
                             if(landing_dash_script_tick&&tick<landing_dash_script_tick+12)device.stick_y=127;
                         }else if(options.source_ground_jump){
                             const auto tick=world.tick();
                             if(tick>=5&&tick<25)device.dpad_down=true;
                             if(tick>=27&&tick<35)device.stick_y=127;
                             if(tick>=35&&tick<71){device.b=true;device.dpad_down=true;}
                         }else if(options.source_ground_ledge){
                             const auto tick=world.tick();
                             if(tick>=5&&(!source.ground_loss_entries()||source.ground_loss_timer698()>0)){
                                 if(tick<70)device.dpad_up=true;else if(tick>=72)device.stick_y=-127;
                             }
                         }else if(options.source_dash_ledge){
                             device.stick_y=world.tick()>=5&&(!source.ground_loss_entries()||source.ground_loss_timer698()>0)?-127:0;
                         }else if(options.source_ordered_ground){
                             device.stick_y=world.tick()>=5&&world.tick()<17?127:0; //one native backward flick; unchanged spawn/stage/arena
                         }else if(options.source_walk_ledge){
                             const auto tick=world.tick();device.dpad_up=tick>=5&&(!source.ground_loss_entries()||source.ground_loss_timer698()>0);
                         }else if(options.source_windup_entry_push){
                             const auto tick=world.tick();device.b=(tick>=5&&tick<8)||(tick>=9&&tick<32);
                             if(source.surface_pushes()&&!windup_entry_push_flick_tick)windup_entry_push_flick_tick=tick;
                             if(windup_entry_push_flick_tick&&tick<windup_entry_push_flick_tick+27)device.stick_y=-127;
                         }else if(options.source_windup_positive_floor){
                             const auto tick=world.tick();device.b=(tick>=5&&tick<31)||(tick>=78&&!source.air_dash_starts());
                             if(tick>=86&&tick<113)device.stick_x=127;
                         }else if(options.source_windup_entry_floor){
                             const auto tick=world.tick();device.b=(tick>=5&&tick<31)||(tick>=78&&!source.air_dash_starts());
                             if(tick>=80&&tick<107)device.stick_x=127;
                         }else if(options.source_air_dash_floor){
                             const auto tick=world.tick();device.b=tick>=5&&tick<31;if(tick>=78&&tick<105)device.stick_x=127;
                         }else if(options.source_relaunch_floor){
                             const auto tick=world.tick();device.b=(tick>=5&&tick<31)||(tick>=78&&source.relaunch_floor_contacts()<2);
                         }else if(options.source_relaunch_entry_surface){
                             const auto tick=world.tick();device.b=(tick>=5&&tick<31)||(tick>=78&&!source.relaunch_surface_pushes());
                             if(source.relaunch_entry_surface_pushes()&&source.selector()==8)device.dpad_left=true;
                         }else if(options.source_relaunch_surface){
                             const auto tick=world.tick();device.b=(tick>=5&&tick<33)||(tick>=81&&!source.relaunch_surface_pushes());
                         }else if(options.source_active_surface){
                             const auto tick=world.tick();device.b=tick>=5&&tick<40;
                             if(tick>=38&&!source.active_pushes())device.stick_x=127; //accepted-counter-driven demo input, never a motion override
                         }else if(options.source_windup_surface){
                             const auto tick=world.tick();device.b=tick>=5&&tick<33;
                             if(tick>=79&&tick<129)device.stick_x=-127; //one-shot native route, not a recovered spawn/input script
                         }else if(options.source_recovery_surface){
                             device.b=t>=5&&t<22;if(t>=20&&t<61)device.stick_y=127;
                         }else if(options.source_fall_surface){
                             device.b=(t>=5&&t<33)||(t>=45&&t<60);
                         }else if(options.source_surface){
                             device.b=(t>=5&&t<8)||(t>=9&&t<(options.source_steep?33u:30u));
                        }else if(options.source_landing_interrupt){
                            const auto tick=world.tick();
                            if(!source.landing_jumps()){
                                device.b=tick>=5&&tick<30;
                                if(source.landing_entry_accepted()){device.b=true;landing_jump_script_tick=tick;}
                            }else if(!source.landing_dashes()){
                                device.b=tick<landing_jump_script_tick+25;
                                if(source.landing_entry_accepted()){device.b=false;device.stick_x=-127;landing_dash_script_tick=tick;}
                            }else if(tick<landing_dash_script_tick+10)device.stick_x=-127;
                        }else if(options.source_dash_jump){
                            if(t>=5&&t<15)device.stick_y=-127;
                            if(t>=180&&t<186)device.stick_x=-127;
                            device.b=(t>=15&&t<45)||(t>=186&&t<216);
                        }else if(options.source_relaunch){
                            device.b=(t>=5&&t<35)||(t>=45&&t<85)||(t>=110&&t<150);
                        }else if(options.source_air_dash){
                            device.b=t>=5&&t<350;
                            if((t>=45&&t<87)||(t>=90&&t<132))device.stick_y=-127;
                        }else if(options.source_moving_jump){
                            device.b=(t>=5&&t<35)||(t>=200&&t<230);
                            device.dpad_right=t<20;device.dpad_up=t>=20&&t<35;device.dpad_left=t>=35&&t<65;
                            if(t>=190&&t<215)device.dpad_left=true;
                            if(t>=215&&t<230)device.dpad_right=true;
                            if(t>=230&&t<260)device.dpad_down=true;
                        }else if(options.source_vertical_jump){
                            device.b=(t>=5&&t<30)||(t>=150&&t<450);
                        }else{
                            if(t>=5&&t<65){device.dpad_up=t<30;device.dpad_right=t>=30;}
                            if(t>=95&&t<125)device.dpad_left=true;
                            if(t>=125&&t<137)device.stick_y=-60;
                            if(t>=230&&t<270)device.dpad_down=true;
                            if(t>=278&&t<310)device.dpad_right=true;
                        }
                    }
                    if(!source.map_input(device,&mapped,&error)){
                        if(!input_refused)gf::log_info(context,"source-ground","%s; no simulation advanced",error.c_str());
                        input_refused=true;continue;
                    }
                    input_refused=false;
                }
                const bool restart=controls.y&&!source_previous.y,pause=controls.start&&!source_previous.start;
                if(restart||pause||world.paused()){
                    if(!world.step({controls,{}},false,&view)){render.shutdown();return 2;}
                    if(restart&&(!view.update(world,&error)||!bind_source())){gf::log_error(context,"source-ground","restart bind refused: %s",error.c_str());render.shutdown();return 2;}
                    if(restart)windup_entry_push_flick_tick=0;
                }else{
                    gf::s16 x=0,y=0;gf::u32 held=0;const unsigned t=world.tick()%360,part=t/90,phase=t%90;
                    if(phase>=5&&phase<17){
                        const std::array<gf::s16,4> xs={0,-60,0,60},ys={60,0,-60,0};
                        const std::array<gf::u32,4> bits={8,1,4,2};x=xs[part];y=ys[part];held=bits[part]<<16;
                    }
                    if(options.source_ground_walk){x=mapped.x;y=mapped.y;held=mapped.held;}
                    gf::NativeBorgMotionOffer offer;
                    if(!source.prepare(world,view.source_binding(),movement,x,y,held,&offer,&error)||
                       !world.step({controls,{}},false,&view,nullptr,&movement,&offer)||!source.commit(world,view.source_binding(),&error)){
                        source.reject();gf::log_error(context,"source-ground","motion acceptance refused: tick=%u kind=%u from-y=%.3f proposed-y=%.3f old-v=%.3f; %s",world.tick(),unsigned(offer.kind),offer.observed.position[1],offer.proposed_position[1],offer.observed.vertical_speed,error.c_str());render.shutdown();return 2;
                    }
                }
                source_previous=controls;++commands;continue;
            }
            if(!world.step({input,{}},!options.demo_locomotion,&view,nullptr,&movement)){gf::log_error(context,"battle-mvp","gameplay/terrain step failed");render.shutdown();return 2;}++commands;
        }
        bool posed=false;
        if(options.demo_source_ground&&world.actors()[0].source_ground_motion){gf::NativeBorgSourcePose pose;posed=source.pose(world,&pose)&&view.update_source(world,pose,&error);}
        else posed=view.update(world,&error);
        if(!posed){gf::log_error(context,"battle-mvp","animation failed: %s",error.c_str());render.shutdown();return 2;}
        if(options.source_vertical_jump)view.source_vertical_jump_controls(source.resource(),options.source_moving_jump,options.source_air_dash,options.source_relaunch,options.source_dash_jump,options.source_landing_interrupt,options.source_surface,options.source_steep,options.source_fall_surface,options.source_recovery_surface,options.source_windup_surface,options.source_active_surface,options.source_relaunch_surface,options.source_relaunch_entry_surface,options.source_relaunch_floor);
        render.begin_frame({frame,.15f,.21f,.3f});render.reset_arena();const auto draws=view.draw(render,world,options.width,options.height,&error);render.end_frame();
        if(!draws){gf::log_error(context,"battle-mvp","draw failed: %s",error.c_str());render.shutdown();return 2;}
        if(!captured)captured=capture_if_requested(context,options,frame);
        render.present();context.advance_frame();++frame;if(options.frames&&frame>=options.frames)break;
    }
    gf::log_info(context,"battle-mvp","frames=%u ticks=%u projectiles-spawned=%u hits=%u health=%u/%u result=%u",frame,world.tick(),world.shots_spawned(),world.hits(),world.actors()[0].health,world.actors()[1].health,unsigned(world.result()));
    if(options.demo_source_ground)gf::log_info(context,"source-ground","accepted native tick=%u G-Red-position=%.2f/%.2f/%.2f; accepted-frames=%llu body-proxy-contacts=%llu; scoped movement, native proxy response",world.tick(),world.actors()[0].position[0],world.actors()[0].position[1],world.actors()[0].position[2],static_cast<unsigned long long>(source.accepted_frames()),static_cast<unsigned long long>(source.body_contacts()));
    if(options.source_vertical_jump)gf::log_info(context,"source-ground","%s-air-frames=%llu landings=%llu resource=%d selector=%u; %s, native contact proxy",options.source_walk_ledge?"ground-loss":options.source_moving_jump?"moving":"vertical",static_cast<unsigned long long>(source.air_frames()),static_cast<unsigned long long>(source.landings()),int(source.resource()),unsigned(source.selector()),options.source_walk_ledge?"walk-only source loss subset":"source jump subset");
    if(options.source_air_dash)gf::log_info(context,"source-ground","air-dash-starts=%llu active-frames=%llu; source policy1, no per-flight count substitution",static_cast<unsigned long long>(source.air_dash_starts()),static_cast<unsigned long long>(source.air_dash_active_frames()));
    if(options.source_relaunch)gf::log_info(context,"source-ground","relaunches=%llu lower-stage=%u upper-stage=%u; source predicate/meter, no fixed jump-count substitution",static_cast<unsigned long long>(source.relaunches()),unsigned(source.relaunch_stages().lower_stage),unsigned(source.relaunch_stages().upper_stage));
    if(options.source_dash_jump)gf::log_info(context,"source-ground","dash-jumps=%llu last-interrupted-selector=%u; source class12 takeoff reset, no MVP dash-momentum carry",static_cast<unsigned long long>(source.dash_jumps()),unsigned(source.last_dash_jump_selector()));
    if(options.source_landing_interrupt)gf::log_info(context,"source-ground","landing-jumps=%llu landing-dashes=%llu; accepted landing entry required, no walk/instant-touchdown cancel",static_cast<unsigned long long>(source.landing_jumps()),static_cast<unsigned long long>(source.landing_dashes()));
    if(options.source_surface)gf::log_info(context,"source-ground","paired-surface-landings=%llu last-angle-s16=%d; selected original static grid, no guessed UP normal or steep-response fallback",static_cast<unsigned long long>(source.surface_landings()),int(source.last_surface_angle()));
    if(options.source_steep)gf::log_info(context,"source-ground","static-surface-pushes=%llu last-push-angle-s16=%d; accepted projection frames only, not landings or grounded refill",static_cast<unsigned long long>(source.surface_pushes()),int(source.last_push_angle()));
    if(options.source_fall_surface)gf::log_info(context,"source-ground","post-dash-fall-pushes=%llu post-relaunch-fall-pushes=%llu last-push-programs=%u/%u; selector6 only, no active-callback push fallback",static_cast<unsigned long long>(source.dash_fall_pushes()),static_cast<unsigned long long>(source.relaunch_fall_pushes()),unsigned(source.last_push_programs()[0]),unsigned(source.last_push_programs()[1]));
    if(options.source_recovery_surface)gf::log_info(context,"source-ground","recovery-surface-pushes=%llu; selector17 callback receipts, lower completion tested after contact, no fabricated landing",static_cast<unsigned long long>(source.recovery_pushes()));
    if(options.source_windup_surface)gf::log_info(context,"source-ground","windup-surface-pushes=%llu; continued selector15 callback receipts, no repeated half-speed entry or landing/refill shortcut",static_cast<unsigned long long>(source.windup_pushes()));
    if(options.source_active_surface)gf::log_info(context,"source-ground","active-surface-pushes=%llu last-active-push-programs=%u/%u; executed selector16 receipts, no entry replay/refill",static_cast<unsigned long long>(source.active_pushes()),unsigned(source.last_active_push_programs()[0]),unsigned(source.last_active_push_programs()[1]));
    if(options.source_relaunch_surface)gf::log_info(context,"source-ground","relaunch-surface-pushes=%llu last-relaunch-push-programs=%u/%u; selector8 receipts, entry projections counted separately; no landing/refill or additive-entry replay",static_cast<unsigned long long>(source.relaunch_surface_pushes()),unsigned(source.last_relaunch_push_programs()[0]),unsigned(source.last_relaunch_push_programs()[1]));
    if(options.source_relaunch_entry_surface)gf::log_info(context,"source-ground","relaunch-entry-surface-pushes=%llu; distinct additive entry receipts, no repeated entry or fabricated landing",static_cast<unsigned long long>(source.relaunch_entry_surface_pushes()));
    if(options.source_relaunch_floor)gf::log_info(context,"source-ground","null-callback-relaunch-floor-contacts=%llu last-floor-programs=%u/%u; floor receipts preserve8/airborne, no65E64 or grounded refill on snaps",static_cast<unsigned long long>(source.relaunch_floor_contacts()),unsigned(source.last_relaunch_floor_programs()[0]),unsigned(source.last_relaunch_floor_programs()[1]));
    if(options.source_air_dash_floor)gf::log_info(context,"source-ground","air-dash-floor-windup=%llu active=%llu last-air-floor-programs=%u/%u; wind-up zero48 / null-active preserves48; snaps retain50 and airborne history",static_cast<unsigned long long>(source.windup_floor_contacts()),static_cast<unsigned long long>(source.active_floor_contacts()),unsigned(source.last_air_dash_floor_programs()[0]),unsigned(source.last_air_dash_floor_programs()[1]));
    if(options.source_windup_entry_floor)gf::log_info(context,"source-ground","windup-entry-floor-contacts=%llu; distinct no-motion half-speed entry receipts, not repeated continuation or grounded refill",static_cast<unsigned long long>(source.windup_entry_floor_contacts()));
    if(options.source_windup_positive_floor)gf::log_info(context,"source-ground","windup-positive-floor-contacts=%llu; exact positive half-speed then zero48 at unchanged position, not rising integration or grounded refill",static_cast<unsigned long long>(source.positive_windup_floor_contacts()));
    if(options.source_windup_entry_push)gf::log_info(context,"source-ground","windup-entry-surface-pushes=%llu; half-speed before .9 ret0 response, no translated entry or floor-zero/refill shortcut",static_cast<unsigned long long>(source.windup_entry_surface_pushes()));
    if(options.source_walk_ledge||options.source_dash_ledge||options.source_ground_ledge)gf::log_info(context,"source-ground","ground-departures=%llu loss-entries=%llu loss-landings=%llu suppressed-drift=%llu timer698=%.1f cached87=%u flags=%x; owned native history, no original actor-frame equivalence",static_cast<unsigned long long>(source.ground_departures()),static_cast<unsigned long long>(source.ground_loss_entries()),static_cast<unsigned long long>(source.ground_loss_landings()),static_cast<unsigned long long>(source.suppressed_ground_drifts()),source.ground_loss_timer698(),unsigned(source.cached_airborne87()),source.movement_flags());
    if(options.source_ordered_ground)gf::log_info(context,"source-ground","ordered-entries=%llu active=%llu recovery=%llu resets=%llu root-updates=%llu root-speed=%.6f selector=%u flags=%x; scoped post-VM samples, not original actor/frame equivalence",static_cast<unsigned long long>(source.ordered_entries()),static_cast<unsigned long long>(source.ordered_active_frames()),static_cast<unsigned long long>(source.ordered_recovery_frames()),static_cast<unsigned long long>(source.ordered_resets()),static_cast<unsigned long long>(source.ordered_root().updates),source.ordered_root().speed1dac,unsigned(source.selector()),source.movement_flags());
    if(options.source_dash_ledge)gf::log_info(context,"source-ground","dash-loss-air-frames=%llu landings=%llu resource=%d selector=%u; last ordered root sample is historical data after priority handoff, not a fresh airborne root lease",static_cast<unsigned long long>(source.air_frames()),static_cast<unsigned long long>(source.landings()),int(source.resource()),unsigned(source.selector()));
    if(options.source_ground_ledge)gf::log_info(context,"source-ground","walk-departures=%llu dash-departures=%llu landings=%llu resource=%d selector=%u; one scoped controller/producer, distinct walking and ordered-dash evidence",static_cast<unsigned long long>(source.walk_departures()),static_cast<unsigned long long>(source.dash_departures()),static_cast<unsigned long long>(source.landings()),int(source.resource()),unsigned(source.selector()));
    if(options.source_ground_jump)gf::log_info(context,"source-ground","ground-jump-air-frames=%llu landings=%llu resource=%d dash-jumps=%llu last-interrupted-selector=%u programs=%u/%u cached87=%u; source subset, no extra ordered root advance on takeoff",static_cast<unsigned long long>(source.air_frames()),static_cast<unsigned long long>(source.landings()),int(source.resource()),static_cast<unsigned long long>(source.dash_jumps()),unsigned(source.last_dash_jump_selector()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    if(options.source_ground_landing)gf::log_info(context,"source-ground","landing-jumps=%llu landing-dashes=%llu landings=%llu resource=%d programs=%u/%u cached87=%u; accepted landing entry before interruption, no touchdown skip or forced refill",static_cast<unsigned long long>(source.landing_jumps()),static_cast<unsigned long long>(source.landing_dashes()),static_cast<unsigned long long>(source.landings()),int(source.resource()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    if(options.source_ground_air)gf::log_info(context,"source-ground","controller-air-starts=%llu active=%llu landings=%llu relaunches=%llu resource=%d selector=%u programs=%u/%u cached87=%u; no floor/push/relaunch authority",static_cast<unsigned long long>(source.air_dash_starts()),static_cast<unsigned long long>(source.air_dash_active_frames()),static_cast<unsigned long long>(source.landings()),static_cast<unsigned long long>(source.relaunches()),int(source.resource()),unsigned(source.selector()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    if(options.source_ground_air_floor)gf::log_info(context,"source-ground","controller-floor-starts=%llu windup=%llu active=%llu entry=%llu positive-entry=%llu landings=%llu relaunches=%llu resource=%d selector=%u programs=%u/%u cached87=%u; no entry-floor/push/relaunch authority",static_cast<unsigned long long>(source.air_dash_starts()),static_cast<unsigned long long>(source.windup_floor_contacts()),static_cast<unsigned long long>(source.active_floor_contacts()),static_cast<unsigned long long>(source.windup_entry_floor_contacts()),static_cast<unsigned long long>(source.positive_windup_floor_contacts()),static_cast<unsigned long long>(source.landings()),static_cast<unsigned long long>(source.relaunches()),int(source.resource()),unsigned(source.selector()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    if(options.source_ground_relaunch)gf::log_info(context,"source-ground","controller-relaunches=%llu lower=%u upper=%u air=%llu floor=%llu pushes=%llu landings=%llu resource=%d selector=%u programs=%u/%u cached87=%u; no relaunch-floor/entry-floor/push authority",static_cast<unsigned long long>(source.relaunches()),unsigned(source.relaunch_stages().lower_stage),unsigned(source.relaunch_stages().upper_stage),static_cast<unsigned long long>(source.air_dash_starts()),static_cast<unsigned long long>(source.relaunch_floor_contacts()),static_cast<unsigned long long>(source.surface_pushes()),static_cast<unsigned long long>(source.landings()),int(source.resource()),unsigned(source.selector()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    if(options.source_ground_relaunch_floor)gf::log_info(context,"source-ground","controller-relaunch-floor=%llu relaunches=%llu lower=%u upper=%u pushes=%llu entry=%llu positive-entry=%llu landings=%llu resource=%d selector=%u programs=%u/%u cached87=%u; no windup-entry-floor/push authority",static_cast<unsigned long long>(source.relaunch_floor_contacts()),static_cast<unsigned long long>(source.relaunches()),unsigned(source.relaunch_stages().lower_stage),unsigned(source.relaunch_stages().upper_stage),static_cast<unsigned long long>(source.surface_pushes()),static_cast<unsigned long long>(source.windup_entry_floor_contacts()),static_cast<unsigned long long>(source.positive_windup_floor_contacts()),static_cast<unsigned long long>(source.landings()),int(source.resource()),unsigned(source.selector()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    if(options.source_ground_windup_entry_floor)gf::log_info(context,"source-ground","controller-windup-entry=%llu positive=%llu windup=%llu active=%llu relaunch-floor=%llu relaunches=%llu pushes=%llu landings=%llu resource=%d selector=%u programs=%u/%u cached87=%u; no push/positive698/combat authority",static_cast<unsigned long long>(source.windup_entry_floor_contacts()),static_cast<unsigned long long>(source.positive_windup_floor_contacts()),static_cast<unsigned long long>(source.windup_floor_contacts()),static_cast<unsigned long long>(source.active_floor_contacts()),static_cast<unsigned long long>(source.relaunch_floor_contacts()),static_cast<unsigned long long>(source.relaunches()),static_cast<unsigned long long>(source.surface_pushes()),static_cast<unsigned long long>(source.landings()),int(source.resource()),unsigned(source.selector()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    if(options.source_ground_surface)gf::log_info(context,"source-ground","controller-surface-landings=%llu angle-s16=%d air=%llu relaunches=%llu pushes=%llu landings=%llu resource=%d selector=%u programs=%u/%u cached87=%u; no push/positive698/combat authority",static_cast<unsigned long long>(source.surface_landings()),int(source.last_surface_angle()),static_cast<unsigned long long>(source.air_dash_starts()),static_cast<unsigned long long>(source.relaunches()),static_cast<unsigned long long>(source.surface_pushes()),static_cast<unsigned long long>(source.landings()),int(source.resource()),unsigned(source.selector()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    render.shutdown();return 0;
}

int run_animation_mosaic(gf::Context& context,gf::RenderEngine& render,const Options& options){
    gf::NativeAnimationMosaic mosaic;std::string error;
    if(!mosaic.load(context.files(),options.battle_mvp_assets,render,options.animation_columns,options.animation_rows,&error)||
       (!options.animation_programs.empty()&&!mosaic.load_source_programs(context.files(),options.animation_programs,options.animation_height_profile,render,&error))||
       (!options.animation_motion_profile.empty()&&!mosaic.load_source_motion(context.files(),options.animation_motion_profile,render,&error))||
       ((options.animation_borg||options.animation_page)&&!mosaic.select(options.animation_borg,options.animation_page,render,&error))){
        gf::log_error(context,"animation-mosaic","load failed: %s",error.c_str());render.shutdown();return 2;}
    if(!options.animation_programs.empty())gf::log_info(context,"animation-mosaic","G-Red source-program timelines: %zu essentials; 96-tick inspection replay, shared VM/height/pose/render systems; no gameplay input, physics or actor admission",mosaic.catalog(0).size());
    else gf::log_info(context,"animation-mosaic","looping model previews: G-Red=%zu Sasuke=%zu; numbers are per-Borg raw group/clip IDs; shared IDs start10000",mosaic.catalog(0).size(),mosaic.catalog(1).size());
    if(!options.animation_motion_profile.empty())gf::log_info(context,"animation-mosaic","held-input ground-dash handler planning enabled: lower/upper source order, VM holds, timer expiry and recovery; final inspection pose held, no world/contact admission");
    if(!options.animation_export.empty()){
        namespace fs=std::filesystem;std::error_code ec;const fs::path output(options.animation_export);
        if(!fs::create_directory(output,ec)){gf::log_error(context,"animation-mosaic","export directory must be new: %s",options.animation_export.c_str());render.shutdown();return 2;}
        auto safe=[](std::string s){for(char& c:s)if(c=='\t'||c=='\n'||c=='\r')c=' ';return s;};
        std::ofstream metadata(output/"previews.tsv");metadata<<"borg\tpage\tcell\tid\tgroup\tchoice\tarchive\tcommon\tend_frame\tsupported\taliases\terror\tx\ty\twidth\theight\n";
        std::ofstream info(output/"layout.tsv");info<<options.width<<'\t'<<options.height<<'\t'<<options.animation_columns<<'\t'<<options.animation_rows<<'\t'<<options.animation_export_frames<<'\n';info.close();
        const unsigned first_page=options.animation_export_page_only?options.animation_page:0,last_page=options.animation_export_page_only?first_page+1:mosaic.page_count();
        if(gf::u64(last_page-first_page)*options.animation_export_frames*options.width*options.height*3>(2ull<<30)){gf::log_error(context,"animation-mosaic","export exceeds2GiB raw-frame budget; use page-only export");render.shutdown();return 2;}
        for(unsigned page=first_page;page<last_page;++page){
            if(!mosaic.select(options.animation_borg,page,render,&error)){gf::log_error(context,"animation-mosaic","export page failed: %s",error.c_str());render.shutdown();return 2;}
            const auto folder=output/("page-"+std::to_string(page));fs::create_directory(folder,ec);
            if(ec){render.shutdown();return 2;}
            const auto previews=mosaic.page_previews();
            for(unsigned cell=0;cell<previews.size();++cell){const auto& p=previews[cell];metadata<<(options.animation_borg?"sasuke":"g-red")<<'\t'<<page<<'\t'<<cell<<'\t'<<p.entry.id<<'\t'<<unsigned(p.entry.group)<<'\t'<<p.entry.choice<<'\t'<<p.entry.archive<<'\t'<<p.entry.common<<'\t'<<p.end_frame<<'\t'<<p.supported<<'\t';
                for(const auto& alias:p.entry.aliases){metadata<<unsigned(alias.first)<<':'<<alias.second<<',';}
                metadata<<'\t'<<safe(p.error);for(auto value:mosaic.preview_bounds(options.width,options.height,cell))metadata<<'\t'<<value;metadata<<'\n';}
            metadata.flush();
            for(unsigned frame=0;frame<options.animation_export_frames;++frame){
                mosaic.sample_phase(gf::f32(frame)/options.animation_export_frames);render.begin_frame({frame,.055f,.065f,.085f});render.reset_arena();const auto draws=mosaic.draw(render,options.width,options.height);render.end_frame();
                if(!draws){gf::log_error(context,"animation-mosaic","export viewport too small or draw failed");render.shutdown();return 2;}
                std::vector<gf::u8> rgba;gf::s32 width=0,height=0;const auto path=folder/("frame-"+std::to_string(frame)+".ppm");
                if(!gf::gl::capture_framebuffer(rgba,&width,&height)||!gf::gl::write_ppm(path.string().c_str(),rgba,width,height)){render.shutdown();return 2;}
                render.present();
            }
            gf::log_info(context,"animation-mosaic","exported Borg=%u page=%u/%u",options.animation_borg,page+1,mosaic.page_count());
        }
        metadata.close();render.shutdown();return 0;
    }
    gf::InputRouter router;unsigned frame=0;bool captured=false;const auto start=context.clock().monotonic_ns();auto previous=start;
    while(context.window().pump_events()&&!context.window().should_close()){
        const auto now=context.clock().monotonic_ns();if(options.seconds&&now-start>=gf::u64(options.seconds)*1000000000ull)break;
        context.input().pump();const auto& snapshot=context.input().snapshot();const auto pad=router.route(snapshot,0);if(snapshot.quit||pad.quit_requested)break;
        const gf::f32 dt=(options.headless||options.frames)?1.f/60:gf::f32(now>=previous?now-previous:0)/1e9f;previous=now;
        if(!mosaic.step(pad,dt,render,&error)){gf::log_error(context,"animation-mosaic","step failed: %s",error.c_str());render.shutdown();return 2;}
        render.begin_frame({frame,.055f,.065f,.085f});render.reset_arena();const auto draws=mosaic.draw(render,options.width,options.height);render.end_frame();
        if(!captured)captured=capture_if_requested(context,options,frame);
        render.present();context.advance_frame();++frame;
        if(!draws){gf::log_error(context,"animation-mosaic","no preview draws");render.shutdown();return 2;}
        if(options.frames&&frame>=options.frames)break;
    }
    gf::log_info(context,"animation-mosaic","frames=%u Borg=%u page=%u",frame,mosaic.borg(),mosaic.page()+1);render.shutdown();return 0;
}

} // namespace

int main(int argc, char** argv) {
    Options options;
    if (!parse(argc, argv, &options)) return 2;

    gf::headless::Bundle headless;
    std::unique_ptr<gf::IFileSystem> headless_files;
    std::unique_ptr<gf::ILogBackend> headless_log;
    gf::Context context(headless.get(), 60);
    if(options.headless&&(options.test_battle||options.battle_mvp||options.animation_mosaic)){
        auto backends=headless.get();headless_files=gf::sdl::create_file_system();headless_log=gf::sdl::create_log_backend();
        backends.files=headless_files.get();backends.log=headless_log.get();context=gf::Context(backends,60);
    }
    std::unique_ptr<gf::sdl::Platform> platform;
    std::unique_ptr<gf::IRenderBackend> renderer;

    if (!options.headless) {
        platform = std::make_unique<gf::sdl::Platform>();
        renderer = gf::gl::create_renderer(nullptr);
        if (!renderer) {
            std::fprintf(stderr, "no render backend available\n");
            return 1;
        }
        std::string error;
        gf::Backends backends =
            platform->create({options.width, options.height, options.animation_mosaic?"Borg Animation Mosaic":"Gotcha Force", false, !options.animation_export.empty()},
                             renderer.get(), &error);
        if (!backends.complete()) {
            std::fprintf(stderr, "platform initialization failed: %s\n",
                         error.empty() ? "backend set incomplete" : error.c_str());
            return 1;
        }
        context = gf::Context(backends, 60);
    }

    gf::RenderEngine render(*context.backends().render);
    std::string error;
    // Windowed runs present with vsync so a --seconds demo runs in real time
    // rather than thousands of frames per second.
    if (!render.initialize({options.width, options.height, !options.headless&&options.animation_export.empty(), true, "Gotcha Force"},
                           context.backends().window, &error)) {
        gf::log_error(context, "render", "initialization failed: %s", error.c_str());
        return 1;
    }
    gf::log_info(context, "host", "backend=%s", render.capabilities().name.c_str());
    if(options.test_battle)return run_battle_test(context,render,options);
    if(options.battle_mvp)return run_battle_mvp(context,render,options);
    if(options.animation_mosaic)return run_animation_mosaic(context,render,options);

    gf::MenuWorld menu_world;
    menu_world.configure(static_cast<gf::s32>(options.width), static_cast<gf::s32>(options.height));
    menu_world.open();
    gf::ArchiveSummary archive;
    if (options.menu) {
        std::string archive_error;
        // Missing archive is not fatal: the menu shell still runs.
        if (!archive.load(context.files(),
                          "original/extracted/files/afs_data.afs", &archive_error)) {
            gf::log_warn(context, "content", "%s", archive_error.c_str());
        } else {
            gf::log_info(context, "content", "archive entries=%zu hsd=%zu unknown=%zu",
                         archive.files(), archive.count(gf::ContentKind::HsdfMesh),
                         archive.count(gf::ContentKind::Unknown));
            if (archive.named()) {
                // Extension counts come from the archive's own names, unlike
                // ContentIndex, which only looks at leading magic bytes.
                unsigned tpl = 0, arc = 0, bin = 0, pzz = 0, arz = 0, other = 0;
                for (std::size_t i = 0; i < archive.files(); ++i) {
                    const std::string_view name = archive.name_of(i);
                    const auto ends = [name](std::string_view suffix) {
                        return name.size() >= suffix.size() &&
                               name.substr(name.size() - suffix.size()) == suffix;
                    };
                    if (ends(".tpl")) ++tpl;
                    else if (ends(".arc")) ++arc;
                    else if (ends(".bin")) ++bin;
                    else if (ends(".pzz")) ++pzz;
                    else if (ends(".arz")) ++arz;
                    else ++other;
                }
                gf::log_info(context, "content",
                             "named members=%zu tpl=%u arc=%u bin=%u pzz=%u arz=%u other=%u",
                             archive.files(), tpl, arc, bin, pzz, arz, other);
                const std::size_t menu_asset = archive.find(options.scene_asset);
                gf::log_info(context, "content", "menu room=%s",
                             menu_asset == gf::AfsArchive::kNoEntry
                                 ? "absent"
                                 : archive.name_of(menu_asset).data());
            }
        }
    }

    // Texture pipeline probe: decode a real TPL member and upload it, so the
    // decode -> upload -> draw path can be exercised on the GPU with game data.
    // It draws beside the menu, never in place of it: a decoded asset that is
    // not the menu's own art must not be passed off as the menu.
    gf::u32 probe_texture = 0;
    gf::u32 probe_width = 0, probe_height = 0;
    if (options.menu && options.probe_texture >= 0) {
        std::string tpl_error;
        gf::TplImage decoded;
        const gf::ByteView member = archive.file(static_cast<std::size_t>(options.probe_texture));
        if (!member.data) {
            gf::log_warn(context, "content", "probe texture %d: no such archive member",
                         options.probe_texture);
        } else if (!gf::tpl_decode(member, 0, &decoded, &tpl_error)) {
            gf::log_warn(context, "content", "probe texture %d: %s", options.probe_texture,
                         tpl_error.c_str());
        } else if (!render.upload_texture(0x10000, {decoded.width, decoded.height, decoded.rgba.data()})) {
            gf::log_warn(context, "content", "probe texture %d: upload failed",
                         options.probe_texture);
        } else {
            probe_texture = 0x10000; // separate from scene handles, starting at 1
            probe_width = decoded.width;
            probe_height = decoded.height;
            gf::log_info(context, "content", "probe texture %d: %ux%u format %u -> handle %u",
                         options.probe_texture, decoded.width, decoded.height, decoded.format,
                         probe_texture);
        }
    }

    // The real menu room, when the archive carries it: decode stff_mdl.arc and
    // draw the game's own geometry. Falls back to the placeholder box set when
    // the member is missing or does not decode.
    gf::MenuScene menu_scene;
    gf::MenuOverlay menu_overlay;
    gf::Camera room_camera;
    gf::HsdAnimation room_animation;
    gf::HsdAnimPose room_pose;
    bool have_animated_camera = false;
    bool have_reference_layout = false;
    gf::OptionCameraProfile option_camera;
    gf::OptionTransition option_transition;
    bool have_option_camera=false;
    gf::OptionScene option_scene;gf::OptionMenu option_menu;
    gf::OptionSettingsStore option_store(context.save());
    bool have_option_widgets=false;
    gf::OptionOutput option_output(context.haptics());bool have_option_output=false;
    gf::CreditsScene credits_scene;bool have_credits=false;
    gf::MenuMusic music;bool have_music=false;gf::u32 music_scene_hash=0;
    gf::CreditsMusic cue_music;bool have_cue_music=false;
    gf::u32 menu_cue_id=0,credits_cue_id=0;
    if (options.menu && !options.placeholder_menu && archive.named()) {
        const std::size_t room = archive.find(options.scene_asset);
        if (room == gf::AfsArchive::kNoEntry) {
            gf::log_warn(context, "content", "menu scene %s is missing",options.scene_asset.c_str());
        } else {
            music_scene_hash=gf::menu_asset_hash(archive.file(room));
            std::string scene_error;
            gf::HsdScene decoded_room;
            bool room_loaded=gf::hsd_load(archive.file(room),&decoded_room,&scene_error);
            if(room_loaded && !options.file_pose && options.animation_frame<0 && context.files().exists(options.menu_layout)){
                std::vector<gf::u8> layout;
                gf::HsdScene instances;
                if(!context.files().read(options.menu_layout,layout) ||
                   !gf::menu_layout_load({layout.data(),layout.size()},archive.file(room),decoded_room,&instances,&room_camera,&scene_error)) {
                    gf::log_warn(context,"content","reference layout refused: %s",scene_error.c_str());
                    room_loaded=false; // don't silently publish a partially matched preset
                } else {decoded_room=std::move(instances);have_reference_layout=true;}
            }
            if(room_loaded&&have_reference_layout&&!options.scene_only&&context.files().exists(options.menu_motion)){
                std::vector<gf::u8> motion;
                if(!context.files().read(options.menu_motion,motion)||
                   !gf::menu_motion_load({motion.data(),motion.size()},gf::menu_asset_hash(archive.file(room)),decoded_room,&decoded_room.menu_motion,&scene_error)){
                    gf::log_warn(context,"content","menu motion refused: %s",scene_error.c_str());room_loaded=false;
                }else gf::log_info(context,"content","original menu motion loaded: rotating identifiers + flat gear selector (per native update)");
            }
            if (!room_loaded || !menu_scene.load_scene(std::move(decoded_room), &scene_error)) {
                gf::log_warn(context, "content", "menu room: %s", scene_error.c_str());
            } else {
                // Evaluate matched joint/visibility/camera time. End-of-clip is
                // an explicit preview policy, not a recovered main-menu trigger.
                if (!options.file_pose && options.animation_frame>=0) {
                    std::string animation_error;
                    if (gf::hsd_anim_load(archive.file(room), &room_animation, &animation_error)) {
                        const gf::f32 time = options.animation_frame >= 0
                            ? options.animation_frame : gf::hsd_anim_end_frame(room_animation);
                        if (gf::hsd_anim_evaluate(room_animation,menu_scene.scene(),time,&room_pose,&animation_error)
                            && menu_scene.set_pose(&room_pose)) {
                            have_animated_camera = room_pose.has_camera;
                            gf::log_info(context,"content","animation preview frame=%.0f end=%.0f (clip selection unverified)",time,gf::hsd_anim_end_frame(room_animation));
                        }
                    }
                    if (!animation_error.empty()) gf::log_warn(context,"content","animation: %s",animation_error.c_str());
                }
                const unsigned uploaded = menu_scene.upload_textures(render);
                if(have_reference_layout&&!options.scene_only&&context.files().exists(options.menu_navigation)){
                    std::vector<gf::u8> rules;gf::MenuNavigationConfig navigation;std::string navigation_error;
                    if(!context.files().read(options.menu_navigation,rules)||
                       !gf::menu_navigation_load({rules.data(),rules.size()},gf::menu_asset_hash(archive.file(room)),&navigation,&navigation_error)||
                       !menu_world.configure_navigation(navigation)){
                        gf::log_error(context,"content","navigation refused: %s",navigation_error.c_str());
                        render.shutdown();return 2;
                    }
                    gf::log_info(context,"content","navigation=source-spatial ports=4 repeat-limit=%u restart=%u (native updates)",
                        unsigned(navigation.repeat_limit),unsigned(navigation.repeat_restart));
                }else gf::log_info(context,"content","navigation=diagnostic-linear (no configured spatial rules)");
                const auto& lights=menu_scene.scene().lighting;
                gf::log_info(context,"content","lighting=%s lights=%zu animation=%s",
                    lights.declared?"archive-ambient-directional":"diagnostic-fixed",
                    lights.lights.size(),lights.animated?"authored-pose-only":"none");
                menu_scene.set_cull_backface(!options.no_cull);
                menu_scene.set_alpha_test(!options.no_alpha_test);
                menu_scene.set_depth_test(!options.no_depth);
                menu_scene.set_only_mesh(options.only_mesh);
                menu_scene.set_interaction(!options.scene_only);
                menu_scene.set_show_source_markers(options.show_source_markers);
                gf::f32 low[3], high[3];
                menu_scene.bounds(low, high);
                gf::log_info(context, "content",
                             "menu room: %zu meshes, %zu joints, %zu textures (%u uploaded)",
                             menu_scene.scene().meshes.size(), menu_scene.scene().joints.size(),
                             menu_scene.scene().textures.size(), uploaded);
                room_camera.viewport_width = static_cast<gf::s32>(options.width);
                room_camera.viewport_height = static_cast<gf::s32>(options.height);
                if(!have_reference_layout)room_camera.fov_y_degrees = options.camera_fov;
                if (options.explicit_camera) {
                    for (unsigned k = 0; k < 3; ++k) {
                        room_camera.eye[k] = options.camera_eye[k];
                        room_camera.target[k] = options.camera_target[k];
                    }
                } else if (have_reference_layout) {
                    gf::log_info(context,"content","observed native scene layout loaded (no guest runtime)");
                } else if (have_animated_camera) {
                    for(unsigned k=0;k<3;++k){room_camera.eye[k]=room_pose.camera_eye[k];room_camera.target[k]=room_pose.camera_interest[k];}
                    room_camera.fov_y_degrees=room_pose.camera_fov_y_degrees;
                } else {
                    // Frame the dense part of the room (the desk and the option
                    // seals), not the far walls: a vertex-weighted focus is the
                    // only camera hint the archive gives us.
                    gf::f32 centre[3] = {(low[0] + high[0]) * 0.5f,
                                         (low[1] + high[1]) * 0.5f,
                                         (low[2] + high[2]) * 0.5f};
                    gf::f32 radius = 600.0f;
                    menu_scene.focus(centre, &radius);
                    room_camera.eye[0] = centre[0];
                    room_camera.eye[1] = centre[1] + radius * 1.0f;
                    room_camera.eye[2] = centre[2] - radius * 2.4f;
                    room_camera.target[0] = centre[0];
                    room_camera.target[1] = centre[1];
                    room_camera.target[2] = centre[2];
                }
                if(!have_reference_layout){
                    room_camera.near_plane = 10.0f;
                    room_camera.far_plane = (high[2] - low[2]) * 8.0f;
                    if (room_camera.far_plane < 10000.0f) room_camera.far_plane = 10000.0f;
                }
                if(have_animated_camera && !options.explicit_camera){room_camera.near_plane=room_pose.camera_near;room_camera.far_plane=room_pose.camera_far;}
                // The interaction layer: our own seal ring around the decoded
                // desk. The room is the game's; this placement is ours.
                gf::f32 overlay_centre[3] = {0.0f, 0.0f, 0.0f};
                gf::f32 overlay_radius = 400.0f;
                menu_scene.focus(overlay_centre, &overlay_radius);
                overlay_centre[1] += overlay_radius * 0.30f;
                menu_overlay.configure(overlay_centre, overlay_radius * 0.85f,
                                       overlay_radius * 0.17f);
                gf::log_info(context, "content",
                             "camera eye=(%.0f,%.0f,%.0f) target=(%.0f,%.0f,%.0f) fov=%.0f "
                             "near=%.6g far=%.6g viewport=%dx%d",
                             room_camera.eye[0], room_camera.eye[1], room_camera.eye[2],
                             room_camera.target[0], room_camera.target[1], room_camera.target[2],
                             room_camera.fov_y_degrees, room_camera.near_plane,
                              room_camera.far_plane, room_camera.viewport_width,
                              room_camera.viewport_height);
                if(have_reference_layout&&!options.scene_only&&!options.explicit_camera&&menu_scene.scene().menu_motion.present&&menu_world.spatial_navigation()&&context.files().exists(options.option_camera)){
                    std::vector<gf::u8> bytes;std::string error;
                    if(!context.files().read(options.option_camera,bytes)||
                       !gf::option_camera_profile_load({bytes.data(),bytes.size()},archive.file(room),gf::menu_asset_hash(archive.file(room)),room_camera,&option_camera,&error)||
                       !option_transition.configure(option_camera.timing)){
                        gf::log_error(context,"content","OPTION camera refused: %s",error.c_str());render.shutdown();return 2;
                    }
                    have_option_camera=true;
                    gf::log_info(context,"content","OPTION camera published desc=%x clip=%x range=%u..%u; checking optional widget content",
                        option_camera.camera.descriptor,option_camera.camera.clip,option_camera.timing.entry_frame,option_camera.timing.option_frame);
                    if(context.files().exists(options.option_widgets)){
                        const auto index=archive.find("optn00_mdl.arc");std::vector<gf::u8> widgets;
                        if(index==gf::AfsArchive::kNoEntry||!context.files().read(options.option_widgets,widgets)||
                           !option_scene.load(archive.file(index),{widgets.data(),widgets.size()},&error)||
                           !option_menu.configure(option_scene.profile().repeat_limit,option_scene.profile().repeat_restart)){
                            gf::log_error(context,"content","OPTION widgets refused: %s",error.c_str());render.shutdown();return 2;
                        }
                        const auto uploaded=option_scene.upload_textures(render);have_option_widgets=true;
                        gf::log_info(context,"content","OPTION widgets: %zu original instances, %u textures; settings slot=%u (native only); output profile checked separately",
                            option_scene.scene().meshes.size(),uploaded,gf::kOptionSettingsSlot);
                        if(context.files().exists(options.option_output)){
                            std::vector<gf::u8> bytes;gf::OptionOutputProfile output_profile;
                            if(!context.files().read(options.option_output,bytes)||
                               !gf::option_output_load({bytes.data(),bytes.size()},gf::menu_asset_hash(archive.file(index)),&output_profile,&error)||
                               !option_output.configure(output_profile)){
                                gf::log_error(context,"option","output profile refused: %s",error.c_str());render.shutdown();return 2;
                            }
                            have_option_output=true;
                            gf::log_info(context,"option","native PCM policy + per-device rumble enabled; source stream binding checked separately; audio-device=%u",
                                context.audio().available());
                        }
                        if(context.files().exists(options.credits)){
                            const auto staff=archive.find("staff00_mdl.arc");std::vector<gf::u8> content;
                            if(staff==gf::AfsArchive::kNoEntry||!context.files().read(options.credits,content)||
                               !credits_scene.load(archive.file(staff),{content.data(),content.size()},&error)||
                               !option_menu.configure_credits(credits_scene.config().entry_wait)){
                                gf::log_error(context,"credits","configured credits refused: %s",error.c_str());render.shutdown();return 2;
                            }
                            const auto textures=credits_scene.upload_textures(render);have_credits=true;
                            gf::log_info(context,"credits","source script rows=%zu instances=%zu textures=%u; per-native-update scrolling, music binding checked separately",
                                credits_scene.config().rows.size(),credits_scene.config().glyphs.size()+1,textures);
                        }
                    }
                }
            }
        }
    }

    gf::Session session(context, render);
    session.bootstrap_scene();
    // Desktop target: on-screen controls are off. A mobile target enables this
    // and adds buttons; the same overlay then supplies hit testing and drawing.
    session.configure_overlay(gf::OverlayConfig{false, 1.0f, 0, 0, false});
    if(have_option_output){
        gf::OptionSettings initial;const auto status=option_store.load(&initial);
        std::array<unsigned,4> output_devices{};for(unsigned i=0;i<4;++i)output_devices[i]=session.router().port(i).device_pad;
        if(!option_output.apply(initial,output_devices,false)){gf::log_error(context,"option","startup output policy refused");render.shutdown();return 2;}
        gf::log_info(context,"option","startup output policy load-status=%u mono=%u volume=%u,%u,%u rumble-mask=%u (no startup rumble)",
            unsigned(status),initial.mono,initial.volume[0],initial.volume[1],initial.volume[2],initial.rumble_mask);
    }
    if(!options.no_audio&&!options.explicit_camera&&!options.scene_only&&have_reference_layout&&menu_scene.loaded()&&have_option_output&&context.files().exists(options.music_profile)){
        std::vector<gf::u8> bytes;gf::MenuMusicProfile profile;gf::AdxAudio decoded;std::string error;
        if(!context.files().read(options.music_profile,bytes)||
           !gf::menu_music_profile_load({bytes.data(),bytes.size()},music_scene_hash,&profile,&error)||
           !gf::menu_music_load(context.files(),options.audio_archive,profile,&decoded,&error)){
            gf::log_error(context,"music","qualified music refused: %s",error.c_str());render.shutdown();return 2;
        }
        if(context.files().exists(options.music_transitions)){
            std::vector<gf::u8> transition_bytes;gf::CreditsMusicProfile transitions;gf::AdxAudio staff;
            if(!context.files().read(options.music_transitions,transition_bytes)||
               !gf::credits_music_profile_load({transition_bytes.data(),transition_bytes.size()},{bytes.data(),bytes.size()},music_scene_hash,&transitions,&error)||
               !gf::menu_music_load(context.files(),options.audio_archive,transitions.credits,&staff,&error)||
               !cue_music.load(std::move(decoded),std::move(staff),profile.output_rate,transitions.fade,option_output.bgm_gain())){
                gf::log_error(context,"music","qualified credits music refused: %s",error.c_str());render.shutdown();return 2;
            }
            have_cue_music=true;
            credits_cue_id=transitions.credits.entry;
            gf::log_info(context,"music","source credits cue=%u menu cue=%u fade-request-updates=%u scalar-full=%g minimum-step=%g; native PCM gain calibration",
                transitions.credits.entry,profile.entry,transitions.fade.updates,transitions.fade.full_scale,transitions.fade.min_step);
        }else if(!music.load(std::move(decoded),profile.output_rate)){gf::log_error(context,"music","menu PCM player refused");render.shutdown();return 2;}
        have_music=true;
        menu_cue_id=profile.entry;
        gf::log_info(context,"music","source-qualified initial menu archive=0 member=%u ADX-v3 stereo rate=%u frames=%u loop=%u..%u output=%u; ranged member bytes=%u",
            profile.entry,profile.source_rate,profile.frames,profile.loop_start,profile.loop_end,profile.output_rate,profile.size);
    }
    if(options.demo_option&&!have_option_camera){gf::log_error(context,"menu","--demo-option requires configured spatial menu and qualified camera content");render.shutdown();return 2;}
    if(options.demo_option_values&&!have_option_widgets){gf::log_error(context,"menu","--demo-option-values requires qualified widget content");render.shutdown();return 2;}
    if(options.demo_credits&&!have_credits){gf::log_error(context,"credits","--demo-credits requires qualified credits content");render.shutdown();return 2;}
    if(options.menu){
        // Enter the authored selection directly. Walking down a list with fake
        // inputs is neither valid spatial navigation nor original startup logic.
        const unsigned selected=menu_scene.loaded()?menu_scene.scene().menu_selection:0;
        if(!menu_world.enter_main_menu(selected)){gf::log_error(context,"menu","invalid startup selection");render.shutdown();return 2;}
    }
    gf::log_info(context, "host", "scene entities=%zu", session.world().size());

    unsigned frame = 0;
    bool screenshot_written = false;
    const auto started = std::chrono::steady_clock::now();
    while (context.backends().window->pump_events() && !context.backends().window->should_close()) {
        if (options.seconds != 0) {
            const auto now = std::chrono::steady_clock::now();
            const auto ran = std::chrono::duration_cast<std::chrono::milliseconds>(now - started);
            if (ran.count() >= static_cast<long long>(options.seconds) * 1000) break;
        }
        context.backends().input->pump();
        const gf::InputSnapshot& devices = context.backends().input->snapshot();
        session.set_device_snapshot(devices);
        std::array<gf::GamePadState,4> menu_pads{};
        for(unsigned port=0;port<menu_pads.size();++port)menu_pads[port]=session.router().route(devices,port);
        if(options.menu&&options.demo_input){menu_pads={};menu_pads[0]=demo_pad(frame,menu_world.spatial_navigation());}
        if(options.menu&&options.demo_option){menu_pads={};menu_pads[0]=option_demo_pad(frame,options.demo_option_values,have_option_widgets);}
        if(options.menu&&options.demo_credits){menu_pads={};menu_pads[0]=credits_demo_pad(frame,options.demo_credits_fast_back);}
        const gf::GamePadState pad=menu_pads[0];
        if (devices.quit) break;

        context.add_elapsed(context.tick_ns());
        while (context.tick_ready()) {
            context.consume_tick();
            session.tick(pad);
        }
        unsigned draws = 0;
        if (options.menu) {
            const unsigned previous_selection=menu_world.selection();
            const auto previous_phase=option_transition.phase();
            gf::u8 back_mask=0;for(unsigned port=0;port<menu_pads.size();++port)if(menu_pads[port].b)back_mask|=gf::u8(1u<<port);
            if(option_transition.active()){
                menu_world.observe_ports(menu_pads);
                if(have_option_widgets&&option_transition.phase()==gf::OptionTransitionPhase::UI){
                    const auto before=option_menu.phase();const auto event=option_menu.update(option_store,menu_pads);
                    if(have_credits){
                        if(before!=gf::OptionMenuPhase::Credits&&option_menu.phase()==gf::OptionMenuPhase::Credits){
                            if(!credits_scene.begin()){gf::log_error(context,"credits","begin refused");render.shutdown();return 2;}
                            gf::log_info(context,"credits","scroll begin row=%u",option_menu.row());
                        }else if(before==gf::OptionMenuPhase::Credits&&option_menu.phase()==gf::OptionMenuPhase::Credits){
                            if(!credits_scene.step()){gf::log_error(context,"credits","timeline/pose refused");render.shutdown();return 2;}
                            if(!credits_scene.running()){
                                (void)option_menu.finish_credits();gf::log_info(context,"credits","automatic return updates=%u peak=%u row=%u",credits_scene.timeline().updates(),credits_scene.timeline().peak(),option_menu.row());
                            }
                        }else if(before==gf::OptionMenuPhase::Credits){credits_scene.cancel();gf::log_info(context,"credits","B return row=%u; no save",option_menu.row());}
                    }
                    if(event.changed)gf::log_info(context,"option","values mono=%u volume=%u,%u,%u rumble-mask=%u (SFX/VOICE and other cue routing NOT PORTED)",
                        option_menu.settings().mono,option_menu.settings().volume[0],option_menu.settings().volume[1],option_menu.settings().volume[2],option_menu.settings().rumble_mask);
                    if(event.changed&&have_option_output){
                        std::array<unsigned,4> devices{};for(unsigned i=0;i<4;++i)devices[i]=session.router().port(i).device_pad;
                        if(!option_output.apply(option_menu.settings(),devices,true)){gf::log_error(context,"option","output policy refused");render.shutdown();return 2;}
                    }
                    if(event.bgm_immediate&&have_cue_music){
                        if(!cue_music.set_level(option_output.bgm_gain(),true)){gf::log_error(context,"music","immediate BGM control refused");render.shutdown();return 2;}
                        gf::log_info(context,"music","immediate BGM control cancels pending cue/fade; native PCM calibration");
                    }
                    if(before!=option_menu.phase()||event.save_attempted)gf::log_info(context,"option","native UI phase=%u write-attempt=%u load-status=%u",
                        unsigned(option_menu.phase()),event.save_attempted,unsigned(option_menu.load_status()));
                    if(event.leave)(void)option_transition.request_leave();
                }else{
                    if(have_option_widgets)option_menu.observe(menu_pads);
                    if(option_transition.update(have_option_widgets?0:back_mask))menu_world.resume_main_menu();
                    if(have_option_widgets&&option_transition.phase()==gf::OptionTransitionPhase::UI){
                        option_menu.begin(option_store,menu_pads);
                        if(have_option_output){
                            std::array<unsigned,4> devices{};for(unsigned i=0;i<4;++i)devices[i]=session.router().port(i).device_pad;
                            if(!option_output.apply(option_menu.settings(),devices,false)){gf::log_error(context,"option","loaded output policy refused");render.shutdown();return 2;}
                        }
                        gf::log_info(context,"option","native settings load-status=%u; row=0",unsigned(option_menu.load_status()));
                        gf::log_info(context,"option","loaded values mono=%u volume=%u,%u,%u rumble-mask=%u",
                            option_menu.settings().mono,option_menu.settings().volume[0],option_menu.settings().volume[1],option_menu.settings().volume[2],option_menu.settings().rumble_mask);
                    }
                }
            }else{
                const auto previous_screen=menu_world.screen();menu_world.update_ports(menu_pads);
                if(have_option_camera&&previous_screen==gf::MenuScreen::MainMenu&&menu_world.screen()==gf::MenuScreen::Option)
                    (void)option_transition.begin(back_mask);
            }
            if(option_transition.active()||previous_phase!=option_transition.phase()){
                std::string error;
                if(!gf::hsd_camera_sample(option_camera.camera,option_transition.frame(),&room_camera,&error)){
                    gf::log_error(context,"menu","OPTION camera sample refused: %s",error.c_str());render.shutdown();return 2;
                }
            }
            if(previous_phase!=option_transition.phase())gf::log_info(context,"menu","OPTION phase=%u camera-frame=%u",unsigned(option_transition.phase()),option_transition.frame());
            if(menu_world.selection()!=previous_selection)gf::log_info(context,"menu","selection=%u %s",menu_world.selection(),gf::MainMenu::item_name(menu_world.selection()));
            if(have_music){
                const bool scope=menu_world.screen()==gf::MenuScreen::MainMenu||
                    (option_transition.active()&&(have_cue_music||option_menu.phase()!=gf::OptionMenuPhase::Credits));
                const bool changed=scope!=(have_cue_music?cue_music.scoped():music.scoped());
                if(have_cue_music){
                    const auto before_cue=cue_music.current(),before_wanted=cue_music.wanted();const auto switches=cue_music.switches();
                    const auto wanted=option_menu.phase()==gf::OptionMenuPhase::Credits?gf::MusicCue::Credits:gf::MusicCue::Menu;
                    if(!cue_music.set_scope(scope,context.audio())||!cue_music.set_level(option_output.bgm_gain())||
                       (scope&&!cue_music.request(wanted))||!cue_music.pump(option_output.pcm(),context.audio())){
                        gf::log_error(context,"music","cue/fade/sink refused; not retried");render.shutdown();return 2;
                    }
                    if(scope&&before_wanted!=cue_music.wanted())gf::log_info(context,"music","fade request source-cue=%u scalar=%g slope=%g native-frame=%u",wanted==gf::MusicCue::Credits?credits_cue_id:menu_cue_id,cue_music.fade().value(),cue_music.fade().slope(),frame);
                    if(switches!=cue_music.switches())gf::log_info(context,"music","cue switched source=%u from=%u at scalar=%g native-frame=%u; queued=%llu discarded=%llu",
                        cue_music.current()==gf::MusicCue::Credits?credits_cue_id:menu_cue_id,before_cue==gf::MusicCue::Credits?credits_cue_id:menu_cue_id,cue_music.fade().value(),
                        frame,static_cast<unsigned long long>(context.audio().frames_available()),static_cast<unsigned long long>(context.audio().frames_discarded()));
                }else if(!music.set_scope(scope,context.audio())||!music.pump(option_output.pcm(),context.audio())){
                    gf::log_error(context,"music","PCM stream/sink refused; not retried");render.shutdown();return 2;
                }
                if(changed)gf::log_info(context,"music","native menu scope=%u queued=%llu discarded=%llu; unsupported cues are not substituted",scope,
                    static_cast<unsigned long long>(context.audio().frames_available()),static_cast<unsigned long long>(context.audio().frames_discarded()));
            }
            render.begin_frame({frame, options.clear[0], options.clear[1], options.clear[2]});
            render.reset_arena();
            if (menu_scene.loaded()) {
                menu_scene.set_selection(menu_world.selection());
                menu_scene.set_menu_focus(option_transition.active()?static_cast<unsigned>(gf::MenuItem::Option):7,!option_transition.active());
                draws = menu_scene.draw(render, room_camera);
                if(have_option_widgets&&option_transition.phase()==gf::OptionTransitionPhase::UI&&
                   (option_menu.phase()==gf::OptionMenuPhase::Editing||option_menu.phase()==gf::OptionMenuPhase::CreditsDelay)){
                    std::string error;
                    const bool cursor=option_menu.phase()!=gf::OptionMenuPhase::CreditsDelay||(option_menu.credits_delay()&4u)==0;
                    if(!option_scene.update(option_menu.row(),option_menu.settings(),true,&error,cursor)){
                        gf::log_error(context,"option","widget pose refused: %s",error.c_str());render.shutdown();return 2;}
                    draws+=option_scene.draw(render,room_camera);
                }
                if(have_credits&&option_transition.phase()==gf::OptionTransitionPhase::UI&&option_menu.phase()==gf::OptionMenuPhase::Credits)
                    draws+=credits_scene.draw(render,room_camera);
                if(menu_world.screen()==gf::MenuScreen::MainMenu||option_transition.active())menu_scene.advance_motion();
                if(!options.scene_only && !have_reference_layout) draws += menu_overlay.draw(render, room_camera, menu_world.selection(),
                                                                   menu_world.selection_count());
            } else {
                draws = menu_world.draw(render);
            }
            if (probe_texture) {
                draws += draw_probe_texture(render, probe_texture, probe_width, probe_height,
                                            options.width, options.height);
            }
            // Destination gameplay is not ported yet. A confirmed item must not
            // silently keep looking like a still-active main menu.
            if(menu_world.screen()!=gf::MenuScreen::MainMenu){
                const auto saved=render.state();render.state()=gf::RenderState{};
                render.state().blend_src=5;render.state().blend_dst=6;
                gf::TextRenderer text;
                std::string notice=menu_world.screen()==gf::MenuScreen::Title
                    ? "PRESS START" : std::string(gf::MainMenu::item_name(menu_world.selection()))+" - NOT PORTED - B RETURN (NATIVE)";
                if(option_transition.active()){
                    if(option_transition.phase()==gf::OptionTransitionPhase::Entering)notice="OPTION - ENTERING TV (NATIVE CAMERA)";
                    else if(option_transition.phase()==gf::OptionTransitionPhase::Leaving)notice="RETURNING TO MAIN MENU (NATIVE CAMERA)";
                    else notice="OPTION UI - NOT PORTED - B RETURN (NATIVE)";
                    if(have_option_widgets&&option_transition.phase()==gf::OptionTransitionPhase::UI){
                        switch(option_menu.phase()){
                        case gf::OptionMenuPhase::Editing:
                            if(have_music)notice=context.audio().available()?"SOURCE BGM - SFX/VOICE NOT PORTED - B SAVE/EXIT":"SOURCE BGM READY - NO AUDIO DEVICE - B SAVE/EXIT";
                            else notice=have_option_output?"NATIVE PCM/RUMBLE POLICY - GAME AUDIO UNAVAILABLE - B SAVE/EXIT":"NATIVE SETTINGS - B SAVE/EXIT - AUDIO/HAPTICS NOT APPLIED";
                            break;
                        case gf::OptionMenuPhase::CreditsUnported:notice="STAFF CREDITS NOT PORTED - B BACK";break;
                        case gf::OptionMenuPhase::CreditsDelay:notice="STAFF CREDITS - PREPARING (NATIVE UPDATES)";break;
                        case gf::OptionMenuPhase::Credits:notice=have_cue_music?"STAFF CREDITS - SOURCE BGM - B BACK":"STAFF CREDITS - B BACK - CREDITS AUDIO NOT PORTED";break;
                        case gf::OptionMenuPhase::Saved:notice="NATIVE SETTINGS SAVED - A RETURN";break;
                        case gf::OptionMenuPhase::SaveError:notice="NATIVE SAVE FAILED/UNREADABLE - A RETRY / B BACK";break;
                        }
                    }
                }
                text.draw_centered(render,notice,static_cast<gf::s32>(options.width/2),
                                   static_cast<gf::s32>(options.height)-24,
                                    gf::TextRenderer::Style{255,255,255,255,1,1,2});
                if(have_option_widgets&&option_transition.phase()==gf::OptionTransitionPhase::UI&&option_menu.phase()!=gf::OptionMenuPhase::Editing&&
                   option_menu.phase()!=gf::OptionMenuPhase::CreditsDelay&&option_menu.phase()!=gf::OptionMenuPhase::Credits){
                    const char* message="STAFF CREDITS NOT PORTED\nB BACK";
                    if(option_menu.phase()==gf::OptionMenuPhase::Saved)message="NATIVE SETTINGS SAVED\nA RETURN";
                    if(option_menu.phase()==gf::OptionMenuPhase::SaveError)message="NATIVE SAVE FAILED\nA RETRY / B BACK\nINVALID/UNREADABLE RECORDS KEPT";
                    text.draw_centered(render,message,static_cast<gf::s32>(options.width/2),static_cast<gf::s32>(options.height/2)-30,
                        gf::TextRenderer::Style{255,255,255,255,2,1,2});
                }
                render.state()=saved;
            }
            render.end_frame();
            if (!screenshot_written) screenshot_written = capture_if_requested(context, options, frame);
            render.present();
        } else {
            draws = session.render_frame(frame);
            if (!screenshot_written) screenshot_written = capture_if_requested(context, options, frame);
        }
        context.advance_frame();
        if(have_option_output)option_output.update();
        ++frame;
        if (options.frames && frame >= options.frames) {
            gf::log_info(context, "host", "frame %u draws=%u%s", frame, draws,
                         options.menu ? " (menu)" : "");
            break;
        }
    }

    gf::log_info(context, "host", "frames executed=%u", frame);
    if(have_option_output)gf::log_info(context,"option","output summary SDL-consumed=%llu queued=%llu discarded=%llu; SFX/VOICE and other cue routing NOT PORTED",
        static_cast<unsigned long long>(context.audio().frames_consumed()),static_cast<unsigned long long>(context.audio().frames_available()),static_cast<unsigned long long>(context.audio().frames_discarded()));
    if(have_music)gf::log_info(context,"music","original source output-frames=%llu accepted=%llu loop-wraps=%llu position=%u; playback/queue counts, NOT simulation FPS",
        static_cast<unsigned long long>(have_cue_music?cue_music.generated():music.generated()),static_cast<unsigned long long>(have_cue_music?cue_music.accepted():music.accepted()),
        static_cast<unsigned long long>(have_cue_music?cue_music.wraps():music.wraps()),have_cue_music?cue_music.source_position():music.source_position());
    if(have_cue_music)gf::log_info(context,"music","cue totals menu=%llu credits=%llu switches=%llu scalar=%g pending=%u; source operations/native output, not hardware parity",
        static_cast<unsigned long long>(cue_music.cue_accepted(gf::MusicCue::Menu)),static_cast<unsigned long long>(cue_music.cue_accepted(gf::MusicCue::Credits)),
        static_cast<unsigned long long>(cue_music.switches()),cue_music.fade().value(),unsigned(cue_music.fade().pending()));

    if (!options.menu && !session.save(0))
        gf::log_warn(context, "host", "save slot 0 not written");
    render.shutdown();
    if (platform) platform->pump_events();
    return 0;
}
