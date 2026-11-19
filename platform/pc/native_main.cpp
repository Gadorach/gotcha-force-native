// Native rewrite entry point.
//
// This is the Linux/desktop host: it assembles the platform backends, hands the
// engine a Context, and runs frames. It contains no game logic; swapping a
// renderer or moving to another OS changes this file and platform/, nothing else.
#include <algorithm>
#include <array>
#include <chrono>
#include <charconv>
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
#include "engine/gameplay/borg_installed_browser_view.hpp"
#include "engine/gameplay/borg_placement_confirmation.hpp"
#include "engine/save/borg_package_placement.hpp"
#include "engine/gameplay/borg_recovery_menu_view.hpp"
#include "engine/gameplay/borg_package_force_view.hpp"
#include "engine/gameplay/borg_mixed_force_view.hpp"
#include "engine/save/force_inventory.hpp"
#include "engine/graphics/render_engine.hpp"
#include "engine/graphics/scene_draw.hpp"
#include "engine/input/input_router.hpp"
#include "engine/log.hpp"
#include "platform/gl/gl_renderer.hpp"
#include "platform/pc/borg_selected_save.hpp"
#include "platform/pc/borg_acquisition_load.hpp"
#include "platform/pc/borg_effect_load.hpp"
#include "platform/pc/borg_form_load.hpp"
#include "platform/pc/borg_fusion_load.hpp"
#include "platform/pc/borg_move_load.hpp"
#include "platform/pc/borg_practice_load.hpp"
#include "platform/pc/borg_spawn_load.hpp"
#include "platform/pc/gred_practice_input.hpp"
#include "platform/pc/sdl_platform.hpp"

namespace {

constexpr gf::f32 kProbeIdentity[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};

struct Options {
    bool headless = false;
    bool list_borgs = false;
    bool browse_borgs=false,demo_borg_browser=false,browse_owned_borgs=false;
    bool practice_from_picker=false;
    std::string selected_save_root,retail_force_catalogue;
    gf::u64 selected_account=0,selected_save=0,recovery_notice=0,inspect_force=0,owned_instance=0,placement_force=0,placement_slot=0;
    std::string check_borg_catalog,check_borg_moves,check_borg_spawn,check_borg_effects,check_borg_forms,check_borg_fusions,
        check_borg_acquisition,check_borg_variants,preview_borg,practice_borg,borg_variant;
    std::optional<gf::pc::BorgOwnedPracticeRequest> owned_practice;
    std::optional<gf::pc::BorgForcePracticeRequest> force_practice;
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
    bool source_ground_loss_surface=false;
    bool source_ground_loss_jump=false;
    bool source_ground_loss_landing=false;
    bool source_ground_loss_air=false;
    bool source_ground_loss_relaunch=false;
    bool source_ground_fall_push=false;
    bool source_ground_air_fall_push=false;
    bool source_ground_relaunch_fall_push=false;
    bool source_ground_recovery_push=false;
    bool source_ground_windup_push=false;
    bool source_ground_active_push=false;
    bool source_ground_relaunch_push=false;
    bool source_ground_relaunch_entry_push=false;
    bool source_ground_windup_entry_push=false;
    bool gred_practice=false;
    bool gred_slope_dash=false;
    bool gred_slope_seam_dash=false;
    bool gred_slope_crease_dash=false;
    bool gred_slope_transition_dash=false;
    bool gred_inclined_dash_ledge=false;
    bool gred_inclined_walk_ledge=false;
    bool gred_practice_start=false;
    bool gred_walk_ledge_fall_push=false;
    bool gred_walk_ledge_high_angle_push=false;
    bool gred_walk_ledge_expiry_push=false;
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
    auto id=[](std::string_view text,gf::u64* out){
        if(text.empty())return false;
        const auto end=text.data()+text.size();
        const auto value=std::from_chars(text.data(),end,*out);
        return value.ec==std::errc{}&&value.ptr==end&&*out!=0;
    };
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--headless") options->headless = true;
        else if (arg == "--list-borgs") options->list_borgs = true;
        else if(arg=="--practice-start"){
            if(options->gred_practice_start)return false;
            options->gred_practice_start=true;
        }
        else if(arg=="--practice-from-picker"){
            if(options->practice_from_picker)return false;
            options->practice_from_picker=true;
        }
        else if (arg=="--browse-borgs"||arg=="--demo-borg-browser"){
            if(options->browse_borgs)return false;
            options->browse_borgs=true;options->demo_borg_browser=arg=="--demo-borg-browser";
        }
        else if(arg=="--recover-borgs"||arg=="--inspect-borg-force"||arg=="--browse-owned-borgs"||arg=="--place-owned-borg"){
            const bool inspect=arg=="--inspect-borg-force";
            const bool place=arg=="--place-owned-borg";
            if(argc-i<=3+int(inspect)+2*int(place)||!options->selected_save_root.empty())return false;
            options->selected_save_root=argv[++i];
            if(options->selected_save_root.empty()||!id(argv[++i],&options->selected_account)||!id(argv[++i],&options->selected_save))return false;
            if(inspect&&(!id(argv[++i],&options->inspect_force)||options->inspect_force>gf::kForceCount))return false;
            if(place&&(!id(argv[++i],&options->placement_force)||options->placement_force>gf::kForceCount||
                       !id(argv[++i],&options->placement_slot)||options->placement_slot>gf::kForceSlots))return false;
            options->browse_owned_borgs=arg=="--browse-owned-borgs"||place;
        }else if(arg=="--owned-instance"){
            if(++i>=argc||options->owned_instance||!id(argv[i],&options->owned_instance))return false;
        }else if(arg=="--recovery-notice"){
            if(++i>=argc||options->recovery_notice||!id(argv[i],&options->recovery_notice))return false;
        }else if(arg=="--retail-catalog"){
            if(++i>=argc||!options->retail_force_catalogue.empty()||!*argv[i])return false;
            options->retail_force_catalogue=argv[i];
        }
        else if (arg=="--check-borg-catalog"){
            if(++i>=argc||!options->check_borg_catalog.empty()||!gf::borg_package_id_valid(argv[i]))return false;
            options->check_borg_catalog=argv[i];
        }else if(arg=="--check-borg-moves"){
            if(++i>=argc||!options->check_borg_moves.empty()||!gf::borg_package_id_valid(argv[i]))return false;
            options->check_borg_moves=argv[i];
        }else if(arg=="--check-borg-spawn"){
            if(++i>=argc||!options->check_borg_spawn.empty()||!gf::borg_package_id_valid(argv[i]))return false;
            options->check_borg_spawn=argv[i];
        }else if(arg=="--check-borg-effects"){
            if(++i>=argc||!options->check_borg_effects.empty()||!gf::borg_package_id_valid(argv[i]))return false;
            options->check_borg_effects=argv[i];
        }else if(arg=="--check-borg-forms"){
            if(++i>=argc||!options->check_borg_forms.empty()||!gf::borg_package_id_valid(argv[i]))return false;
            options->check_borg_forms=argv[i];
        }else if(arg=="--check-borg-fusions"){
            if(++i>=argc||!options->check_borg_fusions.empty()||!gf::borg_package_id_valid(argv[i]))return false;
            options->check_borg_fusions=argv[i];
        }else if(arg=="--check-borg-acquisition"){
            if(++i>=argc||!options->check_borg_acquisition.empty()||!gf::borg_package_id_valid(argv[i]))return false;
            options->check_borg_acquisition=argv[i];
        }else if(arg=="--check-borg-variants"){
            if(++i>=argc||!options->check_borg_variants.empty()||!gf::borg_package_id_valid(argv[i]))return false;
            options->check_borg_variants=argv[i];
        }else if(arg=="--preview-borg"){
            if(++i>=argc||!options->preview_borg.empty()||!gf::borg_package_id_valid(argv[i]))return false;
            options->preview_borg=argv[i];
        }else if(arg=="--practice-borg"){
            if(++i>=argc||!options->practice_borg.empty()||!gf::borg_package_id_valid(argv[i]))return false;
            options->practice_borg=argv[i];
        }else if(arg=="--practice-owned-borg"){
            if(argc-i<=4||options->owned_practice)return false;
            auto& request=options->owned_practice.emplace();
            request.root=argv[++i];
            if(request.root.empty()||!id(argv[++i],&request.key.account)||!id(argv[++i],&request.key.save)||
               !id(argv[++i],&request.instance))return false;
        }else if(arg=="--practice-force-slot"){
            if(argc-i<=5||options->force_practice)return false;
            auto& request=options->force_practice.emplace();gf::u64 force=0,slot=0;
            request.root=argv[++i];
            if(request.root.empty()||!id(argv[++i],&request.key.account)||!id(argv[++i],&request.key.save)||
               !id(argv[++i],&force)||force>gf::kForceCount||!id(argv[++i],&slot)||slot>gf::kForceSlots)return false;
            request.force=unsigned(force-1);request.slot=unsigned(slot-1);
        }else if(arg=="--borg-variant"){
            if(++i>=argc||!options->borg_variant.empty()||!gf::borg_catalog_variant_id_valid(argv[i]))return false;
            options->borg_variant=argv[i];
        }
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
                 arg == "--source-ground-surface" || arg == "--demo-source-ground-surface" ||
                 arg == "--source-ground-loss-surface" || arg == "--demo-source-ground-loss-surface" ||
                 arg == "--source-ground-loss-jump" || arg == "--demo-source-ground-loss-jump" ||
                 arg == "--source-ground-loss-landing" || arg == "--demo-source-ground-loss-landing" ||
                 arg == "--source-ground-loss-air" || arg == "--demo-source-ground-loss-air" ||
                 arg == "--source-ground-loss-relaunch" || arg == "--demo-source-ground-loss-relaunch" ||
                 arg == "--source-ground-fall-push" || arg == "--demo-source-ground-fall-push" ||
                 arg == "--source-ground-air-fall-push" || arg == "--demo-source-ground-air-fall-push" ||
                 arg == "--source-ground-relaunch-fall-push" || arg == "--demo-source-ground-relaunch-fall-push" ||
                 arg == "--source-ground-recovery-push" || arg == "--demo-source-ground-recovery-push" ||
                 arg == "--source-ground-windup-push" || arg == "--demo-source-ground-windup-push" ||
                 arg == "--source-ground-active-push" || arg == "--demo-source-ground-active-push" ||
                 arg == "--source-ground-relaunch-push" || arg == "--demo-source-ground-relaunch-push" ||
                 arg == "--source-ground-relaunch-entry-push" || arg == "--demo-source-ground-relaunch-entry-push" ||
                 arg == "--source-ground-windup-entry-push" || arg == "--demo-source-ground-windup-entry-push" ||
                 arg == "--gred-movement" || arg == "--demo-gred-movement" ||
                 arg == "--gred-slope-dash" || arg == "--demo-gred-slope-dash" ||
                 arg == "--gred-slope-seam-dash" || arg == "--demo-gred-slope-seam-dash" ||
                 arg == "--gred-slope-crease-dash" || arg == "--demo-gred-slope-crease-dash" ||
                 arg == "--gred-slope-transition-dash" || arg == "--demo-gred-slope-transition-dash" ||
                 arg == "--gred-inclined-dash-ledge" || arg == "--demo-gred-inclined-dash-ledge" ||
                 arg == "--gred-inclined-walk-ledge" || arg == "--demo-gred-inclined-walk-ledge" ||
                 arg == "--gred-walk-ledge-fall-push" || arg == "--demo-gred-walk-ledge-fall-push" ||
                 arg == "--gred-walk-ledge-high-angle-push" || arg == "--demo-gred-walk-ledge-high-angle-push" ||
                 arg == "--gred-walk-ledge-expiry-push" || arg == "--demo-gred-walk-ledge-expiry-push"){
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
            options->source_ground_loss_surface=arg=="--source-ground-loss-surface"||arg=="--demo-source-ground-loss-surface";
            options->source_ground_loss_jump=arg=="--source-ground-loss-jump"||arg=="--demo-source-ground-loss-jump";
            options->source_ground_loss_landing=arg=="--source-ground-loss-landing"||arg=="--demo-source-ground-loss-landing";
            options->source_ground_loss_air=arg=="--source-ground-loss-air"||arg=="--demo-source-ground-loss-air";
            options->source_ground_loss_relaunch=arg=="--source-ground-loss-relaunch"||arg=="--demo-source-ground-loss-relaunch";
            options->source_ground_fall_push=arg=="--source-ground-fall-push"||arg=="--demo-source-ground-fall-push";
            options->source_ground_air_fall_push=arg=="--source-ground-air-fall-push"||arg=="--demo-source-ground-air-fall-push";
            options->source_ground_relaunch_fall_push=arg=="--source-ground-relaunch-fall-push"||arg=="--demo-source-ground-relaunch-fall-push";
            options->source_ground_recovery_push=arg=="--source-ground-recovery-push"||arg=="--demo-source-ground-recovery-push";
            options->source_ground_windup_push=arg=="--source-ground-windup-push"||arg=="--demo-source-ground-windup-push";
            options->source_ground_active_push=arg=="--source-ground-active-push"||arg=="--demo-source-ground-active-push";
            options->source_ground_relaunch_push=arg=="--source-ground-relaunch-push"||arg=="--demo-source-ground-relaunch-push";
            options->source_ground_relaunch_entry_push=arg=="--source-ground-relaunch-entry-push"||arg=="--demo-source-ground-relaunch-entry-push";
            options->gred_walk_ledge_expiry_push=arg=="--gred-walk-ledge-expiry-push"||arg=="--demo-gred-walk-ledge-expiry-push";
            options->gred_walk_ledge_high_angle_push=options->gred_walk_ledge_expiry_push||
                arg=="--gred-walk-ledge-high-angle-push"||arg=="--demo-gred-walk-ledge-high-angle-push";
            options->gred_walk_ledge_fall_push=options->gred_walk_ledge_high_angle_push||
                arg=="--gred-walk-ledge-fall-push"||arg=="--demo-gred-walk-ledge-fall-push";
            options->gred_inclined_dash_ledge=options->gred_walk_ledge_fall_push||
                arg=="--gred-inclined-dash-ledge"||arg=="--demo-gred-inclined-dash-ledge";
            options->gred_inclined_walk_ledge=arg=="--gred-inclined-walk-ledge"||arg=="--demo-gred-inclined-walk-ledge";
            options->gred_slope_transition_dash=options->gred_inclined_dash_ledge||options->gred_inclined_walk_ledge||
                arg=="--gred-slope-transition-dash"||arg=="--demo-gred-slope-transition-dash";
            options->gred_slope_crease_dash=options->gred_slope_transition_dash||arg=="--gred-slope-crease-dash"||arg=="--demo-gred-slope-crease-dash";
            options->gred_slope_seam_dash=options->gred_slope_crease_dash||arg=="--gred-slope-seam-dash"||arg=="--demo-gred-slope-seam-dash";
            options->gred_slope_dash=options->gred_slope_seam_dash||arg=="--gred-slope-dash"||arg=="--demo-gred-slope-dash";
            options->gred_practice=options->gred_slope_dash||arg=="--gred-movement"||arg=="--demo-gred-movement";
            options->source_ground_windup_entry_push=(options->gred_practice&&!options->gred_slope_dash)||
                arg=="--source-ground-windup-entry-push"||arg=="--demo-source-ground-windup-entry-push";
            options->source_ordered_ground=arg=="--source-ordered-ground"||arg=="--demo-source-ordered-ground"||options->source_dash_ledge||options->source_ground_ledge||options->source_ground_jump||options->source_ground_landing||options->source_ground_air||options->source_ground_air_floor||options->source_ground_relaunch||options->source_ground_relaunch_floor||options->source_ground_windup_entry_floor||options->source_ground_surface||options->source_ground_loss_surface||options->source_ground_loss_jump||options->source_ground_loss_landing||options->source_ground_loss_air||options->source_ground_loss_relaunch||options->source_ground_fall_push||options->source_ground_air_fall_push;
            if(options->source_ordered_ground)options->source_ground_interactive=arg=="--source-ordered-ground"||arg=="--source-dash-ledge"||arg=="--source-ground-ledge"||arg=="--source-ground-jump"||arg=="--source-ground-landing"||arg=="--source-ground-air"||arg=="--source-ground-air-floor"||arg=="--source-ground-relaunch"||arg=="--source-ground-relaunch-floor"||arg=="--source-ground-windup-entry-floor"||arg=="--source-ground-surface"||arg=="--source-ground-loss-surface"||arg=="--source-ground-loss-jump"||arg=="--source-ground-loss-landing"||arg=="--source-ground-loss-air"||arg=="--source-ground-loss-relaunch"||arg=="--source-ground-fall-push"||arg=="--source-ground-air-fall-push";
            options->source_windup_entry_push=arg=="--source-windup-entry-push"||arg=="--demo-source-windup-entry-push";
            if(options->gred_slope_dash||options->source_ground_relaunch_fall_push||options->source_ground_recovery_push||options->source_ground_windup_push||options->source_ground_active_push||options->source_ground_relaunch_push||options->source_ground_relaunch_entry_push||options->source_ground_windup_entry_push){
                options->source_ordered_ground=true;
                options->source_ground_interactive=!arg.starts_with("--demo-");
            }
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
                         "[--screenshot PATH.ppm]\n"
                          "       gf_native --gred-movement [--practice-start] [--no-audio]\n"
                          "       gf_native --gred-movement --practice-borg NAMESPACE:ID --borg-variant normal\n"
                          "       gf_native --gred-movement --practice-owned-borg ROOT ACCOUNT SAVE INSTANCE\n"
                          "       gf_native --gred-movement --practice-force-slot ROOT ACCOUNT SAVE FORCE SLOT [--retail-catalog FILE]\n"
                         "       gf_native --demo-gred-movement [--frames N] [--no-audio]\n"
                         "       gf_native --check-borg-moves NAMESPACE:ID\n"
                         "       gf_native --check-borg-spawn NAMESPACE:ID\n"
                         "       gf_native --check-borg-effects NAMESPACE:ID\n"
                         "       gf_native --check-borg-forms NAMESPACE:ID\n"
                         "       gf_native --check-borg-fusions NAMESPACE:ID\n"
                         "       gf_native --check-borg-acquisition NAMESPACE:ID\n"
                         "       gf_native --check-borg-variants NAMESPACE:ID\n"
                         "       gf_native --browse-borgs [--headless] [--frames N]\n"
                         "       gf_native --recover-borgs ROOT ACCOUNT SAVE [--recovery-notice INSTANCE] [--headless]\n"
                          "       gf_native --browse-owned-borgs ROOT ACCOUNT SAVE [--owned-instance INSTANCE] [--practice-from-picker]\n"
                         "       gf_native --place-owned-borg ROOT ACCOUNT SAVE FORCE SLOT [--owned-instance INSTANCE] [--retail-catalog FILE]\n"
                         "       gf_native --inspect-borg-force ROOT ACCOUNT SAVE FORCE [--retail-catalog FILE]\n"
                         "       gf_native --preview-borg NAMESPACE:ID [--borg-variant ID] [--headless] [--frames N]\n");
            return false;
        }
    }
    if(unsigned(options->demo_input)+unsigned(options->demo_option)+unsigned(options->demo_credits)>1)return false;
    if(options->test_battle&&(options->menu||options->demo_input||options->demo_option||options->demo_credits||options->probe_texture>=0))return false;
    if(options->battle_mvp&&(options->test_battle||options->menu||options->demo_input||options->demo_option||options->demo_credits||options->probe_texture>=0))return false;
    if(options->animation_mosaic&&(options->battle_mvp||options->test_battle||options->menu||options->demo_input||options->demo_option||options->demo_credits||options->probe_texture>=0))return false;
    if(!options->preview_borg.empty()&&(options->animation_mosaic||options->battle_mvp||options->test_battle||options->menu||
       options->demo_input||options->demo_option||options->demo_credits||options->probe_texture>=0||options->list_borgs||
       !options->check_borg_catalog.empty()||!options->check_borg_moves.empty()||!options->check_borg_spawn.empty()||
       !options->check_borg_effects.empty()||!options->check_borg_forms.empty()||!options->check_borg_fusions.empty()||
       !options->check_borg_acquisition.empty()||!options->check_borg_variants.empty()||options->animation_page||options->animation_borg))return false;
    if(!options->borg_variant.empty()&&options->preview_borg.empty()&&options->practice_borg.empty())return false;
    const unsigned practice_sources=unsigned(!options->practice_borg.empty())+unsigned(options->owned_practice.has_value())+
        unsigned(options->force_practice.has_value());
    if(practice_sources>1||(!options->practice_borg.empty()&&options->borg_variant.empty())||
       ((options->owned_practice||options->force_practice)&&!options->borg_variant.empty()))return false;
    if(practice_sources&&(!options->gred_practice||
       !options->preview_borg.empty()||!options->source_ground_assets.empty()||options->list_borgs||
       !options->check_borg_catalog.empty()||!options->check_borg_moves.empty()||!options->check_borg_spawn.empty()||
       !options->check_borg_effects.empty()||!options->check_borg_forms.empty()||!options->check_borg_fusions.empty()||
       !options->check_borg_acquisition.empty()||!options->check_borg_variants.empty()))return false;
    if(options->browse_borgs&&(options->animation_mosaic||options->battle_mvp||options->test_battle||
       options->menu||options->demo_input||options->demo_option||options->demo_credits||options->probe_texture>=0||
       !options->preview_borg.empty()||options->list_borgs||!options->check_borg_catalog.empty()||
       !options->check_borg_moves.empty()||!options->check_borg_spawn.empty()||!options->check_borg_effects.empty()||
       !options->check_borg_forms.empty()||!options->check_borg_fusions.empty()||!options->check_borg_acquisition.empty()||
       !options->check_borg_variants.empty()||options->width<440||options->height<320||options->width>4096||options->height>4096))return false;
    if(options->recovery_notice&&(options->selected_save_root.empty()||options->inspect_force||options->browse_owned_borgs))return false;
    if(options->owned_instance&&!options->browse_owned_borgs)return false;
    if(options->practice_from_picker&&(!options->browse_owned_borgs||options->placement_force))return false;
    if(!options->retail_force_catalogue.empty()&&!options->inspect_force&&!options->placement_force&&!options->force_practice)return false;
    if(!options->selected_save_root.empty()&&(options->animation_mosaic||options->battle_mvp||options->test_battle||
       options->menu||options->demo_input||options->demo_option||options->demo_credits||options->probe_texture>=0||
       !options->preview_borg.empty()||options->browse_borgs||options->list_borgs||!options->check_borg_catalog.empty()||
       !options->check_borg_moves.empty()||!options->check_borg_spawn.empty()||!options->check_borg_effects.empty()||
       !options->check_borg_forms.empty()||!options->check_borg_fusions.empty()||!options->check_borg_acquisition.empty()||
       !options->check_borg_variants.empty()||(!options->inspect_force&&
        (options->width<520||options->height<360||options->width>4096||options->height>4096))))return false;
    if(!options->animation_export.empty()&&(!options->animation_mosaic||options->headless||options->animation_export_frames<2||options->animation_export_frames>120))return false;
    if(!options->animation_export.empty()&&(options->width>2048||options->height>2048||options->width<320||options->height<240))return false;
    if(options->animation_programs.empty()!=options->animation_height_profile.empty())return false;
    if(!options->animation_programs.empty()&&(!options->animation_mosaic||options->animation_borg||!options->animation_export.empty()))return false;
    if(!options->animation_motion_profile.empty()&&options->animation_programs.empty())return false;
    if(options->demo_source_ground&&(options->demo_battle_mvp||options->demo_locomotion))return false;
    if(options->gred_practice_start&&(!options->gred_practice||!options->source_ground_interactive))return false;
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

struct PickerPracticeHandoff {
    gf::u64 session_started=0;
    bool captured=false;
};

int run_battle_mvp(gf::Context& context,gf::RenderEngine& render,const Options& options,
                   const gf::pc::BorgPracticeFiles* package=nullptr,const gf::pc::BorgOwnedPractice* owned=nullptr,
                   PickerPracticeHandoff* picker=nullptr){
    gf::NativeBattleMvp world;gf::NativeBattleMvpView view;std::string error;
    const bool wants_save=options.owned_practice.has_value()||options.force_practice.has_value();
    const bool wants_package=!options.practice_borg.empty()||wants_save;
    const bool matches_save=!owned||(options.owned_practice?owned->matches(*options.owned_practice):
        options.force_practice&&owned->matches(*options.force_practice));
    if((picker&&(!owned||!options.source_ground_interactive))||
       (options.owned_practice&&options.force_practice)||bool(package)!=wants_package||
       bool(owned)!=wants_save||(package&&!options.gred_practice)||
       (owned&&(package!=&owned->files()||!matches_save))||
       (!owned&&package&&(package->identity.id!=options.practice_borg||package->variant!=options.borg_variant))){
        gf::log_error(context,"borg-practice","exact requested practice snapshot required; no fallback");
        render.shutdown();return 2;
    }
    auto owned_current=[&](){
        if(!owned||owned->current())return true;
        error="selected save changed; reopen it to start practice again";return false;
    };
    const auto practice_bytes=package?package->views():gf::NativeGRedPracticeAssets{};
    const auto* practice=package?&practice_bytes:nullptr;
    gf::BattleMvpConfig config;
    const bool use_demo_start=!options.source_ground_interactive||options.gred_practice_start;
    //Explicit native demo placement at original stage473 floor triangle1297's
    //interior, not a recovered actor constructor/spawn or edited stage asset.
    if(options.source_surface&&!options.source_ground_interactive)config.spawn[0]={-619.2617f,169.4453f};
    if(options.source_steep&&!options.source_ground_interactive)config.spawn[0]={-606.6735f,-1373.0029f}; //native placement on steep original triangle1320
    if(options.source_fall_surface&&!options.source_ground_interactive)config.spawn[0]={-606.4511719f,-1373.5872803f}; //native interior point on the same original steep face
    //Bounded original-stage recovery route; BOTH placements are native demo
    //assumptions, not recovered actor constructor/spawn or edited stage data.
    if((options.source_recovery_surface||options.source_ground_recovery_push)&&!options.source_ground_interactive){
        config.spawn[0]={44.928558350f,-332.878112793f};config.spawn[1]={1200,1200};
    }
    //Unchanged original mountain triangle2536. Wider bounds and a static-floor
    //query reference are declared native DEMO assumptions only; normal play
    //keeps its old arena/spawn and no airborne position is fabricated.
    if((options.source_windup_surface||options.source_ground_windup_push)&&!options.source_ground_interactive){
        config.arena_half=4500;config.spawn[0]={-2302.447265625f,528.269836426f};
        config.spawn_query_y[0]=591.912719727f;config.spawn[1]={240,-300};
    }
    //Native one-shot active-contact route. Both actors stand on real selected
    //floors; only placement/bounds are demo assumptions, not source spawns.
    if((options.source_active_surface||options.source_ground_active_push)&&!options.source_ground_interactive){
        config.arena_half=4500;config.spawn[0]={295.518066406f,1631.279907227f};
        config.spawn[1]={-2850,3000};config.spawn_query_y={0,0};
    }
    //This neutral relaunch demo needs only a native player placement on the
    //original small incline; reset inherited demo-only bounds/target/reference.
    if((options.source_relaunch_surface||options.source_ground_relaunch_push||options.source_ground_relaunch_entry_push)&&!options.source_ground_interactive){
        config={};config.spawn[0]={-606.4511719f,-1373.5872803f};
    }
    if(options.source_relaunch_floor&&!options.source_ground_interactive)config={}; //ordinary native MVP spawn, target, bounds and floor references
    if((options.source_windup_entry_push||options.source_ground_windup_entry_push)&&use_demo_start){
        config={};config.arena_half=4500;config.spawn[0]={-2760.075195312f,-3528.048583984f};config.spawn_query_y[0]=385.716827393f;
    } //native-only grounded placement/reference at original triangle145; opt-in for live practice
    if(options.source_walk_ledge&&!options.source_ground_interactive){
        config={};config.arena_half=4500;config.spawn[0]={1010.166015625f,2315.240722656f};config.spawn_query_y[0]=482.747558594f;
    } //native-only roof249 placement/reference; no actor teleport or edited stage
    if(options.source_dash_ledge&&!options.source_ground_interactive){
        config={};config.arena_half=4500;config.spawn[0]={1012.360657f,1403.599731f};config.spawn_query_y[0]=482.747589f;
    } //native ground spawn on the original roof edge; unchanged target/stage, no airborne teleport
    if(options.source_ground_ledge&&!options.source_ground_interactive){
        config={};config.arena_half=4500;config.spawn[0]={1010.166015625f,2315.240722656f};config.spawn_query_y[0]=482.747558594f;
    } //native ground spawn on original roof249; walk, neutral release, then genuine dash input in the same world
    if((options.source_ground_loss_surface||options.source_ground_loss_jump||options.source_ground_loss_landing||options.source_ground_loss_air||options.source_ground_loss_relaunch)&&!options.source_ground_interactive){
        config={};config.arena_half=4500;config.spawn[0]={-1368.47f,(options.source_ground_loss_landing||options.source_ground_loss_relaunch)?-466.667f:-628.674f};config.spawn_query_y[0]=94.339f;
    } //declared grounded original-stage placement/query reference; not an original spawn, teleport or edited terrain
    if(options.source_ground_fall_push&&!options.source_ground_interactive){
        config={};config.spawn[0]={-606.6735f,-1373.0029f};
    } //native grounded placement at original steep face; ordinary bounds/target, no midair injection
    if(options.source_ground_air_fall_push&&!options.source_ground_interactive){
        config={};config.arena_half=4500;config.spawn[0]={-585.f,-3010.f};
    } //declared grounded approach on unchanged stage; long air-dash route, not an original spawn
    if(options.source_ground_relaunch_fall_push&&!options.source_ground_interactive){
        config={};config.spawn[0]={-606.4511719f,-1373.5872803f};
    } //declared native grounded original-stage fixture, unchanged bounds and target
    if(options.gred_slope_dash&&use_demo_start){
        config.arena_half=4500;config.spawn[0]={-1443.46997f,-703.674011f};config.spawn_query_y[0]=94.339f;
        if(options.gred_slope_seam_dash){config.spawn[0]={1406.01709f,2922.08545f};config.spawn_query_y[0]=485.585205f;}
        if(options.gred_slope_crease_dash){config.spawn[0]={3589.76465f,-2425.93408f};config.spawn_query_y[0]=192.512665f;}
        if(options.gred_slope_transition_dash){config.spawn[0]={830.733398f,2191.48145f};config.spawn_query_y[0]=487.747528f;}
        if(options.gred_inclined_dash_ledge){config.spawn[0]={2582.89014f,-2846.36206f};config.spawn_query_y[0]=116.592773f;}
        if(options.gred_inclined_walk_ledge){config.spawn[0]={2599.00806f,-2758.12109f};config.spawn_query_y[0]=125.757187f;}
        if(options.gred_walk_ledge_fall_push){
            config.spawn[0]={1000.65021f,options.gred_walk_ledge_high_angle_push?-3362.34546f:-3363.34546f};
            config.spawn_query_y[0]=1549.25293f;
        }
        if(options.gred_walk_ledge_expiry_push){config.spawn[0]={-1250.96863f,-3313.80225f};config.spawn_query_y[0]=1846.01367f;}
    }
    if(!owned_current()||!view.load(context.files(),options.battle_mvp_assets,render,&error,practice)||
       !world.start(config,&view)||!view.update(world,&error)){
        gf::log_error(context,"battle-mvp","initialization failed: %s",error.c_str());render.shutdown();return 2;}
    gf::NativeGRedGroundDashDemo source;gf::GamePadState source_previous;
    const auto source_inputs=options.source_ground_assets.empty()?std::string(options.source_walk_ledge?"local/content/source-gred-ground-v8":options.source_relaunch_floor?"local/content/source-gred-ground-v7":options.source_steep?"local/content/source-gred-ground-v6":options.source_surface?"local/content/source-gred-ground-v5":options.source_air_dash?"local/content/source-gred-ground-v4":options.source_moving_jump?"local/content/source-gred-ground-v3":
        options.source_ordered_ground?"local/content/source-gred-ground-v9":options.source_ground_walk?"local/content/source-gred-ground-v2":"local/content/source-gred-ground-v1"):options.source_ground_assets;
    const auto source_mode=options.gred_walk_ledge_expiry_push?gf::NativeGRedGroundDashDemo::Mode::GroundWalkLedgeExpiryPush:
         options.gred_walk_ledge_high_angle_push?gf::NativeGRedGroundDashDemo::Mode::GroundWalkLedgeHighAnglePush:
         options.gred_walk_ledge_fall_push?gf::NativeGRedGroundDashDemo::Mode::GroundWalkLedgeFallPush:
         options.gred_inclined_dash_ledge?gf::NativeGRedGroundDashDemo::Mode::GroundInclinedDashLedge:
         options.gred_slope_transition_dash?gf::NativeGRedGroundDashDemo::Mode::GroundSlopeTransitionDash:
        options.gred_slope_crease_dash?gf::NativeGRedGroundDashDemo::Mode::GroundSlopeCreaseDash:
        options.gred_slope_seam_dash?gf::NativeGRedGroundDashDemo::Mode::GroundSlopeSeamDash:
        options.gred_slope_dash?gf::NativeGRedGroundDashDemo::Mode::GroundSlopeDash:
        options.source_ground_windup_entry_push?gf::NativeGRedGroundDashDemo::Mode::GroundWindupEntryPush:
        options.source_ground_relaunch_entry_push?gf::NativeGRedGroundDashDemo::Mode::GroundRelaunchEntryPush:
        options.source_ground_relaunch_push?gf::NativeGRedGroundDashDemo::Mode::GroundRelaunchPush:
        options.source_ground_active_push?gf::NativeGRedGroundDashDemo::Mode::GroundActivePush:
        options.source_ground_windup_push?gf::NativeGRedGroundDashDemo::Mode::GroundWindupPush:
        options.source_ground_recovery_push?gf::NativeGRedGroundDashDemo::Mode::GroundRecoveryPush:
        options.source_ground_relaunch_fall_push?gf::NativeGRedGroundDashDemo::Mode::GroundRelaunchFallPush:
        options.source_walk_ledge?gf::NativeGRedGroundDashDemo::Mode::WalkLedge:options.source_windup_entry_push?gf::NativeGRedGroundDashDemo::Mode::WindupEntryPush:options.source_windup_positive_floor?gf::NativeGRedGroundDashDemo::Mode::WindupPositiveFloor:options.source_windup_entry_floor?gf::NativeGRedGroundDashDemo::Mode::WindupEntryFloor:options.source_air_dash_floor?gf::NativeGRedGroundDashDemo::Mode::AirDashFloor:options.source_relaunch_floor?gf::NativeGRedGroundDashDemo::Mode::RelaunchFloor:options.source_relaunch_entry_surface?gf::NativeGRedGroundDashDemo::Mode::RelaunchEntrySurface:options.source_relaunch_surface?gf::NativeGRedGroundDashDemo::Mode::RelaunchSurface:options.source_active_surface?gf::NativeGRedGroundDashDemo::Mode::ActiveSurface:options.source_windup_surface?gf::NativeGRedGroundDashDemo::Mode::WindupSurface:options.source_recovery_surface?gf::NativeGRedGroundDashDemo::Mode::RecoverySurface:options.source_fall_surface?gf::NativeGRedGroundDashDemo::Mode::FallingSurface:options.source_steep?gf::NativeGRedGroundDashDemo::Mode::Steep:options.source_surface?gf::NativeGRedGroundDashDemo::Mode::Surface:options.source_landing_interrupt?gf::NativeGRedGroundDashDemo::Mode::LandingInterrupt:options.source_dash_jump?gf::NativeGRedGroundDashDemo::Mode::DashJump:options.source_relaunch?gf::NativeGRedGroundDashDemo::Mode::Relaunch:options.source_air_dash?gf::NativeGRedGroundDashDemo::Mode::AirDash:options.source_moving_jump?gf::NativeGRedGroundDashDemo::Mode::Jump:options.source_vertical_jump?gf::NativeGRedGroundDashDemo::Mode::VerticalJump:
        options.source_ground_air_fall_push?gf::NativeGRedGroundDashDemo::Mode::GroundAirFallPush:options.source_ground_fall_push?gf::NativeGRedGroundDashDemo::Mode::GroundFallPush:options.source_ground_loss_relaunch?gf::NativeGRedGroundDashDemo::Mode::GroundLossRelaunch:options.source_ground_loss_air?gf::NativeGRedGroundDashDemo::Mode::GroundLossAir:options.source_ground_loss_landing?gf::NativeGRedGroundDashDemo::Mode::GroundLossLanding:options.source_ground_loss_jump?gf::NativeGRedGroundDashDemo::Mode::GroundLossJump:options.source_ground_loss_surface?gf::NativeGRedGroundDashDemo::Mode::GroundLossSurface:options.source_ground_surface?gf::NativeGRedGroundDashDemo::Mode::GroundSurface:options.source_ground_windup_entry_floor?gf::NativeGRedGroundDashDemo::Mode::GroundWindupEntryFloor:options.source_ground_relaunch_floor?gf::NativeGRedGroundDashDemo::Mode::GroundRelaunchFloor:options.source_ground_relaunch?gf::NativeGRedGroundDashDemo::Mode::GroundRelaunch:options.source_ground_air_floor?gf::NativeGRedGroundDashDemo::Mode::GroundAirFloor:options.source_ground_air?gf::NativeGRedGroundDashDemo::Mode::GroundAir:options.source_ground_landing?gf::NativeGRedGroundDashDemo::Mode::GroundLanding:options.source_ground_jump?gf::NativeGRedGroundDashDemo::Mode::GroundJump:options.source_ground_ledge?gf::NativeGRedGroundDashDemo::Mode::GroundLedge:options.source_dash_ledge?gf::NativeGRedGroundDashDemo::Mode::DashLedge:options.source_ordered_ground?gf::NativeGRedGroundDashDemo::Mode::OrderedGround:options.source_ground_walk?gf::NativeGRedGroundDashDemo::Mode::Ground:gf::NativeGRedGroundDashDemo::Mode::DashOnly;
    auto bind_source=[&](){
        gf::NativeBorgAnimationSeed seed;
        if(!owned_current()||!view.source_seed(world,&seed)||
           !source.bind(context.files(),options.battle_mvp_assets,source_inputs,world,seed,&error,source_mode,practice))return false;
        if(!owned_current()){source.reject();return false;}
        return true;
    };
    if(options.demo_source_ground&&!bind_source()){gf::log_error(context,"source-ground","bind failed: %s",error.c_str());render.shutdown();return 2;}
    if(package)gf::log_info(context,"borg-practice","bound %s variant=%s; qualified normal G-Red movement only, no save or battle admission",
        package->identity.id.c_str(),package->variant.c_str());
    if(owned){
        view.source_practice_owned_copy(owned->saved().instance);
        gf::log_info(context,"borg-practice","account=%llu save=%llu owned instance=%llu (%s / %s); save is read-only",
            static_cast<unsigned long long>(owned->save_key().account),static_cast<unsigned long long>(owned->save_key().save),
            static_cast<unsigned long long>(owned->saved().instance),owned->saved().identity.name.c_str(),owned->saved().variant->label.c_str());
    }
    if(options.force_practice){
        const auto& force=*options.force_practice;
        view.source_practice_force_slot(force.force+1,force.slot+1);
        gf::log_info(context,"borg-practice","Force %u slot %u supplies owned instance=%llu; one practice copy, not Force admission",
            force.force+1,force.slot+1,static_cast<unsigned long long>(owned->saved().instance));
    }
    if(picker)view.source_practice_returns_to_picker();
    gf::InputRouter router;unsigned frame=0,commands=0;gf::u64 phase=0;bool captured=false;const auto started=context.clock().monotonic_ns();auto previous=started;
    gf::log_info(context,"battle-mvp","G-Red vs Sasuke: original stage/models, provisional native gameplay rules; no source-frame equivalence or audio claim");
     if(options.gred_walk_ledge_expiry_push)gf::log_info(context,"source-ground","walking ledge push at timer698 1 to 0 only after an accepted push, without drift; expired landing gate 10240 and native cap 13653; both travel checks");
     else if(options.gred_walk_ledge_high_angle_push)gf::log_info(context,"source-ground","continued descending walk-ledge push with positive owned timer698; native cap 13653, same selected-plane and pre/post-travel checks");
     else if(options.gred_walk_ledge_fall_push)gf::log_info(context,"source-ground","continued descending walk-ledge push with positive owned timer698 and angle above 6144 through 10240; separate native travel on both sides of projection");
     else if(options.gred_inclined_walk_ledge)gf::log_info(context,"source-ground","%s",use_demo_start?
         "ordinary walk across the verified inclined ledge fixture; no new movement policy or steep push":
         "inclined walk practice uses the ordinary start; add --practice-start for the verified foothold");
     else if(options.gred_inclined_dash_ledge)gf::log_info(context,"source-ground","ordered inclined dash departure requires a verified descending cliff strip over one flat floor; existing falling and landing path; no steep push");
     else if(options.gred_slope_transition_dash)gf::log_info(context,"source-ground","ordered dash between flat ground and shallow inclines; exact shared edge, continuous height and native step/contact limits");
    else if(options.gred_slope_crease_dash)gf::log_info(context,"source-ground","ordered slope dash across verified shallow creases; exact shared edge, continuous height and native step/contact limits");
    else if(options.gred_slope_seam_dash)gf::log_info(context,"source-ground","ordered slope dash across exact coplanar edges; native step/contact limits; creases, gaps and nonflat ledges remain refused");
    else if(options.gred_slope_dash)gf::log_info(context,"source-ground","ordered ground dash on one selected slope triangle; paired tangent and height, native step/contact limits; nonflat seams and ledges remain refused");
    else if(options.source_ground_windup_entry_push)gf::log_info(context,"source-ground","original movement programs/motion; native body/terrain proxies and target-center approximation; one combined controller with descending zero698 wind-up entry push; no original whole-frame equivalence");
    else if(options.source_ground_relaunch_entry_push)gf::log_info(context,"source-ground","original movement programs/motion; native body/terrain proxies and target-center approximation; one combined controller with descending zero698 relaunch-entry push; no original whole-frame equivalence");
    else if(options.source_ground_relaunch_push)gf::log_info(context,"source-ground","original movement programs/motion; native body/terrain proxies and target-center approximation; one combined controller with continued zero698 relaunch push; no original whole-frame equivalence");
    else if(options.source_ground_active_push)gf::log_info(context,"source-ground","original movement programs/motion; native body/terrain proxies and target-center approximation; one combined controller with continued zero698 active push; no original whole-frame equivalence");
    else if(options.source_ground_windup_push)gf::log_info(context,"source-ground","original movement programs/motion; native body/terrain proxies and target-center approximation; one combined controller with continued zero698 wind-up push; no original whole-frame equivalence");
    else if(options.source_ground_recovery_push)gf::log_info(context,"source-ground","original movement programs/motion; native body/terrain proxies and target-center approximation; one combined controller with airborne zero698 recovery push; no original whole-frame equivalence");
    else if(options.source_ground_relaunch_fall_push)gf::log_info(context,"source-ground","original movement programs/motion; native body/terrain proxies and target-center approximation; one combined controller with released-relaunch zero698 falling push; no original whole-frame equivalence");
    else if(options.source_ground_air_fall_push)gf::log_info(context,"source-ground","original movement programs/motion; native body/terrain proxies and target-center approximation; one combined controller with post-air zero698 falling push; no original whole-frame equivalence");
    else
    if(options.demo_source_ground)gf::log_info(context,"source-ground","original movement programs/motion; native body/terrain proxies and target-center approximation; %s; no original whole-frame equivalence",options.source_ground_fall_push?"one combined controller with ordinary zero698 falling push":options.source_ground_loss_relaunch?"one combined controller with owned698 ground-loss relaunch":options.source_ground_loss_air?"one combined controller with owned698 air dash/contact":options.source_ground_loss_landing?"one combined controller with retained-timer landing jump/dash":options.source_ground_loss_jump?"one combined controller with retained-timer landing jump":options.source_ground_loss_surface?"one combined controller with timer-bound ground-loss surface landing":options.source_ground_surface?"one combined controller with paired surface landing":options.source_ground_windup_entry_floor?"one ground/air/relaunch/entry-floor controller":options.source_ground_relaunch_floor?"one ground/air/relaunch-floor controller":options.source_ground_relaunch?"one ground/air/relaunch controller":options.source_ground_air_floor?"one ground/air controller with scoped floor response":options.source_ground_air?"one ground/ledge/jump/landing/air-dash controller":options.source_ground_landing?"one ground/ledge/jump/landing controller":options.source_ground_jump?"one ground/ledge/jump controller":options.source_walk_ledge?"walk-only ground loss and owned native frame history":options.source_moving_jump?"ground plus moving jump/air steering":options.source_vertical_jump?"ground plus neutral vertical jump":options.source_ground_walk?"ground walk/turn/stop/dash only":"scripted dash only, no source walk/jump/combat");
    if(options.source_ground_walk){
        view.source_ground_controls(options.source_ground_interactive);
        if(options.gred_practice){
            view.source_practice_controls();
            gf::log_info(context,"source-ground","G-Red movement practice: arrows walk/steer, WASD flick dash, Shift jump/rise, release and press Shift again to relaunch; Enter pause, P step while paused, C camera, Q restart, Esc close; native proxies, no combat");
            if(options.gred_practice_start){
                const char* hint=options.gred_walk_ledge_expiry_push?"LEFT ARROW THEN RELEASE":
                    options.gred_walk_ledge_fall_push?"RIGHT ARROW THEN RELEASE":
                    options.gred_inclined_walk_ledge?"UP ARROW THEN RELEASE":
                    options.gred_inclined_dash_ledge||(options.gred_slope_crease_dash&&!options.gred_slope_transition_dash)?"FLICK RIGHT":
                    options.gred_slope_dash?"FLICK BACK":"";
                view.source_practice_start(hint);
                gf::log_info(context,"source-ground","practice start: demo placement, live input, no scripted replay; Q restarts at this start");
            }
        }
        if(options.source_ordered_ground){
            view.source_ordered_ground_controls();
            if(options.gred_slope_dash){
                if(!options.source_ground_interactive)
                     gf::log_info(context,"source-ground","%s",options.gred_walk_ledge_expiry_push?
                         "walk ledge expiry demo: D-pad left, then neutral; faces2009/1999 and a separate later landing":options.gred_walk_ledge_high_angle_push?
                         "walk ledge high-angle demo: D-pad right, then neutral; faces1953/1949 and separate later landing":options.gred_walk_ledge_fall_push?
                         "walk ledge-fall push demo: D-pad right to depart, then neutral; selected face1953 and separate later landing":options.gred_inclined_walk_ledge?
                         "inclined walk ledge demo: D-pad up to depart triangle827 toward flat floor2060, then neutral":options.gred_inclined_dash_ledge?
                         "inclined dash ledge demo: one right flick from triangle827 over the cliff toward flat floor2060":options.gred_slope_transition_dash?
                         "slope transition demo: one backward flick from flat triangle249 onto incline1052":options.gred_slope_crease_dash?
                        "slope crease demo: one right flick across original triangles295/508":options.gred_slope_seam_dash?
                        "slope seam demo: one backward flick from original triangle254 across a certified edge":
                        "slope demo: one backward flick down original triangle1014 from a grounded native placement");
            }else if(options.source_ground_windup_entry_push){
                view.source_ground_windup_entry_push_controls();
                gf::log_info(context,"source-ground","wind-up entry push at zero698: half-speed then .9, no pre-contact motion, gravity step or entry spend; deferred handoff; no rising entry, active entry, ground-loss falling or positive698 push authority");
                if(!options.source_ground_interactive)gf::log_info(context,"source-ground","ground-windup-entry-push demo only: grounded native placement/reference, original stage, wider native bounds; jump5-31, forward flick after the accepted falling push");
            }else if(options.source_ground_relaunch_entry_push){
                view.source_ground_relaunch_entry_push_controls();
                gf::log_info(context,"source-ground","relaunch-entry push requires zero698 and nonpositive velocity after the +15 lift; one 150 meter cost and separate lane requests before release; no rising entry, dash entry, ground-loss or positive698 push authority");
                if(!options.source_ground_interactive)gf::log_info(context,"source-ground","ground-relaunch-entry-push demo only: grounded native placement, unchanged bounds/target/stage; jump5-30, relaunch from78 until push, then release with soft left drift");
            }else if(options.source_ground_relaunch_push){
                view.source_ground_relaunch_push_controls();
                gf::log_info(context,"source-ground","continued relaunch8/program14-15 push at zero698 only; separate latches and meter cost, .9 damping, no entry lift replay; no relaunch entry, dash entry, ground-loss or positive698 push authority");
                if(!options.source_ground_interactive)gf::log_info(context,"source-ground","ground-relaunch-push demo only: grounded native placement, unchanged bounds/target/stage; jump5-32, relaunch from81 until the accepted push");
            }else if(options.source_ground_active_push){
                view.source_ground_active_push_controls();
                gf::log_info(context,"source-ground","continued active dash16/program11 push at zero698 only; exact timer, readiness and meter cost, then .9 damping; no active entry, wind-up entry, active relaunch, ground-loss or positive698 push authority");
                if(!options.source_ground_interactive)gf::log_info(context,"source-ground","ground-active-push demo only: native grounded placements, unchanged stage, wider native bounds; jump5-39, right flick from38 until the accepted active push");
            }else if(options.source_ground_windup_push){
                view.source_ground_windup_push_controls();
                gf::log_info(context,"source-ground","continued wind-up15/program10 push at zero698 only; retains damped velocity, exact timer and handoff cost; no wind-up entry, active dash, active relaunch, ground-loss or positive698 push authority");
                if(!options.source_ground_interactive)gf::log_info(context,"source-ground","ground-windup-push demo only: grounded native mountain placement, original stage, wider native bounds; jump5-32, left flick79-128");
            }else if(options.source_ground_recovery_push){
                view.source_ground_recovery_push_controls();
                gf::log_info(context,"source-ground","airborne recovery17/program12 push at zero698 only; .9 damping before lower completion, separate landing; no active/wind-up dash, active relaunch, ground-loss or positive698 push authority");
                if(!options.source_ground_interactive)gf::log_info(context,"source-ground","ground-recovery-push demo only: native grounded placements 44.9286/-332.8781 and 1200/1200, original stage and bounds; jump5-21, backward flick20-60");
            }else if(options.source_ground_relaunch_fall_push){
                view.source_ground_relaunch_fall_push_controls();
                gf::log_info(context,"source-ground","released-relaunch falling6/program21-24 push at zero698 only; exact inherited scalar, selected plane, .9 tail and separate landing; no active relaunch, air-dash callback, ground-loss or positive698 push authority");
                if(!options.source_ground_interactive)gf::log_info(context,"source-ground","ground-relaunch-fall-push demo only: grounded original-stage placement, unchanged bounds/target/stage; jump5-32, relaunch45-59, then release; no midair teleport/rebind");
            }else if(options.source_ground_air_fall_push){
                view.source_ground_air_fall_push_controls();
                gf::log_info(context,"source-ground","ground-air-fall-push controller enabled: completed recovery receipt before zero698/downward falling6/program13 projection; exact .99 integration then .9 scalar/vertical tail, retained gravity/cache/resource and separate callback/landing8; no ground-loss/relaunch/active-callback/positive698 push, nonflat dash, clipping or combat authority; older policies unchanged");
                if(!options.source_ground_interactive)gf::log_info(context,"source-ground","ground-air-fall-push demo only: declared grounded original-stage placement -585/-3010 and4500 native bounds, unchanged target/stage; jump5-54, forward flick45-71, low left after completed recovery; no midair teleport/rebind");
            }else if(options.source_ground_fall_push){
                view.source_ground_fall_push_controls();
                gf::log_info(context,"source-ground","ground-fall-push controller enabled: ordinary downward falling6/zero698 only, exact pre-response numeric/selected-plane projection with .9 scalar/velocity damping and retained gravity/cache; not landing/refill; ground-loss/post-air/relaunch/dash push, positive698 push, nonflat dash, clipping/unsupported contacts and combat refused; older policies unchanged");
                if(!options.source_ground_interactive)gf::log_info(context,"source-ground","ground-fall-push demo only: declared native grounded original-stage placement at -606.6735/-1373.0029, ordinary native bounds/target and unchanged stage; jump5-32 then neutral, actual push82 and callback83; no midair teleport/rebind");
            }else if(options.source_ground_loss_relaunch){
                view.source_ground_loss_relaunch_controls();
                gf::log_info(context,"source-ground","ground-loss-relaunch controller enabled: retained698/additive15 entry, exact conditional planar clamp and independent14/15 latches; relaunch floor receives actual6144/10240 timer gate and negative-only .8 tail; release21 lineage retains actual scalar; push, nonflat dash, clipping/unsupported contacts and combat refused; older policies unchanged");
                if(!options.source_ground_interactive)gf::log_info(context,"source-ground","ground-loss-relaunch demo only: declared original-stage grounded placement/query reference at -1368.47/-466.667/94.339 and4500 native bounds, unchanged target/stage; down-right5 until falling entry, genuine relaunch press146-595 held through source depletion; no midair teleport/rebind");
            }else if(options.source_ground_loss_air){
                view.source_ground_loss_air_controls();
                gf::log_info(context,"source-ground","ground-loss-air controller enabled: accepted ground-loss history may enter air dash with actual retained698; wind-up/active floor and recovery callback use6144/10240 timer gate, recovery direct drift suppressed while positive; no fake clear/default-zero contact; positive698 relaunch, ground-loss relaunch, push, nonflat dash, clipping/unsupported contacts and combat refused; older policies unchanged");
                if(!options.source_ground_interactive)gf::log_info(context,"source-ground","ground-loss-air demo only: declared original-stage grounded placement/query reference at -1368.47/-628.674/94.339 and4500 native bounds, unchanged target/stage; down-right5 until accepted falling entry, genuine right air flick122-171, then recovery/callback landing; no midair teleport/rebind");
            }else if(options.source_ground_loss_landing){
                view.source_ground_loss_landing_controls();
                gf::log_info(context,"source-ground","ground-loss-landing controller enabled: accepted landing8 may choose target4 jump or target12 owned-root dash with retained698; no timer reset/root fabrication or touchdown skip; positive698 flight/relaunch, ground-loss relaunch, push, nonflat ordered-dash geometry, clipping/unsupported contacts and combat refused; older policies unchanged");
                if(!options.source_ground_interactive)gf::log_info(context,"source-ground","ground-loss-landing demo only: declared original-stage grounded placement/query reference at -1368.47/-466.667/94.339, native arena-half4500, unchanged target/stage; down-right5 until accepted falling entry, genuine backward flick149 after accepted landing8 then release; no midair teleport/rebind");
            }else if(options.source_ground_loss_jump){
                view.source_ground_loss_jump_controls();
                gf::log_info(context,"source-ground","ground-loss-jump controller enabled: accepted landing entry may jump with retained698; every role carries exact native-accepted countdown, no reset on selection/takeoff; later actual ascent spends resource; positive698 dash/air/relaunch, ground-loss relaunch, push, clipping/unsupported contacts and combat refused; older policies unchanged");
                if(!options.source_ground_interactive)gf::log_info(context,"source-ground","ground-loss-jump demo only: declared original-stage grounded placement/query reference and4500 native bounds as ground-loss-surface fixture; down-right5 until accepted falling entry, genuine jump140-154 after accepted landing8; not an original spawn, no midair teleport/rebind");
            }else if(options.source_ground_loss_surface){
                view.source_ground_loss_surface_controls();
                gf::log_info(context,"source-ground","ground-loss-surface controller enabled: actual owned timer698 selects6144/10240 angle gate; paired pre-response plane/integration/history evidence, captured13 touchdown then separate8 landing with continued timer decay; push, ground-loss relaunch, positive698 air/landing input, clipping/unsupported contacts and combat refused; older policies unchanged");
                if(!options.source_ground_interactive)gf::log_info(context,"source-ground","ground-loss-surface demo only: declared original-stage grounded placement/query reference at -1368.47/-628.674/94.339, native arena-half4500, unchanged target/stage; D-pad down-right5 until accepted falling entry then neutral; not an original spawn, no midair teleport/rebind");
            }else if(options.source_ground_surface){
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
        if(!options.gred_slope_dash&&!options.source_ground_windup_entry_push&&!options.source_ground_relaunch_entry_push&&!options.source_ground_relaunch_push&&!options.source_ground_active_push&&!options.source_ground_windup_push&&!options.source_ground_recovery_push&&!options.source_ground_relaunch_fall_push&&!options.source_vertical_jump&&!options.source_ground_jump&&!options.source_ground_landing&&!options.source_ground_air&&!options.source_ground_air_floor&&!options.source_ground_relaunch&&!options.source_ground_relaunch_floor&&!options.source_ground_windup_entry_floor&&!options.source_ground_surface&&!options.source_ground_loss_surface&&!options.source_ground_loss_jump&&!options.source_ground_loss_landing&&!options.source_ground_loss_air&&!options.source_ground_loss_relaunch&&!options.source_ground_fall_push&&!options.source_ground_air_fall_push)gf::log_info(context,"source-ground","%s source walk/turn/stop/dash; arrows/D-pad WALK, stick/WASD FLICK DASH; Enter pause, Q restart; jump/flight/combat refused before simulation; native rate664=1",options.source_ground_interactive?"interactive":"scripted");
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
    bool input_refused=false,practice_stopped=false;
    const bool practice_restartable=options.gred_practice&&options.source_ground_interactive;
    // Y launched from the picker. Its held key is not a second action in the
    // new session; an actual release and re-press still requests restart.
    bool practice_restart_held=picker!=nullptr,practice_pause_held=false,practice_step_held=false,practice_camera_held=false;
    bool practice_close_camera=false;
    gf::pc::GRedPracticeInput practice_input;
    gf::u32 landing_jump_script_tick=0,landing_dash_script_tick=0;
    gf::u32 windup_entry_push_flick_tick=0;
    for(unsigned i=0;i<2;++i){gf::BattleMvpVec low,high;if(view.actor_bounds(i,&low,&high))gf::log_info(context,"battle-mvp","actor=%u ground-y=%.1f model-y=%.1f..%.1f",i,world.actors()[i].position[1],low[1],high[1]);}
    while(context.window().pump_events()&&!context.window().should_close()){
        const auto now=context.clock().monotonic_ns();
        if(options.seconds&&now-(picker?picker->session_started:started)>=gf::u64(options.seconds)*1000000000ull)break;
        context.input().pump();const auto& snapshot=context.input().snapshot();const auto pad=router.route(snapshot,0);if(snapshot.quit||pad.quit_requested)break;
        unsigned steps=1;
        if(!options.headless&&!((options.demo_battle_mvp||options.demo_locomotion||(options.demo_source_ground&&!options.source_ground_interactive))&&options.frames)){
            const auto elapsed=now>=previous?std::min<gf::u64>(now-previous,250000000):0;phase+=elapsed*60;steps=unsigned(std::min<gf::u64>(phase/1000000000ull,8));phase%=1000000000ull;}
        previous=now;
        bool practice_step=false;
        const auto tick_before_step=world.tick();
        if(practice_restartable){
            //These are session controls, not source movement inputs. Observe
            //their edges even when this render frame has no scheduled tick.
            const bool restart=pad.y&&!practice_restart_held;
            const bool pause=pad.start&&!practice_pause_held;
            const bool step=snapshot.frame_step&&!practice_step_held;
            const bool camera=snapshot.camera_toggle&&!practice_camera_held;
            practice_restart_held=pad.y;practice_pause_held=pad.start;
            practice_step_held=snapshot.frame_step;
            practice_camera_held=snapshot.camera_toggle;
            if(camera){
                practice_close_camera=!practice_close_camera;
                view.source_practice_close_camera(practice_close_camera);
                gf::log_info(context,"source-ground","practice camera=%s",practice_close_camera?"close":"standard");
            }
            const bool practicing=!restart&&!pause&&!practice_stopped&&!world.paused();
            practice_input.sample(pad,source.practice_flick_direction(pad),practicing);
            if(restart){
                if(!owned_current()||!world.start(config,&view)||!view.update(world,&error)||!bind_source()){
                    gf::log_error(context,"source-ground","practice restart refused: %s",error.c_str());
                    render.shutdown();return 2;
                }
                practice_stopped=false;input_refused=false;steps=0;phase=0;
                gf::log_info(context,"source-ground","practice restarted at tick=%u",world.tick());
            }else if(pause&&!practice_stopped){
                if(!world.set_paused(!world.paused())){render.shutdown();return 2;}
                input_refused=false;steps=0;phase=0;
                gf::log_info(context,"source-ground","practice %s at tick=%u",world.paused()?"paused":"resumed",world.tick());
            }
            if(step&&!restart&&!pause&&!practice_stopped&&world.paused()){
                //Run the existing source transaction once, then restore pause
                //before presenting it. No buffered input or wall-clock catch-up.
                if(!world.set_paused(false)){render.shutdown();return 2;}
                practice_step=true;steps=1;phase=0;
            }else if(practice_stopped||world.paused())steps=0;
        }
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
                gf::GamePadState controls;
                if(!practice_restartable){controls.start=pad.start;controls.y=pad.y;}
                const bool restart=controls.y&&!source_previous.y;
                gf::NativeGRedGroundInput mapped;
                if(options.source_ground_walk){
                    auto device=pad;
                    if(practice_restartable)practice_input.take(device);
                    if(!options.source_ground_interactive){
                        device={};const unsigned t=world.tick()%(options.source_air_dash?480:options.source_moving_jump?480:options.source_vertical_jump?600:360);
                          if(options.gred_walk_ledge_fall_push){
                              if(world.tick()>=5&&world.tick()<30&&!source.ground_departures()){
                                  device.dpad_right=!options.gred_walk_ledge_expiry_push;
                                  device.dpad_left=options.gred_walk_ledge_expiry_push;
                              }
                          }else if(options.gred_inclined_walk_ledge){
                              device.dpad_up=world.tick()>=5&&world.tick()<20&&!source.ground_departures();
                          }else if(options.gred_slope_dash){
                              if(world.tick()==5){
                                   if(options.gred_inclined_dash_ledge||(options.gred_slope_crease_dash&&!options.gred_slope_transition_dash))device.stick_x=127;
                                  else device.stick_y=127;
                              }
                          }else if(options.source_ground_windup_entry_push){
                             const auto tick=world.tick();device.b=tick>=5&&tick<32;
                             if(source.surface_pushes()&&!windup_entry_push_flick_tick)windup_entry_push_flick_tick=tick;
                             if(windup_entry_push_flick_tick&&tick<windup_entry_push_flick_tick+27)device.stick_y=-127;
                         }else if(options.source_ground_relaunch_entry_push){
                             const auto tick=world.tick();device.b=(tick>=5&&tick<31)||(tick>=78&&!source.relaunch_surface_pushes());
                             if(source.relaunch_entry_surface_pushes()&&source.selector()==8)device.dpad_left=true;
                         }else if(options.source_ground_relaunch_push){
                             const auto tick=world.tick();device.b=(tick>=5&&tick<33)||(tick>=81&&!source.relaunch_surface_pushes());
                         }else if(options.source_ground_active_push){
                             const auto tick=world.tick();device.b=tick>=5&&tick<40;
                             if(tick>=38&&!source.active_pushes())device.stick_x=127;
                         }else if(options.source_ground_windup_push){
                             const auto tick=world.tick();device.b=tick>=5&&tick<33;
                             if(tick>=79&&tick<129)device.stick_x=-127;
                         }else if(options.source_ground_recovery_push){
                             const auto tick=world.tick();device.b=tick>=5&&tick<22;
                             if(tick>=20&&tick<61)device.stick_y=127;
                         }else if(options.source_ground_relaunch_fall_push){
                             const auto tick=world.tick();device.b=(tick>=5&&tick<33)||(tick>=45&&tick<60);
                         }else if(options.source_ground_air_fall_push){
                             const auto tick=world.tick();device.b=tick>=5&&tick<55;
                             if(tick>=45&&tick<72)device.stick_y=-127;
                             if(source.selector()==6&&source.air_dash_starts()&&!source.landings())device.dpad_left=true;
                         }else if(options.source_ground_fall_push){
                             const auto tick=world.tick();device.b=tick>=5&&tick<33;
                         }else if(options.source_ground_loss_relaunch){
                             const auto tick=world.tick();
                             if(tick>=5&&!source.ground_loss_entries()){device.dpad_down=true;device.dpad_right=true;}
                             device.b=tick>=146&&tick<596;
                         }else if(options.source_ground_loss_air){
                             const auto tick=world.tick();
                             if(tick>=5&&!source.ground_loss_entries()){device.dpad_down=true;device.dpad_right=true;}
                             if(tick>=122&&tick<172&&!source.landings())device.stick_x=127;
                         }else if(options.source_ground_loss_landing){
                             const auto tick=world.tick();
                             if(tick>=5&&!source.ground_loss_entries()){device.dpad_down=true;device.dpad_right=true;}
                             if(tick==149)device.stick_y=127;
                         }else if(options.source_ground_loss_jump){
                             const auto tick=world.tick();
                             if(tick>=5&&!source.ground_loss_entries()){device.dpad_down=true;device.dpad_right=true;}
                             device.b=tick>=140&&tick<155;
                         }else if(options.source_ground_loss_surface){
                             if(world.tick()>=5&&!source.ground_loss_entries()){device.dpad_down=true;device.dpad_right=true;}
                         }else if(options.source_ground_surface){
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
                const bool pause=controls.start&&!source_previous.start;
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
                    const auto last_tick=world.tick();error.clear();
                    if(!source.prepare(world,view.source_binding(),movement,x,y,held,&offer,&error)||
                       !world.step({controls,{}},false,&view,nullptr,&movement,&offer)||!source.commit(world,view.source_binding(),&error)){
                        source.reject();
                        gf::log_error(context,"source-ground","motion acceptance refused: tick=%u kind=%u from-y=%.3f proposed-y=%.3f old-v=%.3f; %s",world.tick(),unsigned(offer.kind),offer.observed.position[1],offer.proposed_position[1],offer.observed.vertical_speed,error.c_str());
                        if(!practice_restartable||world.tick()!=last_tick){render.shutdown();return 2;}
                        practice_stopped=true;
                        gf::log_info(context,"source-ground","practice stopped at tick=%u; Q restarts, Esc %s",world.tick(),
                            picker?"returns to collection":"closes");
                        break;
                    }
                }
                source_previous=controls;++commands;continue;
            }
            if(!world.step({input,{}},!options.demo_locomotion,&view,nullptr,&movement)){gf::log_error(context,"battle-mvp","gameplay/terrain step failed");render.shutdown();return 2;}++commands;
        }
        if(practice_step&&!practice_stopped){
            if(!world.set_paused(true)){render.shutdown();return 2;}
            if(world.tick()!=tick_before_step)
                gf::log_info(context,"source-ground","practice stepped to tick=%u",world.tick());
        }
        if(!practice_stopped){
            bool posed=false;
            if(options.demo_source_ground&&world.actors()[0].source_ground_motion){gf::NativeBorgSourcePose pose;posed=source.pose(world,&pose)&&view.update_source(world,pose,&error);}
            else posed=view.update(world,&error);
            if(!posed){gf::log_error(context,"battle-mvp","animation failed: %s",error.c_str());render.shutdown();return 2;}
        }
        if(options.source_vertical_jump)view.source_vertical_jump_controls(source.resource(),options.source_moving_jump,options.source_air_dash,options.source_relaunch,options.source_dash_jump,options.source_landing_interrupt,options.source_surface,options.source_steep,options.source_fall_surface,options.source_recovery_surface,options.source_windup_surface,options.source_active_surface,options.source_relaunch_surface,options.source_relaunch_entry_surface,options.source_relaunch_floor);
        render.begin_frame({frame,.15f,.21f,.3f});render.reset_arena();
        const auto draws=practice_stopped?view.draw_practice_stop(render,options.width,options.height,world.tick()):
            view.draw(render,world,options.width,options.height,&error,practice_restartable&&input_refused);
        render.end_frame();
        if(!draws){gf::log_error(context,"battle-mvp","draw failed: %s",error.c_str());render.shutdown();return 2;}
        if(!captured)captured=capture_if_requested(context,options,frame);
        if(picker&&captured)picker->captured=true;
        render.present();context.advance_frame();++frame;if(options.frames&&frame>=options.frames)break;
    }
    gf::log_info(context,"battle-mvp","frames=%u ticks=%u projectiles-spawned=%u hits=%u health=%u/%u result=%u",frame,world.tick(),world.shots_spawned(),world.hits(),world.actors()[0].health,world.actors()[1].health,unsigned(world.result()));
    if(options.demo_source_ground)gf::log_info(context,"source-ground","accepted native tick=%u G-Red-position=%.2f/%.2f/%.2f; accepted-frames=%llu body-proxy-contacts=%llu; scoped movement, native proxy response",world.tick(),world.actors()[0].position[0],world.actors()[0].position[1],world.actors()[0].position[2],static_cast<unsigned long long>(source.accepted_frames()),static_cast<unsigned long long>(source.body_contacts()));
    if(options.gred_practice){
        if(const auto practice=view.practice_readout(world)){
            const auto meter=practice->meter;
            gf::log_info(context,"source-ground","practice-meter=%d/%d at accepted tick=%u",int(meter.value),int(meter.limit),world.tick());
            gf::log_info(context,"source-ground","practice-phase=%s%s",view.practice_phase(*practice),
                gf::native_borg_motion_push(practice->kind)?" / PUSH":"");
            if(const auto ticks=practice->dash_release_ticks)
                gf::log_info(context,"source-ground","practice-dash-release-ticks=%u",unsigned(*ticks));
            if(const auto ticks=practice->ledge_timer_ticks)
                gf::log_info(context,"source-ground","practice-ledge-timer-ticks=%u cue=%s",
                    unsigned(*ticks),view.practice_ledge_cue(*practice).c_str());
        }
    }
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
    if(options.source_walk_ledge||options.source_dash_ledge||options.source_ground_ledge||options.gred_inclined_dash_ledge||options.gred_inclined_walk_ledge)gf::log_info(context,"source-ground","ground-departures=%llu loss-entries=%llu loss-landings=%llu suppressed-drift=%llu timer698=%.1f cached87=%u flags=%x; owned native history, no original actor-frame equivalence",static_cast<unsigned long long>(source.ground_departures()),static_cast<unsigned long long>(source.ground_loss_entries()),static_cast<unsigned long long>(source.ground_loss_landings()),static_cast<unsigned long long>(source.suppressed_ground_drifts()),source.ground_loss_timer698(),unsigned(source.cached_airborne87()),source.movement_flags());
    if(options.source_ordered_ground)gf::log_info(context,"source-ground","ordered-entries=%llu active=%llu recovery=%llu resets=%llu root-updates=%llu root-speed=%.6f selector=%u flags=%x; scoped post-VM samples, not original actor/frame equivalence",static_cast<unsigned long long>(source.ordered_entries()),static_cast<unsigned long long>(source.ordered_active_frames()),static_cast<unsigned long long>(source.ordered_recovery_frames()),static_cast<unsigned long long>(source.ordered_resets()),static_cast<unsigned long long>(source.ordered_root().updates),source.ordered_root().speed1dac,unsigned(source.selector()),source.movement_flags());
    if(options.source_dash_ledge)gf::log_info(context,"source-ground","dash-loss-air-frames=%llu landings=%llu resource=%d selector=%u; last ordered root sample is historical data after priority handoff, not a fresh airborne root lease",static_cast<unsigned long long>(source.air_frames()),static_cast<unsigned long long>(source.landings()),int(source.resource()),unsigned(source.selector()));
    if(options.source_ground_ledge||options.gred_inclined_walk_ledge)gf::log_info(context,"source-ground","walk-departures=%llu dash-departures=%llu landings=%llu resource=%d selector=%u; one scoped controller/producer, distinct walking and ordered-dash evidence",static_cast<unsigned long long>(source.walk_departures()),static_cast<unsigned long long>(source.dash_departures()),static_cast<unsigned long long>(source.landings()),int(source.resource()),unsigned(source.selector()));
    if(options.gred_walk_ledge_fall_push)gf::log_info(context,"source-ground","walk-ledge-fall-pushes=%llu last-angle-s16=%d landings=%llu resource=%d selector=%u",
        static_cast<unsigned long long>(source.walk_ledge_pushes()),int(source.last_push_angle()),
        static_cast<unsigned long long>(source.landings()),int(source.resource()),unsigned(source.selector()));
    if(options.source_ground_jump)gf::log_info(context,"source-ground","ground-jump-air-frames=%llu landings=%llu resource=%d dash-jumps=%llu last-interrupted-selector=%u programs=%u/%u cached87=%u; source subset, no extra ordered root advance on takeoff",static_cast<unsigned long long>(source.air_frames()),static_cast<unsigned long long>(source.landings()),int(source.resource()),static_cast<unsigned long long>(source.dash_jumps()),unsigned(source.last_dash_jump_selector()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    if(options.source_ground_landing)gf::log_info(context,"source-ground","landing-jumps=%llu landing-dashes=%llu landings=%llu resource=%d programs=%u/%u cached87=%u; accepted landing entry before interruption, no touchdown skip or forced refill",static_cast<unsigned long long>(source.landing_jumps()),static_cast<unsigned long long>(source.landing_dashes()),static_cast<unsigned long long>(source.landings()),int(source.resource()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    if(options.source_ground_air)gf::log_info(context,"source-ground","controller-air-starts=%llu active=%llu landings=%llu relaunches=%llu resource=%d selector=%u programs=%u/%u cached87=%u; no floor/push/relaunch authority",static_cast<unsigned long long>(source.air_dash_starts()),static_cast<unsigned long long>(source.air_dash_active_frames()),static_cast<unsigned long long>(source.landings()),static_cast<unsigned long long>(source.relaunches()),int(source.resource()),unsigned(source.selector()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    if(options.source_ground_air_floor)gf::log_info(context,"source-ground","controller-floor-starts=%llu windup=%llu active=%llu entry=%llu positive-entry=%llu landings=%llu relaunches=%llu resource=%d selector=%u programs=%u/%u cached87=%u; no entry-floor/push/relaunch authority",static_cast<unsigned long long>(source.air_dash_starts()),static_cast<unsigned long long>(source.windup_floor_contacts()),static_cast<unsigned long long>(source.active_floor_contacts()),static_cast<unsigned long long>(source.windup_entry_floor_contacts()),static_cast<unsigned long long>(source.positive_windup_floor_contacts()),static_cast<unsigned long long>(source.landings()),static_cast<unsigned long long>(source.relaunches()),int(source.resource()),unsigned(source.selector()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    if(options.source_ground_relaunch)gf::log_info(context,"source-ground","controller-relaunches=%llu lower=%u upper=%u air=%llu floor=%llu pushes=%llu landings=%llu resource=%d selector=%u programs=%u/%u cached87=%u; no relaunch-floor/entry-floor/push authority",static_cast<unsigned long long>(source.relaunches()),unsigned(source.relaunch_stages().lower_stage),unsigned(source.relaunch_stages().upper_stage),static_cast<unsigned long long>(source.air_dash_starts()),static_cast<unsigned long long>(source.relaunch_floor_contacts()),static_cast<unsigned long long>(source.surface_pushes()),static_cast<unsigned long long>(source.landings()),int(source.resource()),unsigned(source.selector()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    if(options.source_ground_relaunch_floor)gf::log_info(context,"source-ground","controller-relaunch-floor=%llu relaunches=%llu lower=%u upper=%u pushes=%llu entry=%llu positive-entry=%llu landings=%llu resource=%d selector=%u programs=%u/%u cached87=%u; no windup-entry-floor/push authority",static_cast<unsigned long long>(source.relaunch_floor_contacts()),static_cast<unsigned long long>(source.relaunches()),unsigned(source.relaunch_stages().lower_stage),unsigned(source.relaunch_stages().upper_stage),static_cast<unsigned long long>(source.surface_pushes()),static_cast<unsigned long long>(source.windup_entry_floor_contacts()),static_cast<unsigned long long>(source.positive_windup_floor_contacts()),static_cast<unsigned long long>(source.landings()),int(source.resource()),unsigned(source.selector()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    if(options.source_ground_windup_entry_floor)gf::log_info(context,"source-ground","controller-windup-entry=%llu positive=%llu windup=%llu active=%llu relaunch-floor=%llu relaunches=%llu pushes=%llu landings=%llu resource=%d selector=%u programs=%u/%u cached87=%u; no push/positive698/combat authority",static_cast<unsigned long long>(source.windup_entry_floor_contacts()),static_cast<unsigned long long>(source.positive_windup_floor_contacts()),static_cast<unsigned long long>(source.windup_floor_contacts()),static_cast<unsigned long long>(source.active_floor_contacts()),static_cast<unsigned long long>(source.relaunch_floor_contacts()),static_cast<unsigned long long>(source.relaunches()),static_cast<unsigned long long>(source.surface_pushes()),static_cast<unsigned long long>(source.landings()),int(source.resource()),unsigned(source.selector()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    if(options.source_ground_surface)gf::log_info(context,"source-ground","controller-surface-landings=%llu angle-s16=%d air=%llu relaunches=%llu pushes=%llu landings=%llu resource=%d selector=%u programs=%u/%u cached87=%u; no push/positive698/combat authority",static_cast<unsigned long long>(source.surface_landings()),int(source.last_surface_angle()),static_cast<unsigned long long>(source.air_dash_starts()),static_cast<unsigned long long>(source.relaunches()),static_cast<unsigned long long>(source.surface_pushes()),static_cast<unsigned long long>(source.landings()),int(source.resource()),unsigned(source.selector()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    if(options.source_ground_loss_surface)gf::log_info(context,"source-ground","controller-loss-landings=%llu surface-callbacks=%llu departures=%llu entries=%llu timer698=%.1f angle-s16=%d landings=%llu resource=%d selector=%u programs=%u/%u cached87=%u; no push/positive698-air-or-landing-input/combat authority",static_cast<unsigned long long>(source.ground_loss_landings()),static_cast<unsigned long long>(source.surface_landings()),static_cast<unsigned long long>(source.ground_departures()),static_cast<unsigned long long>(source.ground_loss_entries()),double(source.ground_loss_timer698()),int(source.last_surface_angle()),static_cast<unsigned long long>(source.landings()),int(source.resource()),unsigned(source.selector()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    if(options.source_ground_loss_jump)gf::log_info(context,"source-ground","controller-loss-jumps=%llu loss-landings=%llu surface-callbacks=%llu timer698=%.1f landings=%llu resource=%d selector=%u programs=%u/%u cached87=%u; no positive698-dash-or-flight/push/combat authority",static_cast<unsigned long long>(source.landing_jumps()),static_cast<unsigned long long>(source.ground_loss_landings()),static_cast<unsigned long long>(source.surface_landings()),double(source.ground_loss_timer698()),static_cast<unsigned long long>(source.landings()),int(source.resource()),unsigned(source.selector()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    if(options.source_ground_loss_landing)gf::log_info(context,"source-ground","controller-loss-input-jumps=%llu dashes=%llu loss-landings=%llu timer698=%.1f landings=%llu resource=%d selector=%u programs=%u/%u cached87=%u; no positive698-flight/push/nonflat-dash/combat authority",static_cast<unsigned long long>(source.landing_jumps()),static_cast<unsigned long long>(source.landing_dashes()),static_cast<unsigned long long>(source.ground_loss_landings()),double(source.ground_loss_timer698()),static_cast<unsigned long long>(source.landings()),int(source.resource()),unsigned(source.selector()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    if(options.source_ground_loss_air)gf::log_info(context,"source-ground","controller-loss-air-starts=%llu timer698=%.1f windup-floor=%llu active-floor=%llu landings=%llu loss-landings=%llu resource=%d selector=%u programs=%u/%u cached87=%u; no positive698-relaunch/push/nonflat-dash/combat authority",static_cast<unsigned long long>(source.air_dash_starts()),double(source.ground_loss_timer698()),static_cast<unsigned long long>(source.windup_floor_contacts()),static_cast<unsigned long long>(source.active_floor_contacts()),static_cast<unsigned long long>(source.landings()),static_cast<unsigned long long>(source.ground_loss_landings()),int(source.resource()),unsigned(source.selector()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    if(options.source_ground_loss_relaunch)gf::log_info(context,"source-ground","controller-loss-relaunches=%llu timer698=%.1f floors=%llu lower=%u upper=%u landings=%llu loss-landings=%llu resource=%d selector=%u programs=%u/%u cached87=%u; no push/nonflat-dash/combat authority",static_cast<unsigned long long>(source.relaunches()),double(source.ground_loss_timer698()),static_cast<unsigned long long>(source.relaunch_floor_contacts()),unsigned(source.relaunch_stages().lower_stage),unsigned(source.relaunch_stages().upper_stage),static_cast<unsigned long long>(source.landings()),static_cast<unsigned long long>(source.ground_loss_landings()),int(source.resource()),unsigned(source.selector()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    if(options.source_ground_fall_push)gf::log_info(context,"source-ground","controller-fall-pushes=%llu angle-s16=%d timer698=%.1f landings=%llu resource=%d selector=%u programs=%u/%u cached87=%u; no ground-loss/post-air/relaunch/dash/positive698 push or combat authority",static_cast<unsigned long long>(source.surface_pushes()),int(source.last_push_angle()),double(source.ground_loss_timer698()),static_cast<unsigned long long>(source.landings()),int(source.resource()),unsigned(source.selector()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    if(options.source_ground_air_fall_push)gf::log_info(context,"source-ground","controller-air-fall-pushes=%llu total-pushes=%llu starts=%llu angle-s16=%d timer698=%.1f landings=%llu resource=%d selector=%u programs=%u/%u cached87=%u; no ground-loss/relaunch/active-callback/positive698 push or combat authority",static_cast<unsigned long long>(source.dash_fall_pushes()),static_cast<unsigned long long>(source.surface_pushes()),static_cast<unsigned long long>(source.air_dash_starts()),int(source.last_push_angle()),double(source.ground_loss_timer698()),static_cast<unsigned long long>(source.landings()),int(source.resource()),unsigned(source.selector()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    if(options.source_ground_relaunch_fall_push)gf::log_info(context,"source-ground","controller-relaunch-fall-pushes=%llu relaunches=%llu angle-s16=%d timer698=%.1f landings=%llu resource=%d selector=%u programs=%u/%u cached87=%u",static_cast<unsigned long long>(source.relaunch_fall_pushes()),static_cast<unsigned long long>(source.relaunches()),int(source.last_push_angle()),double(source.ground_loss_timer698()),static_cast<unsigned long long>(source.landings()),int(source.resource()),unsigned(source.selector()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    if(options.source_ground_recovery_push)gf::log_info(context,"source-ground","controller-recovery-pushes=%llu total-pushes=%llu starts=%llu angle-s16=%d timer698=%.1f landings=%llu resource=%d selector=%u programs=%u/%u cached87=%u",static_cast<unsigned long long>(source.recovery_pushes()),static_cast<unsigned long long>(source.surface_pushes()),static_cast<unsigned long long>(source.air_dash_starts()),int(source.last_push_angle()),double(source.ground_loss_timer698()),static_cast<unsigned long long>(source.landings()),int(source.resource()),unsigned(source.selector()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    if(options.source_ground_windup_push||options.source_ground_windup_entry_push)gf::log_info(context,"source-ground","controller-windup-pushes=%llu entry-pushes=%llu starts=%llu landings=%llu resource=%d selector=%u programs=%u/%u cached87=%u",static_cast<unsigned long long>(source.windup_pushes()),static_cast<unsigned long long>(source.windup_entry_surface_pushes()),static_cast<unsigned long long>(source.air_dash_starts()),static_cast<unsigned long long>(source.landings()),int(source.resource()),unsigned(source.selector()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    if(options.source_ground_active_push)gf::log_info(context,"source-ground","controller-active-pushes=%llu recovery-pushes=%llu starts=%llu landings=%llu resource=%d selector=%u programs=%u/%u cached87=%u",static_cast<unsigned long long>(source.active_pushes()),static_cast<unsigned long long>(source.recovery_pushes()),static_cast<unsigned long long>(source.air_dash_starts()),static_cast<unsigned long long>(source.landings()),int(source.resource()),unsigned(source.selector()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    if(options.source_ground_relaunch_push||options.source_ground_relaunch_entry_push)gf::log_info(context,"source-ground","controller-relaunch-pushes=%llu entry-pushes=%llu relaunches=%llu landings=%llu resource=%d selector=%u programs=%u/%u cached87=%u",static_cast<unsigned long long>(source.relaunch_surface_pushes()),static_cast<unsigned long long>(source.relaunch_entry_surface_pushes()),static_cast<unsigned long long>(source.relaunches()),static_cast<unsigned long long>(source.landings()),int(source.resource()),unsigned(source.selector()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    render.shutdown();return 0;
}

void print_packaged_force_slot(const gf::PackageForceSlotView& row,const char* prefix){
    constexpr const char* status[]{"metadata-ready","scan-unavailable","package-missing","package-incomplete","conflict",
        "variant-unspecified","catalogue-unavailable","variant-missing"};
    std::printf("%sslot=%u\tinstance=%llu\t%s\t%s\tvariant=%s",prefix,
        unsigned(row.slot)+1,static_cast<unsigned long long>(row.saved.instance),row.saved.identity.id.c_str(),
        row.saved.identity.name.c_str(),row.saved.variant?row.saved.variant->id.c_str():"unspecified");
    if(row.saved.variant)std::printf("\t%s",row.saved.variant->label.c_str());
    std::printf("\tstatus=%s\tdisabled=%u\n",status[unsigned(row.status)],unsigned(row.saved.disabled));
}
bool load_retail_force_catalogue(gf::IFileSystem& files,const std::string& path,gf::ForceCatalog* catalogue){
    gf::u64 size=0;std::vector<gf::u8> bytes;
    return catalogue&&files.file_size(path,&size)&&size&&size<=(128u<<10)&&
        files.read_range(path,0,std::size_t(size),bytes)&&bytes.size()==size&&catalogue->load({bytes.data(),bytes.size()},nullptr);
}
int inspect_borg_force(const Options& options,const gf::BorgPackageCatalog& packages){
    std::string error;
    auto selected=gf::pc::open_borg_selected_save(options.selected_save_root,options.selected_account,options.selected_save,&error);
    if(!selected){std::fprintf(stderr,"Borg Force: %s\n",error.c_str());return 2;}
    gf::PackageBorgSaveStore store(*selected->backend);gf::PackageBorgRoster roster;
    const auto loaded=store.load(&roster);
    if(loaded==gf::PackageBorgLoad::Missing&&options.retail_force_catalogue.empty()){
        std::puts("This save has no package roster.");return 0;
    }
    if(loaded!=gf::PackageBorgLoad::Loaded&&loaded!=gf::PackageBorgLoad::Missing){
        std::fputs("Borg Force: package roster unavailable or invalid\n",stderr);return 2;
    }
    const auto metadata=gf::pc::load_borg_owned_metadata("borgs",packages,roster);
    gf::PackageForceView view;
    const auto index=unsigned(options.inspect_force-1);
    if(!gf::package_borg_force_view(roster,packages,metadata.snapshots,index,&view)||!store.current()){
        std::fputs("Borg Force: roster changed or could not be inspected\n",stderr);return 2;
    }
    if(!options.retail_force_catalogue.empty()){
        auto files=gf::sdl::create_file_system();gf::ForceCatalog retail_catalogue;
        if(!files||!load_retail_force_catalogue(*files,options.retail_force_catalogue,&retail_catalogue)){
            std::fputs("Borg Force: could not read the explicit retail catalogue\n",stderr);return 2;
        }
        gf::ForceInventory retail;gf::ForceInventoryStore inventory(*selected->backend);
        const auto original=inventory.load(retail_catalogue,&retail);
        if(original!=gf::ForceInventoryLoad::Loaded&&original!=gf::ForceInventoryLoad::Missing){
            std::fputs("Borg Force: retail record unavailable, invalid or mismatched to the explicit catalogue\n",stderr);return 2;
        }
        gf::MixedBorgForceView mixed;
        if(!gf::borg_mixed_force_view(view,original==gf::ForceInventoryLoad::Loaded?&retail:nullptr,&mixed)||!store.current()){
            std::fputs("Borg Force: selected save changed or views could not be joined\n",stderr);return 2;
        }
        std::printf("Force %u: slots=%zu retail-record=%s overlaps=%u declared-combined-cost=",mixed.force+1,mixed.slots.size(),
            mixed.retail_record_present?"present":"absent",mixed.overlaps);
        if(mixed.declared_cost)std::printf("%u\n",unsigned(*mixed.declared_cost));
        else std::puts("unavailable");
        for(const auto& row:mixed.slots){
            if(row.overlap())std::printf("slot=%u\tOVERLAP: both retail and package claim this position\n",unsigned(row.slot)+1);
            if(row.retail){
                const auto& old=*row.retail;
                std::printf("retail\tslot=%u\tbox=%u\tmodel=%04X\tcollection=%u\tvariant-index=%u\tedited=%u\tcost=%u",
                    unsigned(row.slot)+1,unsigned(old.box_slot)+1,unsigned(gf::u16(old.value.model)),
                    unsigned(old.collection_number)+1,unsigned(old.value.variant),unsigned(old.value.edits.modified),unsigned(old.declared_cost));
                if(gf::display_name_valid(old.source_name,96))std::printf("\t%s",old.source_name.c_str());
                std::putchar('\n');
            }
            if(row.packaged)print_packaged_force_slot(*row.packaged,"package\t");
        }
        std::puts("Read-only joined Force; no conflict resolved, mode budget checked, or save changed.");
        return 0;
    }
    std::printf("Force %u: packaged-slots=%zu selectable-metadata=%u declared-package-cost=",
        view.force+1,view.slots.size(),view.selectable);
    if(view.declared_package_cost)std::printf("%u\n",unsigned(*view.declared_package_cost));
    else std::puts("unavailable");
    for(const auto& row:view.slots)print_packaged_force_slot(row,"");
    std::puts("Package projection only; no retail slots, battle admission, or save changes.");
    return 0;
}

int run_borg_recovery(gf::Context& context,gf::RenderEngine& render,const Options& options,const gf::BorgPackageCatalog& packages){
    std::string error;
    auto fail=[&](const std::string& why){gf::log_error(context,"borg-recovery","%s",why.c_str());render.shutdown();return 2;};
    auto selected=gf::pc::open_borg_selected_save(options.selected_save_root,options.selected_account,options.selected_save,&error);
    if(!selected)return fail(error);
    const auto& save=selected->entry;
    gf::pc::BorgOwnedMetadata metadata;
    if(packages.status==gf::BorgPackageScanStatus::Complete){
        // Read only to find which variant catalogues are needed. The recovery
        // session takes its own checked roster snapshot before any confirmation.
        gf::PackageBorgSaveStore inspector(*selected->backend);gf::PackageBorgRoster roster;
        if(inspector.load(&roster)==gf::PackageBorgLoad::Loaded)metadata=gf::pc::load_borg_owned_metadata("borgs",packages,roster);
    }
    gf::BorgPackageRecoverySession session(*selected->backend);gf::BorgRecoveryMenu menu;
    gf::BorgRecoveryMenuView view;gf::InputRouter router;
    std::string notice_status;
    const auto opened=session.open(packages,metadata.snapshots);
    switch(opened){
    case gf::BorgRecoveryOpen::Ready:
        if(metadata.unavailable)notice_status="SOME VARIANT CATALOGUES COULD NOT BE CHECKED";
        break;
    case gf::BorgRecoveryOpen::NoRoster:notice_status="THIS SAVE HAS NO PACKAGE ROSTER";break;
    case gf::BorgRecoveryOpen::ScanUnavailable:notice_status="PACKAGE SCAN UNAVAILABLE - NO RECOVERY AUTHORIZED";break;
    case gf::BorgRecoveryOpen::SaveUnavailable:notice_status="PACKAGE ROSTER COULD NOT BE READ";break;
    case gf::BorgRecoveryOpen::InvalidSave:notice_status="PACKAGE ROSTER INVALID - NO RECOVERY AUTHORIZED";break;
    }
    if(options.recovery_notice&&opened==gf::BorgRecoveryOpen::Ready&&!session.inspect(options.recovery_notice))
        notice_status="REQUESTED NOTICE UNAVAILABLE";
    gf::log_info(context,"borg-recovery","account=%llu save=%llu notices=%zu headless-read-only=%u",
        static_cast<unsigned long long>(save.key.account),static_cast<unsigned long long>(save.key.save),
        session.notices().size(),unsigned(options.headless));
    if(const auto* notice=session.selected())gf::log_info(context,"borg-recovery","notice=%llu name=%s affected-forces=%zu",
        static_cast<unsigned long long>(notice->saved.instance),notice->saved.identity.name.c_str(),notice->affected.size());
    if(!notice_status.empty())gf::log_info(context,"borg-recovery","%s",notice_status.c_str());
    unsigned frame=0;bool captured=false;const auto start=context.clock().monotonic_ns();
    while(context.window().pump_events()&&!context.window().should_close()){
        const auto now=context.clock().monotonic_ns();
        if(options.seconds&&now-start>=gf::u64(options.seconds)*1000000000ull)break;
        context.input().pump();const auto& input=context.input().snapshot();
        const auto pad=router.route(input,0);if(input.quit||pad.quit_requested)break;
        const auto request=menu.step(pad,session);
        if(request==gf::BorgRecoveryMenuRequest::Exit)break;
        if(request==gf::BorgRecoveryMenuRequest::Confirm){
            if(options.headless){session.cancel();notice_status="HEADLESS INSPECTION DOES NOT WRITE";}
            else{
                const auto current=gf::pc::scan_borg_packages("borgs");
                const auto checked=gf::pc::load_borg_owned_metadata("borgs",current,*session.roster());
                switch(session.confirm(current,checked.snapshots)){
                case gf::BorgRecoveryApply::Saved:notice_status="CHANGE SAVED";break;
                case gf::BorgRecoveryApply::Unchanged:notice_status="NO CHANGE NEEDED";break;
                case gf::BorgRecoveryApply::StaleNotice:notice_status="NOTICE CHANGED - NO WRITE ATTEMPTED";break;
                case gf::BorgRecoveryApply::SaveUnconfirmed:notice_status="SAVE RESULT UNCONFIRMED - REOPEN TO REVIEW";break;
                case gf::BorgRecoveryApply::NotConfirmed:notice_status="NO ACTION CONFIRMED";break;
                }
            }
        }
        render.begin_frame({frame,.055f,.065f,.085f});render.reset_arena();
        const auto draws=view.draw(render,options.width,options.height,session,menu,
            "SAVE: "+save.name,notice_status,options.headless);
        render.end_frame();
        if(!captured)captured=capture_if_requested(context,options,frame);
        render.present();context.advance_frame();++frame;
        if(!draws)return fail("recovery viewport unavailable");
        if(options.frames&&frame>=options.frames)break;
    }
    render.shutdown();return 0;
}

const char* borg_placement_label(gf::BorgPlacementResult result){
    using Result=gf::BorgPlacementResult;
    switch(result){
    case Result::Ready:return "PLACEMENT READY TO CONFIRM";
    case Result::Saved:return "PLACEMENT SAVED";
    case Result::Invalid:return "INVALID FORCE POSITION";
    case Result::OwnedUnavailable:return "OWNED COPY UNAVAILABLE";
    case Result::Disabled:return "OWNED COPY DISABLED";
    case Result::VariantUnavailable:return "EXACT VARIANT UNAVAILABLE";
    case Result::AlreadyMember:return "THIS COPY IS ALREADY IN THIS FORCE";
    case Result::Occupied:return "TARGET FORCE SLOT IS OCCUPIED";
    case Result::RetailUnverified:return "RETAIL RECORD UNVERIFIED - CHECK RETAIL CATALOGUE";
    case Result::SaveUnavailable:return "SELECTED SAVE UNAVAILABLE";
    case Result::NotConfirmed:return "PLACEMENT NOT CONFIRMED";
    case Result::Stale:return "PLACEMENT CHANGED - NO WRITE";
    case Result::SaveUnconfirmed:return "SAVE RESULT UNCONFIRMED";
    }
    return "PLACEMENT REFUSED";
}

int run_borg_browser(gf::Context& context,gf::RenderEngine& render,const Options& options,const gf::BorgPackageCatalog& packages){
    gf::BorgInstalledBrowser browser;
    std::unique_ptr<gf::pc::BorgSelectedSave> owned_save;std::string error;
    bool renderer_active=true;
    auto fail=[&](const std::string& why){
        gf::log_error(context,"borg-picker","%s",why.c_str());if(renderer_active)render.shutdown();return 2;
    };
    if(options.browse_owned_borgs){
        owned_save=gf::pc::open_borg_selected_save(options.selected_save_root,options.selected_account,options.selected_save,&error);
        if(!owned_save)return fail(error);
        gf::PackageBorgSaveStore reader(*owned_save->backend);gf::PackageBorgRoster roster;
        const auto loaded=reader.load(&roster);
        if(loaded!=gf::PackageBorgLoad::Loaded&&loaded!=gf::PackageBorgLoad::Missing)
            return fail("package roster unavailable or invalid");
        if(!browser.open_owned(packages,std::move(roster),options.owned_instance))
            return fail("requested owned instance unavailable");
        if(!reader.current())return fail("selected save changed while opening the collection");
    }else browser.open(packages);
    std::optional<gf::BorgPackagePlacementSession> placement;
    gf::BorgPlacementConfirmation placement_input;
    gf::BorgPlacementPanel placement_panel;
    if(options.placement_force){
        placement.emplace(*owned_save->backend);
        placement_panel.force=unsigned(options.placement_force-1);
        placement_panel.slot=unsigned(options.placement_slot-1);
        placement_panel.headless=options.headless;
    }
    auto set_placement_status=[&](std::string_view status,bool locked=false){
        placement_panel.status=status;placement_panel.locked=locked;
        gf::log_info(context,"borg-placement","%.*s",int(status.size()),status.data());
    };
    auto read_retail=[&](gf::ForceCatalog* retail){
        return options.retail_force_catalogue.empty()||load_retail_force_catalogue(context.files(),options.retail_force_catalogue,retail);
    };
    gf::BorgInstalledBrowserView view;gf::BorgBrowserTile tile;gf::InputRouter router;
    gf::NativeAnimationMosaic preview;
    gf::BorgBrowserPracticePanel practice_panel;
    constexpr gf::u32 tile_handle=59001; // single replaceable texture in this isolated presentation mode
    bool confirmed=false,captured=false,preview_open=false,preview_unavailable=false,previous_back=false;
    bool wait_for_exit_release=false;
    auto refresh_tile=[&](){
        tile={};
        const auto* variant=browser.variant();
        if(!variant)return;
        if(variant->placeholder_icon()){tile.status=gf::BorgBrowserTileStatus::Placeholder;return;}
        if(variant->unresolved_icon()){tile.status=gf::BorgBrowserTileStatus::Unresolved;return;}
        std::optional<gf::TplImage> image;
        if(gf::pc::load_borg_variant_icon("borgs",*browser.metadata(),variant->id,&image,nullptr)&&image&&
            render.upload_texture(tile_handle,{image->width,image->height,image->rgba.data(),0,0,false}))
            tile={gf::BorgBrowserTileStatus::Image,tile_handle,image->width,image->height};
    };
    auto refresh_package=[&](){
        if(const auto* entry=browser.entry()){
            gf::BorgCatalogSnapshot snapshot;
            if(gf::pc::load_borg_catalog_snapshot("borgs",browser.catalog(),entry->manifest.identity.id,&snapshot,nullptr))
                browser.bind(std::move(snapshot));
        }
        refresh_tile();
    };
    refresh_package();
    unsigned frame=0;const auto start=context.clock().monotonic_ns();auto previous=start;
    while(context.window().pump_events()&&!context.window().should_close()){
        const auto now=context.clock().monotonic_ns();
        if(options.seconds&&now-start>=gf::u64(options.seconds)*1000000000ull)break;
        context.input().pump();const auto& input=context.input().snapshot();
        auto pad=router.route(input,0);
        const bool quit=input.quit||pad.quit_requested;
        if(quit&&!wait_for_exit_release)break;
        const bool suppress_return_input=wait_for_exit_release;
        if(suppress_return_input){
            browser.sync_input(pad);
            wait_for_exit_release=quit;
        }
        if(options.demo_borg_browser){
            pad={};pad.dpad_right=frame==1||frame==7;pad.dpad_down=frame==3;pad.dpad_up=frame==5;pad.a=frame==9;
            pad.x=frame==11||frame==17||frame==23;pad.b=frame==19||frame==25;
            pad.dpad_down|=frame==15;pad.dpad_up|=frame==21;
        }
        const bool back=pad.b&&!previous_back;previous_back=pad.b;
        const gf::f32 dt=(options.headless||options.frames)?1.f/60:gf::f32(now>=previous?now-previous:0)/1e9f;previous=now;
        if(suppress_return_input){
            // Keep drawing the restored selection while the exit key is
            // held. No return-frame input can change or launch that copy.
        }else if(placement&&placement->offer()){
            const auto request=placement_input.step(pad);
            if(request==gf::BorgPlacementRequest::Cancel||(options.headless&&request==gf::BorgPlacementRequest::Confirm)){
                placement->cancel();browser.sync_input(pad);
                set_placement_status(options.headless&&request==gf::BorgPlacementRequest::Confirm?
                    "HEADLESS INSPECTION DOES NOT WRITE":"PLACEMENT CANCELLED - NO WRITE");
            }else if(request==gf::BorgPlacementRequest::Confirm){
                const auto id=placement->offer()->owned.identity.id;
                const auto current=gf::pc::scan_borg_packages("borgs");
                gf::BorgCatalogSnapshot snapshot;gf::ForceCatalog retail;
                using Result=gf::BorgPlacementResult;
                auto result=Result::Stale;
                if(read_retail(&retail)&&gf::pc::load_borg_catalog_snapshot("borgs",current,id,&snapshot,nullptr))
                    result=placement->confirm(retail.ready()?&retail:nullptr,current,{&snapshot,1});
                else placement->cancel();
                browser.sync_input(pad);
                set_placement_status(borg_placement_label(result),true);
            }
        }else if(placement_panel.locked){
            // Acknowledged result or stale/uncertain evidence: reopen rather
            // than reusing the old picker snapshot for another write.
        }else if(preview_open){
            if(back||!preview.step(pad,dt,render,&error)){
                preview_unavailable=!back;
                if(!preview.close_package_preview(render)){
                    gf::log_error(context,"borg-picker","could not release preview textures");render.shutdown();return 2;
                }
                browser.sync_input(pad);preview_open=false;
                if(back)gf::log_info(context,"borg-picker","returned from body preview");
                else gf::log_warn(context,"borg-picker","body preview stopped: %s",error.c_str());
            }
        }else switch(browser.step(pad,options.practice_from_picker)){
         case gf::BorgBrowserChange::Package:
             confirmed=preview_unavailable=false;placement_panel.status={};practice_panel.status={};refresh_package();break;
         case gf::BorgBrowserChange::Variant:confirmed=preview_unavailable=false;refresh_tile();break;
         case gf::BorgBrowserChange::PracticeRoute:
             practice_panel.status={};
             gf::log_info(context,"borg-picker","practice route=%.*s",int(browser.practice_route_name().size()),
                 browser.practice_route_name().data());
             break;
         case gf::BorgBrowserChange::Confirm:
             if(placement){
                 if(!owned_save->backend->current()){
                     set_placement_status("SELECTED SAVE CHANGED - NO WRITE",true);break;
                 }
                 gf::ForceCatalog retail;
                 if(!read_retail(&retail)){
                     set_placement_status("EXPLICIT RETAIL CATALOGUE UNAVAILABLE");break;
                 }
                 const auto* snapshot=browser.metadata();
                 const auto result=placement->prepare(browser.owned_entry()->instance,placement_panel.force,placement_panel.slot,
                     retail.ready()?&retail:nullptr,browser.catalog(),{snapshot,1});
                 if(result==gf::BorgPlacementResult::Ready)placement_input.begin(pad);
                 set_placement_status(borg_placement_label(result),result==gf::BorgPlacementResult::SaveUnavailable);
                 break;
             }
             confirmed=true;
            if(const auto selection=browser.selection())gf::log_info(context,"borg-picker","inspected %s variant=%s (no save or battle admission)",
                selection->identity.id.c_str(),selection->variant.c_str());
            break;
         case gf::BorgBrowserChange::Preview:
             practice_panel.status={};
            if(const auto selection=browser.selection()){
                if(owned_save&&!owned_save->backend->current())return fail("selected save changed; reopen to inspect the current collection");
                gf::pc::BorgPackagePreviewFiles files;using Inputs=gf::pc::BorgPackagePreviewFiles;
                if(gf::pc::load_borg_preview_files("borgs",browser.catalog(),selection->identity.id,selection->variant,&files,&error)&&
                    preview.load_package_preview(files.identity,files.profile,Inputs::view(files.model),Inputs::view(files.rig_prefix),
                        Inputs::view(files.pose_profile),Inputs::view(files.motions),render,&error,true)){
                    preview.sync_input(pad);preview_open=true;preview_unavailable=false;
                    gf::log_info(context,"borg-picker","opened body preview %s variant=%s",selection->identity.id.c_str(),selection->variant.c_str());
                }else{
                    if(!preview.close_package_preview(render)){
                        gf::log_error(context,"borg-picker","could not release failed preview textures");render.shutdown();return 2;
                    }
                    preview_unavailable=true;
                    gf::log_warn(context,"borg-picker","body preview unavailable: %s",error.c_str());
                }
             }
             break;
         case gf::BorgBrowserChange::Practice:{
             if(!owned_save||!browser.owned_entry()||!owned_save->backend->current())
                 return fail("selected save changed; reopen the collection");
             const auto& saved=*browser.owned_entry();
             const gf::pc::BorgOwnedPracticeRequest request{options.selected_save_root,owned_save->entry.key,saved.instance};
             const auto current=gf::pc::scan_borg_packages("borgs");
             auto qualified=gf::pc::BorgOwnedPractice::open(request,"borgs",current,&error);
             if(!owned_save->backend->current()||(qualified&&qualified->saved()!=saved))
                 return fail("selected copy changed during qualification; reopen the collection");
             confirmed=preview_unavailable=false;
             if(!qualified){
                 practice_panel.status="PRACTICE UNAVAILABLE";
                 gf::log_warn(context,"borg-picker","practice unavailable for instance=%llu: %s",
                     static_cast<unsigned long long>(saved.instance),error.c_str());
                 break;
             }
             Options practice;
             constexpr std::array<std::string_view,unsigned(gf::GRedPickerPracticeRoute::Count)> modes{
                 "--gred-movement","--gred-slope-transition-dash","--gred-inclined-walk-ledge","--gred-inclined-dash-ledge",
                 "--gred-walk-ledge-fall-push","--gred-walk-ledge-high-angle-push","--gred-walk-ledge-expiry-push"
             };
             const auto route=browser.practice_route();
             const auto route_name=browser.practice_route_name();
             std::string mode(modes[unsigned(route)]);
             char program[]="gf_native",start_option[]="--practice-start";char* args[]={program,mode.data(),start_option};
             if(!parse(route==gf::GRedPickerPracticeRoute::Standard?2:3,args,&practice))
                 return fail("could not select the existing practice route");
             practice.owned_practice=request;
             practice.headless=options.headless;practice.width=options.width;practice.height=options.height;
             practice.battle_mvp_assets=options.battle_mvp_assets;
             practice.frames=options.frames?options.frames-frame:0;practice.seconds=options.seconds;
             if(!captured){
                 practice.screenshot=options.screenshot;practice.capture_frame=options.capture_frame>frame?options.capture_frame-frame:0;
             }
             // One active renderer lifecycle at a time. The preview's
             // private textures are closed, and no picker or practice GL
             // handles survive the context transition.
             if(!preview.close_package_preview(render))return fail("could not release preview textures before practice");
             render.shutdown();renderer_active=false;tile={};
             const auto starting_frame=context.frame_index();
             PickerPracticeHandoff handoff{start};
             int outcome=2;
             {
                 gf::RenderEngine session_render(context.render());
                 context.window().set_title("Owned Borg Practice");
                 if(session_render.initialize({options.width,options.height,!options.headless,true,"Owned Borg Practice"},
                     context.backends().window,&error)){
                     gf::log_info(context,"borg-picker","launched practice for owned instance=%llu route=%.*s",
                         static_cast<unsigned long long>(saved.instance),int(route_name.size()),route_name.data());
                     outcome=run_battle_mvp(context,session_render,practice,&qualified->files(),qualified.get(),&handoff);
                 }else{
                     session_render.shutdown();
                     gf::log_warn(context,"borg-picker","practice renderer unavailable: %s",error.c_str());
                 }
             }
             frame+=context.frame_index()-starting_frame;captured=captured||handoff.captured;
             if(context.window().should_close()||(options.frames&&frame>=options.frames))break;
             previous=context.clock().monotonic_ns();
             if(options.seconds&&previous-start>=gf::u64(options.seconds)*1000000000ull)break;
             if(!owned_save->backend->current())return fail("selected save changed during practice; reopen the collection");
             context.window().set_title("Owned Borg Collection");
             render.clear_shared_texture_upload_error();render.state()={};
             if(!render.initialize({options.width,options.height,!options.headless,true,"Owned Borg Collection"},
                 context.backends().window,&error))return fail(error);
             renderer_active=true;
             refresh_tile();wait_for_exit_release=true;
             practice_panel.status=outcome?"PRACTICE UNAVAILABLE":"RETURNED FROM PRACTICE";
             if(!outcome)gf::log_info(context,"borg-picker","returned to owned instance=%llu after practice route=%.*s",
                 static_cast<unsigned long long>(saved.instance),int(route_name.size()),route_name.data());
             continue;
         }
         case gf::BorgBrowserChange::None:break;
         }
        if(!renderer_active)break;
        if(placement){
            const auto* offer=placement->offer();
            placement_panel.offered=offer?&offer->owned:nullptr;
            placement_panel.declared_cost=offer?offer->declared_cost:0;
            placement_panel.confirm_selected=placement_input.confirm_selected();
        }
        render.begin_frame({frame,.055f,.065f,.085f});render.reset_arena();
        const auto draws=preview_open?preview.draw(render,options.width,options.height):
            view.draw(render,options.width,options.height,browser,tile,confirmed,preview_unavailable,
                owned_save?std::string_view(owned_save->entry.name):std::string_view{},placement?&placement_panel:nullptr,
                options.practice_from_picker?&practice_panel:nullptr);
        render.end_frame();
        if(!captured)captured=capture_if_requested(context,options,frame);
        render.present();context.advance_frame();++frame;
        if(!draws){gf::log_error(context,"borg-picker","no browser draws");render.shutdown();return 2;}
        if(options.frames&&frame>=options.frames)break;
    }
    const auto* selected=browser.entry();const auto* variant=browser.variant();
    gf::log_info(context,"borg-picker","frames=%u packages=%zu selected=%s variant=%s view=%s",
        frame,browser.catalog().discovered.size(),selected?selected->manifest.identity.id.c_str():"none",
        variant?variant->id.c_str():"unavailable",preview_open?"preview":renderer_active?"picker":"practice");
    if(owned_save){
        const auto* saved=browser.owned_entry();const auto status=browser.owned_status();
        gf::log_info(context,"borg-picker","owned-copies=%zu instance=%llu saved-variant=%s disabled=%u status=%s",
            browser.size(),static_cast<unsigned long long>(saved?saved->instance:0),
            saved&&saved->variant?saved->variant->id.c_str():"unspecified",unsigned(saved&&saved->disabled),
            status.empty()?"NONE":status.data());
    }
    if(renderer_active){
        if(!preview.close_package_preview(render))return fail("could not release preview textures");
        render.shutdown();
    }
    return 0;
}

int run_animation_mosaic(gf::Context& context,gf::RenderEngine& render,const Options& options,const gf::BorgPackageCatalog& packages){
    gf::NativeAnimationMosaic mosaic;std::string error;
    const bool package_preview=!options.preview_borg.empty();
    if(package_preview){
        gf::pc::BorgPackagePreviewFiles files;using Inputs=gf::pc::BorgPackagePreviewFiles;
        if(!gf::pc::load_borg_preview_files("borgs",packages,options.preview_borg,options.borg_variant,&files,&error)||
           !mosaic.load_package_preview(files.identity,files.profile,Inputs::view(files.model),Inputs::view(files.rig_prefix),
               Inputs::view(files.pose_profile),Inputs::view(files.motions),render,&error)){
            gf::log_error(context,"borg-preview","load failed: %s",error.c_str());render.shutdown();return 2;
        }
        gf::log_info(context,"borg-preview","%s variant=%s idle=%u:%u; body preview only, no actor or gameplay rules",
            files.identity.id.c_str(),files.profile.variant.c_str(),unsigned(files.profile.idle_group),unsigned(files.profile.idle_choice));
    }else if(!mosaic.load(context.files(),options.battle_mvp_assets,render,options.animation_columns,options.animation_rows,&error)||
       (!options.animation_programs.empty()&&!mosaic.load_source_programs(context.files(),options.animation_programs,options.animation_height_profile,render,&error))||
       (!options.animation_motion_profile.empty()&&!mosaic.load_source_motion(context.files(),options.animation_motion_profile,render,&error))||
       ((options.animation_borg||options.animation_page)&&!mosaic.select(options.animation_borg,options.animation_page,render,&error))){
        gf::log_error(context,"animation-mosaic","load failed: %s",error.c_str());render.shutdown();return 2;}
    if(!options.animation_programs.empty())gf::log_info(context,"animation-mosaic","G-Red source-program timelines: %zu essentials; 96-tick inspection replay, shared VM/height/pose/render systems; no gameplay input, physics or actor admission",mosaic.catalog(0).size());
    else if(!package_preview)gf::log_info(context,"animation-mosaic","looping model previews: G-Red=%zu Sasuke=%zu; numbers are per-Borg raw group/clip IDs; shared IDs start10000",mosaic.catalog(0).size(),mosaic.catalog(1).size());
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
    gf::log_info(context,"animation-mosaic","frames=%u Borg=%u page=%u",frame,mosaic.borg(),mosaic.page()+1);
    if(!mosaic.close_package_preview(render)){gf::log_error(context,"borg-preview","could not release preview textures");render.shutdown();return 2;}
    render.shutdown();return 0;
}

} // namespace

int main(int argc, char** argv) {
    Options options;
    if (!parse(argc, argv, &options)) return 2;
    if(unsigned(options.list_borgs)+unsigned(!options.check_borg_catalog.empty())+unsigned(!options.check_borg_moves.empty())+
       unsigned(!options.check_borg_spawn.empty())+unsigned(!options.check_borg_effects.empty())+
       unsigned(!options.check_borg_forms.empty())+unsigned(!options.check_borg_fusions.empty())+
       unsigned(!options.check_borg_acquisition.empty())+unsigned(!options.check_borg_variants.empty())>1)return 2;
    const auto borg_packages=gf::pc::scan_borg_packages("borgs");
    gf::pc::BorgPracticeFiles practice_package;
    std::unique_ptr<gf::pc::BorgOwnedPractice> owned_practice;
    const gf::pc::BorgPracticeFiles* package=nullptr;
    if(options.force_practice){
        std::string error;gf::ForceCatalog retail;
        if(!options.retail_force_catalogue.empty()){
            auto files=gf::sdl::create_file_system();
            if(!files||!load_retail_force_catalogue(*files,options.retail_force_catalogue,&retail)){
                std::fputs("Force slot practice: could not read the explicit retail catalogue\n",stderr);return 2;
            }
        }
        owned_practice=gf::pc::BorgOwnedPractice::open_force_slot(*options.force_practice,"borgs",borg_packages,
            retail.ready()?&retail:nullptr,&error);
        if(!owned_practice){std::fprintf(stderr,"Force slot practice: %s\n",error.c_str());return 2;}
        package=&owned_practice->files();
    }else if(options.owned_practice){
        std::string error;
        owned_practice=gf::pc::BorgOwnedPractice::open(*options.owned_practice,"borgs",borg_packages,&error);
        if(!owned_practice){std::fprintf(stderr,"Owned Borg practice: %s\n",error.c_str());return 2;}
        package=&owned_practice->files();
    }else if(!options.practice_borg.empty()){
        std::string error;
        if(!gf::pc::load_borg_practice_files("borgs",borg_packages,options.practice_borg,options.borg_variant,&practice_package,&error)){
            std::fprintf(stderr,"Borg practice: %s\n",error.c_str());return 2;
        }
        package=&practice_package;
    }
    if(options.inspect_force)return inspect_borg_force(options,borg_packages);
    using BorgScan=gf::BorgPackageScanStatus;
    const bool scan_failed=borg_packages.status==BorgScan::Unavailable||borg_packages.status==BorgScan::EntryBudget;
    if(options.list_borgs){
        if(scan_failed){std::fprintf(stderr,"Borg package scan could not complete: borgs/\n");return 2;}
        for(const auto& entry:borg_packages.discovered){
            const auto& identity=entry.manifest.identity;
            std::printf("%s\t%s\n",identity.id.c_str(),identity.name.c_str());
        }
        if(!borg_packages.issues.empty())std::fprintf(stderr,"%zu Borg package directories were rejected\n",borg_packages.issues.size());
        return 0;
    }
    if(!options.check_borg_catalog.empty()){
        gf::BorgCatalogCard card;std::string error;
        if(!gf::pc::load_borg_catalog_card("borgs",borg_packages,options.check_borg_catalog,&card,&error)){
            std::fprintf(stderr,"Borg catalogue: %s\n",error.c_str());return 2;
        }
        std::printf("%s\t%s\tdefault=%s\n",card.identity.id.c_str(),card.identity.name.c_str(),card.profile.default_variant.c_str());
        for(std::size_t i=0;i<card.profile.variants.size();++i){
            const auto& variant=card.profile.variants[i];
            std::printf("%s\t%s\tcost=%u\ticon=",variant.id.c_str(),variant.label.c_str(),unsigned(variant.force_cost));
            if(const auto& icon=card.icons[i])std::printf("%ux%u\n",icon->width,icon->height);
            else std::puts("placeholder");
        }
        if(const auto& details=card.profile.details){
            if(details->collection_number)std::printf("collection-number=%u\n",unsigned(*details->collection_number));
            else std::puts("collection-number=unresolved");
            if(!details->source_reference_code.empty())std::printf("source-reference-code=%s\n",details->source_reference_code.c_str());
            std::puts("Extended catalogue fields validated; facet references not loaded.");
        }
        std::puts("Catalogue profile validated; model, animation, and gameplay not loaded.");
        return 0;
    }
    if(!options.check_borg_moves.empty()){
        gf::pc::BorgMoveInspection moves;std::string error;
        if(!gf::pc::load_borg_move_inspection("borgs",borg_packages,options.check_borg_moves,&moves,&error)){
            std::fprintf(stderr,"Borg moves: %s\n",error.c_str());return 2;
        }
        std::printf("%s\t%s\tmoves=%zu\n",moves.identity.id.c_str(),moves.identity.name.c_str(),moves.profile.moves.size());
        for(const auto& move:moves.profile.moves)
            std::printf("%s\t%s\tform=%s\tphases=%zu\toutcomes=%zu\n",move.id.c_str(),move.label.c_str(),
                move.form.c_str(),move.phases.size(),move.outcomes.size());
        std::puts("Move definitions validated; gameplay, effects, and audio not loaded.");
        return 0;
    }
    if(!options.check_borg_spawn.empty()){
        gf::pc::BorgSpawnInspection spawn;std::string error;
        if(!gf::pc::load_borg_spawn_inspection("borgs",borg_packages,options.check_borg_spawn,&spawn,&error)){
            std::fprintf(stderr,"Borg spawn: %s\n",error.c_str());return 2;
        }
        constexpr const char* anchors[]{"unresolved","mode-selected","floor-offset","arena-top-clearance","absolute-y"};
        constexpr const char* horizontal[]{"unresolved","mode-selected","stage-center","absolute-xz"};
        constexpr const char* states[]{"unresolved","grounded","falling","fixed-airborne","flying","hovering"};
        std::printf("%s\t%s\tspawn-rules=%zu\n",spawn.identity.id.c_str(),spawn.identity.name.c_str(),spawn.profile.rules.size());
        for(const auto& rule:spawn.profile.rules){
            std::printf("%s\tform=%s\tmode=%s\tvariant=%s\tanchor=%s\tstate=%s\theight=",rule.id.c_str(),rule.form.c_str(),
                rule.mode.c_str(),rule.variant.c_str(),anchors[unsigned(rule.anchor)],states[unsigned(rule.initial_state)]);
            if(rule.height_kind==gf::BorgSpawnValue::Number)std::printf("%g",double(rule.height));
            else std::fputs(rule.height_kind==gf::BorgSpawnValue::None?"none":"unresolved",stdout);
            std::printf("\thorizontal-anchor=%s\txz=",horizontal[unsigned(rule.horizontal_anchor)]);
            if(rule.horizontal_kind==gf::BorgSpawnValue::Number)std::printf("%g|%g\n",double(rule.xz[0]),double(rule.xz[1]));
            else std::puts(rule.horizontal_kind==gf::BorgSpawnValue::None?"none":"unresolved");
        }
        std::puts("Spawn declarations validated; no stage, placement, or actor loaded.");
        return 0;
    }
    if(!options.check_borg_effects.empty()){
        gf::pc::BorgEffectInspection effects;std::string error;
        if(!gf::pc::load_borg_effect_inspection("borgs",borg_packages,options.check_borg_effects,&effects,&error)){
            std::fprintf(stderr,"Borg effects: %s\n",error.c_str());return 2;
        }
        std::printf("%s\t%s\tvisuals=%zu\tsounds=%zu\tcues=%zu\tmove-references=%s\n",
            effects.identity.id.c_str(),effects.identity.name.c_str(),effects.profile.visuals.size(),effects.profile.sounds.size(),
            effects.profile.cues.size(),effects.checked_move_references?"checked":"not-declared");
        std::puts("Effect declarations validated; assets not decoded, shared references not resolved, no playback or gameplay.");
        return 0;
    }
    if(!options.check_borg_forms.empty()){
        gf::pc::BorgFormInspection forms;std::string error;
        if(!gf::pc::load_borg_form_inspection("borgs",borg_packages,options.check_borg_forms,&forms,&error)){
            std::fprintf(stderr,"Borg forms: %s\n",error.c_str());return 2;
        }
        std::printf("%s\t%s\tdefault-form=%s\tforms=%zu\ttransitions=%zu\tmove-forms=%s\tspawn-forms=%s\n",
            forms.identity.id.c_str(),forms.identity.name.c_str(),forms.profile.default_form.c_str(),
            forms.profile.forms.size(),forms.profile.transitions.size(),forms.checked_moves?"checked":"not-declared",
            forms.checked_spawn?"checked":"not-declared");
        std::puts("Form declarations validated; no transitions, asset swaps, timers, or gameplay executed.");
        return 0;
    }
    if(!options.check_borg_fusions.empty()){
        gf::pc::BorgFusionInspection fusions;std::string error;
        if(!gf::pc::load_borg_fusion_inspection("borgs",borg_packages,options.check_borg_fusions,&fusions,&error)){
            std::fprintf(stderr,"Borg fusions: %s\n",error.c_str());return 2;
        }
        std::printf("%s\t%s\tfusions=%zu\tunresolved-parties=%u\n",fusions.identity.id.c_str(),fusions.identity.name.c_str(),
            fusions.profile.fusions.size(),fusions.unresolved_parties);
        constexpr const char* usage[]{"unresolved","fusion-only","independent"};
        for(const auto& rule:fusions.profile.fusions)
            std::printf("%s\t%s + %s -> %s\tresult-use=%s\n",rule.id.c_str(),rule.first.package.c_str(),
                rule.second.package.c_str(),rule.result.package.c_str(),usage[unsigned(rule.result_use)]);
        std::puts("Battle fusion declarations validated; no actors combined, state transferred, or units unlocked.");
        return 0;
    }
    if(!options.check_borg_acquisition.empty()){
        gf::pc::BorgAcquisitionInspection result;std::string error;
        if(!gf::pc::load_borg_acquisition_inspection("borgs",borg_packages,options.check_borg_acquisition,&result,&error)){
            std::fprintf(stderr,"Borg acquisition: %s\n",error.c_str());return 2;
        }
        std::printf("%s\t%s\troutes=%zu\tcrystal-recipes=%zu\n",result.identity.id.c_str(),result.identity.name.c_str(),
            result.profile.routes.size(),result.profile.recipes.size());
        constexpr const char* grants[]{"unresolved","owned","crystal-part","battle-only","loan"};
        for(const auto& route:result.profile.routes)
            std::printf("%s\tmode=%s\tvariant=%s\tgrant=%s\n",route.id.c_str(),route.mode.c_str(),route.variant.c_str(),
                grants[unsigned(route.grant)]);
        std::puts("Acquisition declarations validated; no rewards, inventory, trade, or save changes.");
        return 0;
    }
    if(!options.check_borg_variants.empty()){
        gf::pc::BorgVariantInspection variants;std::string error;
        if(!gf::pc::load_borg_variant_inspection("borgs",borg_packages,options.check_borg_variants,&variants,&error)){
            std::fprintf(stderr,"Borg variants: %s\n",error.c_str());return 2;
        }
        std::printf("%s\t%s\toverrides=%zu\n",variants.identity.id.c_str(),variants.identity.name.c_str(),
            variants.profile.variants.size());
        auto print_offset=[](const gf::BorgVariantLevelOffset& value){
            if(value.kind==gf::BorgVariantLevelKind::Offset)std::printf("%d",int(value.value));
            else std::fputs(value.kind==gf::BorgVariantLevelKind::Inherit?"inherit":"unresolved",stdout);
        };
        for(const auto& row:variants.profile.variants){
            std::printf("%s\thp-level-offset=",row.variant.c_str());print_offset(row.hp);
            std::fputs("\tammo-level-offset=",stdout);print_offset(row.ammo);std::putchar('\n');
        }
        std::puts("Variant declarations validated; assets not decoded, no live stat or gameplay changes.");
        return 0;
    }

    gf::headless::Bundle headless;
    std::unique_ptr<gf::IFileSystem> headless_files;
    std::unique_ptr<gf::ILogBackend> headless_log;
    gf::Context context(headless.get(), 60);
    if(options.headless&&(options.test_battle||options.battle_mvp||options.animation_mosaic||!options.preview_borg.empty()||
       options.browse_borgs||!options.selected_save_root.empty())){
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
             platform->create({options.width, options.height, options.placement_force?"Place Owned Borg":
                              options.browse_owned_borgs?"Owned Borg Collection":
                              !options.selected_save_root.empty()?"Borg Save Recovery":
                              options.browse_borgs?"Installed Borg Picker":!options.preview_borg.empty()?"Borg Package Preview":
                              options.animation_mosaic?"Borg Animation Mosaic":"Gotcha Force", false, !options.animation_export.empty()},
                             renderer.get(), &error);
        if (!backends.complete()) {
            std::fprintf(stderr, "platform initialization failed: %s\n",
                         error.empty() ? "backend set incomplete" : error.c_str());
            return 1;
        }
        context = gf::Context(backends, 60);
    }
    if(scan_failed)gf::log_warn(context,"borg-packages","could not complete startup scan of borgs/");
    else if(borg_packages.status==BorgScan::Complete)gf::log_info(context,"borg-packages",
        "discovered=%zu rejected=%zu (metadata only; not gameplay-ready)",borg_packages.discovered.size(),borg_packages.issues.size());

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
    if(options.battle_mvp)return run_battle_mvp(context,render,options,package,owned_practice.get());
    if(options.browse_borgs||options.browse_owned_borgs)return run_borg_browser(context,render,options,borg_packages);
    if(!options.selected_save_root.empty())return run_borg_recovery(context,render,options,borg_packages);
    if(options.animation_mosaic||!options.preview_borg.empty())return run_animation_mosaic(context,render,options,borg_packages);

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
