#pragma once
#include <vector>
#include "engine/assets/battle_initial_ui_world.hpp"
#include "engine/gameplay/scene_physical_links.hpp"
#include "engine/gameplay/attachment_ui_local_transform.hpp"
namespace gf {
struct BattleInitialUiWorldVisit {ScenePhysicalLink object;u8 source_list=4;u32 callback=0;bool matrix_written=false;};
struct BattleInitialUiWorldState {
    ScenePhysicalLink head,previous,next;u8 source_list=4,cleared_views=0;
    u16 callbacks=0,matrices=0;std::vector<BattleInitialUiWorldVisit> visits;
    bool repeated_lists_empty=false,traversal_exhausted=false,complete=false;
};
//Only the initial80F88 mask/phase gate. Empty818CC never reads these operands.
inline bool battle_initial_hud_world_inactive(u8 target7d2,u8 parent_phase18){return !(target7d2&1)||parent_phase18!=1;}
}
