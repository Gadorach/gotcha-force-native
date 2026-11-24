#pragma once
#include <vector>
#include "engine/assets/battle_initial_world_dispatch.hpp"
#include "engine/gameplay/scene_physical_links.hpp"
#include "engine/graphics/transform.hpp"
namespace gf {
struct BattleInitialWorldVisit {
    ScenePhysicalLink object;u8 source_list=0;u32 callback=0;bool matrix_written=false;
};
struct BattleInitialWorldDispatchState {
    u8 list_cursor=0,source_list=0;ScenePhysicalLink head,next,previous;
    u16 callbacks=0,matrices=0;std::vector<BattleInitialWorldVisit> visits;
    bool traversal_exhausted=false,complete=false,stage_aggregate_released=false;
};
//Value-only source object fields, not an HSD root world/render receipt.
struct SceneObjectWorldCallbackState {
    Mat4 world114;std::array<f32,3> position20{};
    u64 generation=0,parent_world_generation=0;bool operator==(const SceneObjectWorldCallbackState&)const=default;
};
}
