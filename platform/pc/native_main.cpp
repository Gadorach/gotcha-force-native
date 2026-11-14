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

} // namespace

int main(int argc, char** argv) {
    Options options;
    if (!parse(argc, argv, &options)) return 2;

    gf::headless::Bundle headless;
    std::unique_ptr<gf::IFileSystem> headless_files;
    std::unique_ptr<gf::ILogBackend> headless_log;
    gf::Context context(headless.get(), 60);
    if(options.headless&&options.test_battle){
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
            platform->create({options.width, options.height, "Gotcha Force", false, false},
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
    if (!render.initialize({options.width, options.height, !options.headless, true, "Gotcha Force"},
                           context.backends().window, &error)) {
        gf::log_error(context, "render", "initialization failed: %s", error.c_str());
        return 1;
    }
    gf::log_info(context, "host", "backend=%s", render.capabilities().name.c_str());
    if(options.test_battle)return run_battle_test(context,render,options);

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
