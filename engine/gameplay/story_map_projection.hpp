#pragma once
#include "engine/assets/story_manifest.hpp"

namespace gf {

struct StoryMapNode {
    std::string id, label_key;
    float x = 0.0f, y = 0.0f; // Unit map rectangle, measured from its top left.
    bool selected = false, available = false;
};

struct StoryMapProjection {
    std::string story_id, name, map_art;
    std::vector<StoryMapNode> nodes;
};

namespace story_detail {
inline StoryMapProjection map_projection(const StoryManifest& manifest, std::string_view selected_id,
                                         bool can_prepare) {
    StoryMapProjection map{manifest.id, manifest.name, manifest.map_art, {}};
    map.nodes.reserve(manifest.nodes.size());
    for(const auto& node : manifest.nodes) {
        const bool selected = node.id == selected_id;
        map.nodes.push_back({node.id, node.label_key, float(node.x) / kStoryMapScale, float(node.y) / kStoryMapScale,
            selected, selected && node.encounter.has_value() && can_prepare});
    }
    return map;
}
}

// For discovery-only inspection, show the authored start, not a save or an
// admitted battle. It works even when the named encounters aren't installed.
inline StoryMapProjection story_map_preview(const StoryManifest& validated_manifest) {
    return story_detail::map_projection(validated_manifest, validated_manifest.start, false);
}

} // namespace gf
