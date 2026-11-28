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
#include "engine/gameplay/borg_control_selector.hpp"
#include "engine/gameplay/native_battle_mvp_view.hpp"
#include "engine/gameplay/portable_roster_ai.hpp"
#include "engine/gameplay/native_gred_ground_dash_demo.hpp"
#include "engine/gameplay/native_animation_mosaic.hpp"
#include "engine/gameplay/borg_installed_browser_view.hpp"
#include "engine/gameplay/borg_cheat_picker.hpp"
#include "engine/save/borg_package_placement.hpp"
#include "engine/gameplay/borg_recovery_menu_view.hpp"
#include "engine/gameplay/borg_package_force_view.hpp"
#include "engine/gameplay/borg_mixed_force_view.hpp"
#include "engine/gameplay/borg_force_browser_view.hpp"
#include "engine/save/force_inventory.hpp"
#include "engine/graphics/render_engine.hpp"
#include "engine/graphics/scene_draw.hpp"
#include "engine/input/input_router.hpp"
#include "engine/log.hpp"
#include "platform/gl/gl_renderer.hpp"
#include "platform/pc/battle_roster_load.hpp"
#include "platform/pc/borg_selected_save.hpp"
#include "platform/pc/borg_acquisition_load.hpp"
#include "platform/pc/borg_effect_load.hpp"
#include "platform/pc/borg_form_load.hpp"
#include "platform/pc/borg_controller_load.hpp"
#include "platform/pc/borg_fusion_load.hpp"
#include "platform/pc/borg_force_browse.hpp"
#include "platform/pc/borg_library_load.hpp"
#include "platform/pc/borg_lab_load.hpp"
#include "platform/pc/borg_move_load.hpp"
#include "platform/pc/borg_practice_load.hpp"
#include "platform/pc/borg_gameplay_load.hpp"
#include "platform/pc/borg_spawn_load.hpp"
#include "platform/pc/gred_practice_input.hpp"
#include "platform/pc/portable_battle_host.hpp"
#include "platform/pc/portable_challenge_launch.hpp"
#include "platform/pc/portable_font_load.hpp"
#include "platform/pc/portable_story_map.hpp"
#include "platform/pc/portable_story_campaign.hpp"
#include "platform/pc/sdl_platform.hpp"
#include "platform/pc/stage_package_scan.hpp"

namespace {

constexpr gf::f32 kProbeIdentity[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};

enum class TimedLedgeFollowup { None, Relaunch, AirDash, DirectAirDash, HandoffRelaunch, ExpiryHandoffRelaunch, ExpiryDirectRelaunch,
    DirectTimedRelaunch, DirectTimedUpRelaunch, TimedReleasePush, TimedReleaseTwoPushes, ExpiringDirectRelaunch, ExpiringDirectRelaunchPush,
    ExpiringAdjacentRelaunchPush, ExpiringAirDash, ExpiringAirDashRelaunch, ExpiringShortAirDashRelaunch,
    ExpiringAirDashSnap, ExpiringAirDashSnapRelaunch, TimedReleaseThreePushes, TimedReleaseFourPushes, TimedReleaseExpiryDrift,
    RisingWindupRelaunch, RisingDiagonalWindupRelaunch, SameFaceExpiryPush };

struct Options {
    bool headless = false;
    bool list_borgs = false;
    bool list_stories = false;
    std::string story_map;
    std::optional<gf::pc::PortableStorySelection> portable_story;
    bool browse_borgs=false,demo_borg_browser=false,browse_owned_borgs=false;
    std::string collection_font_root;
    bool cheat_offer_picker=false,arrange_from_force=false;
    bool browse_force=false;
    bool borg_controls=false,demo_borg_controls=false,lab_jump=false,lab_dash=false,lab_air_dash=false,lab_glide=false,lab_landing_audio=false,
         lab_cruise=false,lab_source_pace=false,demo_lab_picker=false;
    bool demo_package_battle=false,demo_roster_battle=false,trace_package_battle=false,battle_picker=false,exercise_battle_mobility=false,
         exercise_gred_touchdown=false;
    std::string demo_battle_pick;
    std::string battle_borg,against_borg,against_variant,walk_borg;
    struct RosterBorg {gf::u8 team;std::string id,variant{};};
    std::vector<RosterBorg> roster_borgs;
    std::array<gf::u8,2> first_roster_teams{0,1};
    gf::u8 roster_local_players=2;
    gf::u8 roster_rounds_to_win=0;
    gf::u16 roster_round_seconds=0;
    std::string battle_roster_file;
    std::string list_battle_rosters;
    std::string browse_battle_rosters,demo_roster_select,demo_roster_next;
    bool check_battle_roster=false,demo_roster_cancel=false;
    bool roster_browser_match=false,demo_roster_pad=false;
    std::string battle_stage;
    std::vector<std::string> battle_stage_spawns;
    bool list_challenges=false;
    std::optional<gf::pc::PortableChallengeSelection> portable_challenge;
    bool list_stages=false,explicit_battle_mvp_assets=false;
    unsigned package_battle_distance=0;
    std::optional<gf::BattleMvpDamageIndices> package_damage_indices;
    std::optional<gf::BattleKoForcePools> package_ko_force;
    bool practice_from_picker=false;
    std::string selected_save_root,retail_force_catalogue;
    gf::u64 selected_account=0,selected_save=0,recovery_notice=0,inspect_force=0,owned_instance=0,placement_force=0,placement_slot=0;
    gf::u64 browse_force_slot=0;
    std::string check_borg_catalog,check_borg_moves,check_borg_spawn,check_borg_effects,check_borg_forms,check_borg_fusions,
        check_borg_acquisition,check_borg_variants,check_borg_controllers,preview_borg,preview_clip,preview_origin,practice_borg,borg_variant;
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
    bool gred_timed_relaunch_floor=false;
    bool gred_timed_relaunch_snap_dash=false;
    bool gred_active_snap_relaunch=false;
    bool gred_timed_snap_windup_relaunch=false;
    bool gred_positive_snap_windup_relaunch=false;
    bool gred_expiring_snap_windup_relaunch=false;
    bool source_ground_fall_push=false;
    bool source_ground_air_fall_push=false;
    bool source_ground_relaunch_fall_push=false;
    bool source_ground_recovery_push=false;
    bool gred_recovery_completion_push=false;
    bool gred_recovery_completion_exit=false;
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
    bool gred_inclined_dash_jump=false;
    bool gred_timed_incline_landing_dash=false;
    bool gred_inclined_dash_ledge=false;
    bool gred_inclined_dash_ledge_relaunch=false;
    bool gred_inclined_walk_ledge=false;
    bool gred_practice_start=false;
    bool gred_walk_ledge_fall_push=false;
    bool gred_walk_ledge_high_angle_push=false;
    TimedLedgeFollowup timed_ledge_followup=TimedLedgeFollowup::None;
    bool gred_walk_ledge_expiry_push=false;
    bool gred_walk_ledge_expiry_air_dash=false;
    bool demo_gred_walk_ledge_expiry_drift=false;
    bool demo_gred_walk_ledge_expiry_relaunch=false;
    bool gred_recovery_recontact=false;
    bool gred_recovery_recontact_drift=false;
    bool gred_recovery_recontact_seam=false;
    bool gred_recovery_first_contact_seam=false;
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

bool apply_battle_roster_preset(Options& options,gf::BattleRosterPreset preset){
    static_assert(gf::kBattleRosterPresetMaxActors<=gf::kBattleMvpMaxActors);
    if(preset.damage_indices){
        if(options.package_damage_indices)return false;
        options.package_damage_indices=preset.damage_indices;
    }
    options.battle_stage=std::move(preset.stage);
    options.roster_local_players=preset.locals;
    options.roster_rounds_to_win=preset.rounds_to_win;
    options.roster_round_seconds=preset.round_seconds;
    for(std::size_t i=0;i<preset.fighters.size();++i){
        auto& fighter=preset.fighters[i];
        options.battle_stage_spawns.push_back(std::move(fighter.spawn));
        if(i<2)options.first_roster_teams[i]=fighter.team;
        if(i==0){options.battle_borg=std::move(fighter.borg);options.borg_variant=std::move(fighter.variant);}
        else if(i==1){options.against_borg=std::move(fighter.borg);options.against_variant=std::move(fighter.variant);}
        else options.roster_borgs.push_back({fighter.team,std::move(fighter.borg),std::move(fighter.variant)});
    }
    return true;
}

bool parse(int argc, char** argv, Options* options) {
    auto id=[](std::string_view text,gf::u64* out){
        if(text.empty())return false;
        const auto end=text.data()+text.size();
        const auto value=std::from_chars(text.data(),end,*out);
        return value.ec==std::errc{}&&value.ptr==end&&*out!=0;
    };
    bool story_map_flags_only = true,challenge_flags_only=true,roster_file_flags_only=true;
    bool story_run_flags_only = true;
    bool roster_browser_flags_only=true;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        // The read-only map accepts only presentation flags, not other host
        // modes or save/battle options (including newly added modes).
        story_map_flags_only = story_map_flags_only &&
            (arg=="--story-map"||arg=="--headless"||arg=="--no-audio"||arg=="--frames"||
             arg=="--seconds"||arg=="--size"||arg=="--screenshot"||arg=="--capture-frame");
        story_run_flags_only = story_run_flags_only &&
            (arg=="--run-story"||arg=="--headless"||arg=="--no-audio"||arg=="--frames"||
             arg=="--seconds"||arg=="--size"||arg=="--retail-catalog");
        challenge_flags_only=challenge_flags_only&&
            (arg=="--run-challenge"||arg=="--headless"||arg=="--no-audio"||arg=="--frames"||
             arg=="--seconds"||arg=="--size"||arg=="--screenshot"||arg=="--capture-frame"||
             arg=="--retail-catalog"||arg=="--trace-package-battle");
        roster_file_flags_only=roster_file_flags_only&&
            (arg=="--battle-roster"||arg=="--check-battle-roster"||arg=="--headless"||arg=="--no-audio"||arg=="--frames"||
             arg=="--seconds"||arg=="--size"||arg=="--screenshot"||arg=="--capture-frame"||
             arg=="--demo-package-battle"||arg=="--trace-package-battle"||arg=="--package-damage-indices"||
             arg=="--package-ko-force");
        roster_browser_flags_only=roster_browser_flags_only&&
            (arg=="--browse-battle-rosters"||arg=="--demo-roster-select"||arg=="--demo-roster-next"||arg=="--headless"||
             arg=="--demo-roster-pad"||arg=="--no-audio"||arg=="--frames"||arg=="--size"||arg=="--trace-package-battle");
        if (arg == "--headless") options->headless = true;
        else if(arg=="--list-stories"){
            if(argc!=2)return false;
            options->list_stories=true;
        }
        else if(arg=="--story-map"){
            if(++i>=argc||!options->story_map.empty()||!gf::borg_package_id_valid(argv[i]))return false;
            options->story_map=argv[i];
        }
        else if(arg=="--run-story"){
            if(argc-i<=5||options->portable_story||!gf::borg_package_id_valid(argv[i+1])||!*argv[i+2])return false;
            gf::pc::PortableStorySelection selection;
            selection.id=argv[++i];selection.save_root=argv[++i];
            gf::u64 force=0;
            if(!id(argv[++i],&selection.save.account)||!id(argv[++i],&selection.save.save)||
               !id(argv[++i],&force)||force>gf::kForceCount)return false;
            selection.force.index=gf::u32(force-1);
            options->portable_story=std::move(selection);
        }
        else if(arg=="--borg-controls"||arg=="--demo-borg-controls"){
            if(options->borg_controls)return false;
            options->borg_controls=true;options->demo_borg_controls=arg=="--demo-borg-controls";
        }
        else if(arg=="--walk-borg"){
            if(++i>=argc||!options->walk_borg.empty()||!gf::borg_package_id_valid(argv[i]))return false;
            options->walk_borg=argv[i];
        }
        else if(arg=="--demo-lab-picker"){
            if(options->demo_lab_picker)return false;
            options->demo_lab_picker=true;
        }
        else if(arg=="--lab-source-pace"){
            if(options->lab_source_pace)return false;
            options->lab_source_pace=true;
        }
        else if(arg=="--lab-jump"||arg=="--lab-dash"||arg=="--lab-air-dash"||arg=="--lab-glide"||arg=="--lab-landing-audio"||arg=="--lab-cruise"){
            auto& capability=arg=="--lab-jump"?options->lab_jump:arg=="--lab-dash"?options->lab_dash:
                arg=="--lab-air-dash"?options->lab_air_dash:arg=="--lab-glide"?options->lab_glide:
                arg=="--lab-cruise"?options->lab_cruise:options->lab_landing_audio;
            if(capability)return false;
            capability=true;
        }
        else if(arg=="--battle-borg"||arg=="--against-borg"||arg=="--against-variant"){
            auto& value=arg=="--battle-borg"?options->battle_borg:arg=="--against-borg"?options->against_borg:options->against_variant;
            if(++i>=argc||!value.empty()||!(arg=="--against-variant"?gf::borg_catalog_variant_id_valid(argv[i]):gf::borg_package_id_valid(argv[i])))return false;
            value=argv[i];
        }
        else if(arg=="--roster-borg"){
            if(argc-i<=2||options->roster_borgs.size()>=gf::kBattleMvpMaxActors-2)return false;
            const std::string_view team=argv[++i];const std::string_view id=argv[++i];
            if((team!="0"&&team!="1")||!gf::borg_package_id_valid(id))return false;
            const auto side=gf::u8(team=="1");
            if(std::count_if(options->roster_borgs.begin(),options->roster_borgs.end(),
                [&](const auto& member){return member.team==side;})>=2)return false;
            options->roster_borgs.push_back({side,std::string(id)});
        }
        else if(arg=="--list-battle-rosters"){
            if(argc!=3||++i>=argc||!*argv[i])return false;
            options->list_battle_rosters=argv[i];
        }
        else if(arg=="--browse-battle-rosters"||arg=="--demo-roster-select"||arg=="--demo-roster-next"){
            auto& value=arg=="--browse-battle-rosters"?options->browse_battle_rosters:
                arg=="--demo-roster-select"?options->demo_roster_select:options->demo_roster_next;
            if(++i>=argc||!value.empty()||!*argv[i])return false;
            value=argv[i];
        }
        else if(arg=="--demo-roster-pad"){
            if(options->demo_roster_pad)return false;
            options->demo_roster_pad=true;
        }
        else if(arg=="--battle-roster"||arg=="--check-battle-roster"){
            if(arg=="--check-battle-roster"){
                if(argc!=3)return false;
                options->check_battle_roster=options->headless=options->no_audio=true;
            }
            if(++i>=argc||!options->battle_roster_file.empty()||!*argv[i])return false;
            options->battle_roster_file=argv[i];
        }
        else if(arg=="--battle-stage"){
            if(++i>=argc||!options->battle_stage.empty()||!gf::borg_package_id_valid(argv[i]))return false;
            options->battle_stage=argv[i];
        }
        else if(arg=="--list-stages"){
            if(argc!=2)return false;
            options->list_stages=true;
        }
        else if(arg=="--list-challenges"){
            if(argc!=2)return false;
            options->list_challenges=true;
        }
        else if(arg=="--run-challenge"){
            if(argc-i<=6||options->portable_challenge||!gf::borg_package_id_valid(argv[i+1])||
               !gf::borg_catalog_variant_id_valid(argv[i+2])||!*argv[i+3])return false;
            gf::pc::PortableChallengeSelection selection;
            selection.id=argv[++i];selection.encounter=argv[++i];selection.save_root=argv[++i];
            gf::u64 force=0;
            if(!id(argv[++i],&selection.save.account)||!id(argv[++i],&selection.save.save)||
               !id(argv[++i],&force)||force>gf::kForceCount)return false;
            selection.force.index=gf::u32(force-1);
            options->portable_challenge=std::move(selection);
        }
        else if(arg=="--stage-spawns"){
            auto& spawns=options->battle_stage_spawns;
            if(!spawns.empty())return false;
            while(i+1<argc&&!std::string_view(argv[i+1]).starts_with('-')){
                if(spawns.size()>=gf::kBattleMvpMaxActors||!gf::borg_catalog_variant_id_valid(argv[i+1]))return false;
                spawns.emplace_back(argv[++i]);
            }
            if(spawns.size()<2)return false;
        }
        else if(arg=="--demo-package-battle")options->demo_package_battle=true;
        else if(arg=="--demo-roster-battle"){options->demo_roster_battle=true;options->demo_package_battle=true;}
        else if(arg=="--battle-picker")options->battle_picker=true;
        else if(arg=="--exercise-battle-mobility")options->exercise_battle_mobility=true;
        else if(arg=="--exercise-gred-touchdown")options->exercise_gred_touchdown=true;
        else if(arg=="--demo-battle-pick"&&i+1<argc)options->demo_battle_pick=argv[++i];
        else if(arg=="--trace-package-battle")options->trace_package_battle=true;
        else if(arg=="--package-battle-distance"){
            gf::u64 value=0;
            if(++i>=argc||options->package_battle_distance||!id(argv[i],&value)||value<90||value>1200)return false;
            options->package_battle_distance=unsigned(value);
        }
        else if(arg=="--package-damage-indices"){
            if(++i>=argc||options->package_damage_indices)return false;
            gf::BattleMvpDamageIndices indices;
            if(!gf::battle_read_damage_indices(argv[i],&indices))return false;
            options->package_damage_indices=indices;
        }
        else if(arg=="--package-ko-force"){
            gf::BattleKoForcePools pools{};
            if(++i>=argc||options->package_ko_force||!gf::battle_read_ko_force_pools(argv[i],&pools))return false;
            options->package_ko_force=pools;
        }
        else if (arg == "--list-borgs") options->list_borgs = true;
        else if(arg=="--practice-start"){
            if(options->gred_practice_start)return false;
            options->gred_practice_start=true;
        }
        else if(arg=="--practice-from-picker"){
            if(options->practice_from_picker)return false;
            options->practice_from_picker=true;
        }
        else if (arg=="--browse-borgs"||arg=="--demo-borg-browser"||arg=="--cheat-offer-picker"){
            if(options->browse_borgs)return false;
            options->browse_borgs=true;options->demo_borg_browser=arg=="--demo-borg-browser";
            options->cheat_offer_picker=arg=="--cheat-offer-picker";
        }
        else if(arg=="--collection-font"){
            if(++i>=argc||!options->collection_font_root.empty()||!*argv[i])return false;
            options->collection_font_root=argv[i];
        }
        else if(arg=="--recover-borgs"||arg=="--inspect-borg-force"||arg=="--browse-borg-force"||
                arg=="--browse-owned-borgs"||arg=="--place-owned-borg"){
            const bool inspect=arg=="--inspect-borg-force"||arg=="--browse-borg-force";
            const bool place=arg=="--place-owned-borg";
            if(argc-i<=3+int(inspect)+2*int(place)||!options->selected_save_root.empty())return false;
            options->selected_save_root=argv[++i];
            if(options->selected_save_root.empty()||!id(argv[++i],&options->selected_account)||!id(argv[++i],&options->selected_save))return false;
            if(inspect&&(!id(argv[++i],&options->inspect_force)||options->inspect_force>gf::kForceCount))return false;
            if(place&&(!id(argv[++i],&options->placement_force)||options->placement_force>gf::kForceCount||
                       !id(argv[++i],&options->placement_slot)||options->placement_slot>gf::kForceSlots))return false;
            options->browse_owned_borgs=arg=="--browse-owned-borgs"||place;
            options->browse_force=arg=="--browse-borg-force";
        }else if(arg=="--arrange-from-force"){
            if(options->arrange_from_force)return false;
            options->arrange_from_force=true;
        }else if(arg=="--force-slot"){
            if(++i>=argc||options->browse_force_slot||!id(argv[i],&options->browse_force_slot)||
               options->browse_force_slot>gf::kForceSlots)return false;
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
        }else if(arg=="--check-borg-controllers"){
            if(argc!=3||++i>=argc||!gf::borg_package_id_valid(argv[i]))return false;
            options->check_borg_controllers=argv[i];
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
        }else if(arg=="--preview-clip"){
            if(++i>=argc||!options->preview_clip.empty()||!gf::borg_catalog_variant_id_valid(argv[i]))return false;
            options->preview_clip=argv[i];
        }else if(arg=="--preview-origin"){
            if(++i>=argc||!options->preview_origin.empty()||!gf::borg_catalog_variant_id_valid(argv[i]))return false;
            options->preview_origin=argv[i];
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
                 arg == "--gred-walk-ledge-timed-relaunch-floor" || arg == "--demo-gred-walk-ledge-timed-relaunch-floor" ||
                 arg == "--gred-walk-ledge-timed-snap-dash" || arg == "--demo-gred-walk-ledge-timed-snap-dash" ||
                 arg == "--gred-walk-ledge-active-snap-relaunch" || arg == "--demo-gred-walk-ledge-active-snap-relaunch" ||
                 arg == "--gred-walk-ledge-timed-snap-handoff" || arg == "--demo-gred-walk-ledge-timed-snap-handoff" ||
                 arg == "--gred-walk-ledge-positive-snap-handoff" || arg == "--demo-gred-walk-ledge-positive-snap-handoff" ||
                 arg == "--gred-walk-ledge-expiring-snap-handoff" || arg == "--demo-gred-walk-ledge-expiring-snap-handoff" ||
                 arg == "--source-ground-fall-push" || arg == "--demo-source-ground-fall-push" ||
                 arg == "--source-ground-air-fall-push" || arg == "--demo-source-ground-air-fall-push" ||
                 arg == "--source-ground-relaunch-fall-push" || arg == "--demo-source-ground-relaunch-fall-push" ||
                 arg == "--source-ground-recovery-push" || arg == "--demo-source-ground-recovery-push" ||
                 arg == "--gred-recovery-completion-push" || arg == "--demo-gred-recovery-completion-push" ||
                 arg == "--gred-recovery-completion-exit" || arg == "--demo-gred-recovery-completion-exit" ||
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
                 arg == "--gred-inclined-dash-jump" || arg == "--demo-gred-inclined-dash-jump" ||
                 arg == "--gred-timed-incline-landing-dash" || arg == "--demo-gred-timed-incline-landing-dash" ||
                 arg == "--gred-inclined-dash-ledge" || arg == "--demo-gred-inclined-dash-ledge" ||
                 arg == "--gred-inclined-dash-ledge-relaunch" || arg == "--demo-gred-inclined-dash-ledge-relaunch" ||
                 arg == "--gred-inclined-walk-ledge" || arg == "--demo-gred-inclined-walk-ledge" ||
                 arg == "--gred-walk-ledge-fall-push" || arg == "--demo-gred-walk-ledge-fall-push" ||
                 arg == "--gred-walk-ledge-high-angle-push" || arg == "--demo-gred-walk-ledge-high-angle-push" ||
                 arg == "--gred-walk-ledge-timed-relaunch" || arg == "--demo-gred-walk-ledge-timed-relaunch" ||
                 arg == "--gred-walk-ledge-timed-air-dash" || arg == "--demo-gred-walk-ledge-timed-air-dash" ||
                 arg == "--gred-walk-ledge-direct-timed-air-dash" || arg == "--demo-gred-walk-ledge-direct-timed-air-dash" ||
                 arg == "--gred-walk-ledge-direct-timed-relaunch" || arg == "--demo-gred-walk-ledge-direct-timed-relaunch" ||
                 arg == "--gred-walk-ledge-direct-timed-up-relaunch" || arg == "--demo-gred-walk-ledge-direct-timed-up-relaunch" ||
                 arg == "--gred-walk-ledge-timed-release-push" || arg == "--demo-gred-walk-ledge-timed-release-push" ||
                 arg == "--gred-walk-ledge-timed-release-two-pushes" || arg == "--demo-gred-walk-ledge-timed-release-two-pushes" ||
                 arg == "--gred-walk-ledge-timed-release-three-pushes" || arg == "--demo-gred-walk-ledge-timed-release-three-pushes" ||
                 arg == "--gred-walk-ledge-timed-release-four-pushes" || arg == "--demo-gred-walk-ledge-timed-release-four-pushes" ||
                 arg == "--gred-walk-ledge-timed-release-expiry-drift" || arg == "--demo-gred-walk-ledge-timed-release-expiry-drift" ||
                 arg == "--gred-walk-ledge-rising-windup-relaunch" || arg == "--demo-gred-walk-ledge-rising-windup-relaunch" ||
                 arg == "--gred-walk-ledge-rising-diagonal-windup-relaunch" || arg == "--demo-gred-walk-ledge-rising-diagonal-windup-relaunch" ||
                 arg == "--gred-walk-ledge-same-face-expiry-push" || arg == "--demo-gred-walk-ledge-same-face-expiry-push" ||
                 arg == "--gred-walk-ledge-timed-handoff-relaunch" || arg == "--demo-gred-walk-ledge-timed-handoff-relaunch" ||
                 arg == "--gred-walk-ledge-expiry-handoff-relaunch" || arg == "--demo-gred-walk-ledge-expiry-handoff-relaunch" ||
                 arg == "--gred-walk-ledge-expiry-direct-relaunch" || arg == "--demo-gred-walk-ledge-expiry-direct-relaunch" ||
                 arg == "--gred-walk-ledge-expiring-direct-relaunch" || arg == "--demo-gred-walk-ledge-expiring-direct-relaunch" ||
                 arg == "--gred-walk-ledge-expiring-relaunch-push" || arg == "--demo-gred-walk-ledge-expiring-relaunch-push" ||
                 arg == "--gred-walk-ledge-expiring-adjacent-relaunch-push" || arg == "--demo-gred-walk-ledge-expiring-adjacent-relaunch-push" ||
                 arg == "--gred-walk-ledge-expiry-push" || arg == "--demo-gred-walk-ledge-expiry-push" ||
                 arg == "--gred-walk-ledge-expiry-air-dash" || arg == "--demo-gred-walk-ledge-expiry-air-dash" ||
                 arg == "--gred-walk-ledge-expiring-air-dash" || arg == "--demo-gred-walk-ledge-expiring-air-dash" ||
                 arg == "--gred-walk-ledge-expiring-air-dash-snap" || arg == "--demo-gred-walk-ledge-expiring-air-dash-snap" ||
                 arg == "--gred-walk-ledge-expiring-air-dash-snap-handoff" || arg == "--demo-gred-walk-ledge-expiring-air-dash-snap-handoff" ||
                 arg == "--gred-walk-ledge-expiring-air-dash-handoff" || arg == "--demo-gred-walk-ledge-expiring-air-dash-handoff" ||
                 arg == "--gred-walk-ledge-expiring-diagonal-dash-handoff" || arg == "--demo-gred-walk-ledge-expiring-diagonal-dash-handoff" ||
                 arg == "--gred-recovery-recontact" || arg == "--demo-gred-recovery-recontact" ||
                 arg == "--gred-recovery-recontact-drift" || arg == "--demo-gred-recovery-recontact-drift" ||
                 arg == "--gred-recovery-recontact-seam" || arg == "--demo-gred-recovery-recontact-seam" ||
                 arg == "--gred-recovery-first-contact-seam" || arg == "--demo-gred-recovery-first-contact-seam" ||
                 arg == "--demo-gred-walk-ledge-expiry-drift" || arg == "--demo-gred-walk-ledge-expiry-relaunch"){
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
            options->gred_positive_snap_windup_relaunch=arg=="--gred-walk-ledge-positive-snap-handoff"||arg=="--demo-gred-walk-ledge-positive-snap-handoff";
            options->gred_expiring_snap_windup_relaunch=arg=="--gred-walk-ledge-expiring-snap-handoff"||arg=="--demo-gred-walk-ledge-expiring-snap-handoff";
            options->gred_timed_snap_windup_relaunch=options->gred_positive_snap_windup_relaunch||options->gred_expiring_snap_windup_relaunch||
                arg=="--gred-walk-ledge-timed-snap-handoff"||arg=="--demo-gred-walk-ledge-timed-snap-handoff";
            options->gred_active_snap_relaunch=arg=="--gred-walk-ledge-active-snap-relaunch"||arg=="--demo-gred-walk-ledge-active-snap-relaunch";
            options->gred_timed_relaunch_snap_dash=options->gred_timed_snap_windup_relaunch||options->gred_active_snap_relaunch||
                arg=="--gred-walk-ledge-timed-snap-dash"||arg=="--demo-gred-walk-ledge-timed-snap-dash";
            options->gred_timed_relaunch_floor=options->gred_timed_relaunch_snap_dash||
                arg=="--gred-walk-ledge-timed-relaunch-floor"||arg=="--demo-gred-walk-ledge-timed-relaunch-floor";
            options->source_ground_loss_relaunch=options->gred_timed_relaunch_floor||
                arg=="--source-ground-loss-relaunch"||arg=="--demo-source-ground-loss-relaunch";
            options->source_ground_fall_push=arg=="--source-ground-fall-push"||arg=="--demo-source-ground-fall-push";
            options->source_ground_air_fall_push=arg=="--source-ground-air-fall-push"||arg=="--demo-source-ground-air-fall-push";
            options->source_ground_relaunch_fall_push=arg=="--source-ground-relaunch-fall-push"||arg=="--demo-source-ground-relaunch-fall-push";
            options->gred_recovery_completion_exit=arg=="--gred-recovery-completion-exit"||arg=="--demo-gred-recovery-completion-exit";
            options->gred_recovery_completion_push=options->gred_recovery_completion_exit||
                arg=="--gred-recovery-completion-push"||arg=="--demo-gred-recovery-completion-push";
            options->source_ground_recovery_push=options->gred_recovery_completion_push||
                arg=="--source-ground-recovery-push"||arg=="--demo-source-ground-recovery-push";
            options->source_ground_windup_push=arg=="--source-ground-windup-push"||arg=="--demo-source-ground-windup-push";
            options->source_ground_active_push=arg=="--source-ground-active-push"||arg=="--demo-source-ground-active-push";
            options->source_ground_relaunch_push=arg=="--source-ground-relaunch-push"||arg=="--demo-source-ground-relaunch-push";
            options->source_ground_relaunch_entry_push=arg=="--source-ground-relaunch-entry-push"||arg=="--demo-source-ground-relaunch-entry-push";
             options->demo_gred_walk_ledge_expiry_drift=arg=="--demo-gred-walk-ledge-expiry-drift";
             options->demo_gred_walk_ledge_expiry_relaunch=arg=="--demo-gred-walk-ledge-expiry-relaunch";
             options->gred_walk_ledge_expiry_air_dash=arg=="--gred-walk-ledge-expiry-air-dash"||arg=="--demo-gred-walk-ledge-expiry-air-dash";
             options->gred_walk_ledge_expiry_push=options->demo_gred_walk_ledge_expiry_drift||options->demo_gred_walk_ledge_expiry_relaunch||
                 options->gred_walk_ledge_expiry_air_dash||
                arg=="--gred-walk-ledge-expiry-push"||arg=="--demo-gred-walk-ledge-expiry-push";
             options->gred_walk_ledge_high_angle_push=options->gred_walk_ledge_expiry_push||
                 arg=="--gred-walk-ledge-high-angle-push"||arg=="--demo-gred-walk-ledge-high-angle-push";
            options->timed_ledge_followup=
                arg=="--gred-walk-ledge-timed-relaunch"||arg=="--demo-gred-walk-ledge-timed-relaunch"?TimedLedgeFollowup::Relaunch:
                arg=="--gred-walk-ledge-timed-air-dash"||arg=="--demo-gred-walk-ledge-timed-air-dash"?TimedLedgeFollowup::AirDash:
                 arg=="--gred-walk-ledge-direct-timed-air-dash"||arg=="--demo-gred-walk-ledge-direct-timed-air-dash"?TimedLedgeFollowup::DirectAirDash:
                 arg=="--gred-walk-ledge-expiring-air-dash"||arg=="--demo-gred-walk-ledge-expiring-air-dash"?TimedLedgeFollowup::ExpiringAirDash:
                 arg=="--gred-walk-ledge-expiring-air-dash-snap"||arg=="--demo-gred-walk-ledge-expiring-air-dash-snap"?TimedLedgeFollowup::ExpiringAirDashSnap:
                 arg=="--gred-walk-ledge-expiring-air-dash-snap-handoff"||arg=="--demo-gred-walk-ledge-expiring-air-dash-snap-handoff"?TimedLedgeFollowup::ExpiringAirDashSnapRelaunch:
                 arg=="--gred-walk-ledge-expiring-air-dash-handoff"||arg=="--demo-gred-walk-ledge-expiring-air-dash-handoff"?TimedLedgeFollowup::ExpiringAirDashRelaunch:
                 arg=="--gred-walk-ledge-expiring-diagonal-dash-handoff"||arg=="--demo-gred-walk-ledge-expiring-diagonal-dash-handoff"?TimedLedgeFollowup::ExpiringShortAirDashRelaunch:
                arg=="--gred-walk-ledge-direct-timed-relaunch"||arg=="--demo-gred-walk-ledge-direct-timed-relaunch"?TimedLedgeFollowup::DirectTimedRelaunch:
                arg=="--gred-walk-ledge-direct-timed-up-relaunch"||arg=="--demo-gred-walk-ledge-direct-timed-up-relaunch"?TimedLedgeFollowup::DirectTimedUpRelaunch:
                arg=="--gred-walk-ledge-timed-release-push"||arg=="--demo-gred-walk-ledge-timed-release-push"?TimedLedgeFollowup::TimedReleasePush:
                arg=="--gred-walk-ledge-timed-release-two-pushes"||arg=="--demo-gred-walk-ledge-timed-release-two-pushes"?TimedLedgeFollowup::TimedReleaseTwoPushes:
                arg=="--gred-walk-ledge-timed-release-three-pushes"||arg=="--demo-gred-walk-ledge-timed-release-three-pushes"?TimedLedgeFollowup::TimedReleaseThreePushes:
                arg=="--gred-walk-ledge-timed-release-four-pushes"||arg=="--demo-gred-walk-ledge-timed-release-four-pushes"?TimedLedgeFollowup::TimedReleaseFourPushes:
                arg=="--gred-walk-ledge-timed-release-expiry-drift"||arg=="--demo-gred-walk-ledge-timed-release-expiry-drift"?TimedLedgeFollowup::TimedReleaseExpiryDrift:
                arg=="--gred-walk-ledge-rising-windup-relaunch"||arg=="--demo-gred-walk-ledge-rising-windup-relaunch"?TimedLedgeFollowup::RisingWindupRelaunch:
                arg=="--gred-walk-ledge-rising-diagonal-windup-relaunch"||arg=="--demo-gred-walk-ledge-rising-diagonal-windup-relaunch"?TimedLedgeFollowup::RisingDiagonalWindupRelaunch:
                arg=="--gred-walk-ledge-same-face-expiry-push"||arg=="--demo-gred-walk-ledge-same-face-expiry-push"?TimedLedgeFollowup::SameFaceExpiryPush:
                arg=="--gred-walk-ledge-timed-handoff-relaunch"||arg=="--demo-gred-walk-ledge-timed-handoff-relaunch"?TimedLedgeFollowup::HandoffRelaunch:
                 arg=="--gred-walk-ledge-expiry-handoff-relaunch"||arg=="--demo-gred-walk-ledge-expiry-handoff-relaunch"?TimedLedgeFollowup::ExpiryHandoffRelaunch:
                  arg=="--gred-walk-ledge-expiry-direct-relaunch"||arg=="--demo-gred-walk-ledge-expiry-direct-relaunch"?TimedLedgeFollowup::ExpiryDirectRelaunch:
                  arg=="--gred-walk-ledge-expiring-direct-relaunch"||arg=="--demo-gred-walk-ledge-expiring-direct-relaunch"?TimedLedgeFollowup::ExpiringDirectRelaunch:
                  arg=="--gred-walk-ledge-expiring-relaunch-push"||arg=="--demo-gred-walk-ledge-expiring-relaunch-push"?TimedLedgeFollowup::ExpiringDirectRelaunchPush:
                  arg=="--gred-walk-ledge-expiring-adjacent-relaunch-push"||arg=="--demo-gred-walk-ledge-expiring-adjacent-relaunch-push"?TimedLedgeFollowup::ExpiringAdjacentRelaunchPush:
                    TimedLedgeFollowup::None;
            if(options->timed_ledge_followup==TimedLedgeFollowup::DirectAirDash||
                options->timed_ledge_followup==TimedLedgeFollowup::DirectTimedRelaunch||
                options->timed_ledge_followup==TimedLedgeFollowup::DirectTimedUpRelaunch)options->gred_walk_ledge_high_angle_push=true;
             if(options->timed_ledge_followup==TimedLedgeFollowup::ExpiryHandoffRelaunch||
                options->timed_ledge_followup==TimedLedgeFollowup::ExpiryDirectRelaunch){
                 options->gred_walk_ledge_high_angle_push=true;
                 options->gred_walk_ledge_expiry_push=true;
                 options->gred_walk_ledge_expiry_air_dash=options->timed_ledge_followup==TimedLedgeFollowup::ExpiryHandoffRelaunch;
            }
            options->gred_walk_ledge_fall_push=options->gred_walk_ledge_high_angle_push||options->timed_ledge_followup!=TimedLedgeFollowup::None||
                arg=="--gred-walk-ledge-fall-push"||arg=="--demo-gred-walk-ledge-fall-push";
             options->gred_inclined_dash_ledge_relaunch=arg=="--gred-inclined-dash-ledge-relaunch"||
                 arg=="--demo-gred-inclined-dash-ledge-relaunch";
             options->gred_inclined_dash_ledge=options->gred_walk_ledge_fall_push||options->gred_inclined_dash_ledge_relaunch||
                arg=="--gred-inclined-dash-ledge"||arg=="--demo-gred-inclined-dash-ledge";
            options->gred_inclined_walk_ledge=arg=="--gred-inclined-walk-ledge"||arg=="--demo-gred-inclined-walk-ledge";
            options->gred_inclined_dash_jump=arg=="--gred-inclined-dash-jump"||arg=="--demo-gred-inclined-dash-jump";
            options->gred_timed_incline_landing_dash=arg=="--gred-timed-incline-landing-dash"||arg=="--demo-gred-timed-incline-landing-dash";
            options->gred_slope_transition_dash=options->gred_inclined_dash_ledge||options->gred_inclined_walk_ledge||options->gred_inclined_dash_jump||
                options->gred_timed_incline_landing_dash||arg=="--gred-slope-transition-dash"||arg=="--demo-gred-slope-transition-dash";
            options->gred_slope_crease_dash=options->gred_slope_transition_dash||arg=="--gred-slope-crease-dash"||arg=="--demo-gred-slope-crease-dash";
            options->gred_slope_seam_dash=options->gred_slope_crease_dash||arg=="--gred-slope-seam-dash"||arg=="--demo-gred-slope-seam-dash";
            options->gred_slope_dash=options->gred_slope_seam_dash||arg=="--gred-slope-dash"||arg=="--demo-gred-slope-dash";
            options->gred_recovery_first_contact_seam=arg=="--gred-recovery-first-contact-seam"||arg=="--demo-gred-recovery-first-contact-seam";
            options->gred_recovery_recontact_seam=options->gred_recovery_first_contact_seam||
                arg=="--gred-recovery-recontact-seam"||arg=="--demo-gred-recovery-recontact-seam";
            options->gred_recovery_recontact_drift=options->gred_recovery_recontact_seam||
                arg=="--gred-recovery-recontact-drift"||arg=="--demo-gred-recovery-recontact-drift";
            options->gred_recovery_recontact=options->gred_recovery_recontact_drift||
                arg=="--gred-recovery-recontact"||arg=="--demo-gred-recovery-recontact";
            options->gred_practice=options->gred_recovery_recontact||options->gred_recovery_completion_push||options->gred_timed_relaunch_floor||
                options->gred_slope_dash||arg=="--gred-movement"||arg=="--demo-gred-movement";
            options->source_ground_windup_entry_push=(options->gred_practice&&!options->gred_slope_dash&&
                !options->gred_recovery_recontact&&!options->gred_recovery_completion_push&&!options->gred_timed_relaunch_floor)||
                arg=="--source-ground-windup-entry-push"||arg=="--demo-source-ground-windup-entry-push";
            options->source_ordered_ground=arg=="--source-ordered-ground"||arg=="--demo-source-ordered-ground"||options->source_dash_ledge||options->source_ground_ledge||options->source_ground_jump||options->source_ground_landing||options->source_ground_air||options->source_ground_air_floor||options->source_ground_relaunch||options->source_ground_relaunch_floor||options->source_ground_windup_entry_floor||options->source_ground_surface||options->source_ground_loss_surface||options->source_ground_loss_jump||options->source_ground_loss_landing||options->source_ground_loss_air||options->source_ground_loss_relaunch||options->source_ground_fall_push||options->source_ground_air_fall_push;
            if(options->source_ordered_ground)options->source_ground_interactive=arg=="--source-ordered-ground"||arg=="--source-dash-ledge"||arg=="--source-ground-ledge"||arg=="--source-ground-jump"||arg=="--source-ground-landing"||arg=="--source-ground-air"||arg=="--source-ground-air-floor"||arg=="--source-ground-relaunch"||arg=="--source-ground-relaunch-floor"||arg=="--source-ground-windup-entry-floor"||arg=="--source-ground-surface"||arg=="--source-ground-loss-surface"||arg=="--source-ground-loss-jump"||arg=="--source-ground-loss-landing"||arg=="--source-ground-loss-air"||arg=="--source-ground-loss-relaunch"||arg=="--source-ground-fall-push"||arg=="--source-ground-air-fall-push";
            options->source_windup_entry_push=arg=="--source-windup-entry-push"||arg=="--demo-source-windup-entry-push";
            if(options->gred_recovery_recontact||options->gred_slope_dash||options->gred_timed_relaunch_floor||options->source_ground_relaunch_fall_push||options->source_ground_recovery_push||options->source_ground_windup_push||options->source_ground_active_push||options->source_ground_relaunch_push||options->source_ground_relaunch_entry_push||options->source_ground_windup_entry_push){
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
        else if (arg == "--battle-mvp-assets" && i+1<argc){
            options->battle_mvp_assets=argv[++i];options->explicit_battle_mvp_assets=true;
        }
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
                          "       gf_native --borg-controls | --demo-borg-controls [--no-audio]\n"
                          "       gf_native --battle-borg ID --against-borg ID --battle-picker [--battle-mvp-assets DIR]\n"
                          "       gf_native --borg-controls --walk-borg NAMESPACE:ID --borg-variant VARIANT [--lab-jump] [--lab-dash] [--battle-mvp-assets DIR]\n"
                          "                 [--lab-air-dash | --lab-glide] [--lab-landing-audio] (require --lab-jump)\n"
                           "               or --lab-cruise (exact flight profile, no jump or dash flags)\n"
                          "                 [--lab-source-pace] (grounded travel only, not cruise)\n"
                          "       gf_native --demo-borg-controls --walk-borg NAMESPACE:ID --borg-variant VARIANT [--demo-lab-picker]\n"
                         "       gf_native --check-borg-moves NAMESPACE:ID\n"
                         "       gf_native --check-borg-spawn NAMESPACE:ID\n"
                         "       gf_native --check-borg-controllers NAMESPACE:ID\n"
                         "       gf_native --check-borg-effects NAMESPACE:ID\n"
                         "       gf_native --check-borg-forms NAMESPACE:ID\n"
                         "       gf_native --check-borg-fusions NAMESPACE:ID\n"
                         "       gf_native --check-borg-acquisition NAMESPACE:ID\n"
                         "       gf_native --check-borg-variants NAMESPACE:ID\n"
                          "       gf_native --browse-borgs [--collection-font FONT_ROOT] [--headless] [--frames N]\n"
                         "       gf_native --recover-borgs ROOT ACCOUNT SAVE [--recovery-notice INSTANCE] [--headless]\n"
                          "       gf_native --browse-owned-borgs ROOT ACCOUNT SAVE [--owned-instance INSTANCE] [--practice-from-picker]\n"
                         "       gf_native --place-owned-borg ROOT ACCOUNT SAVE FORCE SLOT [--owned-instance INSTANCE] [--retail-catalog FILE]\n"
                          "       gf_native --inspect-borg-force ROOT ACCOUNT SAVE FORCE [--retail-catalog FILE]\n"
                          "       gf_native --browse-borg-force ROOT ACCOUNT SAVE FORCE [--force-slot SLOT] [--retail-catalog FILE]\n"
                          "                 [--arrange-from-force | --practice-from-picker]\n"
                          "       gf_native --cheat-offer-picker (metadata offers only; no ownership or save changes)\n"
                          "       gf_native --preview-borg NAMESPACE:ID [--borg-variant ID] [--preview-clip ID] [--preview-origin ID] [--headless] [--frames N]\n"
                          "       gf_native --battle-borg NAMESPACE:ID --against-borg NAMESPACE:ID [--demo-package-battle] [--demo-roster-battle] [--trace-package-battle]\n"
                           "       [--roster-borg TEAM NAMESPACE:ID ...] (up to two CPU fighters per team)\n"
                           "       gf_native --battle-roster FILE [--trace-package-battle] [--headless]\n"
                           "       gf_native --list-battle-rosters DIRECTORY\n"
                           "       gf_native --browse-battle-rosters DIRECTORY [--size W H] [--no-audio]\n"
                           "       gf_native --check-battle-roster FILE (validate and draw tick zero headless)\n"
                             "       [--package-damage-indices TEAM0,TEAM1:R0,R1[,R2...][:B0,B1[,B2...]]]\n"
                             "       [--package-ko-force TEAM0,TEAM1 (standalone KO accounting only)]\n"
                           "       gf_native --list-stages\n"
                           "       gf_native --list-challenges\n"
                           "       gf_native --run-challenge NAMESPACE:ID ENCOUNTER SAVE_ROOT ACCOUNT SAVE FORCE [--retail-catalog FILE]\n"
                           "       gf_native --list-stories\n"
                            "       gf_native --story-map NAMESPACE:ID [--headless] [--frames N] [--size W H]\n"
                            "       gf_native --run-story NAMESPACE:ID SAVE_ROOT ACCOUNT SAVE FORCE [--retail-catalog FILE]\n"
                          "       gf_native --battle-borg NAMESPACE:ID --borg-variant VARIANT --against-borg NAMESPACE:ID --against-variant VARIANT\n"
                           "                 --battle-stage NAMESPACE:ID --stage-spawns P1_MARKER P2_MARKER [CPU_MARKER ...]\n");
            return false;
        }
    }
    if(!options->browse_battle_rosters.empty()||!options->demo_roster_select.empty()||!options->demo_roster_next.empty()||options->demo_roster_pad){
        const auto& scripted=options->demo_roster_select;
        const auto& followup=options->demo_roster_next;
        const auto valid_row=[](std::string_view name){
            return name.ends_with(".roster")&&name.find('/')==std::string_view::npos&&gf::borg_package_asset_path_valid(name);
        };
        if(options->browse_battle_rosters.empty()||!roster_browser_flags_only||
           options->width<200||options->width>4096||options->height<160||options->height>4096||
           ((options->headless||options->demo_roster_pad)&&scripted.empty())||
           (!scripted.empty()&&(!options->frames||options->frames>240||!valid_row(scripted)))||
           (!followup.empty()&&(scripted.empty()||!valid_row(followup)))){
            std::fputs("Roster browser: unsupported options or scripted selection\n",stderr);return false;
        }
    }
    if(!options->battle_roster_file.empty()){
        if(!roster_file_flags_only){
            std::fputs("Borg Battle Test preset: cannot combine --battle-roster with other modes or match overrides\n",stderr);
            return false;
        }
        gf::BattleRosterPreset preset;
        std::string error;
        if(!gf::pc::load_battle_roster_preset(options->battle_roster_file,&preset,&error)){
            std::fprintf(stderr,"Borg Battle Test preset: %s\n",error.c_str());return false;
        }
        if(!apply_battle_roster_preset(*options,std::move(preset))){
            std::fputs("Borg Battle Test preset: damage indices supplied by both the preset and the command line\n",stderr);
            return false;
        }
    }
    if(!options->story_map.empty()&&(!story_map_flags_only||options->width<112||options->height<112||
       options->width>4096||options->height>4096||(options->headless&&!options->screenshot.empty())))return false;
    if(options->portable_story&&(!story_run_flags_only||options->width<320||options->height<240||
       options->width>4096||options->height>4096))return false;
    if(options->portable_challenge&&(!challenge_flags_only||options->width<320||options->height<240||
       options->width>4096||options->height>4096||(options->headless&&!options->screenshot.empty())))return false;
    if(unsigned(options->demo_input)+unsigned(options->demo_option)+unsigned(options->demo_credits)>1)return false;
    if(options->test_battle&&(options->menu||options->demo_input||options->demo_option||options->demo_credits||options->probe_texture>=0))return false;
    if(options->battle_mvp&&(options->test_battle||options->menu||options->demo_input||options->demo_option||options->demo_credits||options->probe_texture>=0))return false;
    if(options->animation_mosaic&&(options->battle_mvp||options->test_battle||options->menu||options->demo_input||options->demo_option||options->demo_credits||options->probe_texture>=0))return false;
    if(!options->preview_borg.empty()&&(options->animation_mosaic||options->battle_mvp||options->test_battle||options->menu||
       options->demo_input||options->demo_option||options->demo_credits||options->probe_texture>=0||options->list_borgs||
       !options->check_borg_catalog.empty()||!options->check_borg_moves.empty()||!options->check_borg_spawn.empty()||
       !options->check_borg_effects.empty()||!options->check_borg_forms.empty()||!options->check_borg_fusions.empty()||
       !options->check_borg_acquisition.empty()||!options->check_borg_variants.empty()||options->animation_page||options->animation_borg))return false;
    if(!options->borg_variant.empty()&&options->preview_borg.empty()&&options->practice_borg.empty()&&options->battle_borg.empty()&&options->walk_borg.empty())return false;
    if(!options->walk_borg.empty()&&(!options->borg_controls||options->borg_variant.empty()||options->gred_practice||
       options->portable_story||options->portable_challenge||options->cheat_offer_picker||options->placement_force||
       options->owned_practice||options->force_practice||!options->source_ground_assets.empty()||
       options->list_stages||options->list_stories||options->list_challenges||!options->story_map.empty()))return false;
    if((options->lab_jump||options->lab_dash)&&options->walk_borg.empty())return false;
    if(options->demo_lab_picker&&(!options->demo_borg_controls||options->walk_borg.empty()))return false;
    if(options->lab_source_pace&&(options->walk_borg.empty()||options->lab_cruise))return false;
    if(((options->lab_landing_audio||options->lab_air_dash||options->lab_glide)&&!options->lab_jump)||
       (options->lab_glide&&options->lab_air_dash)||
       (options->lab_cruise&&(options->walk_borg.empty()||options->lab_jump||options->lab_dash)))return false;
    if(options->battle_borg.empty()!=options->against_borg.empty())return false;
    if(!options->roster_borgs.empty()&&(options->battle_borg.empty()||options->demo_roster_battle||
       options->battle_picker||options->exercise_gred_touchdown))return false;
    const unsigned battle_actor_count=options->demo_roster_battle?4u:2u+unsigned(options->roster_borgs.size());
    if(options->package_damage_indices&&(options->battle_borg.empty()||options->battle_picker||
        !options->package_damage_indices->valid(battle_actor_count)))return false;
    if(options->package_ko_force&&(options->battle_borg.empty()||options->battle_picker||
        options->portable_story||options->portable_challenge||options->demo_roster_battle))return false;
    if(options->exercise_battle_mobility&&!options->demo_package_battle)return false;
    if(options->exercise_gred_touchdown&&(!options->demo_package_battle||!options->frames||options->frames>240))return false;
    if(options->battle_picker&&(options->battle_borg.empty()||options->demo_package_battle||
       !options->battle_stage.empty()||options->width<160||options->height<120||options->width>4096||options->height>4096||
       (options->headless&&options->demo_battle_pick.empty())))return false;
    if(!options->demo_battle_pick.empty()&&(!options->battle_picker||!options->frames||options->frames>240||
       !gf::borg_package_id_valid(options->demo_battle_pick)))return false;
    if(options->battle_stage.empty()!=options->battle_stage_spawns.empty())return false;
    if(!options->battle_stage.empty()&&(options->battle_borg.empty()||options->borg_variant.empty()||
        options->against_variant.empty()||options->explicit_battle_mvp_assets||!options->source_ground_assets.empty()||
        options->package_battle_distance||options->demo_roster_battle))return false;
    if(!options->battle_stage.empty()&&options->battle_stage_spawns.size()!=battle_actor_count){
        std::fprintf(stderr,"--stage-spawns requires one marker per actor (%u), in slot order\n",battle_actor_count);
        return false;
    }
    if(options->battle_borg.empty()){
        if(options->demo_package_battle||(!options->portable_challenge&&options->browse_battle_rosters.empty()&&options->trace_package_battle)||
           options->package_battle_distance||!options->against_variant.empty())return false;
    }else{
        if(options->battle_mvp||options->test_battle||options->borg_controls||options->menu||options->animation_mosaic||
           options->browse_borgs||!options->selected_save_root.empty()||options->owned_practice||options->force_practice||
           !options->practice_borg.empty()||!options->preview_borg.empty()||options->list_borgs||options->probe_texture>=0||
           options->demo_input||options->demo_option||options->demo_credits||
           !options->check_borg_catalog.empty()||!options->check_borg_moves.empty()||!options->check_borg_spawn.empty()||
           !options->check_borg_effects.empty()||!options->check_borg_forms.empty()||!options->check_borg_fusions.empty()||
           !options->check_borg_acquisition.empty()||!options->check_borg_variants.empty()||!options->check_borg_controllers.empty())return false;
        if(options->demo_package_battle&&!options->frames&&!options->seconds)options->frames=150;
    }
    if((!options->preview_clip.empty()||!options->preview_origin.empty())&&options->preview_borg.empty())return false;
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
        !options->check_borg_variants.empty()||options->width<160||options->height<120||options->width>4096||options->height>4096))return false;
    if(options->recovery_notice&&(options->selected_save_root.empty()||options->inspect_force||options->browse_owned_borgs))return false;
    if(options->owned_instance&&!options->browse_owned_borgs)return false;
    if(options->browse_force_slot&&!options->browse_force)return false;
    if(options->arrange_from_force&&(!options->browse_force||options->practice_from_picker))return false;
    if(options->practice_from_picker&&((!options->browse_owned_borgs&&!options->browse_force)||options->placement_force))return false;
    if(!options->collection_font_root.empty()&&(!(options->browse_borgs||options->browse_owned_borgs)||
       options->practice_from_picker||options->placement_force||options->cheat_offer_picker||
       options->list_stages||options->list_stories))return false;
    if(!options->retail_force_catalogue.empty()&&!options->inspect_force&&!options->placement_force&&
        !options->force_practice&&!options->portable_challenge&&!options->portable_story)return false;
    if(!options->selected_save_root.empty()&&(options->animation_mosaic||options->battle_mvp||options->test_battle||
       options->menu||options->demo_input||options->demo_option||options->demo_credits||options->probe_texture>=0||
       !options->preview_borg.empty()||options->browse_borgs||options->list_borgs||!options->check_borg_catalog.empty()||
       !options->check_borg_moves.empty()||!options->check_borg_spawn.empty()||!options->check_borg_effects.empty()||
       !options->check_borg_forms.empty()||!options->check_borg_fusions.empty()||!options->check_borg_acquisition.empty()||
         !options->check_borg_variants.empty()||((!options->inspect_force||options->browse_force)&&
          (options->width<(options->browse_owned_borgs&&!options->practice_from_picker?160u:520u)||
           options->height<(options->browse_owned_borgs&&!options->practice_from_picker?120u:360u)||
          options->width>4096||options->height>4096))))return false;
    if(!options->animation_export.empty()&&(!options->animation_mosaic||options->headless||options->animation_export_frames<2||options->animation_export_frames>120))return false;
    if(!options->animation_export.empty()&&(options->width>2048||options->height>2048||options->width<320||options->height<240))return false;
    if(options->animation_programs.empty()!=options->animation_height_profile.empty())return false;
    if(!options->animation_programs.empty()&&(!options->animation_mosaic||options->animation_borg||!options->animation_export.empty()))return false;
    if(!options->animation_motion_profile.empty()&&options->animation_programs.empty())return false;
    if(options->demo_source_ground&&(options->demo_battle_mvp||options->demo_locomotion))return false;
    if(options->gred_practice_start&&(!options->gred_practice||!options->source_ground_interactive))return false;
    if(!options->source_ground_assets.empty()&&!options->demo_source_ground&&!options->borg_controls&&options->battle_borg.empty())return false;
    if(options->borg_controls&&(options->battle_mvp||options->test_battle||options->menu||!options->preview_borg.empty()||
       options->animation_mosaic||options->browse_borgs||options->list_borgs||!options->selected_save_root.empty()||
       options->demo_input||options->demo_option||options->demo_credits||options->probe_texture>=0||
       !options->check_borg_catalog.empty()||!options->check_borg_moves.empty()||!options->check_borg_spawn.empty()||
       !options->check_borg_effects.empty()||!options->check_borg_forms.empty()||!options->check_borg_fusions.empty()||
       !options->check_borg_acquisition.empty()||!options->check_borg_variants.empty()||!options->check_borg_controllers.empty()||
       options->width<440||options->width>4096||options->height<320||options->height>4096))return false;
    if(options->demo_borg_controls&&!options->frames&&!options->seconds)options->frames=options->lab_dash?216:144;
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

int run_borg_walk_controls(gf::Context& context,gf::RenderEngine& render,const Options& options,
                          const gf::BorgPackageCatalog& catalogue){
    gf::NativeBattleMvp world;gf::NativeBattleMvpView view;
    gf::pc::BorgPackagePreviewFiles files;using Asset=gf::pc::BorgPackagePreviewFiles;
    std::string error;
    auto fail=[&](const char* stage){gf::log_error(context,"walk-lab","%s: %s",stage,error.c_str());render.shutdown();return 2;};
    const gf::pc::BorgLabModes modes{options.lab_jump,options.lab_dash,options.lab_air_dash,options.lab_glide,options.lab_cruise,
        options.lab_landing_audio,options.lab_source_pace};
    const bool jump=modes.jump,dash=modes.dash,air_dash=modes.air_dash,glide=modes.glide;
    gf::BorgMovementSoundBank sounds;
    auto& landing_sample=sounds[2];
    gf::BattleMvpConfig config;
    auto bind=[&](std::string_view id,std::string_view variant)->const char*{
        error.clear();
        Asset pending;
        if(!gf::pc::load_borg_preview_files("borgs",catalogue,id,variant,&pending,&error,modes.landing_audio))
            return "exact package refused";
        std::vector<gf::s16> sample;
        if(modes.landing_audio&&(!gf::borg_movement_sound_decode(pending.landing_pcm,&sample)||
           std::none_of(sample.begin(),sample.end(),[](gf::s16 n){return n!=0;}))){
            error="expected a bounded, nonempty stereo s16le landing sample";return "lab audio refused";
        }
        gf::pc::BorgLabMapping map;const char* stage=nullptr;
        if(!gf::pc::borg_lab_mapping(pending.profile,modes,&map,&stage,&error))return stage;
        if(!view.load_provisional_walk(context.files(),options.battle_mvp_assets,render,
                                      Asset::view(pending.model),Asset::view(pending.rig_prefix),Asset::view(pending.motions),
                                      map.ground,map.config,&error,map.jump?&*map.jump:nullptr,pending.profile.mounts,
                                      map.dash?&*map.dash:nullptr,map.air_dash?&*map.air_dash:nullptr,
                                       map.glide?&*map.glide:nullptr,map.cruise_entry?&*map.cruise_entry:nullptr))
            return "walk-lab assets refused";
        config=map.config;landing_sample=std::move(sample);files=std::move(pending);
        return nullptr;
    };
    if(const auto refused=bind(options.walk_borg,options.borg_variant))return fail(refused);
    const bool audible=options.lab_landing_audio&&!options.no_audio&&!options.headless&&
                       context.audio().open(gf::kBorgMovementSampleRate,2);
    struct CloseLabAudio {gf::IAudioBackend* device;~CloseLabAudio(){if(device)device->close();}}
        close_lab_audio{audible?&context.audio():nullptr};
    const std::array banks{gf::BorgAudioSlot{.movement=&sounds}};
    gf::BorgMovementAudio movement_audio;
    std::vector<gf::s16> mixed;
    unsigned audio_cues=0,audio_blocks=0;
    bool landed_this_run=false,audio_nonzero=false;
    gf::u64 submitted_frames=0;
    auto reset=[&](){
        view.reset_control_test_pose();
        movement_audio.reset();
        landed_this_run=false;
        if(audible)context.audio().discard_queued();
        return world.start(config,&view)&&view.update(world,&error);
    };
    if(!reset())return fail("initial body refused");
    auto spawn=world.actors()[0].position;
    std::optional<gf::BorgLabPicker> picker;
    std::size_t first_pick=0,second_pick=0;
    if((!options.headless&&!options.demo_borg_controls)||options.demo_lab_picker){
        auto rows=gf::pc::borg_lab_rows("borgs",catalogue,modes);
        const auto current=std::find_if(rows.begin(),rows.end(),[&](const auto& r){
            return r.id==files.identity.id&&r.variant==files.profile.variant;
        });
        if(current!=rows.end()){
            first_pick=std::size_t(current-rows.begin());
            const auto page=first_pick/gf::BorgLabPicker::page_size;
            const auto begin=page*gf::BorgLabPicker::page_size;
            second_pick=first_pick==begin?begin+1:begin;
            if(options.demo_lab_picker&&(second_pick>=rows.size()||second_pick/gf::BorgLabPicker::page_size!=page)){
                error="picker smoke requires two mapped looks on the initial page";return fail("lab picker refused");
            }
            picker.emplace(std::move(rows));picker->activate(first_pick);
            gf::log_info(context,"walk-lab","picker looks=%zu (declared mode; assets checked on selection)",picker->entries().size());
        }else if(options.demo_lab_picker){error="initial exact look is not listed";return fail("lab picker refused");}
    }
    unsigned picker_switches=0,picker_refusals=0;
    std::string picker_notice;
    bool picker_block_buttons=false,previous_click=false;
    gf::InputRouter router;
    const auto started=context.clock().monotonic_ns();auto previous=started;gf::u64 phase=0;
    unsigned frame=0,takeoffs=0,landings=0,air_presses=0,air_dash_presses=0,dash_starts=0,resets=0,paused_frames=0;
    unsigned air_bursts=0,spent_air_presses=0;
    unsigned glide_starts=0,glide_releases=0,glide_steps=0;
    unsigned boost_entries=0,airborne_refill_ticks=0;
    bool boost_depleted=false,boost_speed=false,boost_full_after_depletion=false;
    gf::f32 ground_peak=0;
    const unsigned jump_at=dash?150:8,air_press_at=jump_at+6;
    const unsigned air_burst_at=jump_at+10,spent_air_at=jump_at+32;
    const unsigned pause_at=dash?200:70,restart_at=dash?210:85,smoke_frames=dash?216:90;
    bool moved=false,captured=false,paused=false,restart_held=false,pause_held=false;
    std::array<bool,unsigned(gf::NativeBorgMotion::Count)> poses{};
    while(context.window().pump_events()&&!context.window().should_close()){
        const auto now=context.clock().monotonic_ns();
        if(options.seconds&&now-started>=gf::u64(options.seconds)*1000000000ull)break;
        context.input().pump();const auto& snapshot=context.input().snapshot();auto pad=router.route(snapshot,0);
        if(snapshot.quit||pad.quit_requested)break;
        bool picker_pause=false;
        if(picker){
            auto pointer=snapshot.pointer;
            pointer.left_pressed|=pointer.left&&!previous_click;previous_click=pointer.left;
            unsigned logical_w=options.width,logical_h=options.height;context.window().logical_size(&logical_w,&logical_h);
            pointer.x=logical_w?gf::s32(gf::s64(pointer.x)*options.width/logical_w):-1;
            pointer.y=logical_h?gf::s32(gf::s64(pointer.y)*options.height/logical_h):-1;
            if(options.demo_lab_picker){
                pointer={};pointer.within_window=true;pointer.x=gf::BorgLabPicker::left+8;
                pointer.left_pressed=frame<4;
                const auto index=frame==1?second_pick:first_pick;
                pointer.y=gf::BorgLabPicker::top+8+(frame%2?gf::s32(index%gf::BorgLabPicker::page_size+1)*gf::BorgLabPicker::row_height:0);
            }
            const bool was_open=picker->open();
            const auto choice=picker->step(pointer,options.width);
            picker_pause=was_open||picker->open();
            if(!was_open&&picker->open()&&audible)context.audio().discard_queued();
            if(picker_pause)picker_block_buttons=true;
            if(choice){
                const auto& target=picker->entries()[*choice];
                picker_notice.clear();
                if(target.id!=files.identity.id||target.variant!=files.profile.variant){
                    if(const auto refused=bind(target.id,target.variant)){
                        ++picker_refusals;picker_notice="SELECTED LOOK REFUSED - CURRENT BORG UNCHANGED";
                        gf::log_warn(context,"walk-lab","picker refused id=%s variant=%s: %s: %s",
                            target.id.c_str(),target.variant.c_str(),refused,error.c_str());
                        error.clear();
                    }else{
                        if(!reset())return fail("selected look reset refused");
                        spawn=world.actors()[0].position;
                        ground_peak=0;
                        picker->activate(*choice);paused=false;++picker_switches;
                        gf::log_info(context,"walk-lab","picker active id=%s variant=%s actors=%u tick=%u",
                            files.identity.id.c_str(),files.profile.variant.c_str(),world.actor_count(),world.tick());
                    }
                }
            }
        }
        unsigned steps=1;
        if(options.demo_borg_controls){
            pad={};pad.a=pad.x=true; // always press the disabled attacks
            if(dash){
                pad.dpad_up=(frame>=4&&frame<10)||frame==16;
                pad.dpad_left=frame==50;pad.dpad_down=frame==84;pad.dpad_right=frame==118;
                pad.l=frame==16||frame==50||frame==84||frame==118||(jump&&!air_dash&&frame==air_press_at);
            }else{
                pad.dpad_up=frame>=4&&frame<50;pad.dpad_right=frame>=55&&frame<95;pad.l=!air_dash;
            }
            if(air_dash&&(frame==air_burst_at||frame==spent_air_at))pad.l=pad.dpad_up=true;
            pad.b=!jump||frame==jump_at||frame==air_press_at||
                (glide&&frame>air_press_at&&frame<jump_at+37);
            if(config.provisional_cruise){
                pad.b=frame>=16&&frame<48;pad.dpad_up=false;pad.dpad_right=frame>=3&&frame<12;
            }
            pad.start=(frame>=pause_at&&frame<pause_at+3)||frame==pause_at+4;
            pad.y=frame>=restart_at&&frame<restart_at+2;
        }else if(!options.headless){
            phase+=(now>=previous?std::min<gf::u64>(now-previous,250000000):0)*60;
            steps=unsigned(std::min<gf::u64>(phase/1000000000ull,8));phase%=1000000000ull;
        }
        previous=now;
        const auto before_tick=world.tick();
        const bool restart=pad.y&&!restart_held,pause=pad.start&&!pause_held;
        restart_held=pad.y;pause_held=pad.start;
        if(restart&&!picker_pause){
            if(!reset())return fail("restart refused");
            if(world.tick()||world.actors()[0].position!=spawn)return fail("restart did not restore spawn");
            paused=false;++resets;
        }else if(pause&&!picker_pause){
            paused=!paused;
            if(paused&&audible)context.audio().discard_queued();
        }
        if(paused||pause||restart||picker_pause){steps=0;phase=0;}
        if(picker_block_buttons){
            const bool active_b=(jump||modes.cruise)&&pad.b;
            const bool active_l=(dash||air_dash)&&(pad.l||pad.trigger_l_pressed);
            if(!picker_pause&&!active_b&&!active_l)picker_block_buttons=false;
            else pad.b=pad.l=pad.trigger_l_pressed=false;
        }
        pad.y=pad.start=false;
        while(steps--){
            const auto before=world.actors()[0].position;
            const bool was_grounded=world.actors()[0].grounded;
            const bool was_dashing=world.actors()[0].dash_ticks!=0;
            const bool was_gliding=world.actors()[0].lab_gliding;
            const bool was_boosting=world.actors()[0].package_boosting;
            const auto old_boost=world.actors()[0].package_boost_meter.value736;
            const auto vertical_speed=world.actors()[0].vertical_speed;
            const auto movement=gf::NativeBattleMvpView::movement_frame(world,options.width,options.height);
            if(!world.step({pad,{}},false,&view,nullptr,&movement)||!view.update(world,&error))return fail("movement refused");
            const auto& body=world.actors()[0];
            if(world.actor_count()!=1||(!jump&&!config.provisional_cruise&&!body.grounded)||body.attack_ticks||
               (body.dash_ticks&&(body.dash_airborne?(!air_dash||body.grounded):(!dash||!body.grounded)))||
               (body.lab_gliding&&(!glide||body.grounded||!pad.b))||
               world.shots_spawned()||world.hits()){
                error="no-combat walk-lab invariant violated";return fail("session stopped");
            }
            if(config.provisional_cruise){
                const auto meter=body.package_boost_meter.value736;
                if(body.grounded||body.landing_ticks||body.takeoff_ticks||body.dash_ticks||
                   meter<0||meter>config.provisional_cruise->capacity||
                   (was_boosting&&body.package_boosting&&meter>old_boost)){
                    error="cruise must remain airborne with bounded nonboosting refill";return fail("cruise invariant refused");
                }
                boost_entries+=unsigned(!was_boosting&&body.package_boosting);
                boost_depleted|=meter==0;
                boost_speed|=body.package_boosting&&body.movement_speed>config.move_speed;
                airborne_refill_ticks+=unsigned(!body.package_boosting&&meter>old_boost);
                boost_full_after_depletion|=boost_depleted&&meter==config.provisional_cruise->capacity;
            }
            if(config.provisional_ground_pace&&was_grounded&&body.grounded&&!was_dashing&&!body.dash_ticks&&
               (!dash||(!pad.l&&!pad.trigger_l_pressed))){
                ground_peak=std::max(ground_peak,body.movement_speed);
                if(ground_peak>gf::f32(*config.provisional_ground_pace)*.01f+.001f){
                    error="ground speed exceeded the declared lab pace";return fail("ground pace refused");
                }
            }
            if(options.demo_borg_controls&&jump&&frame==air_press_at){
                if(was_grounded||body.grounded||body.takeoff_ticks||
                   std::abs(body.vertical_speed-(vertical_speed-config.gravity))>.0001f){
                    error="airborne press must leave ballistic motion unchanged";return fail("jump smoke failed");
                }
                ++air_presses;
                air_dash_presses+=unsigned(dash&&pad.l&&!body.dash_ticks);
            }
            if(options.demo_borg_controls&&air_dash&&frame==spent_air_at){
                if(body.grounded||body.dash_ticks||body.lab_air_windup_ticks||body.air_dashes_used!=1){
                    error="second airborne dash must refuse after spending the lab allowance";return fail("air dash smoke failed");
                }
                ++spent_air_presses;
            }
            if(!was_dashing&&body.dash_ticks){
                if(body.dash_airborne)++air_bursts;
                else ++dash_starts;
            }
            if(body.lab_gliding){
                const auto drop=before[1]-body.position[1];
                if(drop<0||drop>gf::BattleMvpActor::lab_glide_sink+.0001f){
                    error="glide must descend without hover lift";return fail("glide smoke failed");
                }
                glide_starts+=unsigned(!was_gliding);++glide_steps;
            }
            glide_releases+=unsigned(was_gliding&&!body.lab_gliding&&!pad.b&&!body.grounded);
            takeoffs+=unsigned(was_grounded&&!body.grounded&&body.takeoff_ticks);
            const bool landed=!was_grounded&&body.grounded&&body.landing_ticks;
            landings+=unsigned(landed);
            if(options.lab_landing_audio){
                const auto cue=landed?gf::u8(gf::BorgMovementCue::Landing):0;
                if(body.package_movement_cues!=cue||!movement_audio.visit(world,banks,&mixed)){
                    error="landing cue/mixer transaction refused";return fail("lab audio refused");
                }
                audio_cues+=unsigned(landed);
                landed_this_run|=landed;
                if(!mixed.empty()){
                    if(!landed_this_run){error="mixed before accepted landing";return fail("lab audio refused");}
                    ++audio_blocks;
                    audio_nonzero|=std::any_of(mixed.begin(),mixed.end(),[](gf::s16 sample){return sample!=0;});
                    if(audible&&context.audio().frames_available()<gf::kBorgMovementAudioFrames*3&&
                       context.audio().try_submit(mixed.data(),mixed.size()/2))submitted_frames+=mixed.size()/2;
                }
            }
            moved|=before!=body.position;
        }
        if(paused){
            if(world.tick()!=before_tick)return fail("paused tick advanced");
            ++paused_frames;
        }
        render.begin_frame({frame,.15f,.21f,.3f});render.reset_arena();
        const auto draws=view.draw(render,world,options.width,options.height,&error);
        if(picker)picker->draw(render,options.width,picker_notice);
        render.end_frame();
        if(!draws)return fail("draw refused");
        const auto pose=view.provisional_motion(world);
        if(!pose)return fail("presented movement role missing");
        poses[unsigned(*pose)]=true;
        if(!captured)captured=capture_if_requested(context,options,frame);
        render.present();context.advance_frame();++frame;
        if(options.frames&&frame>=options.frames)break;
    }
    const auto saw=[&](gf::NativeBorgMotion pose){return poses[unsigned(pose)];};
    const bool walk_pose=saw(gf::NativeBorgMotion::Walk);
    const bool jump_poses=saw(gf::NativeBorgMotion::Takeoff)&&saw(gf::NativeBorgMotion::Fall)&&saw(gf::NativeBorgMotion::Land);
    const bool dash_poses=saw(gf::NativeBorgMotion::DashForward)&&saw(gf::NativeBorgMotion::DashLeft)&&
        saw(gf::NativeBorgMotion::DashBackward)&&saw(gf::NativeBorgMotion::DashRight);
    const bool air_dash_poses=saw(gf::NativeBorgMotion::LabAirWindup)&&saw(gf::NativeBorgMotion::AirDash)&&
        saw(gf::NativeBorgMotion::LabAirRecovery);
    const bool glide_poses=saw(gf::NativeBorgMotion::LabGlideEntry)&&saw(gf::NativeBorgMotion::LabGlide);
    gf::log_info(context,"walk-lab","id=%s variant=%s frames=%u actors=%u moved=%u walk=%u shots=%u hits=%u",
                 files.identity.id.c_str(),files.profile.variant.c_str(),frame,world.actor_count(),
                 unsigned(moved),unsigned(walk_pose),world.shots_spawned(),world.hits());
    if(jump)gf::log_info(context,"walk-lab","jump-lab takeoffs=%u landings=%u grounded=%u poses=%u ignored_air_presses=%u",
                        takeoffs,landings,unsigned(world.actors()[0].grounded),unsigned(jump_poses),air_presses);
    if(dash)gf::log_info(context,"walk-lab","dash-lab starts=%u poses=%u ignored_air_presses=%u",
                        dash_starts,unsigned(dash_poses),air_dash_presses);
    if(air_dash)gf::log_info(context,"walk-lab","air-dash-lab bursts=%u poses=%u spent_press_refusals=%u",
                            air_bursts,unsigned(air_dash_poses),spent_air_presses);
    if(glide)gf::log_info(context,"walk-lab","glide-lab entries=%u releases=%u poses=%u steps=%u",
                         glide_starts,glide_releases,unsigned(glide_poses),glide_steps);
    const bool cruise_poses=saw(gf::NativeBorgMotion::LabCruise)&&saw(gf::NativeBorgMotion::LabBoost);
    if(config.provisional_cruise)gf::log_info(context,"walk-lab",
        "cruise-lab boost_entries=%u depleted=%u speed=%u airborne_refill_ticks=%u full=%u poses=%u",
        boost_entries,unsigned(boost_depleted),unsigned(boost_speed),airborne_refill_ticks,
        unsigned(boost_full_after_depletion),unsigned(cruise_poses));
    const auto ground_peak_x100=unsigned(std::lround(ground_peak*100));
    if(config.provisional_ground_pace)gf::log_info(context,"walk-lab","ground-pace target=%u measured_peak=%u provisional",
        unsigned(*config.provisional_ground_pace),ground_peak_x100);
    if(options.lab_landing_audio)gf::log_info(context,"walk-lab",
        "landing-audio cues=%u blocks=%u nonzero=%u sample_frames=%zu device=%s accepted_frames=%llu",
        audio_cues,audio_blocks,unsigned(audio_nonzero),landing_sample.size()/2,audible?"open":"off",static_cast<unsigned long long>(submitted_frames));
    if(options.demo_borg_controls&&frame>=smoke_frames)
        gf::log_info(context,"walk-lab","controls resets=%u paused_frames=%u",resets,paused_frames);
    if(picker)gf::log_info(context,"walk-lab","picker switches=%u refusals=%u",picker_switches,picker_refusals);
    const bool smoke_failed=options.demo_borg_controls&&
        ((frame>=18&&(!moved||(!config.provisional_cruise&&!walk_pose)))||
         (jump&&frame>=jump_at+72&&(takeoffs!=1||landings!=1||!world.actors()[0].grounded||!jump_poses||air_presses!=1))||
         (frame>=smoke_frames&&(resets!=1||paused_frames!=4||
          (dash&&(dash_starts!=4||!dash_poses||(jump&&!air_dash&&air_dash_presses!=1)))||
          (air_dash&&(air_bursts!=1||!air_dash_poses||spent_air_presses!=1))||
          (glide&&(glide_starts!=1||glide_releases!=1||!glide_poses||!glide_steps))||
          (config.provisional_cruise&&(boost_entries!=1||!boost_speed||!boost_depleted||!boost_full_after_depletion||!cruise_poses))||
          (options.lab_landing_audio&&(audio_cues!=1||!audio_blocks||!audio_nonzero))))||
         (options.demo_lab_picker&&frame>=4&&(picker_switches!=2||picker_refusals))||
         (config.provisional_ground_pace&&!jump&&!dash&&frame>=24&&ground_peak_x100!=*config.provisional_ground_pace));
    render.shutdown();return smoke_failed?2:0;
}

int run_borg_controls(gf::Context& context,gf::RenderEngine& render,const Options& options){
    gf::NativeBattleMvp world;gf::NativeBattleMvpView view;gf::NativeGRedGroundDashDemo source;
    gf::BorgControlSelector selector;gf::InputRouter router;gf::pc::GRedPracticeInput practice_input;
    std::string error;
    auto fail=[&](const char* stage){
        gf::log_error(context,"borg-controls","%s: %s",stage,error.c_str());render.shutdown();return 2;
    };
    if(!view.load(context.files(),options.battle_mvp_assets,render,&error))return fail("scene load failed");
    const auto input_assets=options.source_ground_assets.empty()?"local/content/source-gred-ground-v9":options.source_ground_assets;
    unsigned selected=0,frame=0;bool stopped=false,paused=false,restart_held=false,pause_held=false,captured=false;
    bool pointer_held=false,motion_failed=false;
    std::array<bool,2> moved{},airborne{};
    unsigned switches=0;
    auto reset=[&](unsigned next){
        gf::BattleMvpConfig config;config.solo_model=gf::BorgControlSelector::entries[next].model;
        view.reset_control_test_pose();
        source.reject();
        if(!world.start(config,&view)||!view.update(world,&error))return false;
        if(config.solo_model==0x0615){
            gf::NativeBorgAnimationSeed seed;
            if(!view.source_seed(world,&seed)||!source.bind(context.files(),options.battle_mvp_assets,input_assets,
               world,seed,&error,gf::NativeGRedGroundDashDemo::Mode::GroundWindupEntryPush))return false;
        }
        selected=next;stopped=paused=false;practice_input={};
        gf::log_info(context,"borg-controls","active=%04x actors=%u (%s); no opponent or combat",
                     unsigned(config.solo_model),world.actor_count(),next?"provisional MVP":"recovered movement subset");
        return true;
    };
    if(!reset(0))return fail("initial actor refused");
    const auto started=context.clock().monotonic_ns();auto previous=started;gf::u64 phase=0;
    while(context.window().pump_events()&&!context.window().should_close()){
        const auto now=context.clock().monotonic_ns();
        if(options.seconds&&now-started>=gf::u64(options.seconds)*1000000000ull)break;
        context.input().pump();const auto& snapshot=context.input().snapshot();
        auto pad=router.route(snapshot,0);auto pointer=snapshot.pointer;
        pointer.left_pressed|=pointer.left&&!pointer_held;pointer_held=pointer.left;
        unsigned logical_w=options.width,logical_h=options.height;
        context.window().logical_size(&logical_w,&logical_h);
        pointer.x=logical_w?gf::s32(gf::s64(pointer.x)*options.width/logical_w):-1;
        pointer.y=logical_h?gf::s32(gf::s64(pointer.y)*options.height/logical_h):-1;
        if(snapshot.quit||pad.quit_requested)break;
        if(options.demo_borg_controls){
            pad={};pointer={};pointer.within_window=true;pointer.x=32;
            if(frame==58||frame==119){pointer.y=30;pointer.left_pressed=true;}
            if(frame==62||frame==123){pointer.y=frame==62?94:62;pointer.left_pressed=true;}
            if(frame>=4&&frame<13)pad.dpad_up=true;
            if(frame>=16&&frame<29)pad.b=true;
            if(frame==4){pad.stick_y=-95;pad.dpad_up=false;}
            if(frame>=70&&frame<82)pad.stick_x=65;
            if(frame==73)pad.l=true;
            if(frame>=86&&frame<99)pad.b=true;
        }
        const auto next=selector.select(pointer);
        const bool restart=pad.y&&!restart_held,pause=pad.start&&!pause_held;
        restart_held=pad.y;pause_held=pad.start;
        const bool changed=next&&*next!=selected;
        const bool reset_actor=changed||restart;
        if(reset_actor&&!reset(next.value_or(selected)))return fail("actor switch refused");
        switches+=unsigned(changed);
        if(pause&&!reset_actor&&!stopped)paused=!paused;
        unsigned steps=1;
        if(!options.headless&&!options.demo_borg_controls){
            phase+=(now>=previous?std::min<gf::u64>(now-previous,250000000):0)*60;
            steps=unsigned(std::min<gf::u64>(phase/1000000000ull,8));phase%=1000000000ull;
        }
        previous=now;
        const bool running=!selector.open()&&!paused&&!stopped&&!reset_actor&&!pause;
        if(!running){steps=0;phase=0;}
        // Strip combat and session buttons before either movement controller.
        pad.a=pad.x=pad.y=pad.start=false;
        if(!selected){
            pad.l=pad.trigger_l_pressed=false;
            practice_input.sample(pad,source.practice_flick_direction(pad),running);
        }
        while(steps--){
            const auto before=world.actors()[0].position;
            const auto movement=gf::NativeBattleMvpView::movement_frame(world,options.width,options.height);
            error.clear();
            if(!selected){
                auto input=pad;practice_input.take(input);
                gf::NativeGRedGroundInput mapped;gf::NativeBorgMotionOffer offer;
                if(!source.map_input(input,&mapped,&error))break;
                if(!source.prepare(world,view.source_binding(),movement,mapped.x,mapped.y,mapped.held,&offer,&error)||
                   !world.step({},false,&view,nullptr,&movement,&offer)){
                    source.reject();stopped=motion_failed=true;
                    gf::log_info(context,"borg-controls","motion refused at tick %u: %s",world.tick(),error.c_str());
                    break;
                }
                gf::NativeBorgSourcePose pose;
                if(!source.commit(world,view.source_binding(),&error)||!source.pose(world,&pose)||
                   !view.update_source(world,pose,&error))return fail("source pose transaction failed");
            }else if(!world.step({pad,{}},false,&view,nullptr,&movement)||!view.update(world,&error))return fail("MVP movement failed");
            moved[selected]|=before!=world.actors()[0].position;
            airborne[selected]|=!world.actors()[0].grounded;
        }
        const auto readout=!selected?"FLIGHT "+std::to_string(source.resource())+"/10000   SOLO / NO COMBAT":
                                   std::string("SOLO / NO COMBAT");
        render.begin_frame({frame,.15f,.21f,.3f});render.reset_arena();
        const auto draws=view.draw(render,world,options.width,options.height,&error);
        const auto ui=selector.draw(render,selected,options.width,options.height,paused,stopped,readout);
        render.end_frame();
        if(!draws||!ui)return fail("draw failed");
        if(!captured)captured=capture_if_requested(context,options,frame);
        render.present();context.advance_frame();++frame;
        if(options.frames&&frame>=options.frames)break;
    }
    gf::log_info(context,"borg-controls","frames=%u actors=%u switches=%u moved=%u/%u airborne=%u/%u shots=%u hits=%u",
                 frame,world.actor_count(),switches,unsigned(moved[0]),unsigned(moved[1]),
                 unsigned(airborne[0]),unsigned(airborne[1]),world.shots_spawned(),world.hits());
    const bool smoke_failed=options.demo_borg_controls&&options.frames>=144&&frame>=144&&
        (switches!=2||!moved[0]||!moved[1]||!airborne[0]||!airborne[1]||motion_failed);
    render.shutdown();return smoke_failed?2:0;
}

struct PickerPracticeHandoff {
    gf::u64 session_started=0;
    bool captured=false;
};

struct BattlePickerSession {
    gf::BorgLabPicker picker;
    const gf::BorgPackageCatalog& catalog;
    std::size_t active=0;
    std::optional<std::size_t> scripted_row;
    std::optional<gf::pc::BorgBattlePackageFiles> next_player;
    unsigned elapsed_frames=0;
    std::size_t next_row=0;
    bool suppress_actions=false;
};

struct PackageBattleFiles {
    std::array<gf::pc::BorgBattlePackageFiles,2> first;
    std::vector<gf::pc::BorgBattlePackageFiles> extra;
    std::shared_ptr<const gf::StagePackageSnapshot> stage;
};

bool load_package_battle_files(const Options& options,const gf::BorgPackageCatalog& catalog,
                               PackageBattleFiles& out,std::string& error){
    PackageBattleFiles next;
    if(!gf::pc::load_borg_battle_package("borgs",catalog,options.battle_borg,options.borg_variant,&next.first[0],&error)||
       !gf::pc::load_borg_battle_package("borgs",catalog,options.against_borg,options.against_variant,&next.first[1],&error))
        return false;
    next.extra.reserve(options.roster_borgs.size());
    for(const auto& member:options.roster_borgs){
        gf::pc::BorgBattlePackageFiles candidate;
        if(!gf::pc::load_borg_battle_package("borgs",catalog,member.id,member.variant,&candidate,&error)){
            error="roster "+member.id+": "+error;return false;
        }
        next.extra.push_back(std::move(candidate));
    }
    if(!options.battle_stage.empty()&&!gf::pc::pin_stage_package("stages",gf::pc::scan_stage_packages("stages"),
       options.battle_stage,&next.stage,&error))return false;
    out=std::move(next);return true;
}

int run_package_battle(gf::Context& context,gf::RenderEngine& render,const Options& options,
                        const std::array<gf::pc::BorgBattlePackageFiles,2>& test_packages,const gf::StagePackageSnapshot* stage,
                        gf::pc::PortableBattleHost* hosted=nullptr,gf::pc::PortableBattleHostReport* report=nullptr,
                        BattlePickerSession* selection=nullptr,
                        std::span<const gf::pc::BorgBattlePackageFiles> extra_packages={},std::string* probe_error=nullptr){
    using namespace gf;
    NativeBattleMvp world;NativeGRedGroundDashDemo source,source2;
    std::string error;
    auto fail=[&](const char* reason){
        if(probe_error)*probe_error=error.empty()?std::string(reason):error;
        else gf::log_error(context,"package-battle","%s: %s",reason,error.c_str());
        render.shutdown();return 2;
    };
    const auto* accepted=hosted?hosted->accepted():nullptr;
    const bool explicit_roster=!options.roster_borgs.empty();
    if(options.check_battle_roster&&(!options.headless||!options.no_audio||!explicit_roster||!stage||hosted||selection)){
        error="roster check requires a standalone preset and bound stage on the silent headless host";
        return fail("roster check refused");
    }
    BattleRosterSeries series(options.roster_rounds_to_win,options.roster_round_seconds);
    if((options.roster_rounds_to_win||options.roster_round_seconds)&&(!series.enabled()||!explicit_roster||!stage)){
        error="series rules require a standalone roster preset and bound stage";
        return fail("series admission refused");
    }
    const bool local_roster=explicit_roster||options.demo_roster_battle;
    if(report)*report={};
    if(bool(hosted)!=bool(report)||(selection&&(hosted||stage))||(hosted&&(!accepted||stage||options.demo_package_battle||
        !options.battle_stage.empty()||options.package_battle_distance||options.package_damage_indices))){
        error="hosted duel requires its accepted request with no Battle Test overrides";
        return fail("battle handoff refused");
    }
    if(extra_packages.size()!=options.roster_borgs.size()||extra_packages.size()>kBattleMvpMaxActors-2||
       (explicit_roster&&(hosted||selection||options.demo_roster_battle))){
        error="explicit roster needs its own admitted standalone package list";
        return fail("roster handoff refused");
    }
    const auto& packages=hosted?hosted->packages():test_packages;
    const auto actor_package=[&](unsigned slot)->const gf::pc::BorgBattlePackageFiles&{
        return explicit_roster&&slot>=2?extra_packages[slot-2]:packages[slot%packages.size()];
    };
    auto test_view=hosted?nullptr:std::make_unique<NativeBattleMvpView>();
    auto& view=hosted?hosted->view():*test_view;
    auto config=hosted?hosted->config():BattleMvpConfig{};
    const auto tick_limit=accepted?accepted->request.tick_limit:0;
    if(!hosted)for(unsigned i=0;i<2;++i)config.packages[i]=packages[i].definition;
    if(explicit_roster){
        config.declared_actors=2+unsigned(extra_packages.size());config.teams_declared=true;
        std::copy(options.first_roster_teams.begin(),options.first_roster_teams.end(),config.teams.begin());
        config.local_aim_cycle=true;config.roster_local_players=options.roster_local_players;
        std::array<unsigned,2> lanes{};
        const f32 half=f32(options.package_battle_distance?options.package_battle_distance:600)*.5f;
        for(unsigned i=0;i<config.declared_actors;++i){
            auto& definition=config.packages[i];
            definition=actor_package(i).definition;
            if(!definition||definition->movement==BorgBattleMovement::NormalGRed){
                error="explicit rosters require native package movement; use duel or mirrored demo for borrowed source motion";
                return fail("roster actor admission refused");
            }
            if(i>=2)config.teams[i]=options.roster_borgs[i-2].team;
            const u8 team=config.teams[i];
            if(team>1||lanes[team]>=3){
                error="explicit roster allows three actors per team";
                return fail("roster actor admission refused");
            }
            const auto lane=lanes[team]++;
            if(!stage)config.spawn[i]={team?half:-half,lane==0?0.f:lane==1?-240.f:240.f};
        }
        if(!lanes[0]||!lanes[1]){
            error="explicit roster needs at least one fighter per team";
            return fail("roster actor admission refused");
        }
    }else if(!hosted&&options.demo_roster_battle){
        config.packages[2]=packages[0].definition;config.packages[3]=packages[1].definition;
        config.declared_actors=4;
        config.teams={0,1,0,1};config.teams_declared=true;
        config.allow_multi_source=true;
        config.source_allow_active_opponent=true;
    }
    if(!hosted&&options.package_damage_indices){
        config.damage_indices=options.package_damage_indices;
        if(!battle_mvp_damage_indices_valid(config,config.declared_actors)){
            error="indexed damage selection requires matching complete package tables";
            return fail("damage selection refused");
        }
    }
    if(config.declared_actors<2||config.declared_actors>kBattleMvpMaxActors){
        error="package actor count outside the native roster capacity";
        return fail("roster actor admission refused");
    }
    if(options.package_ko_force){
        config.ko_force_pools=options.package_ko_force;
        if(hosted||selection||!battle_mvp_ko_force_valid(config,config.declared_actors)){
            error="KO Force accounting requires standalone normal packages with a KO cost on every fighter";
            return fail("KO Force accounting refused");
        }
    }
    config.package_strafe=selection||options.exercise_battle_mobility||explicit_roster;
    if(!hosted&&!explicit_roster&&options.package_battle_distance){
        const auto scale=f32(options.package_battle_distance)/std::hypot(config.spawn[0].x-config.spawn[1].x,config.spawn[0].z-config.spawn[1].z);
        for(auto& spawn:config.spawn){spawn.x*=scale;spawn.z*=scale;}
    }
    const bool source_movement=config.packages[0]->movement==BorgBattleMovement::NormalGRed;
    const bool demo_held_b=std::any_of(config.packages[0]->moves.begin(),config.packages[0]->moves.end(),
        [](const auto& move){return move.button==BorgBattleButton::B&&move.held_fire.has_value();});
    const auto charge=std::find_if(config.packages[0]->moves.begin(),config.packages[0]->moves.end(),[](const auto& move){
        return move.button==BorgBattleButton::B&&move.activation==BorgBattleActivation::ReleaseWindow;
    });
    // A release-window move is exercised by holding one tick past its minimum
    // charge. Longer authored charges are still scripted, up to a tick budget
    // that keeps both holds inside their round.
    const unsigned demo_charge_hold=charge!=config.packages[0]->moves.end()&&charge->minimum_hold>=2&&
        charge->minimum_hold<=90?std::min<unsigned>(charge->minimum_hold+1,charge->maximum_hold):0u;
    // A charged X move needs the same hold on its own button.
    const auto x_charge=std::find_if(config.packages[0]->moves.begin(),config.packages[0]->moves.end(),[](const auto& move){
        return move.button==BorgBattleButton::X&&move.activation==BorgBattleActivation::ReleaseWindow;
    });
    const unsigned demo_x_charge_hold=x_charge!=config.packages[0]->moves.end()&&x_charge->minimum_hold>=2&&
        x_charge->minimum_hold<=90?std::min<unsigned>(x_charge->minimum_hold+1,x_charge->maximum_hold):0u;
    const bool demo_scope=std::any_of(config.packages[0]->moves.begin(),config.packages[0]->moves.end(),
        [](const auto& move){return move.button==BorgBattleButton::X&&(move.power_field||move.slow_field||move.field);});
    const bool demo_gravity=std::any_of(config.packages[0]->moves.begin(),config.packages[0]->moves.end(),
        [](const auto& move){return move.field&&move.field->shot_pull;});
    const bool demo_magnet=std::any_of(config.packages[0]->moves.begin(),config.packages[0]->moves.end(),
        [](const auto& move){return move.field&&move.field->actor_pull;});
    const bool demo_fence=std::any_of(config.packages[0]->anchor_sets.begin(),config.packages[0]->anchor_sets.end(),
        [](const auto& set){return bool(set.fence);});
    const bool demo_accuracy=std::any_of(config.packages[0]->projectiles.begin(),config.packages[0]->projectiles.end(),
        [](const auto& shot){return shot.accuracy.has_value();})&&
        std::any_of(config.packages[1]->projectiles.begin(),config.packages[1]->projectiles.end(),
        [](const auto& shot){return shot.accuracy_sensitive;});
    const auto inputs=options.source_ground_assets.empty()?"local/content/source-gred-ground-v9":options.source_ground_assets;
    const auto practice=packages[0].gred.views();
    if(!hosted&&bool(stage)!=!options.battle_stage.empty()){
        error="exact requested stage snapshot required; no legacy fallback";
        return fail("stage admission refused");
    }
    std::array<BorgBattleVisualAssets,kBattleMvpMaxActors> visuals{};
    std::array<BorgAudioSlot,kBattleMvpMaxActors> audio_slots{};
    for(unsigned i=0;i<config.declared_actors;++i){
        visuals[i]=actor_package(i).visuals();
        audio_slots[i]=actor_package(i).audio();
    }
    const auto view_packages=std::span<const std::shared_ptr<const BorgGameplayProfile>>(config.packages).first(config.declared_actors);
    const auto view_visuals=std::span<const BorgBattleVisualAssets>(visuals).first(config.declared_actors);
    if(stage){
        std::array<std::string_view,kBattleMvpMaxActors> markers{};
        if(options.battle_stage_spawns.size()!=config.declared_actors||options.demo_roster_battle){
            error="exact stage markers required for every actor; mirrored placement is not supported";
            return fail("portable stage admission refused");
        }
        std::copy(options.battle_stage_spawns.begin(),options.battle_stage_spawns.end(),markers.begin());
        if(!view.bind_stage(*stage,render,&error)||
           !view.configure_stage_roster(options.battle_stage,std::span<const std::string_view>(markers).first(config.declared_actors),&config,&error)||
           !view.load_packages(render,view_packages,view_visuals,&error))return fail("portable stage admission refused");
    }else if(!hosted&&!view.load_packages(context.files(),options.battle_mvp_assets,render,view_packages,view_visuals,&error,inputs))
        return fail("asset binding refused");
    if(!hosted&&options.demo_roster_battle){
        config.spawn[2]={config.spawn[0].x,-config.spawn[0].z};
        config.spawn[3]={config.spawn[1].x,-config.spawn[1].z};
    }
    gf::pc::GRedPracticeInput practice_input;
    BorgMovementAudio movement_audio;
    const auto battle_audio=std::span<const BorgAudioSlot>(audio_slots).first(config.declared_actors);
    const auto has_pcm=[&](auto member){
        return std::any_of(battle_audio.begin(),battle_audio.end(),[&](const BorgAudioSlot& slot){
            const auto* bank=slot.*member;
            return bank&&std::any_of(bank->begin(),bank->end(),[](const auto& clip){
                if constexpr(requires{clip.pcm;})return !clip.pcm.empty();
                else return !clip.empty();
            });
        });
    };
    const bool has_launch_sounds=has_pcm(&BorgAudioSlot::launch);
    const bool has_impact_sounds=has_pcm(&BorgAudioSlot::impact);
    const bool has_release_sounds=has_pcm(&BorgAudioSlot::release);
    const bool has_contact_sounds=has_pcm(&BorgAudioSlot::contact);
    const bool has_sounds=has_pcm(&BorgAudioSlot::movement)||has_launch_sounds||has_release_sounds||has_impact_sounds||
        has_pcm(&BorgAudioSlot::action)||has_contact_sounds;
    const bool audible=has_sounds&&!options.no_audio&&!options.headless&&context.audio().open(kBorgMovementSampleRate,2);
    const bool mix_audio=audible||(has_sounds&&options.trace_package_battle&&!options.no_audio);
    struct CloseMovementAudio {IAudioBackend* device;~CloseMovementAudio(){if(device)device->close();}}
        close_movement_audio{audible?&context.audio():nullptr};
    std::vector<s16> movement_pcm;
    u64 movement_audio_frames=0;
    unsigned launch_audio_starts=0,launch_audio_blocks=0;
    bool launch_audio_nonzero=false;
    unsigned release_audio_starts=0,release_audio_blocks=0;
    bool release_audio_nonzero=false;
    unsigned impact_audio_starts=0;
    unsigned contact_audio_starts=0;
    unsigned audio_mixed_actors=0;
    u64 audio_dropped=0;
    bool contact_audio_nonzero=false;
    bool wait_for_jump_release=false;
    PortableRosterAi roster_ai;
    const bool second_human=local_roster&&config.roster_local_players==2;
    const bool trace_roster=explicit_roster&&options.trace_package_battle;
    const auto roster_alive=[&](){
        unsigned mask=0;
        for(unsigned i=0;i<world.actor_count();++i)if(world.actors()[i].health)mask|=1u<<i;
        return mask;
    };
    unsigned previous_roster_alive=0;
    auto restart=[&](){
        source.reject();source2.reject();view.reset_control_test_pose();practice_input={};wait_for_jump_release=false;
        roster_ai.reset();
        movement_audio.reset();if(audible)context.audio().discard_queued();
        if(!world.start(config,&view)){
            error=stage||hosted?"pinned stage or Borg definitions refused":
                "v1 source G-Red requires slot 1 against an explicit stationary package; check placement and definitions";
            return false;
        }
        if(!view.update(world,&error))return false;
        if(series.enabled()&&!series.open_round(world)){
            error="series could not bind the new round";return false;
        }
        if(source_movement){
            NativeBorgAnimationSeed seed;
            if(!view.source_seed(0,world,&seed)||!source.bind(0,context.files(),options.battle_mvp_assets,inputs,
               world,seed,&error,NativeGRedGroundDashDemo::Mode::GroundWindupEntryPush,&practice))return false;
            if(options.demo_roster_battle){
                NativeBorgAnimationSeed seed2;
                if(!view.source_seed(2,world,&seed2)||!source2.bind(2,context.files(),options.battle_mvp_assets,inputs,
                   world,seed2,&error,NativeGRedGroundDashDemo::Mode::GroundWindupEntryPush,&practice))return false;
            }
        }
        if(trace_roster)previous_roster_alive=roster_alive();
        return true;
    };
    if(!restart())return fail("actor admission refused");
    if(options.check_battle_roster){
        // Use the identical prepared world and first draw as play, but
        // never enter the event loop or advance its simulation clock.
        render.begin_frame({0,.15f,.21f,.3f,false});render.reset_arena();
        const auto draws=view.draw(render,world,options.width,options.height,&error,false,0,nullptr,true,
                                   series.enabled()?&series:nullptr);
        const auto submitted=render.end_frame();
        if(!draws||!submitted)return fail("initial headless draw refused");
        for(unsigned i=0;i<world.actor_count();++i)if(!view.package_body_draws(i)){
            error="slot "+std::to_string(i)+" ("+actor_package(i).identity.id+") did not submit body geometry";
            return fail("initial headless draw refused");
        }
        if(!probe_error){
            std::printf("Roster admission passed: stage=%s slots=%u locals=%u ticks=%u (headless)\n",
                    options.battle_stage.c_str(),world.actor_count(),unsigned(config.roster_local_players),world.tick());
            for(unsigned i=0;i<world.actor_count();++i){
                const auto& p=world.actors()[i].position;
                std::printf("slot=%u team=%u driver=%s id=%s variant=%s spawn=%s position=%.2f,%.2f,%.2f body-draws=%u\n",
                    i,unsigned(config.teams[i]),i<config.roster_local_players?"local":"cpu",
                    actor_package(i).identity.id.c_str(),config.packages[i]->variant.c_str(),options.battle_stage_spawns[i].c_str(),
                    double(p[0]),double(p[1]),double(p[2]),view.package_body_draws(i));
            }
        }else probe_error->clear();
        render.shutdown();
        return 0;
    }
    if(accepted){
        const auto& request=accepted->request;
        gf::log_info(context,"package-battle","stage=%s player-spawn=%s enemy-spawn=%s",
            request.stage.id.c_str(),request.player_spawns[0].id.c_str(),request.enemy_spawns[0].id.c_str());
    }else if(stage&&explicit_roster&&config.roster_local_players==1)
        gf::log_info(context,"package-battle","stage=%s p1-spawn=%s locals=1",
                     options.battle_stage.c_str(),options.battle_stage_spawns[0].c_str());
    else if(stage&&explicit_roster&&config.teams[0]==config.teams[1])
        gf::log_info(context,"package-battle","stage=%s p1-spawn=%s p2-spawn=%s local-coop=1",
                     options.battle_stage.c_str(),options.battle_stage_spawns[0].c_str(),options.battle_stage_spawns[1].c_str());
    else if(stage)gf::log_info(context,"package-battle","stage=%s player-spawn=%s enemy-spawn=%s",
        options.battle_stage.c_str(),options.battle_stage_spawns[0].c_str(),options.battle_stage_spawns[1].c_str());
    for(unsigned i=0;i<(explicit_roster?config.declared_actors:2u);++i){
        const auto tally=view.package_texture_tally(world,i);
        gf::log_info(context,"package-battle","slot=%u id=%s variant=%s moves=%zu textured=%u untextured=%u unresolved=%u",
            i,actor_package(i).identity.id.c_str(),config.packages[i]->variant.c_str(),
            config.packages[i]->moves.size(),tally[0],tally[1],tally[2]);
        if(explicit_roster)gf::log_info(context,"borg-roster-slot","actor=%u team=%u driver=%s id=%s",i,unsigned(config.teams[i]),
                                       i<config.roster_local_players?"local":"cpu",actor_package(i).identity.id.c_str());
        if(explicit_roster&&stage)gf::log_info(context,"borg-stage-spawn","actor=%u marker=%s",i,options.battle_stage_spawns[i].c_str());
    }
    gf::log_info(context,"package-battle",hosted?
         "authored duel, not retail-equivalent; arrows move, Alt+Arrow dash, Shift jump, Space B, E X if available, Q cancel":
         "native test combat, not retail-equivalent; arrows move, Alt+Arrow dash, Shift jump, Space B, E X if available, Q restart");
    constexpr std::array kinds{"start","marker","debit","stock-refused","refill","emit","emit-refused","interrupt","complete","hit","retire",
        "modifier-applied","modifier-refused","modifier-expired","pull-queued","pull-applied","pull-refused","pull-cancelled",
        "button-release","repeat","repeat-stock-refused","melee-armed","melee-refused","melee-retired",
        "held-fire-start","held-fire-cadence","held-fire-stop","blast-triggered","blast-hit","projectile-contact",
        "projectile-landed","detonation","stock-returned","guidance-bound","guidance-ended",
        "guard-armed","guard-blocked","guard-broken","guard-retired","stage-impact","emitter-deployed","emitter-deploy-refused",
        "emitter-fired","emitter-fire-refused","emitter-retired","anchor-placed","anchor-refused","rewind-queued","rewind-reached",
        "rewind-completed","rewind-stopped","anchor-path-sealed","rewind-refused","anchor-planted","tap","double-tap","anchor-fallback",
        "socket-launch","socket-refused","aim-committed","bomb-proximity","bomb-fuse-committed",
        "blast-children","blast-children-refused","smoke-created","smoke-refused","ink-applied",
        "power-field-placed","power-field-refused","shot-powered","slow-field-placed","slow-field-refused","field-guidance-broken",
        "field-placed","field-refused","field-anchor-lost","accuracy-applied","accuracy-affected",
        "flight-children","flight-children-refused","impact-children","impact-children-refused",
        "impact-trail-started","impact-trail-refused","impact-hazard-started","impact-hazard-placed","impact-hazard-refused",
        "extra-emit","extra-emit-refused","walk-pulse-created","walk-pulse-refused","field-shot-pulled",
        "field-actor-pulled","field-actor-pull-refused","fence-recoil","fence-recoil-refused","entry-stock-refused","field-replaced",
        "launch-sound","release-sound","impact-sound","action-sound","contact-sound","projectile-shape-refused","projectile-contact-closed"};
    struct Origins final:IBorgBattleOrigins {
        explicit Origins(const NativeBattleMvpView& observed):view(observed){}
        const NativeBattleMvpView& view;
        std::array<const CharacterLocalPose*,kBattleMvpMaxActors> source_local{};
        bool socket(unsigned owner,const BattleMvpActor& actor,u32 tick,u8 move,u16 frame,const BorgBattleSocket& spec,
                    BattleMvpVec* out)const override{
            return view.package_socket(owner,actor,tick,move,frame,spec,
                owner<source_local.size()?source_local[owner]:nullptr,out);
        }
    } origins(view);
    InputRouter router;std::array<GamePadState,2> previous{};std::array<BorgBattleInputs,2> action_input{};
    u8 pending_aim_cycles=0;
    const auto action_bits=[](const GamePadState& pad){return u8((pad.a?BorgBattleInputs::b:0)|(pad.x?BorgBattleInputs::x:0));};
    const bool demo_guard_x=std::any_of(config.packages[1]->moves.begin(),config.packages[1]->moves.end(),
        [](const auto& move){return move.button==BorgBattleButton::X&&move.guard.has_value();});
    const bool demo_emitters=std::any_of(config.packages[0]->moves.begin(),config.packages[0]->moves.end(),
        [](const auto& move){return move.button==BorgBattleButton::X&&!move.emitters.empty();});
    const bool demo_wire_carrier=std::any_of(config.packages[0]->projectiles.begin(),config.packages[0]->projectiles.end(),
        [](const auto& shot){return shot.rewind_hit.has_value();});
    const bool demo_anchor_projectile=std::any_of(config.packages[0]->projectiles.begin(),config.packages[0]->projectiles.end(),
        [](const auto& shot){return shot.plant_on_stage!=255;});
    const bool demo_anchor_double_tap=std::any_of(config.packages[0]->moves.begin(),config.packages[0]->moves.end(),
        [](const auto& move){return move.button==BorgBattleButton::X&&move.activation==BorgBattleActivation::DoubleTap;});
    const auto start=context.clock().monotonic_ns();auto last=start;u64 phase=0;
    unsigned frame=0,restarts=0,total_shots=0,total_hits=0,total_reactions=0;
    unsigned max_flight=0;
    bool captured=false,paused=false,first_hit=false,after_restart_hit=false,moved=false,airborne=false;
    auto end=PortableDuelEnd::Fighting;
    std::array<u8,gf::kBattleMvpMaxActors> previous_cues{};
    std::array<BorgStockHud,2> previous_stock{};
    std::array<std::optional<BorgMobilityHud>,2> previous_mobility{};
    std::array<unsigned,gf::kBattleMvpMaxActors> previous_reaction_ticks{};
    f32 previous_fade=-1.f; // no sample yet; admitted fade alpha is nonnegative
    std::string picker_notice;
    std::string movement_notice;
    bool source_blocked=false;
    bool pointer_held=false;
    bool wait_for_confirm=options.roster_browser_match,wait_for_start=options.roster_browser_match;
    TextRenderer mechanics_text;
    unsigned ground_dashes=0,air_dashes=0,peak_jumps=0,boost_frames=0;
    bool strafed=false;
    f32 peak_walk=0;
    bool touchdown_attempt=false,touchdown_entry=false,landing_followup=false;
    const auto block_movement=[&](){
        source_blocked=true;movement_notice="MOVEMENT NOT SUPPORTED HERE - Q RESTART OR SWITCH BORG";
        gf::log_warn(context,"package-battle","G-Red movement paused at tick %u (selector %u): %s",
                     world.tick(),unsigned(source.selector()),error.c_str());
        error.clear();action_input={};phase=0;if(audible)context.audio().discard_queued();
    };
    while(context.window().pump_events()&&!context.window().should_close()){
        const auto now=context.clock().monotonic_ns();
        if(options.seconds&&now-start>=u64(options.seconds)*1000000000ull)break;
        unsigned width=options.width,height=options.height;
        if((selection||explicit_roster)&&!options.headless)context.window().size(&width,&height);
        if(!width||!height){last=now;phase=0;continue;}
        context.input().pump();const auto& snapshot=context.input().snapshot();
        std::array<GamePadState,2> pads{router.route(snapshot,0),router.route(snapshot,1)};
        if(snapshot.alt_arrow){pads[0].l=true;pads[0].stick_x=pads[0].stick_y=0;}
        const bool demo_exit=options.demo_roster_cancel&&frame==4;
        const bool menu_back=snapshot.menu_back||(demo_exit&&options.demo_roster_pad);
        if(snapshot.quit||pads[0].quit_requested||(options.roster_browser_match&&menu_back)||
           (demo_exit&&!options.demo_roster_pad))break;
        if(options.demo_roster_pad&&frame<3)pads[0].a=pads[0].start=true; // exercise held picker confirm
        if(wait_for_confirm){wait_for_confirm=pads[0].a;pads[0].a=false;}
        if(wait_for_start){wait_for_start=pads[0].start;pads[0].start=false;}
        bool choosing=false;
        if(selection){
            auto& picker=selection->picker;
            auto pointer=snapshot.pointer;
            pointer.left_pressed|=pointer.left&&!pointer_held;pointer_held=pointer.left;
            unsigned logical_w=width,logical_h=height;context.window().logical_size(&logical_w,&logical_h);
            pointer.x=logical_w?s32(s64(pointer.x)*width/logical_w):-1;
            pointer.y=logical_h?s32(s64(pointer.y)*height/logical_h):-1;
            picker.fit_height(s32(height));
            if(selection->scripted_row){
                using Picker=BorgLabPicker;
                const auto target=*selection->scripted_row;
                const auto rows=picker.rows_per_page();
                const auto active_page=selection->active/rows,target_page=target/rows;
                const auto pages=active_page>target_page?active_page-target_page:target_page-active_page;
                pointer={};pointer.within_window=true;pointer.x=Picker::left+8;pointer.y=Picker::top+8;
                if(frame>0&&frame<=pages)pointer.wheel=target_page>active_page?-1:1;
                else if(frame==pages+1){pointer.left_pressed=true;pointer.y+=s32(target%rows+1)*Picker::row_height;}
                else pointer.left_pressed=frame==0;
            }
            const bool open=picker.open();
            const auto picked=picker.step(pointer,s32(width));
            choosing=open||picker.open()||selection->suppress_actions;
            if(!open&&picker.open()&&audible)context.audio().discard_queued();
            if(open||picker.open())selection->suppress_actions=true;
            else if(!pads[0].a&&!pads[0].x&&!pads[0].b&&!pads[0].y&&!pads[0].start&&
                    !pads[1].a&&!pads[1].x&&!pads[1].b&&!pads[1].y&&!pads[1].start)selection->suppress_actions=false;
            if(picked&&*picked!=selection->active){
                const auto& row=picker.entries()[*picked];
                pc::BorgBattlePackageFiles candidate;
                bool ok=pc::load_borg_battle_package("borgs",selection->catalog,row.id,row.variant,&candidate,&error);
                if(ok){
                    // Validate the pair and all exact asset/texture bindings before
                    // ending the active round. No live GPU state is replaced here.
                    headless::Render backend;RenderEngine probe(backend);
                    auto pending=std::make_unique<NativeBattleMvpView>();
                    auto trial_config=config;trial_config.packages[0]=candidate.definition;
                    NativeBattleMvp trial;
                    const std::array trial_visuals{candidate.visuals(),packages[1].visuals()};
                    const std::array<std::shared_ptr<const BorgGameplayProfile>,2> trial_packages{trial_config.packages[0],trial_config.packages[1]};
                    ok=pending->load_packages(context.files(),options.battle_mvp_assets,probe,trial_packages,trial_visuals,&error,inputs)&&
                       !probe.shared_texture_upload_failed()&&trial.start(trial_config,pending.get())&&pending->update(trial,&error);
                    if(ok&&candidate.definition->movement==BorgBattleMovement::NormalGRed){
                        NativeGRedGroundDashDemo trial_source;NativeBorgAnimationSeed seed;const auto source_files=candidate.gred.views();
                        ok=pending->source_seed(trial,&seed)&&trial_source.bind(context.files(),options.battle_mvp_assets,inputs,
                            trial,seed,&error,NativeGRedGroundDashDemo::Mode::GroundWindupEntryPush,&source_files);
                    }
                    if(!ok&&error.empty())error="incompatible scene, assets or opponent";
                }
                if(ok){
                    selection->next_player=std::move(candidate);selection->next_row=*picked;
                    selection->elapsed_frames=frame+1;
                    break;
                }
                picker_notice="BORG REFUSED - CURRENT ROUND UNCHANGED";
                gf::log_warn(context,"battle-picker","refused id=%s: %s",row.id.c_str(),error.c_str());error.clear();
            }
        }
        if(hosted)pads[1]={}; // Campaign slot 1 gets only the engine AI commands.
        if(options.demo_package_battle){
            pads={};pads[0].a=(frame>=6&&frame<43)||frame==93;pads[0].x=frame==41||(frame>=126&&frame<138);
            if(demo_charge_hold)
                pads[0].a=frame==5||(frame>=12&&frame<12+demo_charge_hold)||(frame>=94&&frame<94+demo_charge_hold);
            if(demo_x_charge_hold)
                pads[0].x=frame==5||(frame>=12&&frame<12+demo_x_charge_hold)||(frame>=94&&frame<94+demo_x_charge_hold);
            if(demo_held_b)pads[0].a|=(frame>=47&&frame<55)||(frame>93&&frame<123);
            if(demo_scope){pads[0].x=frame==2||frame==90;pads[0].a=frame==20||frame==109;}
            if(demo_accuracy)pads[1].a=frame==78;
            if(demo_guard_x)pads[1].x=frame==12||frame==98;
            if(demo_emitters){pads[0].x=frame==2||frame==90;pads[0].a=frame==18||frame==45||frame==109;}
            if(!config.packages[0]->anchor_sets.empty()){
                pads[0].a=frame==4||frame==93;
                pads[0].x=frame==18||frame==27||(frame>=38&&frame<48)||(frame>=126&&frame<138);
                pads[0].dpad_right|=frame>=23&&frame<32;
            }
            if(demo_wire_carrier){
                pads[0].a=frame==4||frame==93||(frame>=45&&frame<55)||(frame>=111&&frame<123);
                pads[0].x=frame==23||frame==32;
                pads[0].b=frame>=70&&frame<75;
            }
            if(demo_anchor_projectile){
                pads[0].x=frame==23;
                pads[0].a=frame==4||frame==93||(frame>=39&&frame<49)||(frame>=126&&frame<138);
            }
            if(demo_anchor_double_tap)
                pads[0].x=frame==18||frame==29||frame==44||frame==47||frame==127||frame==130;
            if(demo_fence)pads[0].x=frame==2;
            if(explicit_roster){
                // A hold is one cycle; a second press advances again.
                pads[0].z=pads[1].z=(frame>=2&&frame<=4)||frame==6;
            }
            // The roster demo exercises the second local player's session
            // control; the ordinary demo keeps P1 restart.
            if(second_human)pads[1].y=frame==85;
            else pads[0].y=frame==85;
            pads[0].dpad_right=!demo_gravity&&!demo_magnet&&!demo_fence&&frame>=2&&frame<=4;
            if(config.packages[0]->walk_pulse)pads[0].dpad_right|=frame>=10&&frame<43;
            if(demo_gravity)pads[1].a=frame==11;
            pads[0].b=frame>=56&&frame<=60;
            if(config.packages[0]->air_jump_points)pads[0].b|=frame==64||frame==70||frame==104;
            if(config.packages[0]->flight_ticks)pads[0].b=(frame>=51&&frame<85)||(frame>=98&&frame<134);
            if(options.exercise_battle_mobility){
                pads[0].r=frame>=2&&frame<=4;
                pads[0].l=frame==46||frame==77;
                pads[0].dpad_left=frame==46||(frame>=74&&frame<=79);
                if(config.packages[0]->cruise)pads[0].dpad_up|=frame>=56&&frame<=62;
                if(config.packages[0]->trailing_chain)pads[0].b=frame>=30&&frame<=33;
                // A grounded charged B plus an airborne B needs its own press
                // once the scripted jump is up; other packages keep their
                // existing timings so their traces stay comparable.
                if(!demo_charge_hold){}else if(std::any_of(config.packages[0]->moves.begin(),
                    config.packages[0]->moves.end(),[](const auto& move){
                        return move.button==BorgBattleButton::B&&move.context==BorgBattleContext::Airborne;})){
                    pads[0].a=frame>=40&&frame<=44;
                    // Chain bodies already jump at frames 30-33. Others need
                    // their own lift so the airborne press is really airborne.
                    if(!config.packages[0]->trailing_chain)pads[0].b|=frame>=33&&frame<=37;
                }
            }
            if(options.exercise_gred_touchdown){
                pads={};
                if(!source_movement){error="touchdown smoke needs the source-movement package";return fail("script refused");}
                pads[0].b=frame>=3&&frame<=19;
                if(source.battle_landing_entry_pending()&&!touchdown_attempt){
                    pads[0].b=true;pads[0].stick_x=127;touchdown_attempt=true;
                }
                if(touchdown_entry&&!landing_followup&&source.landing_entry_accepted()&&!wait_for_jump_release&&frame%3==0){
                    pads[0].b=true;landing_followup=true;
                }
            }
        }
        // The CPU seat must never consume P2's physical pad or aim edge.
        if(explicit_roster&&!second_human)pads[1]={};
        const bool reset=!choosing&&((pads[0].y&&!previous[0].y)||
            (second_human&&pads[1].y&&!previous[1].y)),
            pause=!choosing&&!source_blocked&&((pads[0].start&&!previous[0].start)||
            (second_human&&pads[1].start&&!previous[1].start));
        const auto dash_held=[](const GamePadState& pad){return pad.l||pad.trigger_l_pressed;};
        const bool source_dash_edge=!choosing&&dash_held(pads[0])&&!dash_held(previous[0]);
        if(hosted&&reset)break;
        for(unsigned i=0;i<2;++i){
            if(explicit_roster&&i<config.roster_local_players&&pads[i].z&&!previous[i].z)pending_aim_cycles|=u8(1u<<i);
            auto& input=action_input[i];
            const auto current=action_bits(pads[i]),prior=action_bits(previous[i]);
            if(const auto pressed=u8(current&~prior);pressed)input.pressed=pressed; // don't merge separate press samples
            input.released=u8((input.released|u8(prior&~current))&~current);
            input.held=current;
        }
        previous=pads;
        if(reset){
            total_shots+=world.shots_spawned();total_hits+=world.hits();if(!restart())return fail("restart refused");
            ++restarts;paused=false;source_blocked=false;movement_notice.clear();
            previous_cues={};
            previous_stock={};
            previous_mobility={};
            previous_fade=-1.f;
        }else if(pause){
            paused=!paused;
            if(!world.set_paused(paused))return fail("pause refused");
            if(audible)context.audio().discard_queued();
        }
        unsigned steps=1;
        if(!options.headless&&!options.demo_package_battle){
            phase+=std::min<u64>(now>=last?now-last:0,250000000)*60;
            steps=unsigned(std::min<u64>(phase/1000000000ull,8));phase%=1000000000ull;
        }
        last=now;
        const bool running=!choosing&&!paused&&!source_blocked&&!reset&&!pause&&
            !series.ended()&&world.result()==BattleMvpResult::Fighting;
        if(!running){action_input={};pending_aim_cycles=0;steps=0;phase=0;}
        for(auto& pad:pads)pad.a=pad.x=pad.y=pad.start=false;
        if(source_movement){
            pads[0]=pc::gred_battle_pad(pads[0],source_dash_edge);
            practice_input.sample(pads[0],source.practice_flick_direction(pads[0]),running);
        }
        while(steps--&&!series.ended()&&world.result()==BattleMvpResult::Fighting&&(!tick_limit||world.tick()<tick_limit)){
            if(hosted){
                const auto enemy=portable_duel_enemy_input(world);
                pads[1]=enemy.movement;action_input[1]=enemy.attack;
            }
            const auto before=world.actors()[0].position;
            const auto prior_facing=world.actors()[0].facing;
            const auto prior_dash=world.actors()[0].dash_ticks;
            const auto movement=explicit_roster?
                NativeBattleMvpView::roster_movement_frame(world):
                NativeBattleMvpView::movement_frame(world,width,height);
            if(source_movement&&options.demo_roster_battle){
                auto input=pads[0];practice_input.take(input);NativeBorgMotionOffer offer;
                const bool touchdown=source.battle_landing_entry_pending();
                if(!input.b)wait_for_jump_release=false;
                if(touchdown&&input.b)wait_for_jump_release=true;
                if(wait_for_jump_release)input.b=false;
                if(!source.prepare_battle_tick(world,view.source_binding(0),movement,input,&offer,&error)){
                    source.reject();return fail("recovered movement refused; no provisional fallback");
                }
                NativeBorgMotionOffer offer2;GamePadState pad2{};pad2.stick_y=-127;
                const auto movement2=NativeBattleMvpView::movement_frame_for(world,2,width,height);
                if(!source2.prepare_battle_tick(world,view.source_binding(2),movement2,pad2,&offer2,&error)){
                    source.reject();source2.reject();return fail("second source movement refused");
                }
                origins.source_local[0]=source.prepared_local(world,offer);
                origins.source_local[2]=source2.prepared_local(world,offer2);
                std::array<const NativeBorgMotionOffer*,4> offers{&offer,nullptr,&offer2,nullptr};
                // The source offer already consumed the input; the validator
                // must see the same empty pads as the duel source path.
                std::array<GamePadState,4> roster_pads{};
                roster_pads[1]=pads[1];
                roster_pads[3]=router.route(snapshot,3);
                std::array<BorgBattleInputs,4> roster_input{action_input[0],action_input[1],{},{}};
                const unsigned ai_cycle=world.tick()%90;
                roster_input[2].pressed=u8(ai_cycle==20?BorgBattleInputs::b:0);
                roster_input[2].released=u8(ai_cycle==22?BorgBattleInputs::b:0);
                roster_input[3].pressed=u8(ai_cycle==65?BorgBattleInputs::b:0);
                roster_input[3].released=u8(ai_cycle==67?BorgBattleInputs::b:0);
                const bool advanced=origins.source_local[0]&&origins.source_local[2]&&
                    world.step_roster(roster_pads,roster_input,false,&view,{},&movement,nullptr,&origins,offers);
                origins.source_local={};
                if(!advanced){source.reject();source2.reject();return fail("roster source contact refused");}
                NativeBorgSourcePose pose0,pose2;
                if(!source.commit(world,view.source_binding(0),&error)||!source.pose(world,&pose0)||
                   !source2.commit(world,view.source_binding(2),&error)||!source2.pose(world,&pose2))
                    return fail("roster source pose was not published");
                const std::array<NativeBattleMvpView::SourcePoseView,2> poses{
                    NativeBattleMvpView::SourcePoseView{0,&pose0},
                    NativeBattleMvpView::SourcePoseView{2,&pose2}};
                if(!view.update_sources(world,poses,&error))return fail("roster source publication refused");
                if(touchdown&&source.landing_entry_accepted()&&offer.kind==NativeBorgMotionKind::JumpLanding)touchdown_entry=true;
            }else if(source_movement){
                auto input=pads[0];practice_input.take(input);NativeBorgMotionOffer offer;
                const bool touchdown=source.battle_landing_entry_pending();
                if(!input.b)wait_for_jump_release=false;
                if(touchdown&&input.b)wait_for_jump_release=true;
                if(wait_for_jump_release)input.b=false;
                if(!source.prepare_battle_tick(world,view.source_binding(),movement,input,&offer,&error,selection!=nullptr)){
                    if(selection){
                        block_movement();
                        break;
                    }
                    source.reject();return fail("recovered movement refused; no provisional fallback");
                }
                origins.source_local[0]=source.prepared_local(world,offer);
                const bool advanced=origins.source_local[0]&&world.step({},false,&view,nullptr,&movement,&offer,action_input,&origins);
                origins.source_local={};
                if(!advanced){
                    if(error.empty())error="native world declined the staged contact offer";
                    if(selection&&source.withdraw_battle_offer(world,offer)){block_movement();break;}
                    source.reject();return fail("recovered movement refused; no provisional fallback");
                }
                NativeBorgSourcePose pose;
                if(!source.commit(world,view.source_binding(),&error)||!source.pose(world,&pose)||!view.update_source(world,pose,&error))
                    return fail("accepted source pose was not published");
                if(touchdown&&source.landing_entry_accepted()&&offer.kind==NativeBorgMotionKind::JumpLanding)touchdown_entry=true;
            }else if(local_roster){
                std::array<GamePadState,kBattleMvpMaxActors> roster_pads{pads[0],pads[1]};
                std::array<BorgBattleInputs,kBattleMvpMaxActors> roster_input{action_input[0],action_input[1]};
                const auto count=world.actor_count();
                const unsigned first_cpu=config.roster_local_players;
                std::array<u8,kBattleMvpMaxActors-1> cpu_aim{};
                bool has_cpu_aim=false;
                if(explicit_roster)for(unsigned i=first_cpu;i<count;++i){
                    const auto ai=roster_ai.input(world,i,&view);
                    roster_pads[i]=ai.movement;roster_input[i]=ai.attack;
                    cpu_aim[i-first_cpu]=ai.aim_target;has_cpu_aim|=ai.aim_target!=255;
                }
                const BattleMvpLocalRosterInput roster_controls{explicit_roster&&second_human?&movement:nullptr,pending_aim_cycles,
                    has_cpu_aim?std::span<const u8>(cpu_aim).first(count-first_cpu):std::span<const u8>{}};
                if(!world.step_roster(std::span<const GamePadState>(roster_pads).first(count),
                                      std::span<const BorgBattleInputs>(roster_input).first(count),
                                      false,&view,{},&movement,nullptr,&origins,{},roster_controls)||
                   !view.update(world,&error))
                    return fail("authored roster update refused");
                if(options.trace_package_battle)for(unsigned i=0;i<config.roster_local_players;++i)if(pending_aim_cycles&(1u<<i))
                    gf::log_info(context,"borg-aim-cycle","round=%u tick=%u actor=%u target=%u",
                                 restarts,world.tick(),i,unsigned(world.aim_target_of(i)));
                pending_aim_cycles=0;
            }else if(!world.step(pads,false,&view,nullptr,&movement,nullptr,action_input,&origins)||!view.update(world,&error))
                return fail("authored battle update refused");
            if(trace_roster){
                const auto alive=roster_alive(),knocked_out=previous_roster_alive&~alive;
                for(unsigned i=0;i<world.actor_count();++i)if(knocked_out&(1u<<i))
                    log_info(context,"borg-roster-ko","round=%u tick=%u actor=%u",restarts,world.tick(),i);
                previous_roster_alive=alive;
            }
            if(options.trace_package_battle&&config.ko_force_pools)
                for(unsigned i=0;i<world.actor_count();++i)
                    if(const auto& debit=world.ko_force_debits()[i];debit&&debit->tick==world.tick())
                        log_info(context,"borg-ko-force","round=%u tick=%u receiver=%u source=%u team=%u cost=%u debit=%u before=%u after=%u exhausted=%u",
                            restarts,debit->tick,i,unsigned(debit->source),unsigned(debit->team),unsigned(debit->cost),
                            unsigned(debit->debited()),unsigned(debit->before),unsigned(debit->after),unsigned(debit->exhausted()));
            if(series.observe(world)){
                const char* outcome=series.outcome()==BattleMvpResult::Draw?"draw":
                    series.outcome()==BattleMvpResult::GRedWins?"team0":"team1";
                if(series.keeps_score())gf::log_info(context,"borg-series","round=%u tick=%u outcome=%s score=%u:%u goal=%u winner=%u",
                    restarts,world.tick(),outcome,unsigned(series.wins()[0]),unsigned(series.wins()[1]),
                    unsigned(series.goal()),unsigned(series.winner()));
                if(series.timed_out())gf::log_info(context,"borg-round-clock","round=%u tick=%u timeout=%s",
                    restarts,world.tick(),outcome);
            }
            for(auto& input:action_input)input.pressed=input.released=0;
            for(unsigned i=0;i<world.actor_count();++i){
                const unsigned ticks=world.actors()[i].reaction_ticks;
                total_reactions+=ticks>previous_reaction_ticks[i];
                previous_reaction_ticks[i]=ticks;
            }
            moved|=before!=world.actors()[0].position;airborne|=!world.actors()[0].grounded;
            if(options.exercise_battle_mobility){
                const auto& actor=world.actors()[0];
                ground_dashes+=!prior_dash&&actor.dash_ticks&&actor.grounded;
                air_dashes+=!prior_dash&&actor.dash_ticks&&!actor.grounded;
                peak_jumps=std::max(peak_jumps,unsigned(actor.package_air_points_used));
                boost_frames+=actor.package_boosting;
                strafed|=pads[0].r&&actor.grounded&&actor.facing==prior_facing&&std::abs(actor.move_right)>.1f&&
                    actor.movement_speed>.05f;
                if(actor.grounded&&!actor.dash_ticks)peak_walk=std::max(peak_walk,actor.movement_speed);
            }
            max_flight=std::max(max_flight,unsigned(world.actors()[0].package_flight_used));
            if(mix_audio){
                if(!movement_audio.visit(world,battle_audio,&movement_pcm)){
                    error="actor slots or round audio bindings changed";return fail("package audio refused");
                }
                launch_audio_starts+=movement_audio.launch_started();
                release_audio_starts+=movement_audio.release_started();
                impact_audio_starts+=movement_audio.impact_started();
                contact_audio_starts+=movement_audio.contact_started();
                contact_audio_nonzero|=movement_audio.contact_nonzero();
                audio_mixed_actors|=movement_audio.mixed_actors();
                audio_dropped+=movement_audio.dropped();
                if(has_launch_sounds&&!movement_pcm.empty()){
                    ++launch_audio_blocks;
                    launch_audio_nonzero|=std::any_of(movement_pcm.begin(),movement_pcm.end(),[](s16 sample){return sample!=0;});
                }
                if(has_release_sounds&&!movement_pcm.empty()){
                    ++release_audio_blocks;
                    release_audio_nonzero|=std::any_of(movement_pcm.begin(),movement_pcm.end(),[](s16 sample){return sample!=0;});
                }
                if(audible&&!movement_pcm.empty()&&context.audio().frames_available()<kBorgMovementAudioFrames*3&&
                   context.audio().try_submit(movement_pcm.data(),movement_pcm.size()/2))
                    movement_audio_frames+=movement_pcm.size()/2;
            }
            for(unsigned i=0;i<world.actor_count();++i){
                if(options.trace_package_battle)
                    if(const auto* tail=view.package_trailing_chain(world,i)){
                        const auto& tip=*tail->tip();const auto& actor=world.actors()[i];
                        gf::log_info(context,"borg-chain","round=%u tick=%u actor=%u grounded=%u landing=%u contacts=%u tip=%.2f,%.2f,%.2f",
                            restarts,world.tick(),i,unsigned(actor.grounded),actor.landing_ticks,tail->contacts(),
                            double(tip[0]),double(tip[1]),double(tip[2]));
                    }
                const auto cues=world.actors()[i].package_movement_cues;
                if(options.trace_package_battle&&cues!=previous_cues[i]&&
                   (!config.packages[i]->movement_fx.empty()||config.packages[i]->air_jump_points))
                    gf::log_info(context,"borg-fx","round=%u tick=%u actor=%u cues=%u",restarts,world.tick(),i,unsigned(cues));
                previous_cues[i]=cues;
            }
            for(const auto& e:world.package_events()){
                const bool credited_hit=e.actor==0&&(local_roster?
                    (e.recipient<world.actor_count()&&world.actors()[e.recipient].team!=world.actors()[0].team):
                    e.recipient==1);
                if((e.kind==BorgBattleEventKind::Hit||e.kind==BorgBattleEventKind::BlastHit)&&credited_hit)
                    (restarts?after_restart_hit:first_hit)=true;
                if(!options.trace_package_battle)continue;
                const auto& moves=config.packages[e.actor]->moves;
                const char* reason=e.kind==BorgBattleEventKind::PullRefused||e.kind==BorgBattleEventKind::PullCancelled||
                    e.kind==BorgBattleEventKind::RewindStopped||e.kind==BorgBattleEventKind::RewindRefused?
                    borg_battle_pull_reason(e.value):nullptr;
                gf::log_info(context,"borg-api","round=%u tick=%u actor=%u move=%s event=%s frame=%u object=%u recipient=%u value=%u%s%s",
                    restarts,e.tick,unsigned(e.actor),e.move<moves.size()?moves[e.move].id.c_str():"-",
                    kinds[unsigned(e.kind)],unsigned(e.frame),e.object,unsigned(e.recipient),unsigned(e.value),
                    reason?" reason=":"",reason?reason:"");
            }
        }
        if(explicit_roster&&options.trace_package_battle)for(unsigned i=0;i<config.roster_local_players;++i){
            if(!world.actors()[i].health)continue;
            const auto& profile=*world.config().packages[i];
            const auto stock=borg_stock_hud(profile,world.package_action(i));
            if(stock!=previous_stock[i]){
                for(unsigned j=0;j<stock.count;++j){
                    const auto& row=stock.rows[j];
                    gf::log_info(context,"borg-ammo","round=%u tick=%u actor=%u pool=%s buttons=%s stock=%u/%u phase=%s",
                        restarts,world.tick(),i,profile.stocks[row.channel].id.c_str(),borg_stock_hud_buttons(row.buttons),
                        unsigned(row.current),unsigned(row.capacity),borg_stock_hud_phase(row.phase));
                }
                previous_stock[i]=stock;
            }
            const auto mobility=borg_mobility_hud(profile,world.actors()[i]);
            if(mobility!=previous_mobility[i]){
                if(mobility)gf::log_info(context,"borg-mobility","round=%u tick=%u actor=%u resource=%s remaining=%u/%u",
                    restarts,world.tick(),i,borg_mobility_labels(mobility->kind).trace,
                    unsigned(mobility->remaining),unsigned(mobility->capacity));
                previous_mobility[i]=mobility;
            }
        }
        render.begin_frame({frame,.15f,.21f,.3f,selection!=nullptr||explicit_roster});render.reset_arena();
        const auto draws=view.draw(render,world,width,height,&error,false,0,nullptr,explicit_roster,
                                   series.enabled()?&series:nullptr);
        if(selection){
            selection->picker.draw(render,s32(width),source_blocked?movement_notice:picker_notice);
            if(width>=440&&height>=270&&!selection->picker.open()){
                const auto& p=*config.packages[0];const auto& a=world.actors()[0];
                const auto meter=borg_mobility_hud(p,a);
                std::string mobility=source_movement?"FLIGHT "+std::to_string(source.resource())+"/10000":
                    meter?borg_mobility_text(*meter):"";
                std::string keys=source_movement?"ARROWS MOVE  SHIFT FLY  ALT+ARROW DASH  SPACE B":
                    p.cruise?"ARROWS STEER  SHIFT BOOST  SPACE B":"ARROWS MOVE  SHIFT JUMP  R STRAFE  SPACE B";
                if(!source_movement&&!p.cruise&&!p.dash_disabled)keys+="  ALT+ARROW DASH";
                auto state=render.state();render.state()={};render.state().blend=true;render.state().blend_src=5;
                render.state().blend_dst=6;render.state().depth_test=render.state().depth_write=render.state().cull_backface=false;
                const Mat4 identity;render.upload_matrix(0,identity.m);
                TextRenderer::Style style;style.scale=1;
                if(!mobility.empty())mechanics_text.draw(render,mobility,20,s32(height)-31,style);
                mechanics_text.draw(render,keys,20,s32(height)-20,style);
                render.state()=state;
            }
        }
        const auto submitted=render.end_frame();
        if(!draws||(hosted&&!submitted))return fail("draw failed");
        if(options.trace_package_battle&&view.projectile_record_draws())
            gf::log_info(context,"projectile-record","round=%u tick=%u draws=%u",restarts,world.tick(),view.projectile_record_draws());
        if(options.trace_package_battle&&view.projectile_trail_draws())
            gf::log_info(context,"projectile-record-trail","round=%u tick=%u draws=%u",restarts,world.tick(),view.projectile_trail_draws());
        if(options.trace_package_battle&&view.impact_record_draws())
            gf::log_info(context,"impact-record","round=%u tick=%u draws=%u",restarts,world.tick(),view.impact_record_draws());
        if(options.trace_package_battle)for(unsigned i=0;i<world.actor_count();++i)
            if(view.package_body_draws(i)&&view.package_pose_held(world,i))
                gf::log_info(context,"contact-pose-hold","round=%u tick=%u actor=%u remaining=%u",restarts,world.tick(),i,
                             unsigned(world.actors()[i].contact_pose_hold_ticks));
        if(options.trace_package_battle)for(unsigned i=0;i<world.actor_count();++i)
            if(const auto parts=view.package_part_draws(i);parts)
                gf::log_info(context,"borg-parts","round=%u tick=%u actor=%u draws=%u",restarts,world.tick(),i,parts);
        if(options.trace_package_battle)
        {
            BattleMvpVec low{},high{};const bool bounded=view.package_body_bounds(world,0,&low,&high);
            gf::log_info(context,"borg-body","round=%u tick=%u actor=0 draws=%u floor=%.2f..%.2f",
                restarts,world.tick(),view.package_body_draws(0),bounded?low[1]:0.f,bounded?high[1]:0.f);
        }
        if(options.trace_package_battle)if(const auto fade=view.stage_view_alpha(0);fade&&*fade!=previous_fade){
            gf::log_info(context,"stage-view","viewer=0 tick=%u alpha=%.3f",world.tick(),double(*fade));
            previous_fade=*fade;
        }
        if(!captured)captured=capture_if_requested(context,options,frame);
        render.present();context.advance_frame();++frame;
        if(hosted){
            end=portable_duel_end(world,tick_limit);
            if(end!=PortableDuelEnd::Fighting)break;
        }
        if(options.frames&&frame>=options.frames)break;
    }
    gf::log_info(context,"package-battle","frames=%u actors=%u restarts=%u moved=%u airborne=%u shots=%u hits=%u reactions=%u",
        frame,world.actor_count(),restarts,unsigned(moved),unsigned(airborne),total_shots+world.shots_spawned(),total_hits+world.hits(),total_reactions);
    if(trace_roster){
        const auto result=series.ended()?series.outcome():world.result();
        const char* name=result==BattleMvpResult::GRedWins?"team0":result==BattleMvpResult::SasukeWins?"team1":
            result==BattleMvpResult::Draw?"draw":"fighting";
        log_info(context,"borg-roster-round","round=%u tick=%u result=%s alive=0x%x",restarts,world.tick(),name,roster_alive());
        for(unsigned i=0;i<world.actor_count();++i){
            const auto& actor=world.actors()[i];
            log_info(context,"borg-roster-state","round=%u tick=%u actor=%u hp=%u position=%.2f,%.2f,%.2f travel=%.2f nearest=%u aim=%u",
                     restarts,world.tick(),i,unsigned(actor.health),double(actor.position[0]),double(actor.position[1]),
                     double(actor.position[2]),actor.travel_distance,unsigned(world.target_of(i)),unsigned(world.aim_target_of(i)));
        }
    }
    if(options.trace_package_battle)if(const auto& force=world.ko_force_remaining();force){
        log_info(context,"borg-ko-force","round=%u tick=%u remaining=%u,%u",restarts,world.tick(),
                 unsigned((*force)[0]),unsigned((*force)[1]));
    }
    if(options.trace_package_battle&&world.actor_count()>2)for(unsigned i=0;i<world.actor_count();++i){
        const auto& actor=world.actors()[i];const auto& history=actor.hit_history;
        const auto& last=history[(actor.hit_history_index+history.size()-1)%history.size()];
        gf::log_info(context,"borg-hit-history","round=%u actor=%u cursor=%u ally-hits=%u ally-damage=%u last=%u:%u",
            restarts,i,unsigned(actor.hit_history_index),unsigned(actor.hit_history_count),actor.hit_history_total,
            unsigned(last.type),unsigned(last.slot));
    }
    if(options.trace_package_battle&&has_sounds)
        gf::log_info(context,"borg-audio","device=%s frames-accepted=%llu",audible?"open":"off",
            static_cast<unsigned long long>(movement_audio_frames));
    if(options.trace_package_battle&&has_sounds&&world.actor_count()>2)
        gf::log_info(context,"borg-roster-audio","mixed-actors=0x%x dropped=%llu",audio_mixed_actors,
            static_cast<unsigned long long>(audio_dropped));
    if(options.trace_package_battle&&has_launch_sounds)
        gf::log_info(context,"borg-launch-audio","starts=%u blocks=%u nonzero=%u",
                     launch_audio_starts,launch_audio_blocks,unsigned(launch_audio_nonzero));
    if(options.trace_package_battle&&has_release_sounds)
        gf::log_info(context,"borg-release-audio","starts=%u blocks=%u nonzero=%u",
                     release_audio_starts,release_audio_blocks,unsigned(release_audio_nonzero));
    if(options.trace_package_battle&&has_impact_sounds)
        gf::log_info(context,"borg-impact-audio","starts=%u",impact_audio_starts);
    if(options.trace_package_battle&&has_contact_sounds)
        gf::log_info(context,"borg-contact-audio","starts=%u nonzero=%u",contact_audio_starts,unsigned(contact_audio_nonzero));
    if(options.trace_package_battle&&config.packages[0]->flight_ticks)
        gf::log_info(context,"package-flight","peak=%u cap=%u",max_flight,unsigned(config.packages[0]->flight_ticks));
    if(options.exercise_battle_mobility)
        gf::log_info(context,"battle-mobility","strafed=%u ground-dashes=%u air-dashes=%u jump-points-used=%u boost-frames=%u peak-walk=%.2f",
            unsigned(strafed),ground_dashes,air_dashes,peak_jumps,boost_frames,double(peak_walk));
    const bool smoke=options.demo_package_battle&&!options.exercise_gred_touchdown&&!series.tick_limit()&&frame>=150;
    if(options.exercise_gred_touchdown){
        gf::log_info(context,"gred-touchdown","attempt=%u entry=%u fresh-jump=%llu",
                     unsigned(touchdown_attempt),unsigned(touchdown_entry),static_cast<unsigned long long>(source.landing_jumps()));
        if(!touchdown_attempt||!touchdown_entry||source.landing_jumps()!=1)return fail("touchdown smoke failed");
    }
    if(report)*report={end==PortableDuelEnd::Fighting?PortableDuelEnd::Cancelled:end,world.tick()};
    render.shutdown();return smoke&&(!moved||!airborne||restarts!=1||!first_hit||!after_restart_hit)?2:0;
}

bool probe_browser_roster(const Options& next,const PackageBattleFiles& files,std::string& error){
    // The preflight consumes the pinned files only. It needs no real I/O,
    // audio, or save backend and never touches the live renderer.
    gf::headless::Bundle devices;
    const auto services=devices.get();
    gf::Context probe_context(services,60);
    gf::RenderEngine probe(*services.render);
    if(!probe.initialize({next.width,next.height,false,true,"Roster check"},services.window,&error))return false;
    auto check=next;
    check.headless=check.no_audio=check.check_battle_roster=true;
    return run_package_battle(probe_context,probe,check,files.first,files.stage.get(),nullptr,nullptr,nullptr,files.extra,&error)==0;
}

int run_roster_browser(gf::Context& context,gf::RenderEngine& render,const Options& options,
                       const gf::BorgPackageCatalog& catalog){
    using namespace gf;
    std::vector<pc::BattleRosterListing> entries;
    std::string error,notice;
    const auto fail=[&](const char* why){
        log_error(context,"roster-browser","%s",why);render.shutdown();return 2;
    };
    if(!pc::scan_battle_roster_presets(options.browse_battle_rosters,&entries,&error))return fail(error.c_str());
    for(const auto& entry:entries)if(!entry.preset)
        log_warn(context,"roster-browser","skipped %s: %s",borg_package_asset_path_valid(entry.filename)?
                 entry.filename.c_str():"<invalid filename>",entry.error.c_str());
    std::erase_if(entries,[](const auto& entry){return !entry.preset;});
    if(entries.empty())return fail("no valid preset metadata in the selected folder");
    std::vector<BorgLabPicker::Entry> rows;
    for(const auto& entry:entries){
        const auto& preset=*entry.preset;
        auto summary=std::to_string(preset.fighters.size())+" slots "+std::to_string(preset.locals)+" local";
        if(preset.rounds_to_win)summary+=" first-to-"+std::to_string(preset.rounds_to_win);
        if(preset.round_seconds)summary+=" "+std::to_string(preset.round_seconds)+"s";
        if(preset.damage_indices)summary+=preset.damage_indices->bank_count?" damage banks":" indexed damage";
        rows.push_back({entry.filename,preset.stage,std::move(summary)});
    }
    const auto row_for=[&](std::string_view name)->std::optional<std::size_t>{
        const auto row=std::find_if(entries.begin(),entries.end(),
            [&](const auto& entry){return entry.filename==name;});
        return row==entries.end()?std::nullopt:std::optional<std::size_t>(row-entries.begin());
    };
    auto scripted=row_for(options.demo_roster_select);
    const auto followup=row_for(options.demo_roster_next);
    if((!options.demo_roster_select.empty()&&!scripted)||(!options.demo_roster_next.empty()&&!followup))
        return fail("scripted preset was not listed");
    const unsigned expected_matches=followup?2u:scripted?1u:0u;
    constexpr s32 controls_height=36;
    BorgLabPicker picker(std::move(rows),BorgLabPicker::fitting_rows(s32(options.height)-controls_height));
    using Command=BorgLabPicker::Command;
    TextRenderer text;
    TextRenderer::Style style;style.scale=1;
    InputRouter router;GamePadState previous_pad;
    bool pointer_held=false,wait_for_pointer_release=false,wait_for_back_release=false,scripted_back_held=false;
    bool wait_for_menu_release=false;
    unsigned menu_frames=0,matches=0,script_frame=0;
    std::size_t active=0;
    const auto start_frame=context.frame_index();
    const auto elapsed=[&](){return context.frame_index()-start_frame;};
    const auto expired=[&](){return options.frames&&elapsed()>=options.frames;};
    log_info(context,"roster-browser","presets=%zu (metadata only)",entries.size());
    while(context.window().pump_events()&&!context.window().should_close()&&!expired()){
        unsigned width=options.width,height=options.height;
        if(!options.headless)context.window().size(&width,&height);
        if(!width||!height)continue;
        context.input().pump();const auto& snapshot=context.input().snapshot();
        auto pad=router.route(snapshot,0);
        const bool back=pad.quit_requested||snapshot.menu_back||scripted_back_held;
        scripted_back_held=false;
        if(snapshot.quit||(back&&!wait_for_back_release))break;
        wait_for_back_release=back;
        const auto menu_active=[](const GamePadState& p){
            return p.a||p.b||p.start||p.dpad_up||p.dpad_down||p.dpad_left||p.dpad_right;
        };
        if(wait_for_menu_release)wait_for_menu_release=menu_active(pad);
        auto pointer=snapshot.pointer;
        pointer.left_pressed|=pointer.left&&!pointer_held;pointer_held=pointer.left;
        if(wait_for_pointer_release){
            pointer.left_pressed=false;wait_for_pointer_release=pointer.left;
        }
        unsigned logical_w=width,logical_h=height;
        if(!options.headless)context.window().logical_size(&logical_w,&logical_h);
        pointer.x=logical_w?s32(s64(pointer.x)*width/logical_w):-1;
        pointer.y=logical_h?s32(s64(pointer.y)*height/logical_h):-1;
        picker.fit_height(s32(height)-controls_height);
        if(scripted&&!back&&!wait_for_menu_release&&options.demo_roster_pad){
            pointer={};pad={};
            if(script_frame++%2==0){
                const auto current=picker.focused_row(),target=*scripted;
                const auto page=picker.rows_per_page();
                if(!picker.open()||current==target)pad.a=true;
                else if(target>current&&target-current>=page)pad.dpad_right=true;
                else if(current>target&&current-target>=page)pad.dpad_left=true;
                else if(current<target)pad.dpad_down=true;
                else pad.dpad_up=true;
            }
        }else if(scripted&&!back&&!options.demo_roster_pad){
            const auto rows=picker.rows_per_page(),from=active/rows,to=*scripted/rows;
            const auto pages=to>from?to-from:from-to;
            const auto frame=script_frame++;
            pointer={};pointer.within_window=true;
            pointer.x=BorgLabPicker::left+8;pointer.y=BorgLabPicker::top+8;
            if(frame==0)pointer.left_pressed=true;
            else if(frame<=pages){
                const auto page=to>from?from+frame-1:from-frame+1;
                const auto visible=std::min(rows,entries.size()-page*rows);
                pointer.left_pressed=true;
                if(to>from)pointer.x+=BorgLabPicker::width(s32(width))/2;
                pointer.y+=s32(visible+1)*BorgLabPicker::row_height;
            }
            else if(frame==pages+1){
                pointer.left_pressed=true;
                pointer.y+=s32(*scripted%rows+1)*BorgLabPicker::row_height;
            }
        }
        std::optional<std::size_t> picked;
        if(!back){
            if(pointer.left_pressed||pointer.wheel)picked=picker.step(pointer,s32(width));
            else if(!wait_for_menu_release){
                if(pad.b&&!previous_pad.b)picker.navigate(Command::Cancel);
                else if((pad.a&&!previous_pad.a)||(pad.start&&!previous_pad.start))picked=picker.navigate(Command::Confirm);
                else if(pad.dpad_up&&!previous_pad.dpad_up&&!pad.dpad_down)picker.navigate(Command::Up);
                else if(pad.dpad_down&&!previous_pad.dpad_down&&!pad.dpad_up)picker.navigate(Command::Down);
                else if(pad.dpad_left&&!previous_pad.dpad_left&&!pad.dpad_right)picker.navigate(Command::PreviousPage);
                else if(pad.dpad_right&&!previous_pad.dpad_right&&!pad.dpad_left)picker.navigate(Command::NextPage);
            }
        }
        previous_pad=pad;
        if(picked){
            active=*picked;picker.activate(active);scripted.reset();
            const auto& entry=entries[*picked];
            const auto file=std::filesystem::path(options.browse_battle_rosters)/entry.filename;
            BattleRosterPreset selected;
            bool ok=pc::load_battle_roster_preset(file,&selected,&error);
            if(ok&&selected!=*entry.preset){ok=false;error="preset changed since listing; reopen the browser";}
            Options next=options;
            next.browse_battle_rosters.clear();next.demo_roster_select.clear();next.demo_roster_next.clear();
            next.width=width;next.height=height;
            if(next.frames)next.frames-=elapsed();
            PackageBattleFiles files;
            if(ok&&!apply_battle_roster_preset(next,std::move(selected))){
                ok=false;error="damage indices supplied by both the preset and the caller";
            }
            if(ok)ok=load_package_battle_files(next,catalog,files,error)&&probe_browser_roster(next,files,error);
            if(ok){
                next.demo_roster_cancel=followup&&matches==0;
                next.roster_browser_match=true;
                const auto battle_start=context.frame_index();
                log_info(context,"roster-browser","selected %s menu-frames=%u",entry.filename.c_str(),menu_frames);
                context.window().set_title("Borg API Battle Test (Esc / Back: Presets)");render.state()={};
                const int code=run_package_battle(context,render,next,files.first,files.stage.get(),nullptr,nullptr,nullptr,files.extra);
                if(!code)++matches;
                // Battle Test has released its renderer and audio by here.
                if(context.window().should_close()||expired())return code?code:matches<expected_matches?2:0;
                context.window().set_title("Battle Roster Presets");
                render.clear_scene_lighting();render.clear_shared_texture_upload_error();render.state()={};
                if(!render.initialize({width,height,!options.headless,true,"Battle Roster Presets"},
                                      context.backends().window,&error))return fail(error.c_str());
                wait_for_back_release=wait_for_pointer_release=wait_for_menu_release=true;
                previous_pad={};
                if(!code&&next.demo_roster_cancel){
                    scripted=followup;script_frame=0;scripted_back_held=true;
                }
                notice=code?"BATTLE FAILED - CHOOSE A PRESET":"";
                log_info(context,"roster-browser","returned %s battle-frames=%u status=%s",entry.filename.c_str(),
                         context.frame_index()-battle_start,code?"failed":"closed");
                continue;
            }
            log_warn(context,"roster-browser","refused %s: %s",entry.filename.c_str(),error.c_str());
            notice="REFUSED: "+error;error.clear();
        }
        render.begin_frame({context.frame_index(),.065f,.09f,.14f,true});render.reset_arena();
        picker.draw(render,s32(width),notice);
        const auto state=render.state();render.state()={};render.state().blend=true;
        render.state().blend_src=5;render.state().blend_dst=6;
        text.draw(render,"MOVE: ARROWS / D-PAD",20,std::max(0,s32(height)-34),style);
        text.draw(render,"PICK: CLICK / ENTER / A",20,std::max(0,s32(height)-23),style);
        text.draw(render,"EXIT: ESC / BACK",20,std::max(0,s32(height)-12),style);
        render.state()=state;
        if(!render.end_frame())return fail("could not draw the preset browser");
        render.present();context.advance_frame();++menu_frames;
    }
    log_info(context,"roster-browser","closed menu-frames=%u%s",menu_frames,matches?"":" without launch");
    render.shutdown();
    return matches<expected_matches?2:0;
}

int run_battle_picker(gf::Context& context,gf::RenderEngine& render,const Options& options,
                      const gf::BorgPackageCatalog& catalog,std::array<gf::pc::BorgBattlePackageFiles,2>& packages){
    using namespace gf;
    using Entry=BorgLabPicker::Entry;
    std::vector<Entry> rows;
    const bool stationary_opponent=packages[1].definition->movement==BorgBattleMovement::Stationary;
    for(const auto& entry:catalog.discovered){
        if(entry.manifest.gameplay.empty())continue;
        std::filesystem::path folder;BorgCatalogProfile info;BorgGameplayProfile rules;
        std::string bytes,error;
        if(!pc::borg_catalog_detail::read_profile("borgs",entry,&folder,&info,&error)||
           !pc::borg_package_scan_detail::read_local(folder,entry.manifest.gameplay,kBorgGameplayBytes,&bytes)||
           !borg_read_gameplay_profile(bytes,&rules,&error)||rules.variant!=info.default_variant||
           rules.movement==BorgBattleMovement::Stationary||
           (rules.movement==BorgBattleMovement::NormalGRed&&!stationary_opponent))continue;
        if(info.details&&(info.details->references[unsigned(BorgDefinitionFacet::Forms)].kind!=BorgDefinitionReferenceKind::None||
                         info.details->references[unsigned(BorgDefinitionFacet::VariantOverrides)].kind!=BorgDefinitionReferenceKind::None))continue;
        rows.push_back({entry.manifest.identity.id,entry.manifest.identity.name,rules.variant});
    }
    const auto tail=[](const Entry& row){return row.id=="example:flight-stick"||row.id=="example:jump-stick";};
    std::stable_partition(rows.begin(),rows.end(),[&](const Entry& row){return !tail(row);});
    const auto row_for=[&](std::string_view id)->std::optional<std::size_t>{
        const auto found=std::find_if(rows.begin(),rows.end(),[&](const auto& row){return row.id==id;});
        return found==rows.end()?std::nullopt:std::optional<std::size_t>(found-rows.begin());
    };
    const auto initial=row_for(packages[0].identity.id), scripted=row_for(options.demo_battle_pick);
    if(!initial||(!options.demo_battle_pick.empty()&&(!scripted||scripted==initial))){
        log_error(context,"battle-picker","the exact active and requested test Borgs must be selectable");
        render.shutdown();return 2;
    }
    BattlePickerSession session{BorgLabPicker(std::move(rows),BorgLabPicker::fitting_rows(s32(options.height))),
                                catalog,*initial,scripted,std::nullopt};
    session.picker.activate(session.active);
    log_info(context,"battle-picker","players=%zu (exact base gameplay; assets validated on selection)",session.picker.entries().size());
    if(options.trace_package_battle)for(std::size_t i=0;i<session.picker.entries().size();++i)
        log_info(context,"battle-picker","row=%zu id=%s",i,session.picker.entries()[i].id.c_str());
    Options round=options;
    const auto started=context.clock().monotonic_ns();
    bool switched=false;
    for(;;){
        const int result=run_package_battle(context,render,round,packages,nullptr,nullptr,nullptr,&session);
        if(result)return result;
        if(!session.next_player)return scripted&&!switched?2:0;
        if(round.frames){
            if(session.elapsed_frames>=round.frames)return scripted?2:0;
            round.frames-=session.elapsed_frames;
        }
        if(options.seconds){
            const auto elapsed=(context.clock().monotonic_ns()-started)/1000000000ull;
            if(elapsed>=options.seconds)return 0;
            round.seconds=unsigned(options.seconds-elapsed);
        }
        packages[0]=std::move(*session.next_player);session.next_player.reset();
        session.active=session.next_row;session.picker.activate(session.active);session.scripted_row.reset();
        render.clear_scene_lighting();render.clear_shared_texture_upload_error();render.state()={};
        std::string error;
        if(!render.initialize({options.width,options.height,!options.headless,true,"Borg API Battle Test"},
                              context.backends().window,&error)){
            log_error(context,"battle-picker","render restart refused: %s",error.c_str());return 2;
        }
        switched=true;
        log_info(context,"battle-picker","active id=%s variant=%s (new round)",packages[0].identity.id.c_str(),
                 packages[0].definition->variant.c_str());
    }
}

int run_portable_challenge(gf::Context& context,gf::RenderEngine& render,const Options& options,
                           const gf::BorgPackageCatalog& borgs,gf::pc::PortableChallengeLaunch& launch){
    gf::NativeBattleMvpView view;
    gf::pc::PortableBattleHost host(borgs,*launch.saved_force,view,render);
    if(const auto check=host.admit(*launch.run);!check){
        gf::log_error(context,"portable-challenge","admission refused: %s",check.reason.c_str());
        render.shutdown();return 2;
    }
    gf::pc::PortableBattleHostReport report;
    const auto code=run_package_battle(context,render,options,host.packages(),nullptr,&host,&report);
    if(!host.finish(*launch.run,code?gf::pc::PortableBattleHostReport{}:report)){
        gf::log_error(context,"portable-challenge","battle report did not match the accepted request");
        return 2;
    }
    if(!code){
        const auto& choice=*options.portable_challenge;
        gf::log_info(context,"portable-challenge","challenge=%s encounter=%s result=%s ticks=%u completed=%zu; read-only save",
            choice.id.c_str(),choice.encounter.c_str(),gf::portable_duel_end_name(report.end),report.ticks,launch.run->completed_count());
    }
    return code;
}

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
    const auto timed_followup=options.timed_ledge_followup;
    const bool expiring_short_relaunch=timed_followup==TimedLedgeFollowup::ExpiringShortAirDashRelaunch;
    const bool expiring_air_relaunch=expiring_short_relaunch||timed_followup==TimedLedgeFollowup::ExpiringAirDashRelaunch;
    const bool expiring_snap_relaunch=timed_followup==TimedLedgeFollowup::ExpiringAirDashSnapRelaunch;
    const bool expiring_air_snap=expiring_snap_relaunch||timed_followup==TimedLedgeFollowup::ExpiringAirDashSnap;
    const bool expiring_air_dash=expiring_air_relaunch||expiring_air_snap||timed_followup==TimedLedgeFollowup::ExpiringAirDash;
    const bool direct_expiry=timed_followup==TimedLedgeFollowup::ExpiryDirectRelaunch;
    const bool direct_expiring=timed_followup==TimedLedgeFollowup::ExpiringDirectRelaunch;
    const bool adjacent_expiring_push=timed_followup==TimedLedgeFollowup::ExpiringAdjacentRelaunchPush;
    const bool direct_expiring_push=adjacent_expiring_push||timed_followup==TimedLedgeFollowup::ExpiringDirectRelaunchPush;
    const bool direct_timed_up=timed_followup==TimedLedgeFollowup::DirectTimedUpRelaunch;
    const bool direct_timed=direct_timed_up||timed_followup==TimedLedgeFollowup::DirectTimedRelaunch;
    const bool timed_release_two=timed_followup==TimedLedgeFollowup::TimedReleaseTwoPushes;
     const bool rising_diagonal_windup=timed_followup==TimedLedgeFollowup::RisingDiagonalWindupRelaunch;
     const bool rising_windup_relaunch=rising_diagonal_windup||timed_followup==TimedLedgeFollowup::RisingWindupRelaunch;
    const bool same_face_expiry=timed_followup==TimedLedgeFollowup::SameFaceExpiryPush;
    const bool timed_release_expiry=same_face_expiry||rising_windup_relaunch||timed_followup==TimedLedgeFollowup::TimedReleaseExpiryDrift;
    const bool timed_release_four=timed_release_expiry||timed_followup==TimedLedgeFollowup::TimedReleaseFourPushes;
    const bool timed_release_three=timed_release_four||timed_followup==TimedLedgeFollowup::TimedReleaseThreePushes;
    const bool timed_release_push=timed_release_three||timed_release_two||timed_followup==TimedLedgeFollowup::TimedReleasePush;
    struct PracticeTimingGuide {const char* hud;const char* name;const char* inputs;};
    constexpr PracticeTimingGuide seam_guide{"RECOVERY SEAM\nSHIFT 6-39 / W 43 / RIGHT ARROW 105-109",
        "recovery seam","Shift 6-39, W 43, Right Arrow 105-109"};
    constexpr PracticeTimingGuide push_guide{"RECOVERY COMPLETION PUSH\nSHIFT 6-38 / W 46-72 / LEFT ARROW 111-112",
        "recovery completion push","Shift 6-38, W 46-72, Left Arrow 111-112"};
    constexpr PracticeTimingGuide exit_guide{"RECOVERY EXIT\nSHIFT 6-38 / W 46-72\nLEFT 111 / UP+LEFT 112",
        "recovery completion exit","Shift 6-38, W 46-72, Left 111, Up+Left 112"};
    constexpr PracticeTimingGuide ledge_relaunch_guide{"TIMED LEDGE RELAUNCH\nRIGHT ARROW 6-8 / SHIFT 30-34",
        "timed ledge relaunch","Right Arrow 6-8, Shift 30-34"};
    constexpr PracticeTimingGuide ledge_air_dash_guide{"TIMED LEDGE AIR DASH\nRIGHT ARROW 6-8 / W 30",
        "timed ledge air dash","Right Arrow 6-8, W 30"};
    constexpr PracticeTimingGuide ledge_direct_dash_guide{"DIRECT TIMED LEDGE AIR DASH\nRIGHT ARROW 6-8 / A+W 28",
        "direct timed ledge air dash","Right Arrow 6-8, A+W 28"};
    constexpr PracticeTimingGuide ledge_expiring_dash_guide{"AIR DASH ON EXPIRY\nLEFT ARROW 6-7 / A 38 / NEUTRAL AFTER",
        "air dash on expiry","Left Arrow 6-7, tap A on 38, then neutral"};
    constexpr PracticeTimingGuide ledge_expiring_dash_snap_guide{"WIND-UP ENTRY SNAP ON EXPIRY\nLEFT ARROW 6-9 / A 40 / NEUTRAL AFTER",
        "wind-up entry snap on expiry","Left Arrow 6-9, tap A on 40, then neutral"};
    constexpr PracticeTimingGuide ledge_expiring_dash_snap_handoff_guide{"EXPIRY SNAP WIND-UP TO RELAUNCH\nLEFT ARROW 6-9 / A 40 / SHIFT 47 OR NEUTRAL DASH",
        "expiry snap wind-up relaunch","Left Arrow 6-9, A on 40, then neutral Shift on 47 to relaunch or no input to dash"};
    constexpr PracticeTimingGuide ledge_expiring_dash_handoff_guide{"EXPIRY WIND-UP TO RELAUNCH\nLEFT ARROW 6-7 / A 38 / SHIFT 45 OR NEUTRAL DASH",
        "expiry-step wind-up relaunch","Left Arrow 6-7, tap A on 38, then neutral Shift on 45 to relaunch or no input to dash"};
    constexpr PracticeTimingGuide ledge_expiring_short_handoff_guide{"DIAGONAL EXPIRY WIND-UP TO RELAUNCH\nLEFT ARROW 6-7 / A+W 38 / SHIFT 44\nRELEASE SHIFT / DOWN ARROW 45-50",
        "diagonal expiry-step wind-up relaunch","Left Arrow 6-7, A+W on 38, tap neutral Shift on 44, then Down Arrow 45-50"};
    constexpr PracticeTimingGuide ledge_handoff_guide{"TIMED WIND-UP TO RELAUNCH\nRIGHT ARROW 6-8 / W 30 / SHIFT 36",
        "timed wind-up relaunch","Right Arrow 6-8, W 30, Shift 36"};
    constexpr PracticeTimingGuide ledge_expiry_handoff_guide{"EXPIRY WIND-UP TO RELAUNCH\nLEFT ARROW 6-25 / A 57 / SHIFT 63",
        "expiry wind-up relaunch","Left Arrow 6-25, A 57, Shift 63"};
    constexpr PracticeTimingGuide ledge_expiry_direct_guide{"DIRECT EXPIRY RELAUNCH\nLEFT ARROW 6-25 / UP+SHIFT 57-76",
        "direct expiry relaunch","Left Arrow 6-25, Up Arrow+Shift 57-76"};
    constexpr PracticeTimingGuide ledge_expiring_direct_guide{"DIRECT RELAUNCH ON EXPIRY\nLEFT ARROW 6-7 / UP+SHIFT 38 / NEUTRAL AFTER",
        "direct relaunch on expiry","Left Arrow 6-7, tap Up Arrow+Shift on 38, then neutral"};
    constexpr PracticeTimingGuide ledge_expiring_push_guide{"RELAUNCH ENTRY PUSH ON EXPIRY\nLEFT ARROW 6-25 / UP+SHIFT 56-59 / NEUTRAL AFTER",
        "relaunch entry push on expiry","Left Arrow 6-25, hold Up Arrow+Shift 56-59, then neutral"};
    constexpr PracticeTimingGuide ledge_adjacent_expiring_push_guide{"ADJACENT ENTRY PUSH ON EXPIRY / FACE 1998\nLEFT ARROW 6-25 / UP+SHIFT 56-59 / NEUTRAL AFTER",
        "adjacent entry push on expiry","Left Arrow 6-25, hold Up Arrow+Shift 56-59, then neutral"};
    constexpr PracticeTimingGuide ledge_timed_direct_guide{"DIRECT TIMED RELAUNCH\nUP+LEFT ARROWS 6-7 / SHIFT 25-32",
        "direct timed relaunch","Up+Left Arrows 6-7, then Shift 25-32"};
    constexpr PracticeTimingGuide ledge_timed_direct_up_guide{"DIRECT TIMED UP RELAUNCH\nUP+LEFT ARROWS 6-7 / UP+SHIFT 25-32",
        "direct timed Up relaunch","Up+Left Arrows 6-7, then Up Arrow+Shift 25-32"};
    constexpr PracticeTimingGuide ledge_timed_release_push_guide{"TIMED RELAUNCH-FALL PUSH\nRIGHT ARROW 6-8 / SHIFT 23 / NEUTRAL AFTER",
        "timed relaunch-fall push","Right Arrow 6-8, tap Shift on 23, then neutral"};
    constexpr PracticeTimingGuide ledge_timed_release_two_guide{"TWO TIMED RELAUNCH-FALL PUSHES\nDOWN+RIGHT ARROWS 6-7 / SHIFT 20 / NEUTRAL AFTER",
        "two timed relaunch-fall pushes","Down+Right Arrows 6-7, tap Shift on 20, then neutral"};
    constexpr PracticeTimingGuide ledge_timed_release_three_guide{"THREE TIMED RELAUNCH-FALL PUSHES\nUP+LEFT ARROWS 6-7 / SHIFT 20 / NEUTRAL AFTER",
        "three timed relaunch-fall pushes","Up+Left Arrows 6-7, tap Shift on 20, then neutral"};
    constexpr PracticeTimingGuide ledge_timed_release_four_guide{"TIMED DOWN-DRIFT PUSH\nUP+LEFT ARROWS 6-7 / SHIFT 20 / NEUTRAL THROUGH 36\nDOWN ARROW 37 / NEUTRAL AFTER",
        "timed Down-drift push","Up+Left Arrows 6-7, tap Shift on 20, neutral through 36, Down Arrow on 37 only"};
    constexpr PracticeTimingGuide ledge_timed_release_expiry_guide{"EXPIRY DRIFT PUSH\nUP+LEFT 6-7 / TAP SHIFT 20 / DOWN 37\nDOWN+RIGHT 38 / ELSE NEUTRAL\nOPT: SHIFT 41-50 OR SHIFT 41-47 + W ON 47",
        "expiry drift push","Up+Left Arrows 6-7, tap Shift on 20, neutral through 36, Down on 37, Down+Right on 38, neutral on 39-40, "
        "optional fresh Shift on 41-50 or Shift on 41-47 with a W tap on 47"};
    constexpr PracticeTimingGuide rising_windup_relaunch_guide{"RISING WIND-UP RELAUNCH\nUP+LEFT 6-7 / SHIFT 20 / DOWN 37 / DOWN+RIGHT 38\nSHIFT 41-47 + W ON 47 / TAP SHIFT 53\nELSE NEUTRAL",
        "rising wind-up relaunch","Up+Left Arrows 6-7, tap Shift 20, Down 37, Down+Right 38, Shift 41-47 with W on 47, then a neutral Shift tap on 53"};
    constexpr PracticeTimingGuide rising_diagonal_windup_guide{"DIAGONAL RISING RELAUNCH\nUP+LEFT 6-7 / SHIFT 20 / DOWN 37 / DOWN+RIGHT 38\nSHIFT 41-47 + W+D ON 47 / TAP SHIFT 54\nELSE NEUTRAL",
        "diagonal rising wind-up relaunch","Up+Left Arrows 6-7, tap Shift 20, Down 37, Down+Right 38, Shift 41-47 with W+D on 47, then a neutral Shift tap on 54"};
    constexpr PracticeTimingGuide same_face_expiry_guide{"SAME-FACE EXPIRY PUSH\nUP+LEFT 6-7 / TAP SHIFT 20 / NEUTRAL THROUGH 36\nHOLD DOWN ARROW 37-38 / NEUTRAL AFTER",
        "same-face expiry push","Up+Left Arrows 6-7, tap Shift on 20, neutral through 36, hold Down Arrow on 37-38, then neutral"};
    constexpr PracticeTimingGuide ledge_floor_guide{"TIMED RELAUNCH FLOOR\nDOWN+RIGHT ARROWS 6-122 / SHIFT 147-157",
        "timed relaunch floor","Down+Right Arrows 6-122, Shift 147-157"};
    constexpr PracticeTimingGuide ledge_snap_dash_guide{"TIMED SNAP TO AIR DASH\nDOWN+RIGHT ARROWS 6-122 / SHIFT 148-150 / D+S 150",
        "timed snap to air dash","Down+Right Arrows 6-122, Shift 148-150 with D+S on 150"};
    constexpr PracticeTimingGuide ledge_snap_handoff_guide{"FLOOR WIND-UP TO RELAUNCH\nDOWN+RIGHT ARROWS 6-122 / SHIFT 148-150\nD+S 150 / SHIFT 156",
        "floor wind-up relaunch","Down+Right Arrows 6-122, Shift 148-150 with D+S on 150, then Shift 156"};
    constexpr PracticeTimingGuide positive_snap_handoff_guide{"POSITIVE FLOOR WIND-UP\nUP ARROW 6-8 / SHIFT 25-26\nW 26 / SHIFT 32",
        "positive floor wind-up relaunch","Up Arrow 6-8, Shift 25-26 with W on 26, then Shift 32"};
    constexpr PracticeTimingGuide active_snap_relaunch_guide{"ACTIVE SNAP TO RELAUNCH\nUP ARROW 6-8 / SHIFT 25-26\nW 26 / NEUTRAL 27-32 / SHIFT 33",
        "active snap relaunch","Up Arrow 6-8, Shift 25-26 with W on 26, neutral 27-32, then Shift 33"};
    constexpr PracticeTimingGuide expiring_snap_handoff_guide{"FLOOR HANDOFF ON EXPIRY\nUP ARROW 6-25 / SHIFT 46-47\nA 47 / SHIFT 55",
        "expiring floor wind-up relaunch","Up Arrow 6-25, Shift 46-47 with A on 47, then Shift 55"};
    constexpr PracticeTimingGuide incline_jump_guide{"INCLINE DASH JUMP\nS 6 / SHIFT 14-37",
        "incline dash jump","S 6, Shift 14-37"};
    constexpr PracticeTimingGuide incline_ledge_relaunch_guide{"INCLINED DASH LEDGE RELAUNCH\nD 6 / SHIFT 8 / NEUTRAL AFTER",
        "inclined dash ledge relaunch","Tap D on 6, tap neutral Shift on 8, then stay neutral"};
    constexpr PracticeTimingGuide incline_landing_guide{"TIMED INCLINE LANDING DASH\nLEFT ARROW 6-9 / A 34",
        "timed incline landing dash","Left Arrow 6-9, A 34"};
    const PracticeTimingGuide* guide=nullptr;
    if(options.gred_practice_start&&options.source_ground_interactive){
        if(timed_followup==TimedLedgeFollowup::Relaunch)guide=&ledge_relaunch_guide;
        else if(timed_followup==TimedLedgeFollowup::AirDash)guide=&ledge_air_dash_guide;
        else if(timed_followup==TimedLedgeFollowup::DirectAirDash)guide=&ledge_direct_dash_guide;
        else if(expiring_short_relaunch)guide=&ledge_expiring_short_handoff_guide;
        else if(expiring_air_relaunch)guide=&ledge_expiring_dash_handoff_guide;
        else if(expiring_snap_relaunch)guide=&ledge_expiring_dash_snap_handoff_guide;
        else if(expiring_air_snap)guide=&ledge_expiring_dash_snap_guide;
        else if(expiring_air_dash)guide=&ledge_expiring_dash_guide;
        else if(timed_followup==TimedLedgeFollowup::HandoffRelaunch)guide=&ledge_handoff_guide;
        else if(timed_followup==TimedLedgeFollowup::ExpiryHandoffRelaunch)guide=&ledge_expiry_handoff_guide;
        else if(direct_expiry)guide=&ledge_expiry_direct_guide;
        else if(direct_expiring)guide=&ledge_expiring_direct_guide;
        else if(adjacent_expiring_push)guide=&ledge_adjacent_expiring_push_guide;
        else if(direct_expiring_push)guide=&ledge_expiring_push_guide;
        else if(direct_timed_up)guide=&ledge_timed_direct_up_guide;
        else if(direct_timed)guide=&ledge_timed_direct_guide;
        else if(same_face_expiry)guide=&same_face_expiry_guide;
        else if(rising_diagonal_windup)guide=&rising_diagonal_windup_guide;
        else if(rising_windup_relaunch)guide=&rising_windup_relaunch_guide;
        else if(timed_release_expiry)guide=&ledge_timed_release_expiry_guide;
        else if(timed_release_four)guide=&ledge_timed_release_four_guide;
        else if(timed_release_three)guide=&ledge_timed_release_three_guide;
        else if(timed_release_two)guide=&ledge_timed_release_two_guide;
        else if(timed_release_push)guide=&ledge_timed_release_push_guide;
        else if(options.gred_expiring_snap_windup_relaunch)guide=&expiring_snap_handoff_guide;
        else if(options.gred_positive_snap_windup_relaunch)guide=&positive_snap_handoff_guide;
        else if(options.gred_active_snap_relaunch)guide=&active_snap_relaunch_guide;
        else if(options.gred_timed_snap_windup_relaunch)guide=&ledge_snap_handoff_guide;
        else if(options.gred_timed_relaunch_snap_dash)guide=&ledge_snap_dash_guide;
        else if(options.gred_timed_relaunch_floor)guide=&ledge_floor_guide;
        else if(options.gred_inclined_dash_jump)guide=&incline_jump_guide;
        else if(options.gred_inclined_dash_ledge_relaunch)guide=&incline_ledge_relaunch_guide;
        else if(options.gred_timed_incline_landing_dash)guide=&incline_landing_guide;
        else if(options.gred_recovery_completion_exit)guide=&exit_guide;
        else if(options.gred_recovery_completion_push)guide=&push_guide;
        else if(options.gred_recovery_first_contact_seam)guide=&seam_guide;
    }
    //Explicit native demo placement at original stage473 floor triangle1297's
    //interior, not a recovered actor constructor/spawn or edited stage asset.
    if(options.source_surface&&!options.source_ground_interactive)config.spawn[0]={-619.2617f,169.4453f};
    if(options.source_steep&&!options.source_ground_interactive)config.spawn[0]={-606.6735f,-1373.0029f}; //native placement on steep original triangle1320
    if(options.source_fall_surface&&!options.source_ground_interactive)config.spawn[0]={-606.4511719f,-1373.5872803f}; //native interior point on the same original steep face
    //Bounded original-stage recovery route; BOTH placements are native demo
    //assumptions, not recovered actor constructor/spawn or edited stage data.
    if(options.gred_recovery_completion_push&&use_demo_start){
        config.arena_half=4500;config.spawn[0]={options.gred_recovery_completion_exit?-580.f:-575.f,-2990};
    }else if((options.source_recovery_surface||options.source_ground_recovery_push)&&!options.source_ground_interactive){
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
    if((options.source_ground_loss_surface||options.source_ground_loss_jump||options.source_ground_loss_landing||options.source_ground_loss_air||options.source_ground_loss_relaunch)&&
        (!options.source_ground_interactive||(options.gred_timed_relaunch_floor&&use_demo_start))){
        config={};config.arena_half=4500;config.spawn[0]={-1368.47f,(options.source_ground_loss_landing||options.source_ground_loss_relaunch)?-466.667f:-628.674f};config.spawn_query_y[0]=94.339f;
        if(options.gred_expiring_snap_windup_relaunch){config.spawn[0]={3895.56787f,-1816.47241f};config.spawn_query_y[0]=262.01004f;}
        else if(options.gred_positive_snap_windup_relaunch||options.gred_active_snap_relaunch){
            config.spawn[0]={2892.57739f,-2198.41479f};config.spawn_query_y[0]=151.148178f;
        }
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
        if(options.gred_timed_incline_landing_dash){config.spawn[0]={-1719.38672f,-508.356781f};config.spawn_query_y[0]=343.13092f;}
        if(options.gred_inclined_dash_ledge){config.spawn[0]={2582.89014f,-2846.36206f};config.spawn_query_y[0]=116.592773f;}
        if(options.gred_inclined_walk_ledge){config.spawn[0]={2599.00806f,-2758.12109f};config.spawn_query_y[0]=125.757187f;}
        if(options.gred_walk_ledge_fall_push){
            config.spawn[0]={1000.65021f,options.gred_walk_ledge_high_angle_push?-3362.34546f:-3363.34546f};
            config.spawn_query_y[0]=1549.25293f;
        }
        if(options.gred_walk_ledge_expiry_push||direct_expiring_push){
            config.spawn[0]={-1250.96863f,adjacent_expiring_push?-3315.80225f:-3313.80225f};config.spawn_query_y[0]=1846.01367f;
        }
        if(direct_expiring){config.spawn[0]={-1080.47473f,-3357.9165f};config.spawn_query_y[0]=1846.01367f;}
        if(expiring_air_dash){
            config.spawn[0]=expiring_air_snap?gf::BattlePoint{-1078.91504f,-3407.48291f}:gf::BattlePoint{-1067.30347f,-3415.41772f};
            config.spawn_query_y[0]=1846.01367f;
        }
        if(direct_timed){config.spawn[0]={-4066.18164f,-3400.83032f};config.spawn_query_y[0]=1846.01367f;}
        if(timed_release_three){config.spawn[0]={-4080.66113f,-3463.83618f};config.spawn_query_y[0]=1846.01367f;}
        else if(timed_release_two){config.spawn[0]={-1298.54333f,-3395.96802f};config.spawn_query_y[0]=1846.01367f;}
        else if(timed_release_push){config.spawn[0]={1000.65021f,-3365.34546f};config.spawn_query_y[0]=1549.25293f;}
    }
    if(!owned_current()||!view.load(context.files(),options.battle_mvp_assets,render,&error,practice)||
       !world.start(config,&view)||!view.update(world,&error)){
        gf::log_error(context,"battle-mvp","initialization failed: %s",error.c_str());render.shutdown();return 2;}
    gf::NativeGRedGroundDashDemo source;gf::GamePadState source_previous;
    const auto source_inputs=options.source_ground_assets.empty()?std::string(options.source_walk_ledge?"local/content/source-gred-ground-v8":options.source_relaunch_floor?"local/content/source-gred-ground-v7":options.source_steep?"local/content/source-gred-ground-v6":options.source_surface?"local/content/source-gred-ground-v5":options.source_air_dash?"local/content/source-gred-ground-v4":options.source_moving_jump?"local/content/source-gred-ground-v3":
        options.source_ordered_ground?"local/content/source-gred-ground-v9":options.source_ground_walk?"local/content/source-gred-ground-v2":"local/content/source-gred-ground-v1"):options.source_ground_assets;
    const auto source_mode=expiring_snap_relaunch?gf::NativeGRedGroundDashDemo::Mode::GroundWalkExpiringAirDashSnapRelaunch:
          expiring_air_snap?gf::NativeGRedGroundDashDemo::Mode::GroundWalkExpiringAirDashSnap:
          expiring_short_relaunch?gf::NativeGRedGroundDashDemo::Mode::GroundWalkExpiringShortAirDashRelaunch:
          expiring_air_relaunch?gf::NativeGRedGroundDashDemo::Mode::GroundWalkExpiringAirDashRelaunch:
          expiring_air_dash?gf::NativeGRedGroundDashDemo::Mode::GroundWalkExpiringAirDash:
          adjacent_expiring_push?gf::NativeGRedGroundDashDemo::Mode::GroundWalkExpiringAdjacentRelaunchPush:
          direct_expiring_push?gf::NativeGRedGroundDashDemo::Mode::GroundWalkExpiringDirectRelaunchPush:
          direct_expiring?gf::NativeGRedGroundDashDemo::Mode::GroundWalkExpiringDirectRelaunch:
            same_face_expiry?gf::NativeGRedGroundDashDemo::Mode::GroundWalkExpirySameFacePush:
            rising_diagonal_windup?gf::NativeGRedGroundDashDemo::Mode::GroundRisingDiagonalWindupRelaunch:
            rising_windup_relaunch?gf::NativeGRedGroundDashDemo::Mode::GroundRisingPlainWindupRelaunch:
           timed_release_four?gf::NativeGRedGroundDashDemo::Mode::GroundTimedWalkReleaseDownPush:
          timed_release_three?gf::NativeGRedGroundDashDemo::Mode::GroundTimedWalkReleaseThirdPush:
          timed_release_two?gf::NativeGRedGroundDashDemo::Mode::GroundTimedWalkReleaseSecondPush:
          timed_release_push?gf::NativeGRedGroundDashDemo::Mode::GroundTimedWalkReleasePush:
          direct_timed_up?gf::NativeGRedGroundDashDemo::Mode::GroundWalkTimedDirectUpRelaunch:
          direct_timed?gf::NativeGRedGroundDashDemo::Mode::GroundWalkTimedDirectRelaunch:
          options.gred_expiring_snap_windup_relaunch?gf::NativeGRedGroundDashDemo::Mode::GroundExpiringSnapWindupRelaunch:
          options.gred_positive_snap_windup_relaunch?gf::NativeGRedGroundDashDemo::Mode::GroundPositiveSnapWindupRelaunch:
          options.gred_timed_snap_windup_relaunch?gf::NativeGRedGroundDashDemo::Mode::GroundTimedSnapWindupRelaunch:
          direct_expiry?gf::NativeGRedGroundDashDemo::Mode::GroundWalkExpiryDirectRelaunch:
          timed_followup==TimedLedgeFollowup::ExpiryHandoffRelaunch?gf::NativeGRedGroundDashDemo::Mode::GroundExpiryPlainWindupRelaunch:
         timed_followup==TimedLedgeFollowup::HandoffRelaunch?gf::NativeGRedGroundDashDemo::Mode::GroundTimedPlainWindupRelaunch:
         timed_followup==TimedLedgeFollowup::DirectAirDash?gf::NativeGRedGroundDashDemo::Mode::GroundWalkLedgeDirectTimedAirDash:
         options.gred_recovery_first_contact_seam?gf::NativeGRedGroundDashDemo::Mode::GroundRecoveryFirstContactSeam:
         options.gred_recovery_recontact_seam?gf::NativeGRedGroundDashDemo::Mode::GroundRecoveryRecontactSeam:
         options.gred_recovery_recontact_drift?gf::NativeGRedGroundDashDemo::Mode::GroundRecoveryRecontactDrift:
         options.gred_recovery_recontact?gf::NativeGRedGroundDashDemo::Mode::GroundRecoveryRecontact:
         options.gred_walk_ledge_expiry_air_dash?gf::NativeGRedGroundDashDemo::Mode::GroundWalkLedgeExpiryAirDash:
         options.gred_walk_ledge_expiry_push?gf::NativeGRedGroundDashDemo::Mode::GroundWalkLedgeExpiryPush:
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
    if(picker)view.source_practice_back(options.force_practice?gf::PracticeBackScreen::ForceBrowser:gf::PracticeBackScreen::OwnedCollection);
    gf::InputRouter router;unsigned frame=0,commands=0;gf::u64 phase=0;bool captured=false;const auto started=context.clock().monotonic_ns();auto previous=started;
    gf::log_info(context,"battle-mvp","G-Red vs Sasuke: original stage/models, provisional native gameplay rules; no source-frame equivalence or audio claim");
     if(same_face_expiry)gf::log_info(context,"source-ground",
         "same-face Down continuation only on the 1 to 0 step; proven projected exit, no raised-side entry");
     else if(rising_diagonal_windup)gf::log_info(context,"source-ground",
         "separate six-tick non-contact rising wind-up handoff; same paid charge and neutral relaunch at zero ledge timer");
     else if(rising_windup_relaunch)gf::log_info(context,"source-ground",
         "neutral relaunch from a paid five-tick, non-contact wind-up directly after rising powered relaunch; zero ledge timer");
     else if(timed_release_expiry)gf::log_info(context,"source-ground",
         "the verified Down-drift push followed by the existing zero-timer response on face 2011; no added contact permission");
     else if(timed_release_four)gf::log_info(context,"source-ground",
         "one exact Down-drift response from face 2021 onto face 2011 after the three neutral pushes; only timer 2 to 1");
     else if(timed_release_three)gf::log_info(context,"source-ground",
         "one final consecutive positive-timer push on face 2021 after two accepted same-face pushes; no extra expiry or active-contact permission");
     else if(timed_release_two)gf::log_info(context,"source-ground",
         "one consecutive same-face successor to the accepted positive-timer relaunch-fall push; no third positive push or new expiry permission");
     else if(timed_release_push)gf::log_info(context,"source-ground",
         "one positive inter-gate push after a plain neutral walking-ledge relaunch tap and an accepted ordinary fall; active contact and repeated positive pushes are refused");
     else if(direct_timed_up)gf::log_info(context,"source-ground",
         "soft camera-relative Up may steer plain rising entry from an accepted still-positive walking push; exact turn, clock and travel required");
     else if(direct_timed)gf::log_info(context,"source-ground",
         "plain neutral lift directly from a still-positive walking push only; descent must reverse without entry contact or facing change");
     else if(direct_expiry)gf::log_info(context,"source-ground",
         "soft camera-relative Up with Shift may start plain descending relaunch directly from an accepted zero-timer walking push; no positive-timer or entry contact response");
     else if(direct_expiring)gf::log_info(context,"source-ground",
         "soft Up admits plain descending relaunch on the 1 to 0 update after an accepted timer-one walking push; entry floor and steep responses remain refused here");
     else if(adjacent_expiring_push)gf::log_info(context,"source-ground",
         "the same soft-Up expiry-entry proof may select adjacent face 1998 at angle 13653 after face 2009; no other new face, clock or contact response");
     else if(direct_expiring_push)gf::log_info(context,"source-ground",
         "soft Up admits a separate descending relaunch entry push on 1 to 0 for the verified 2009 to 1999 transition; exact projection and travel, no snap");
     else if(timed_followup==TimedLedgeFollowup::ExpiryHandoffRelaunch)gf::log_info(context,"source-ground",
         "plain relaunch may replace active entry after an uninterrupted plain wind-up from the accepted walking expiry push; unrelated zero timers and direct relaunch still refused");
     else if(timed_followup==TimedLedgeFollowup::HandoffRelaunch)gf::log_info(context,"source-ground",
         "plain relaunch may replace active entry after a completed plain wind-up with a still-positive ledge countdown; no direct relaunch from the push");
     else if(timed_followup==TimedLedgeFollowup::DirectAirDash)gf::log_info(context,"source-ground",
         "plain wind-up may follow an accepted positive-timer walking push only while the next countdown remains positive; contact entry and relaunch still refused");
     else if(expiring_short_relaunch)gf::log_info(context,"source-ground",
         "the qualified five-tick expiry wind-up may also relaunch; this guide needs Down drift on release to clear the next travel checks");
     else if(expiring_air_relaunch)gf::log_info(context,"source-ground",
         "neutral Shift may replace active dash after the qualified expiry-step entry, two face-2007 pushes and the paid six-tick wind-up");
     else if(expiring_snap_relaunch)gf::log_info(context,"source-ground",
         "neutral Shift may replace active dash only after the exact expiry snap and all six ordinary wind-up updates through the 450 charge");
     else if(expiring_air_snap)gf::log_info(context,"source-ground",
         "same-height wind-up entry snap on 1 to 0 from face 2001 to face 2000; no steep entry, grounding, entry cost or relaunch handoff");
     else if(expiring_air_dash)gf::log_info(context,"source-ground",
         "stationary plain wind-up on 1 to 0 after an accepted timer-one walking push on face 2001; no contact on entry or new relaunch handoff");
     else if(options.gred_recovery_first_contact_seam)gf::log_info(context,"source-ground","descending recovery contact and touchdown may each cross one independently verified shared flat edge with soft drift and zero timer698");
     else if(options.gred_recovery_recontact_seam)gf::log_info(context,"source-ground","steered recovery recontact may cross one verified shared edge on identical flat planes; first contact still requires one triangle; zero timer698");
     else if(options.gred_recovery_recontact_drift)gf::log_info(context,"source-ground","drift at recovery floor completion or recontact: soft camera-relative cardinal input, retained body heading; same flat triangle and zero timer698 only");
     else if(options.gred_recovery_recontact)gf::log_info(context,"source-ground","recovery completion at zero timer698: floor contact then falling, not landing entry; neutral recontact on the same flat triangle only");
     else if(options.gred_walk_ledge_expiry_air_dash)gf::log_info(context,"source-ground","plain air-dash wind-up after the accepted zero-timer walking expiry push only; no entry contact response, later costs and travel unchanged");
     else if(options.gred_walk_ledge_expiry_push)gf::log_info(context,"source-ground","walking ledge push at timer698 1 to 0 only after an accepted push, without drift; expired landing gate 10240 and native cap 13653; both travel checks");
     else if(options.gred_walk_ledge_high_angle_push)gf::log_info(context,"source-ground","continued descending walk-ledge push with positive owned timer698; native cap 13653, same selected-plane and pre/post-travel checks");
     else if(options.gred_walk_ledge_fall_push)gf::log_info(context,"source-ground","continued descending walk-ledge push with positive owned timer698 and angle above 6144 through 10240; separate native travel on both sides of projection");
     else if(options.gred_inclined_walk_ledge)gf::log_info(context,"source-ground","%s",use_demo_start?
         "ordinary walk across the verified inclined ledge fixture; no new movement policy or steep push":
         "inclined walk practice uses the ordinary start; add --practice-start for the verified foothold");
      else if(options.gred_inclined_dash_ledge_relaunch)gf::log_info(context,"source-ground",
          "after an ordered incline departure, relaunch from the accepted ordinary fall using the existing timer and meter rules; no steep push permission");
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
                const char* hint=guide?guide->hud:
                    options.gred_walk_ledge_expiry_air_dash?"WALK LEFT / FLICK LEFT AFTER EXPIRY PUSH":
                    options.gred_walk_ledge_expiry_push?"LEFT ARROW THEN RELEASE":
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
             if(options.gred_recovery_recontact){
                 if(!options.source_ground_interactive)
                     gf::log_info(context,"source-ground","%s",options.gred_recovery_first_contact_seam?
                         "recovery first-contact seam demo: jump on ticks 6-39, backward flick at 43, D-pad Right on 105-109":
                         options.gred_recovery_recontact_seam?
                         "recovery recontact seam demo: jump on ticks 6-39, backward flick at 43, D-pad Right on 108-109":
                         options.gred_recovery_recontact_drift?
                         "recovery recontact drift demo: jump on ticks 6-39, backward flick at 43, D-pad Up on 108-109":
                         "recovery recontact demo: ordinary native start; jump on ticks 6-39, backward flick at 43, then neutral");
             }else if(options.gred_slope_dash){
                 if(!options.source_ground_interactive)
                      gf::log_info(context,"source-ground","%s",timed_followup==TimedLedgeFollowup::Relaunch?
                           "timed ledge relaunch demo: walk right, two pushes, four ordinary falling updates, then Shift on 30-34":
                           timed_followup==TimedLedgeFollowup::AirDash?
                           "timed ledge air dash demo: walk right, two pushes, four ordinary falling updates, then W on 30":
                            timed_followup==TimedLedgeFollowup::DirectAirDash?
                            "direct timed ledge air dash demo: walk right, five pushes, one A+W flick on 28 without an intervening fall":
                            expiring_short_relaunch?
                            "diagonal expiry-step wind-up relaunch demo: Left Arrow on 6-7, A+W on 38, Shift on 44, Down Arrow on 45-50":
                            expiring_air_relaunch?
                            "expiry-step wind-up relaunch demo: Left Arrow on 6-7, A on 38, Shift on 45 instead of active dash":
                            expiring_snap_relaunch?
                            "expiry snap wind-up relaunch demo: Left Arrow on 6-9, A on 40, Shift on 47 instead of active dash":
                            expiring_air_snap?
                            "wind-up entry snap on expiry demo: Left Arrow on 6-9, A on 40, then neutral":
                            expiring_air_dash?
                            "air dash on expiry demo: Left Arrow on 6-7, A on 38, then neutral":
                           timed_followup==TimedLedgeFollowup::HandoffRelaunch?
                           "timed wind-up relaunch demo: walk right, W on 30 after the ordinary falls, Shift on 36 instead of active dash":
                           options.gred_inclined_dash_jump?
                           "incline dash jump demo: S on 6, cross the flat/incline join, then Shift on 14-37":
                           options.gred_timed_incline_landing_dash?
                           "timed incline landing dash demo: Left Arrow on 6-9, then A on 34 after the accepted landing":
                           timed_followup==TimedLedgeFollowup::ExpiryHandoffRelaunch?
                           "expiry wind-up relaunch demo: walk left, expiry push at 56, A on 57, Shift on 63 instead of active dash":
                           direct_expiry?
                           "direct expiry relaunch demo: Left Arrow on 6-25 then Up Arrow+Shift on 57-76, no wind-up dash":
                           direct_expiring?
                           "direct relaunch on expiry demo: Left Arrow on 6-7, Up Arrow+Shift on 38, then neutral":
                           adjacent_expiring_push?
                           "adjacent entry push on expiry demo: Left Arrow on 6-25, Up Arrow+Shift on 56-59, then neutral":
                           direct_expiring_push?
                           "relaunch entry push on expiry demo: Left Arrow on 6-25, Up Arrow+Shift on 56-59, then neutral":
                           direct_timed_up?
                           "direct timed Up relaunch demo: Up+Left Arrows on 6-7 then Up Arrow+Shift on 25-32":
                           direct_timed?
                           "direct timed relaunch demo: Up+Left Arrows on 6-7 then Shift on 25-32, no wind-up dash":
                            same_face_expiry?
                            "same-face expiry push demo: Up+Left Arrows on 6-7, Shift on 20, hold Down Arrow on 37-38, otherwise neutral":
                            rising_diagonal_windup?
                            "diagonal rising wind-up relaunch demo: use the expiry drift and second relaunch, flick W+D with Shift on 47, then tap neutral Shift on 54":
                            rising_windup_relaunch?
                           "rising wind-up relaunch demo: use the expiry drift and second relaunch, flick W with Shift on 47, then tap neutral Shift on 53":
                           timed_release_expiry?
                           "expiry drift push demo: Up+Left Arrows on 6-7, Shift on 20, neutral through 36, Down on 37, Down+Right on 38":
                           timed_release_four?
                           "timed Down-drift push demo: Up+Left Arrows on 6-7, Shift on 20, neutral through 36, Down Arrow on 37 only":
                           timed_release_three?
                           "three timed relaunch-fall pushes demo: Up+Left Arrows on 6-7, Shift on 20 then neutral; responses on 34-36":
                           timed_release_two?
                           "two timed relaunch-fall pushes demo: Down+Right Arrows on 6-7, Shift on 20 then neutral; same-face responses on 33 and 34":
                           timed_release_push?
                           "timed relaunch-fall push demo: Right Arrow on 6-8, Shift on 23 then neutral; an ordinary fall precedes the push":
                           options.gred_walk_ledge_expiry_air_dash?
                          "walk ledge expiry air-dash demo: D-pad left then neutral, one left flick after accepted tick56":options.demo_gred_walk_ledge_expiry_drift?
                          "walk ledge expiry drift demo: neutral on the expiry push, then 36 accepted D-pad up updates":options.demo_gred_walk_ledge_expiry_relaunch?
                          "walk ledge expiry relaunch demo: neutral fall at tick57, hold jump on ticks58-77, then release":options.gred_walk_ledge_expiry_push?
                         "walk ledge expiry demo: D-pad left, then neutral; faces2009/1999 and a separate later landing":options.gred_walk_ledge_high_angle_push?
                         "walk ledge high-angle demo: D-pad right, then neutral; faces1953/1949 and separate later landing":options.gred_walk_ledge_fall_push?
                         "walk ledge-fall push demo: D-pad right to depart, then neutral; selected face1953 and separate later landing":options.gred_inclined_walk_ledge?
                          "inclined walk ledge demo: D-pad up to depart triangle827 toward flat floor2060, then neutral":
                          options.gred_inclined_dash_ledge_relaunch?
                          "inclined dash ledge relaunch demo: tap D on 6, tap Shift on 8 after ordinary falling starts":
                          options.gred_inclined_dash_ledge?
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
                 if(!options.source_ground_interactive)gf::log_info(context,"source-ground","%s",options.gred_recovery_completion_exit?
                     "recovery completion exit demo: native grounded start -580/-2990; Shift 6-38, W 46-72, Left 111, Up+Left 112":
                     options.gred_recovery_completion_push?
                     "recovery completion push demo: native grounded start -575/-2990; Shift on ticks 6-38, W on 46-72, Left on 111-112":
                     "ground-recovery-push demo only: native grounded placements 44.9286/-332.8781 and 1200/1200, original stage and bounds; jump5-21, backward flick20-60");
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
                 if(options.gred_expiring_snap_windup_relaunch)gf::log_info(context,"source-ground",
                     "completed uninterrupted flat-floor wind-up can request neutral relaunch on the actual 1 to 0 update; earlier scopes and relaunch contact checks are unchanged");
                 else if(options.gred_positive_snap_windup_relaunch)gf::log_info(context,"source-ground",
                     "completed uninterrupted flat-floor wind-up can request neutral relaunch while the actual countdown stays positive; 1 to 0 and relaunch entry contact still refused");
                 else if(options.gred_timed_snap_windup_relaunch)gf::log_info(context,"source-ground",
                     "completed flat-floor wind-up from an accepted positive-timer relaunch snap can request neutral relaunch after expiry; live plane and shared edge proofs required");
                 if(!options.source_ground_interactive)gf::log_info(context,"source-ground","%s",options.gred_active_snap_relaunch?
                     "active snap relaunch demo: Up Arrow 6-8, Shift 25-26 with W on 26, neutral through 32, then Shift 33":
                     options.gred_expiring_snap_windup_relaunch?
                     "expiring floor wind-up relaunch demo: Up Arrow 6-25, Shift 46-47 with A on 47, then Shift 55":
                     options.gred_positive_snap_windup_relaunch?
                     "positive floor wind-up relaunch demo: Up Arrow 6-8, Shift 25-26 with W on 26, then Shift 32":
                     options.gred_timed_snap_windup_relaunch?
                     "floor wind-up relaunch demo: follow the timed snap to wind-up, then tap Shift on 156 before active dash":
                     options.gred_timed_relaunch_snap_dash?
                     "timed snap to air dash demo: Down+Right 6-122, Shift 148-150 with D+S on 150; watch free entry and separate charges":
                     options.gred_timed_relaunch_floor?
                     "timed relaunch floor demo: Down+Right 6-122, Shift 147-157; observe the timed snap, expiry, liftoff and separate landing":
                     "ground-loss-relaunch demo only: declared original-stage grounded placement/query reference at -1368.47/-466.667/94.339 and4500 native bounds, unchanged target/stage; down-right5 until falling entry, genuine relaunch press146-595 held through source depletion; no midair teleport/rebind");
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
         if(!options.gred_practice&&!options.source_ground_windup_entry_push&&!options.source_ground_relaunch_entry_push&&!options.source_ground_relaunch_push&&!options.source_ground_active_push&&!options.source_ground_windup_push&&!options.source_ground_recovery_push&&!options.source_ground_relaunch_fall_push&&!options.source_vertical_jump&&!options.source_ground_jump&&!options.source_ground_landing&&!options.source_ground_air&&!options.source_ground_air_floor&&!options.source_ground_relaunch&&!options.source_ground_relaunch_floor&&!options.source_ground_windup_entry_floor&&!options.source_ground_surface&&!options.source_ground_loss_surface&&!options.source_ground_loss_jump&&!options.source_ground_loss_landing&&!options.source_ground_loss_air&&!options.source_ground_loss_relaunch&&!options.source_ground_fall_push&&!options.source_ground_air_fall_push)gf::log_info(context,"source-ground","%s source walk/turn/stop/dash; arrows/D-pad WALK, stick/WASD FLICK DASH; Enter pause, Q restart; jump/flight/combat refused before simulation; native rate664=1",options.source_ground_interactive?"interactive":"scripted");
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
    auto pause_guided_practice=[&](){
        if(!guide)return true;
        if(!world.set_paused(true))return false;
        gf::log_info(context,"source-ground","%s guide: paused at tick=%u; P steps; %s",guide->name,world.tick(),guide->inputs);
        return true;
    };
    if(!pause_guided_practice()){render.shutdown();return 2;}
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
                if(!owned_current()||!world.start(config,&view)||!view.update(world,&error)||!bind_source()||!pause_guided_practice()){
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
                           if(options.gred_recovery_recontact){
                               const auto tick=world.tick();device.b=tick>=5&&tick<39;
                               if(tick==42)device.stick_y=-127;
                               const unsigned drift_start=options.gred_recovery_first_contact_seam?104:107;
                               if(options.gred_recovery_recontact_drift&&tick>=drift_start&&tick<109){
                                   device.dpad_right=options.gred_recovery_recontact_seam;
                                   device.dpad_up=!options.gred_recovery_recontact_seam;
                               }
                            }else if(direct_expiring){
                                device.dpad_left=world.tick()==5||world.tick()==6;
                                device.b=device.dpad_up=world.tick()==37;
                            }else if(expiring_air_snap){
                                device.dpad_left=world.tick()>=5&&world.tick()<9;
                                if(world.tick()==39)device.stick_x=-127;
                                device.b=expiring_snap_relaunch&&world.tick()==46;
                            }else if(expiring_air_dash){
                                device.dpad_left=world.tick()==5||world.tick()==6;
                                if(world.tick()==37){device.stick_x=-127;if(expiring_short_relaunch)device.stick_y=-127;}
                                device.b=expiring_air_relaunch&&world.tick()==(expiring_short_relaunch?43u:44u);
                                device.dpad_down=expiring_short_relaunch&&world.tick()>=44&&world.tick()<50;
                            }else if(direct_expiring_push){
                                device.dpad_left=world.tick()>=5&&world.tick()<25;
                                device.b=device.dpad_up=world.tick()>=55&&world.tick()<59;
                            }else if(timed_release_three){
                                device.dpad_up=device.dpad_left=world.tick()==5||world.tick()==6;
                                device.b=world.tick()==19;
                                device.dpad_right=timed_release_expiry&&!same_face_expiry&&world.tick()==37;
                                device.dpad_down=device.dpad_right||(timed_release_four&&world.tick()==36)||(same_face_expiry&&world.tick()==37);
                                if(rising_windup_relaunch){
                                    const auto tick=world.tick()+1;
                                    device.b=device.b||(tick>=41&&tick<=47)||tick==(rising_diagonal_windup?54u:53u);
                                    if(tick==47){device.stick_y=-80;if(rising_diagonal_windup)device.stick_x=80;}
                                }
                            }else if(options.gred_walk_ledge_fall_push){
                              if(world.tick()>=5&&world.tick()<30&&!source.ground_departures()){
                                  device.dpad_right=!options.gred_walk_ledge_expiry_push&&!direct_timed;
                                  device.dpad_left=options.gred_walk_ledge_expiry_push||direct_timed;
                                  device.dpad_up=direct_timed;
                                  device.dpad_down=timed_release_two;
                               }else if(options.gred_walk_ledge_expiry_air_dash&&world.tick()==56)device.stick_x=-127;
                               else if(options.demo_gred_walk_ledge_expiry_drift&&world.tick()>=56&&world.tick()<92)device.dpad_up=true;
                                 else if(options.demo_gred_walk_ledge_expiry_relaunch&&world.tick()>=57&&world.tick()<77)device.b=true;
                                else if(direct_expiry&&world.tick()>=56&&world.tick()<76)device.b=device.dpad_up=true;
                                else if(direct_timed&&world.tick()>=24&&world.tick()<32){device.b=true;device.dpad_up=direct_timed_up;}
                                else if(timed_release_push&&world.tick()==(timed_release_two?19u:22u))device.b=true;
                               else if(timed_followup==TimedLedgeFollowup::ExpiryHandoffRelaunch&&world.tick()==62)device.b=true;
                               else if(timed_followup==TimedLedgeFollowup::Relaunch&&world.tick()>=29&&world.tick()<34)device.b=true;
                               else if(timed_followup==TimedLedgeFollowup::AirDash&&world.tick()==29)device.stick_y=-127;
                                else if(timed_followup==TimedLedgeFollowup::DirectAirDash&&world.tick()==27)device.stick_x=device.stick_y=-127;
                                else if(timed_followup==TimedLedgeFollowup::HandoffRelaunch){
                                    if(world.tick()==29)device.stick_y=-127;
                                    device.b=world.tick()==35;
                                }
                          }else if(options.gred_inclined_walk_ledge){
                              device.dpad_up=world.tick()>=5&&world.tick()<20&&!source.ground_departures();
                          }else if(options.gred_timed_incline_landing_dash){
                              const auto tick=world.tick();
                              device.dpad_left=tick>=5&&tick<9;
                              if(tick==33)device.stick_x=-127;
                          }else if(options.gred_slope_dash){
                              if(world.tick()==5){
                                    if(options.gred_inclined_dash_ledge||(options.gred_slope_crease_dash&&!options.gred_slope_transition_dash))device.stick_x=127;
                                  else device.stick_y=127;
                              }
                               if(options.gred_inclined_dash_jump)device.b=world.tick()>=13&&world.tick()<37;
                               if(options.gred_inclined_dash_ledge_relaunch)device.b=world.tick()==7;
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
                              const auto tick=world.tick();
                              if(options.gred_recovery_completion_push){
                                  device.b=tick>=5&&tick<38;
                                  if(tick>=45&&tick<72)device.stick_y=-127;
                                  device.dpad_left=tick>=110&&tick<112;
                                  device.dpad_up=options.gred_recovery_completion_exit&&tick==111;
                              }else{
                                  device.b=tick>=5&&tick<22;
                                  if(tick>=20&&tick<61)device.stick_y=127;
                              }
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
                              if(options.gred_expiring_snap_windup_relaunch){
                                  device.dpad_up=tick>=5&&tick<25;
                                  device.b=tick==45||tick==46||tick==54;
                                  if(tick==46)device.stick_x=-127;
                              }else if(options.gred_positive_snap_windup_relaunch||options.gred_active_snap_relaunch){
                                  device.dpad_up=tick>=5&&tick<8;
                                  device.b=tick==24||tick==25||tick==(options.gred_active_snap_relaunch?32u:31u);
                                  if(tick==25)device.stick_y=-127;
                              }else{
                                  if(tick>=5&&!source.ground_loss_entries()){device.dpad_down=true;device.dpad_right=true;}
                                  if(options.gred_timed_relaunch_snap_dash){
                                      device.b=(tick>=147&&tick<150)||(options.gred_timed_snap_windup_relaunch&&tick==155);
                                      if(tick==149)device.stick_x=device.stick_y=127;
                                  }else device.b=tick>=146&&tick<(options.gred_timed_relaunch_floor?157u:596u);
                              }
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
                     const bool planned=source.prepare(world,view.source_binding(),movement,x,y,held,&offer,&error);
                     if(!planned||
                        !world.step({controls,{}},false,&view,nullptr,&movement,&offer)||!source.commit(world,view.source_binding(),&error)){
                        source.reject();
                         gf::log_error(context,"source-ground","motion acceptance refused: tick=%u kind=%u from-y=%.3f proposed-y=%.3f old-v=%.3f; %s",world.tick(),unsigned(offer.kind),offer.observed.position[1],offer.proposed_position[1],offer.observed.vertical_speed,error.c_str());
                         if(planned&&world.tick()==last_tick&&offer.static_push){
                             const auto& push=*offer.static_push;
                             gf::StageFloorApproachTrace approach,projection;
                             if(view.trace_floor_approach(push.surface.triangle,offer.observed.position,push.integrated_position,&approach)&&
                                view.trace_floor_approach(push.surface.triangle,offer.observed.position,offer.proposed_position,&projection))
                                 gf::log_info(context,"source-ground","candidate floor face=%u approach=%s projection=%s; approach span=%.5f..%.5f clearance=%.5f..%.5f",
                                     push.surface.triangle,gf::stage_floor_approach_name(approach.kind),gf::stage_floor_approach_name(projection.kind),
                                     approach.enter,approach.exit,approach.clearance_at_entry,approach.clearance_at_exit);
                         }
                        if(!practice_restartable||world.tick()!=last_tick){render.shutdown();return 2;}
                        practice_stopped=true;
                        gf::log_info(context,"source-ground","practice stopped at tick=%u; Q restarts, Esc %s",world.tick(),
                            picker?(options.force_practice?"returns to Force browser":"returns to collection"):"closes");
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
                 view.practice_contact_suffix(practice->kind));
            if(const auto ticks=practice->dash_release_ticks)
                gf::log_info(context,"source-ground","practice-dash-release-ticks=%u",unsigned(*ticks));
             if(const auto ticks=practice->ledge_timer_ticks)
                 gf::log_info(context,"source-ground","practice-ledge-timer-ticks=%u cue=%s",
                     unsigned(*ticks),view.practice_ledge_cue(*practice).c_str());
             if(practice->recovery_drift_active)gf::log_info(context,"source-ground","practice-recovery-drift=DRIFT ACTIVE");
             if(const auto& seam=practice->recovery_seam)
                 gf::log_info(context,"source-ground","practice-recovery-seam=%u->%u cue=FLAT SEAM",(*seam)[0],(*seam)[1]);
             if(practice->recovery_push_complete)gf::log_info(context,"source-ground","practice-recovery-push=RECOVERY COMPLETE");
             if(practice->recovery_push_exit_drift)gf::log_info(context,"source-ground","practice-recovery-exit=EXIT DRIFT");
             if(practice->plain_windup_relaunch_ready)
                 gf::log_info(context,"source-ground","practice-windup-handoff=%s",view.windup_handoff_hint);
             if(practice->floor_windup_relaunch_ready)
                 gf::log_info(context,"source-ground","practice-floor-windup-handoff=%s",view.windup_handoff_hint);
             if(const char* cue=view.practice_relaunch_floor_cue(*practice))
                 gf::log_info(context,"source-ground","practice-relaunch-floor=%s",cue);
             if(practice->dash_interrupted_takeoff)
                 gf::log_info(context,"source-ground","practice-dash-takeoff=%s",view.dash_interrupt_hint);
             if(practice->landing_dash_started)
                 gf::log_info(context,"source-ground","practice-landing-dash=%s",view.landing_dash_hint);
             if(practice->direct_expiry_relaunch)
                 gf::log_info(context,"source-ground","practice-direct-expiry-relaunch=%s",view.direct_expiry_relaunch_hint);
             if(const char* cue=view.practice_direct_timed_relaunch_cue(*practice))
                 gf::log_info(context,"source-ground","practice-direct-timed-relaunch=%s",cue);
             if(practice->direct_expiry_relaunch_ready)
                 gf::log_info(context,"source-ground","practice-direct-expiry-request=%s",view.direct_expiry_request_hint);
             if(practice->direct_expiring_relaunch_ready)
                 gf::log_info(context,"source-ground","practice-direct-expiring-request=%s",view.direct_expiring_request_hint);
             if(practice->direct_expiring_relaunch)
                 gf::log_info(context,"source-ground","practice-direct-expiring-relaunch=%s",view.direct_expiring_relaunch_hint);
             if(practice->direct_expiring_entry_push)
                 gf::log_info(context,"source-ground","practice-direct-expiring-entry-push=%s",view.direct_expiring_entry_push_hint);
             if(practice->expiring_walk_air_dash)
                 gf::log_info(context,"source-ground","practice-expiring-walk-air-dash=%s",view.expiring_walk_air_dash_hint);
             if(practice->expiring_walk_air_dash_relaunch)
                 gf::log_info(context,"source-ground","practice-expiring-walk-air-dash-relaunch=%s",view.expiring_walk_air_dash_relaunch_hint);
             if(practice->expiring_walk_air_dash_snap)
                 gf::log_info(context,"source-ground","practice-expiring-walk-air-dash-snap=%s",view.expiring_walk_air_dash_snap_hint);
             if(practice->windup_from_relaunch_snap)
                 gf::log_info(context,"source-ground","practice-relaunch-snap-windup=%s",view.windup_from_relaunch_snap_hint);
             if(practice->relaunch_from_active_snap)
                 gf::log_info(context,"source-ground","practice-active-snap-relaunch=%s",view.relaunch_from_active_snap_hint);
             if(const char* cue=view.practice_timed_release_push_cue(*practice))
                 gf::log_info(context,"source-ground","practice-timed-release-push=%s",cue);
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
     if(direct_expiry)gf::log_info(context,"source-ground","relaunches=%llu air-dash-starts=%llu floors=%llu timer698=%.1f",
         static_cast<unsigned long long>(source.relaunches()),static_cast<unsigned long long>(source.air_dash_starts()),
         static_cast<unsigned long long>(source.relaunch_floor_contacts()),double(source.ground_loss_timer698()));
     if(direct_expiring)gf::log_info(context,"source-ground","direct-expiring-relaunches=%llu air-dash-starts=%llu floors=%llu entry-pushes=%llu timer698=%.1f",
         static_cast<unsigned long long>(source.relaunches()),static_cast<unsigned long long>(source.air_dash_starts()),
         static_cast<unsigned long long>(source.relaunch_floor_contacts()),static_cast<unsigned long long>(source.relaunch_entry_surface_pushes()),
         double(source.ground_loss_timer698()));
     if(direct_expiring_push)gf::log_info(context,"source-ground",
         "direct-expiring-push-relaunches=%llu air-dash-starts=%llu floors=%llu entry-pushes=%llu relaunch-fall-pushes=%llu timer698=%.1f",
         static_cast<unsigned long long>(source.relaunches()),static_cast<unsigned long long>(source.air_dash_starts()),
         static_cast<unsigned long long>(source.relaunch_floor_contacts()),static_cast<unsigned long long>(source.relaunch_entry_surface_pushes()),
         static_cast<unsigned long long>(source.relaunch_fall_pushes()),double(source.ground_loss_timer698()));
      if(direct_timed)gf::log_info(context,"source-ground","direct-timed-relaunches=%llu air-dash-starts=%llu floors=%llu timer698=%.1f",
         static_cast<unsigned long long>(source.relaunches()),static_cast<unsigned long long>(source.air_dash_starts()),
         static_cast<unsigned long long>(source.relaunch_floor_contacts()),double(source.ground_loss_timer698()));
     if(expiring_air_dash)gf::log_info(context,"source-ground",
         "expiring-air-dash-starts=%llu windup-pushes=%llu active-frames=%llu relaunches=%llu timer698=%.1f",
         static_cast<unsigned long long>(source.air_dash_starts()),static_cast<unsigned long long>(source.windup_pushes()),
         static_cast<unsigned long long>(source.air_dash_active_frames()),static_cast<unsigned long long>(source.relaunches()),
         double(source.ground_loss_timer698()));
     if(expiring_air_snap)gf::log_info(context,"source-ground",
         "expiring-dash-entry-snaps=%llu continued-windup-snaps=%llu active-snaps=%llu entry-pushes=%llu",
         static_cast<unsigned long long>(source.windup_entry_floor_contacts()),
         static_cast<unsigned long long>(source.windup_floor_contacts()-source.windup_entry_floor_contacts()),
         static_cast<unsigned long long>(source.active_floor_contacts()),
         static_cast<unsigned long long>(source.windup_entry_surface_pushes()));
     if(options.gred_inclined_dash_ledge_relaunch)gf::log_info(context,"source-ground",
         "inclined-dash-ledge-relaunches=%llu air-dash-starts=%llu surface-pushes=%llu timer698=%.1f",
         static_cast<unsigned long long>(source.relaunches()),static_cast<unsigned long long>(source.air_dash_starts()),
         static_cast<unsigned long long>(source.surface_pushes()),double(source.ground_loss_timer698()));
     if(timed_release_push)gf::log_info(context,"source-ground","relaunch-fall-pushes=%llu relaunches=%llu timer698=%.1f landings=%llu",
         static_cast<unsigned long long>(source.relaunch_fall_pushes()),static_cast<unsigned long long>(source.relaunches()),
         double(source.ground_loss_timer698()),static_cast<unsigned long long>(source.landings()));
     if(options.gred_timed_relaunch_snap_dash)gf::log_info(context,"source-ground","snap-dash-starts=%llu windup-entry-floors=%llu windup-floors=%llu active-floors=%llu",
         static_cast<unsigned long long>(source.air_dash_starts()),static_cast<unsigned long long>(source.windup_entry_floor_contacts()),
         static_cast<unsigned long long>(source.windup_floor_contacts()),static_cast<unsigned long long>(source.active_floor_contacts()));
    if(options.source_ground_jump)gf::log_info(context,"source-ground","ground-jump-air-frames=%llu landings=%llu resource=%d dash-jumps=%llu last-interrupted-selector=%u programs=%u/%u cached87=%u; source subset, no extra ordered root advance on takeoff",static_cast<unsigned long long>(source.air_frames()),static_cast<unsigned long long>(source.landings()),int(source.resource()),static_cast<unsigned long long>(source.dash_jumps()),unsigned(source.last_dash_jump_selector()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    if(options.source_ground_landing)gf::log_info(context,"source-ground","landing-jumps=%llu landing-dashes=%llu landings=%llu resource=%d programs=%u/%u cached87=%u; accepted landing entry before interruption, no touchdown skip or forced refill",static_cast<unsigned long long>(source.landing_jumps()),static_cast<unsigned long long>(source.landing_dashes()),static_cast<unsigned long long>(source.landings()),int(source.resource()),source.movement_programs()[0],source.movement_programs()[1],unsigned(source.cached_airborne87()));
    if(options.gred_timed_incline_landing_dash)gf::log_info(context,"source-ground","landing-dashes=%llu loss-landings=%llu timer698=%.1f resource=%d; retained countdown and independently checked shallow dash joins",
        static_cast<unsigned long long>(source.landing_dashes()),static_cast<unsigned long long>(source.ground_loss_landings()),
        double(source.ground_loss_timer698()),int(source.resource()));
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
    if(options.gred_recovery_recontact)gf::log_info(context,"source-ground","recovery-recontact landings=%llu resource=%d selector=%u programs=%u/%u",
        static_cast<unsigned long long>(source.landings()),int(source.resource()),unsigned(source.selector()),
        source.movement_programs()[0],source.movement_programs()[1]);
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

bool picker_practice_options(gf::GRedPickerPracticeRoute route,Options* practice){
    constexpr std::array<std::string_view,unsigned(gf::GRedPickerPracticeRoute::Count)> modes{
        "--gred-movement","--gred-recovery-first-contact-seam","--gred-walk-ledge-timed-relaunch-floor","--gred-inclined-dash-jump",
        "--gred-timed-incline-landing-dash","--gred-walk-ledge-expiry-direct-relaunch",
        "--gred-walk-ledge-positive-snap-handoff","--gred-walk-ledge-timed-snap-handoff","--gred-walk-ledge-timed-snap-dash",
        "--gred-walk-ledge-expiring-snap-handoff","--gred-walk-ledge-direct-timed-relaunch","--gred-walk-ledge-direct-timed-up-relaunch",
        "--gred-walk-ledge-active-snap-relaunch",
        "--gred-walk-ledge-expiring-direct-relaunch","--gred-walk-ledge-expiring-relaunch-push",
        "--gred-walk-ledge-expiring-adjacent-relaunch-push","--gred-walk-ledge-expiring-air-dash","--gred-walk-ledge-expiring-air-dash-handoff",
        "--gred-walk-ledge-rising-diagonal-windup-relaunch","--gred-walk-ledge-rising-windup-relaunch",
        "--gred-walk-ledge-timed-release-four-pushes","--gred-walk-ledge-timed-release-expiry-drift",
        "--gred-inclined-dash-ledge-relaunch","--gred-walk-ledge-timed-release-three-pushes",
        "--gred-walk-ledge-expiring-air-dash-snap","--gred-walk-ledge-expiring-air-dash-snap-handoff",
        "--gred-walk-ledge-expiring-diagonal-dash-handoff",
        "--gred-walk-ledge-timed-release-push","--gred-walk-ledge-timed-release-two-pushes",
        "--gred-slope-transition-dash",
        "--gred-inclined-walk-ledge","--gred-inclined-dash-ledge",
        "--gred-walk-ledge-fall-push","--gred-walk-ledge-high-angle-push",
        "--gred-walk-ledge-timed-handoff-relaunch","--gred-walk-ledge-expiry-handoff-relaunch","--gred-walk-ledge-expiry-air-dash",
        "--gred-walk-ledge-expiry-push"
    };
    if(unsigned(route)>=modes.size())return false;
    std::string mode(modes[unsigned(route)]);
    char program[]="gf_native",start_option[]="--practice-start";char* args[]={program,mode.data(),start_option};
    return parse(route==gf::GRedPickerPracticeRoute::Standard?2:3,args,practice);
}

enum class BrowserPracticeReturn {Returned,Unavailable,Closed,RendererError};
struct BrowserPracticeBudget {
    gf::u64 started;
    unsigned& frame;
    bool& captured;
    bool& renderer_active;
};
BrowserPracticeReturn run_practice_from_browser(gf::Context& context,gf::RenderEngine& render,const Options& options,
    Options practice,const gf::pc::BorgOwnedPractice& qualified,BrowserPracticeBudget budget,const char* return_title,
    const char* component,std::string_view launch_message,std::string* error){
    practice.headless=options.headless;practice.width=options.width;practice.height=options.height;
    practice.battle_mvp_assets=options.battle_mvp_assets;
    practice.frames=options.frames?options.frames-budget.frame:0;practice.seconds=options.seconds;
    if(!budget.captured){
        practice.screenshot=options.screenshot;
        practice.capture_frame=options.capture_frame>budget.frame?options.capture_frame-budget.frame:0;
    }
    // One active renderer lifecycle at a time. Both browser types rebuild
    // from their CPU snapshot; no GL handles cross this transition.
    render.shutdown();budget.renderer_active=false;
    const auto starting_frame=context.frame_index();
    PickerPracticeHandoff handoff{budget.started};
    int outcome=2;
    {
        gf::RenderEngine session_render(context.render());
        context.window().set_title("Owned Borg Practice");
        if(session_render.initialize({options.width,options.height,!options.headless,true,"Owned Borg Practice"},
            context.backends().window,error)){
            gf::log_info(context,component,"%.*s",int(launch_message.size()),launch_message.data());
            outcome=run_battle_mvp(context,session_render,practice,&qualified.files(),&qualified,&handoff);
        }else{
            session_render.shutdown();
            gf::log_warn(context,component,"practice renderer unavailable: %s",error->c_str());
        }
    }
    budget.frame+=context.frame_index()-starting_frame;budget.captured=budget.captured||handoff.captured;
    if(context.window().should_close()||(options.frames&&budget.frame>=options.frames)||
       (options.seconds&&context.clock().monotonic_ns()-budget.started>=gf::u64(options.seconds)*1000000000ull))
        return BrowserPracticeReturn::Closed;
    context.window().set_title(return_title);
    render.clear_shared_texture_upload_error();render.state()={};
    if(!render.initialize({options.width,options.height,!options.headless,true,return_title},context.backends().window,error))
        return BrowserPracticeReturn::RendererError;
    budget.renderer_active=true;
    return outcome?BrowserPracticeReturn::Unavailable:BrowserPracticeReturn::Returned;
}

struct BorgArrangeHandoff {
    const gf::AccountSaveEntry& save;
    const gf::ForceCatalog* retail=nullptr;
    std::unique_ptr<gf::BorgForceArrange> arranger;
    gf::GamePadState opening_input;
    gf::u64 start=0;
    unsigned frame=0;
    bool captured=false;
};
int run_borg_browser(gf::Context&,gf::RenderEngine&,const Options&,const gf::BorgPackageCatalog&,BorgArrangeHandoff* =nullptr);

int run_borg_force_browser(gf::Context& context,gf::RenderEngine& render,const Options& options,
                           const gf::BorgPackageCatalog& packages,gf::pc::BorgForceBrowseSnapshot& snapshot){
    gf::BorgForceBrowser cursor;
    if(!cursor.open(unsigned(options.inspect_force-1),options.browse_force_slot?unsigned(options.browse_force_slot-1):0)){
        render.shutdown();return 2;
    }
    gf::BorgForceBrowserView view;gf::MixedBorgForceView force;gf::InputRouter router;
    bool live=snapshot.project(packages,cursor.force(),&force);
    auto report=[&](){
        if(!live){gf::log_warn(context,"force-browser","selected save changed or is unreadable; reopen to inspect");return;}
        const auto* row=force.find_slot(cursor.slot());
        const auto claim=!row?"empty":row->overlap()?"overlap":row->retail?"retail":"package";
        gf::log_info(context,"force-browser","force=%u slot=%u claims=%s owned-instance=%llu retail-record=%s overlaps=%u",
            cursor.force()+1,cursor.slot()+1,claim,static_cast<unsigned long long>(row&&row->packaged?row->packaged->saved.instance:0),
            force.retail_record_present?"present":"absent",force.overlaps);
    };
    report();
    std::string error;std::string_view practice_status,arrange_status;
    unsigned frame=0;bool captured=false,renderer_active=true,wait_for_exit_release=false;
    const auto start=context.clock().monotonic_ns();
    while(context.window().pump_events()&&!context.window().should_close()){
        const auto now=context.clock().monotonic_ns();
        if(options.seconds&&now-start>=gf::u64(options.seconds)*1000000000ull)break;
        context.input().pump();const auto& input=context.input().snapshot();const auto pad=router.route(input,0);
        const bool quit=input.quit||pad.quit_requested;
        if(quit&&!wait_for_exit_release)break;
        auto changed=gf::BorgForceBrowseChange::None;
        if(wait_for_exit_release){cursor.sync_input(pad);wait_for_exit_release=quit;}
        else changed=cursor.step(pad,options.practice_from_picker&&live,options.arrange_from_force&&live);
        if(changed==gf::BorgForceBrowseChange::Close)break;
        if(live&&changed==gf::BorgForceBrowseChange::Arrange){
            if(!(live=snapshot.current()))report();
            else if(force.find_slot(cursor.slot())){
                arrange_status="POSITION OCCUPIED - NO WRITE";
                gf::log_info(context,"borg-placement","Force %u slot %u occupied - no Arrange handoff",cursor.force()+1,cursor.slot()+1);
            }else if(auto arranger=snapshot.make_arranger()){
                Options arrange=options;
                arrange.browse_force=arrange.arrange_from_force=false;
                arrange.browse_owned_borgs=true;
                arrange.placement_force=cursor.force()+1;
                arrange.placement_slot=cursor.slot()+1;
                BorgArrangeHandoff handoff{snapshot.save(),snapshot.retail_catalogue(),std::move(arranger),pad,start,frame,captured};
                context.window().set_title("Arrange Saved Force");
                gf::log_info(context,"borg-placement","opened Arrange from Force %u slot %u on the same selected save",cursor.force()+1,cursor.slot()+1);
                return run_borg_browser(context,render,arrange,packages,&handoff);
            }else{live=false;report();}
        }else if(live&&changed==gf::BorgForceBrowseChange::Practice){
            if(!(live=snapshot.current()))report();
            else{
                const auto* row=force.find_slot(cursor.slot());
                const gf::pc::BorgForcePracticeRequest request{options.selected_save_root,snapshot.save().key,cursor.force(),cursor.slot()};
                const auto current=gf::pc::scan_borg_packages("borgs");
                auto qualified=gf::pc::BorgOwnedPractice::open_force_slot(request,"borgs",current,snapshot.retail_catalogue(),&error);
                live=snapshot.current()&&(!qualified||(row&&row->packaged&&!row->overlap()&&qualified->saved()==row->packaged->saved));
                if(!live)report();
                else if(!qualified){
                    practice_status="PRACTICE UNAVAILABLE";
                    gf::log_warn(context,"force-browser","practice unavailable for Force %u slot %u: %s",
                        cursor.force()+1,cursor.slot()+1,error.c_str());
                }else{
                    Options practice;
                    if(!picker_practice_options(cursor.practice_route(),&practice)){
                        gf::log_error(context,"force-browser","could not select the existing practice route");render.shutdown();return 2;
                    }
                    practice.force_practice=request;
                    const auto route=cursor.practice_route_name();
                    const auto launch="launched practice for Force "+std::to_string(cursor.force()+1)+
                        " slot "+std::to_string(cursor.slot()+1)+" instance="+std::to_string(qualified->saved().instance)+
                        " route="+std::string(route);
                    const auto outcome=run_practice_from_browser(context,render,options,std::move(practice),*qualified,
                        {start,frame,captured,renderer_active},"Saved Force Browser","force-browser",launch,&error);
                    if(outcome==BrowserPracticeReturn::Closed)break;
                    if(outcome==BrowserPracticeReturn::RendererError){
                        gf::log_error(context,"force-browser","could not restore Force browser: %s",error.c_str());return 2;
                    }
                    live=snapshot.current();wait_for_exit_release=true;
                    if(!live)report();
                    else{
                        practice_status=outcome==BrowserPracticeReturn::Returned?"RETURNED FROM PRACTICE":"PRACTICE UNAVAILABLE";
                        if(outcome==BrowserPracticeReturn::Returned)
                            gf::log_info(context,"force-browser","returned to Force %u slot %u after practice route=%.*s",
                                cursor.force()+1,cursor.slot()+1,int(route.size()),route.data());
                    }
                    continue;
                }
            }
        }else if(live&&changed!=gf::BorgForceBrowseChange::None){
            practice_status={};arrange_status={};
            live=changed==gf::BorgForceBrowseChange::Force?snapshot.project(packages,cursor.force(),&force):snapshot.current();
            if(live&&changed==gf::BorgForceBrowseChange::PracticeRoute){
                const auto route=cursor.practice_route_name();
                gf::log_info(context,"force-browser","practice route=%.*s",int(route.size()),route.data());
            }
            report();
        }
        render.begin_frame({frame,.055f,.065f,.085f});render.reset_arena();
        const auto draws=view.draw(render,options.width,options.height,cursor,live?&force:nullptr,snapshot.save().name,
            options.practice_from_picker?std::optional{practice_status}:std::nullopt,
            options.arrange_from_force?std::optional{arrange_status}:std::nullopt);
        render.end_frame();
        if(!draws){gf::log_error(context,"force-browser","could not draw read-only Force view");render.shutdown();return 2;}
        if(!captured)captured=capture_if_requested(context,options,frame);
        render.present();context.advance_frame();++frame;
        if(options.frames&&frame>=options.frames)break;
    }
    if(renderer_active)render.shutdown();
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
    case Result::ForceConflict:return "FORCE HAS CONFLICTING MEMBERS";
    case Result::CostUnverified:return "EXACT FORCE COST UNVERIFIED";
    case Result::OverBudget:return "FORCE WOULD EXCEED ITS BUDGET";
    case Result::EditedRetail:return "EDITED RETAIL MEMBER NOT ALLOWED";
    }
    return "PLACEMENT REFUSED";
}

int run_borg_browser(gf::Context& context,gf::RenderEngine& render,const Options& options,
                     const gf::BorgPackageCatalog& packages,BorgArrangeHandoff* handoff){
    gf::BorgInstalledBrowser browser;
    const bool collection_ui=!options.practice_from_picker;
    std::unique_ptr<gf::pc::BorgSelectedSave> owned_save;std::string error;
    const gf::AccountSaveEntry* selected_save=handoff?&handoff->save:nullptr;
    bool renderer_active=true;
    auto fail=[&](const std::string& why){
        gf::log_error(context,"borg-picker","%s",why.c_str());if(renderer_active)render.shutdown();return 2;
    };
    gf::PackageBorgRoster roster;
    if(options.browse_owned_borgs&&!handoff){
        owned_save=gf::pc::open_borg_selected_save(options.selected_save_root,options.selected_account,options.selected_save,&error);
        if(!owned_save)return fail(error);
        selected_save=&owned_save->entry;
    }
    auto read_retail=[&](gf::ForceCatalog* retail){
        return options.retail_force_catalogue.empty()||load_retail_force_catalogue(context.files(),options.retail_force_catalogue,retail);
    };
    std::unique_ptr<gf::BorgForceArrange> placement;
    gf::BorgPlacementPanel placement_panel;
    if(options.placement_force){
        placement=handoff?std::move(handoff->arranger):std::make_unique<gf::BorgForceArrange>(*owned_save->backend);
        placement_panel.force=unsigned(options.placement_force-1);
        placement_panel.slot=unsigned(options.placement_slot-1);
        placement_panel.headless=options.headless;
    }else if(owned_save){
        gf::PackageBorgSaveStore reader(*owned_save->backend);
        const auto loaded=reader.load(&roster);
        if(loaded!=gf::PackageBorgLoad::Loaded&&loaded!=gf::PackageBorgLoad::Missing)
            return fail("package roster unavailable or invalid");
        if(!reader.current())return fail("selected save changed while opening the collection");
    }
    if(collection_ui){
        auto metadata=gf::pc::load_borg_library_metadata("borgs",packages);
        if(placement){
            gf::ForceCatalog retail;
            if(!handoff&&!read_retail(&retail))return fail("could not read the explicit retail catalogue");
            const auto* catalog=handoff?handoff->retail:retail.ready()?&retail:nullptr;
            using Open=gf::BorgForceArrangeOpen;
            switch(placement->open(placement_panel.force,placement_panel.slot,catalog,packages,metadata.snapshots,{},metadata.issues)){
            case Open::Ready:break;
            case Open::InvalidPosition:return fail("invalid Force position");
            case Open::SaveUnavailable:return fail("selected save or package roster is unavailable");
            case Open::RetailUnverified:return fail("retail Force record is unverified; supply its matching --retail-catalog");
            case Open::Stale:return fail("selected save changed; reopen before arranging");
            }
            const auto* target=placement->force_view()->find_slot(placement_panel.slot);
            placement_panel.target_claim=!target?"VERIFIED EMPTY":target->overlap()?"OVERLAP":
                target->retail?"RETAIL OCCUPIED":"PACKAGE OCCUPIED";
        }else if(owned_save&&!owned_save->backend->current())return fail("selected save changed while opening the collection");
        if(!browser.open_collection(packages,std::move(metadata.snapshots),metadata.issues,
                placement?placement->roster():owned_save?&roster:nullptr,options.owned_instance))
            return fail("requested owned instance unavailable");
    }else if(!browser.open_owned(packages,std::move(roster),options.owned_instance))
        return fail("requested owned instance unavailable");
    if(handoff)browser.sync_input(handoff->opening_input);
    gf::BorgCheatPickerAuthority cheat_authority;
    std::optional<gf::BorgCheatPicker> cheat_picker;
    if(options.cheat_offer_picker){
        // This standalone host capability is explicit and offer-only. It
        // does not open a save or consult package declarations or save flags.
        cheat_authority.set_authorized(true);
        cheat_picker.emplace(browser.collection()->library(),cheat_authority.permit());
    }
    auto set_placement_status=[&](std::string_view status,bool locked=false){
        placement_panel.status=status;placement_panel.locked=locked;
        gf::log_info(context,"borg-placement","%.*s",int(status.size()),status.data());
    };
    if(placement)gf::log_info(context,"borg-placement","arranging account=%llu save=%llu Force %u slot %u%s",
        static_cast<unsigned long long>(selected_save->key.account),static_cast<unsigned long long>(selected_save->key.save),
        placement_panel.force+1,placement_panel.slot+1,options.headless?" (headless cannot write)":"");
    gf::BorgInstalledBrowserView view;gf::BorgBrowserTile tile;gf::InputRouter router;
    gf::PortableFont collection_font;
    const gf::PortableFont* text_font=nullptr;
    if(collection_ui&&!placement&&!cheat_picker&&!options.collection_font_root.empty()){
        constexpr gf::u32 font_handle=59002; // Collection-only immutable slot; not preview or icon art.
        std::string font_error;
        if(gf::pc::load_local_portable_font(options.collection_font_root,render,font_handle,&collection_font,&font_error)){
            text_font=&collection_font;
            gf::log_info(context,"borg-picker","Collection font bound");
        }else gf::log_warn(context,"borg-picker","Collection font unavailable; using bitmap fallback: %s",font_error.c_str());
    }
    gf::NativeAnimationMosaic preview;
    gf::BorgBrowserPracticePanel practice_panel;
    constexpr gf::u32 tile_handle=59001; // single replaceable texture in this isolated presentation mode
    bool confirmed=false,captured=handoff&&handoff->captured,preview_open=false,preview_unavailable=false,previous_back=false;
    bool wait_for_exit_release=false;
    std::optional<gf::BorgLibraryKey> loaded_tile_key;
    auto refresh_tile=[&](){
        if(collection_ui){
            const auto& collection=*browser.collection();
            const auto* row=collection.installed_row(collection.selected());
            const auto key=row?row->key:std::nullopt;
            if(key==loaded_tile_key)return;
            loaded_tile_key=key;
        }
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
        if(const auto* entry=collection_ui?nullptr:browser.entry()){
            gf::BorgCatalogSnapshot snapshot;
            if(gf::pc::load_borg_catalog_snapshot("borgs",browser.catalog(),entry->manifest.identity.id,&snapshot,nullptr))
                browser.bind(std::move(snapshot));
        }
        refresh_tile();
    };
    refresh_package();
    unsigned frame=handoff?handoff->frame:0;
    const auto start=handoff?handoff->start:context.clock().monotonic_ns();
    auto previous=context.clock().monotonic_ns();
    while(context.window().pump_events()&&!context.window().should_close()){
        const auto now=context.clock().monotonic_ns();
        if(options.seconds&&now-start>=gf::u64(options.seconds)*1000000000ull)break;
        unsigned width=options.width,height=options.height,logical_w=width,logical_h=height;
        if(collection_ui&&!options.headless){
            context.window().size(&width,&height);
            context.window().logical_size(&logical_w,&logical_h);
        }
        const auto layout=gf::BorgCollectionLayout::fit(width,height,logical_w,logical_h);
        const bool preview_fits=!collection_ui||(width>=200&&height>=212);
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
            pad={};pad.dpad_down=frame==1||frame==15||frame==17||frame==23;pad.dpad_up=frame==7;
            pad.a=frame==3||frame==9||frame==25;pad.x=frame==5||frame==11||frame==19;pad.b=frame==13||frame==21;
        }
        const bool back=pad.b&&!previous_back;previous_back=pad.b;
        const gf::f32 dt=(options.headless||options.frames)?1.f/60:gf::f32(now>=previous?now-previous:0)/1e9f;previous=now;
        if(suppress_return_input){
            // Keep drawing the restored selection while the exit key is
            // held. No return-frame input can change or launch that copy.
        }else if(placement&&placement->offer()){
            if(!gf::BorgPlacementPanel::review_fits(layout)||(!options.headless&&!context.window().focused())){
                placement->cancel();browser.sync_input(pad);
                set_placement_status(gf::BorgPlacementPanel::review_fits(layout)?
                    "WINDOW LOST FOCUS - PLACEMENT CANCELLED":"ENLARGE WINDOW TO REVIEW PLACEMENT - NO WRITE");
            }else{
                const auto request=placement->step_confirmation(pad);
                if(request==gf::BorgPlacementRequest::Cancel||(options.headless&&request==gf::BorgPlacementRequest::Confirm)){
                    placement->cancel();browser.sync_input(pad);
                    set_placement_status(options.headless&&request==gf::BorgPlacementRequest::Confirm?
                        "HEADLESS INSPECTION DOES NOT WRITE":"PLACEMENT CANCELLED - NO WRITE");
                }else if(request==gf::BorgPlacementRequest::Confirm){
                    gf::BorgPackageCatalog current;
                    gf::pc::BorgOwnedMetadata checked;
                    gf::ForceCatalog retail;
                    if(read_retail(&retail)){
                        current=gf::pc::scan_borg_packages("borgs");
                        checked=gf::pc::load_borg_owned_metadata("borgs",current,*placement->roster());
                    }
                    // Even a failed reload consumes the explicit consent.
                    // The arranger rechecks both records and the full cost basis.
                    const auto result=placement->confirm(retail.ready()?&retail:nullptr,current,checked.snapshots);
                    browser.sync_input(pad);
                    set_placement_status(borg_placement_label(result),true);
                }else if(placement->state()==gf::BorgForceArrangeState::ReopenRequired)
                    set_placement_status("SELECTED SAVE CHANGED - NO WRITE",true);
            }
        }else if(placement_panel.locked){
            // Acknowledged result or stale/uncertain evidence: reopen rather
            // than reusing the old picker snapshot for another write.
            if(back)break;
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
         }else if(placement&&back)break;
         else if(const auto clicked=collection_ui&&input.pointer.within_window&&input.pointer.left_pressed?
                        layout.hit_row(input.pointer.x,input.pointer.y,*browser.collection()):std::nullopt;
                    clicked&&browser.select_collection_row(*clicked)){
             if(cheat_picker)cheat_picker->cancel();
             confirmed=preview_unavailable=false;refresh_tile();browser.sync_input(pad);
          }else switch(browser.step(pad,options.practice_from_picker,layout.rows,placement!=nullptr)){
          case gf::BorgBrowserChange::Package:
              if(cheat_picker)cheat_picker->cancel();
              confirmed=preview_unavailable=false;placement_panel.status={};practice_panel.status={};refresh_package();break;
         case gf::BorgBrowserChange::Variant:confirmed=preview_unavailable=false;refresh_tile();break;
         case gf::BorgBrowserChange::PracticeRoute:
             practice_panel.status={};
             gf::log_info(context,"borg-picker","practice route=%.*s",int(browser.practice_route_name().size()),
                 browser.practice_route_name().data());
             break;
          case gf::BorgBrowserChange::Confirm:
              if(placement){
                  if(!placement->current()){
                      set_placement_status("SELECTED SAVE CHANGED - NO WRITE",true);break;
                  }
                  if(!gf::BorgPlacementPanel::review_fits(layout)){
                      set_placement_status("ENLARGE WINDOW TO REVIEW PLACEMENT - NO WRITE");break;
                  }
                  if(!options.headless&&!context.window().focused())break;
                  const auto* copy=browser.owned_entry();
                  using Result=gf::BorgPlacementResult;
                  const auto result=copy&&placement->select_owned(copy->instance)?placement->stage(pad):Result::OwnedUnavailable;
                  if(const auto* offer=placement->offer()){
                      gf::log_info(context,"borg-placement","offered Force %u slot %u owned-instance=%llu package=%s variant=%s force-cost=%u->%u",
                          offer->force+1,offer->slot+1,static_cast<unsigned long long>(offer->owned.instance),
                          offer->owned.identity.id.c_str(),offer->owned.variant->id.c_str(),offer->before_cost,offer->after_cost);
                  }
                  set_placement_status(borg_placement_label(result),result==Result::SaveUnavailable||
                      placement->state()==gf::BorgForceArrangeState::ReopenRequired);
                  break;
              }
              if(cheat_picker){
                  const auto* row=browser.collection()->installed_row(browser.collection()->selected());
                  if(row&&row->key&&cheat_picker->select(*row->key)==gf::BorgCheatPickerResult::OfferReady){
                      const auto* offer=cheat_picker->offer();
                      confirmed=true;
                      gf::log_info(context,"borg-picker","cheat offer %s variant=%s (no ownership, save, or battle admission)",
                          offer->key->package_id.c_str(),offer->key->variant_id.c_str());
                  }
                  break;
              }
             confirmed=true;
            if(const auto selection=browser.selection())gf::log_info(context,"borg-picker","inspected %s variant=%s (no save or battle admission)",
                selection->identity.id.c_str(),selection->variant.c_str());
            break;
         case gf::BorgBrowserChange::Preview:
             practice_panel.status={};
            if(const auto selection=browser.selection()){
                 if((placement&&!placement->current())||(!placement&&owned_save&&!owned_save->backend->current()))
                     return fail("selected save changed; reopen to inspect the current collection");
                gf::pc::BorgPackagePreviewFiles files;using Inputs=gf::pc::BorgPackagePreviewFiles;
                 const bool files_loaded=!preview_fits?(error="window too small for body preview",false):
                      collection_ui?gf::pc::load_borg_preview_files("borgs",*browser.metadata(),selection->variant,&files,&error):
                     gf::pc::load_borg_preview_files("borgs",browser.catalog(),selection->identity.id,selection->variant,&files,&error);
                 if(files_loaded&&
                      preview.load_package_preview(collection_ui?selection->identity:files.identity,files.profile,
                         Inputs::view(files.model),Inputs::view(files.rig_prefix),
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
             const auto route_name=browser.practice_route_name();
             if(!picker_practice_options(browser.practice_route(),&practice))
                 return fail("could not select the existing practice route");
             practice.owned_practice=request;
             if(!preview.close_package_preview(render))return fail("could not release preview textures before practice");
             tile={};
             const auto launch="launched practice for owned instance="+std::to_string(saved.instance)+" route="+std::string(route_name);
             const auto outcome=run_practice_from_browser(context,render,options,std::move(practice),*qualified,
                 {start,frame,captured,renderer_active},"Owned Borg Collection","borg-picker",launch,&error);
             if(outcome==BrowserPracticeReturn::Closed)break;
             if(outcome==BrowserPracticeReturn::RendererError)return fail(error);
             previous=context.clock().monotonic_ns();
             if(!owned_save->backend->current())return fail("selected save changed during practice; reopen the collection");
             refresh_tile();wait_for_exit_release=true;
             practice_panel.status=outcome==BrowserPracticeReturn::Returned?"RETURNED FROM PRACTICE":"PRACTICE UNAVAILABLE";
             if(outcome==BrowserPracticeReturn::Returned)gf::log_info(context,"borg-picker","returned to owned instance=%llu after practice route=%.*s",
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
             placement_panel.confirm_selected=placement->confirm_selected();
             placement_panel.force_name=offer?offer->retail_force_name:std::nullopt;
             placement_panel.before_cost=offer?offer->before_cost:0;
             placement_panel.after_cost=offer?offer->after_cost:0;
         }
         render.begin_frame({frame,.055f,.065f,.085f,collection_ui});render.reset_arena();
        const auto draws=preview_open&&preview_fits?preview.draw(render,width,height):
            view.draw(render,width,height,browser,tile,confirmed,preview_unavailable||(preview_open&&!preview_fits),
                  selected_save?std::string_view(selected_save->name):std::string_view{},placement?&placement_panel:nullptr,
                  options.practice_from_picker?&practice_panel:nullptr,collection_ui?&layout:nullptr,cheat_picker.has_value(),text_font);
        render.end_frame();
        if(!captured)captured=capture_if_requested(context,options,frame);
        render.present();context.advance_frame();++frame;
         if(!draws&&(!collection_ui||(width>=100&&height>=60))){
            gf::log_error(context,"borg-picker","no browser draws");render.shutdown();return 2;
        }
        if(options.frames&&frame>=options.frames)break;
    }
    const auto* selected=browser.entry();const auto* variant=browser.variant();
    gf::log_info(context,"borg-picker","frames=%u packages=%zu selected=%s variant=%s view=%s rows=%zu",
        frame,browser.catalog().discovered.size(),selected?selected->manifest.identity.id.c_str():"none",
        variant?variant->id.c_str():"unavailable",preview_open?"preview":renderer_active?"picker":"practice",browser.size());
     if(selected_save){
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
        unsigned chosen=0;
        if(files.profile.static_body&&!options.preview_clip.empty()){
            gf::log_error(context,"borg-preview","this variant declares a static body with no animation clips");
            mosaic.close_package_preview(render);render.shutdown();return 2;
        }
        if(!options.preview_clip.empty()&&options.preview_clip!="idle"){
            const auto next=std::find_if(files.profile.clips.begin(),files.profile.clips.end(),
                [&](const auto& clip){return clip.id==options.preview_clip;});
            if(next==files.profile.clips.end()){
                gf::log_error(context,"borg-preview","clip %s is not declared for this variant",options.preview_clip.c_str());
                mosaic.close_package_preview(render);render.shutdown();return 2;
            }
            chosen=unsigned(next-files.profile.clips.begin())+1;
            if(!mosaic.select(0,chosen,render,&error)){
                gf::log_error(context,"borg-preview","clip selection refused: %s",error.c_str());
                mosaic.close_package_preview(render);render.shutdown();return 2;
            }
        }
        if(!options.preview_origin.empty()&&!mosaic.select_package_origin(options.preview_origin,render,&error)){
            gf::log_error(context,"borg-preview","origin selection refused: %s",error.c_str());
            mosaic.close_package_preview(render);render.shutdown();return 2;
        }
        gf::log_info(context,"borg-preview","%s variant=%s clip=%s (%u/%u); body preview only, no actor or gameplay rules",
            files.identity.id.c_str(),files.profile.variant.c_str(),chosen?files.profile.clips[chosen-1].id.c_str():
                files.profile.static_body?"none":"idle",chosen+1,mosaic.page_count());
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
    gf::InputRouter router;unsigned frame=0,marker_frames=0;bool captured=false;const auto start=context.clock().monotonic_ns();auto previous=start;
    while(context.window().pump_events()&&!context.window().should_close()){
        const auto now=context.clock().monotonic_ns();if(options.seconds&&now-start>=gf::u64(options.seconds)*1000000000ull)break;
        context.input().pump();const auto& snapshot=context.input().snapshot();const auto pad=router.route(snapshot,0);if(snapshot.quit||pad.quit_requested)break;
        const gf::f32 dt=(options.headless||options.frames)?1.f/60:gf::f32(now>=previous?now-previous:0)/1e9f;previous=now;
        if(!mosaic.step(pad,dt,render,&error)){gf::log_error(context,"animation-mosaic","step failed: %s",error.c_str());render.shutdown();return 2;}
        unsigned body_submissions=0;
        render.begin_frame({frame,.055f,.065f,.085f});render.reset_arena();
        const auto draws=mosaic.draw(render,options.width,options.height,&body_submissions);
        const auto submitted=render.end_frame();
        if(package_preview&&!body_submissions){
            const auto why=mosaic.package_draw_error();
            gf::log_error(context,"borg-preview","draw failed: %s",why.empty()?"no body geometry submitted":why.c_str());
            mosaic.close_package_preview(render);render.shutdown();return 2;
        }
        if(!options.preview_origin.empty()){
            if(!submitted||!mosaic.package_origin_submitted()){
                gf::log_error(context,"borg-preview","draw failed: selected origin did not submit");
                mosaic.close_package_preview(render);render.shutdown();return 2;
            }
            ++marker_frames;
        }
        if(!captured)captured=capture_if_requested(context,options,frame);
        render.present();context.advance_frame();++frame;
        if(!draws){gf::log_error(context,"animation-mosaic","no preview draws");render.shutdown();return 2;}
        if(options.frames&&frame>=options.frames)break;
    }
    gf::log_info(context,"animation-mosaic","frames=%u Borg=%u page=%u",frame,mosaic.borg(),mosaic.page()+1);
    if(!options.preview_origin.empty())gf::log_info(context,"borg-preview","origin=%s marker-frames=%u point-only",
        options.preview_origin.c_str(),marker_frames);
    if(!mosaic.close_package_preview(render)){gf::log_error(context,"borg-preview","could not release preview textures");render.shutdown();return 2;}
    render.shutdown();return 0;
}

} // namespace

int main(int argc, char** argv) {
    Options options;
    if (!parse(argc, argv, &options)) return 2;
    if(!options.list_battle_rosters.empty()){
        std::vector<gf::pc::BattleRosterListing> entries;
        std::string error;
        if(!gf::pc::scan_battle_roster_presets(options.list_battle_rosters,&entries,&error)){
            std::fprintf(stderr,"Battle rosters: %s\n",error.c_str());return 2;
        }
        for(const auto& entry:entries){
            if(!entry.preset){
                std::fprintf(stderr,"Battle roster %s: %s\n",gf::borg_package_asset_path_valid(entry.filename)?
                             entry.filename.c_str():"<invalid filename>",entry.error.c_str());
                continue;
            }
            const auto& preset=*entry.preset;
            std::printf("%s | stage=%s | slots=%zu | locals=%u",entry.filename.c_str(),preset.stage.c_str(),
                        preset.fighters.size(),unsigned(preset.locals));
            if(preset.rounds_to_win)std::printf(" | first-to=%u",unsigned(preset.rounds_to_win));
            if(preset.round_seconds)std::printf(" | seconds=%u",unsigned(preset.round_seconds));
            std::puts(" | metadata only");
        }
        if(entries.empty())std::puts("No .roster files");
        return 0;
    }
    if(options.list_stories)return gf::pc::list_portable_stories("stories");
    if(options.list_challenges)return gf::pc::list_portable_challenges("challenges");
    std::unique_ptr<gf::pc::PortableStoryLaunch> portable_story;
    if(options.portable_story){
        gf::ForceCatalog retail;
        if(!options.retail_force_catalogue.empty()){
            auto files=gf::sdl::create_file_system();
            if(!files||!load_retail_force_catalogue(*files,options.retail_force_catalogue,&retail)){
                std::fputs("Portable Story: could not read the explicit retail catalogue\n",stderr);return 2;
            }
        }
        const auto check=gf::pc::PortableStoryLaunch::open(*options.portable_story,std::move(retail),&portable_story);
        if(!check){std::fprintf(stderr,"Portable Story: %s\n",check.reason.c_str());return 2;}
    }
    if(options.list_stages){
        const auto catalog=gf::pc::scan_stage_packages("stages");
        using Status=gf::StagePackageScanStatus;
        if(catalog.status==Status::NoDirectory){std::puts("No stages directory");return 0;}
        if(catalog.status!=Status::Complete){std::fputs("Stage discovery unavailable\n",stderr);return 2;}
        for(const auto& entry:catalog.entries)std::printf("%s | %s | %s\n",entry.manifest.id.c_str(),entry.manifest.name.c_str(),
            entry.issue==gf::StagePackageIssue::None?"discovered; battle not validated":entry.reason.c_str());
        for(const auto& issue:catalog.issues)std::fprintf(stderr,"Stage package %s%s%s: %s\n",
            gf::display_name_valid(issue.folder,255)?issue.folder.c_str():"<unprintable folder>",
            issue.manifest.id.empty()?"":" | ",issue.manifest.id.c_str(),issue.reason.c_str());
        return 0;
    }
    if(unsigned(options.list_borgs)+unsigned(!options.check_borg_catalog.empty())+unsigned(!options.check_borg_moves.empty())+
       unsigned(!options.check_borg_spawn.empty())+unsigned(!options.check_borg_effects.empty())+
       unsigned(!options.check_borg_forms.empty())+unsigned(!options.check_borg_fusions.empty())+
       unsigned(!options.check_borg_acquisition.empty())+unsigned(!options.check_borg_variants.empty())>1)return 2;
    const auto borg_packages=gf::pc::scan_borg_packages("borgs");
    gf::pc::PortableChallengeLaunch portable_challenge;
    if(options.portable_challenge){
        gf::ForceCatalog retail;
        if(!options.retail_force_catalogue.empty()){
            auto files=gf::sdl::create_file_system();
            if(!files||!load_retail_force_catalogue(*files,options.retail_force_catalogue,&retail)){
                std::fputs("Portable Challenge: could not read the explicit retail catalogue\n",stderr);return 2;
            }
        }
        const auto check=gf::pc::load_portable_challenge_launch(*options.portable_challenge,borg_packages,
            std::move(retail),&portable_challenge);
        if(!check){std::fprintf(stderr,"Portable Challenge: %s\n",check.reason.c_str());return 2;}
    }
    PackageBattleFiles battle_files;
    if(!options.battle_borg.empty()){
        std::string error;
        if(!load_package_battle_files(options,borg_packages,battle_files,error)){
            std::fprintf(stderr,"Borg Battle Test: %s\n",error.c_str());return 2;
        }
    }
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
    if(options.inspect_force&&!options.browse_force)return inspect_borg_force(options,borg_packages);
    std::unique_ptr<gf::pc::BorgForceBrowseSnapshot> force_browser;
    if(options.browse_force){
        gf::ForceCatalog catalogue;std::string error;
        if(!options.retail_force_catalogue.empty()){
            auto files=gf::sdl::create_file_system();
            if(!files||!load_retail_force_catalogue(*files,options.retail_force_catalogue,&catalogue)){
                std::fputs("Force browser: could not read the explicit retail catalogue\n",stderr);return 2;
            }
        }
        force_browser=gf::pc::BorgForceBrowseSnapshot::open(options.selected_save_root,{options.selected_account,options.selected_save},
            "borgs",borg_packages,std::move(catalogue),&error);
        if(!force_browser){std::fprintf(stderr,"Force browser: %s\n",error.c_str());return 2;}
    }
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
    if(!options.check_borg_controllers.empty()){
        gf::pc::BorgControllerInspection controllers;std::string error;
        if(!gf::pc::load_borg_controller_inspection("borgs",borg_packages,options.check_borg_controllers,&controllers,&error)){
            std::fprintf(stderr,"Borg controllers: %s\n",error.c_str());return 2;
        }
        std::printf("%s\t%s\tcontrollers=%zu\n",controllers.identity.id.c_str(),controllers.identity.name.c_str(),
            controllers.profile.definitions.size());
        using namespace gf::borg_controller_detail;
        for(const auto& row:controllers.profile.definitions){
            const auto type=row.source_movement_type?std::to_string(*row.source_movement_type):"unresolved";
            const auto air=row.air_resource==gf::BorgControllerAirResource::AirJumps?
                std::string("air-jumps:")+(row.air_jump_points?std::to_string(*row.air_jump_points):"unresolved"):
                std::string(resources[unsigned(row.air_resource)]);
            std::printf("%s\tform=%s\tsource=%s\tsource-movement-type=%s\tselector-family=%s\tmovement=%s\tcontact=%s"
                 "\tair-resource=%s\trecovery=%s\tmotion-bank=%s\tpose=%s\tactions=%s\tevidence=%s\tadd-on=%s\n",
                row.variant.c_str(),row.form.c_str(),row.source.c_str(),type.c_str(),row.selector_family.c_str(),
                movements[unsigned(row.movement)].data(),contacts[unsigned(row.contact)].data(),air.c_str(),
                recoveries[unsigned(row.recovery)].data(),banks[unsigned(row.motion_bank)].data(),
                 poses[unsigned(row.pose)].data(),actions[unsigned(row.actions)].data(),evidence[unsigned(row.evidence)].data(),
                 gf::borg_controller_add_on_name(row.add_on).data());
        }
        std::puts("Controller declarations validated; provenance not authenticated and gameplay not loaded.");
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
    if(options.headless&&(options.test_battle||options.battle_mvp||options.borg_controls||!options.battle_borg.empty()||options.animation_mosaic||!options.preview_borg.empty()||
       !options.browse_battle_rosters.empty()||
       options.browse_borgs||!options.selected_save_root.empty()||!options.story_map.empty()||options.portable_challenge||portable_story)){
        auto backends=headless.get();headless_files=gf::sdl::create_file_system();headless_log=gf::sdl::create_log_backend();
        backends.files=headless_files.get();backends.log=headless_log.get();context=gf::Context(backends,60);
    }
    std::unique_ptr<gf::sdl::Platform> platform;
    std::unique_ptr<gf::IRenderBackend> renderer;

    const bool story_screen=portable_story||!options.story_map.empty();
    const bool roster_browser=!options.browse_battle_rosters.empty();
    if (!options.headless) {
        platform = std::make_unique<gf::sdl::Platform>();
        renderer = gf::gl::create_renderer(nullptr);
        if (!renderer) {
            std::fprintf(stderr, "no render backend available\n");
            return 1;
        }
        std::string error;
        const bool resizable_collection=options.browse_borgs||
            (options.browse_owned_borgs&&!options.practice_from_picker);
        gf::Backends backends =
             platform->create({options.width, options.height, story_screen?"Story Map":
                                 options.portable_challenge?"Portable Challenge":
                                 roster_browser?"Battle Roster Presets":
                                options.placement_force?"Arrange Saved Force":
                               force_browser?"Saved Force Browser":
                              options.browse_owned_borgs?"Owned Borg Collection":
                              !options.selected_save_root.empty()?"Borg Save Recovery":
                                options.cheat_offer_picker?"Cheat Picker (Offer Only)":
                                options.browse_borgs?"Installed Borg Collection":!options.preview_borg.empty()?"Borg Package Preview":
                                 options.animation_mosaic?"Borg Animation Mosaic":options.borg_controls?"Borg Control Test":
                                  !options.battle_borg.empty()?"Borg API Battle Test":"Gotcha Force",
                                     resizable_collection||story_screen||roster_browser||options.battle_picker||!options.roster_borgs.empty(),
                                 !options.animation_export.empty()},
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
    if (!render.initialize({options.width, options.height, !options.headless&&options.animation_export.empty(), true,
                            story_screen?"Story Map":roster_browser?"Battle Roster Presets":"Gotcha Force"},
                           context.backends().window, &error)) {
        gf::log_error(context, "render", "initialization failed: %s", error.c_str());
        return 1;
    }
    gf::log_info(context, "host", "backend=%s", render.capabilities().name.c_str());
    if(portable_story)
        return gf::pc::run_portable_story_campaign(context,render,*portable_story,
            {options.headless,options.frames,options.seconds},
            [&](gf::RenderEngine& battle,gf::pc::PortableBattleHost& host,gf::pc::StoryDuelBudget budget,
                gf::pc::PortableBattleHostReport* report){
                Options duel;
                duel.headless=options.headless;duel.no_audio=options.no_audio;
                duel.width=budget.width;duel.height=budget.height;duel.frames=budget.frames;duel.seconds=budget.seconds;
                return run_package_battle(context,battle,duel,host.packages(),nullptr,&host,report);
            });
    if(!options.story_map.empty()){
        bool captured=false;
        const auto result=gf::pc::run_portable_story_map(context,render,"stories",options.story_map,
            {options.headless,options.frames,options.seconds},[&](unsigned frame){
                if(!captured)captured=capture_if_requested(context,options,frame);
            });
        render.shutdown();
        return result;
    }
    if(options.test_battle)return run_battle_test(context,render,options);
    if(options.portable_challenge)return run_portable_challenge(context,render,options,borg_packages,portable_challenge);
    if(roster_browser)return run_roster_browser(context,render,options,borg_packages);
    if(!options.battle_borg.empty())return options.battle_picker?run_battle_picker(context,render,options,borg_packages,battle_files.first):
        run_package_battle(context,render,options,battle_files.first,battle_files.stage.get(),nullptr,nullptr,nullptr,battle_files.extra);
    if(options.borg_controls)return options.walk_borg.empty()?run_borg_controls(context,render,options):
        run_borg_walk_controls(context,render,options,borg_packages);
    if(options.battle_mvp)return run_battle_mvp(context,render,options,package,owned_practice.get());
    if(force_browser)return run_borg_force_browser(context,render,options,borg_packages,*force_browser);
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
