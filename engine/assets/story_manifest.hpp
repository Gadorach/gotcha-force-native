#pragma once
#include <algorithm>
#include <array>
#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "engine/assets/borg_catalog_profile.hpp"

namespace gf {

inline constexpr std::size_t kStoryManifestBytes = 128u << 10;
inline constexpr std::size_t kStoryNodes = 128, kStoryEdges = 256;
inline constexpr unsigned kStoryMapScale = 1000;

struct StoryEncounterRef {
    std::string challenge_id, encounter_id;
    bool operator==(const StoryEncounterRef&) const = default;
};

struct StoryNode {
    std::string id, label_key;
    unsigned x = 0, y = 0;
    std::optional<StoryEncounterRef> encounter;
    bool operator==(const StoryNode&) const = default;
};

enum class StoryEdgeOutcome { Win, Loss };

struct StoryEdge {
    std::string from, to;
    StoryEdgeOutcome outcome = StoryEdgeOutcome::Win;
    bool operator==(const StoryEdge&) const = default;
};

struct StoryManifest {
    std::string id, name, start, map_art;
    std::vector<StoryNode> nodes; // Declared order, independent of progression.
    std::vector<StoryEdge> edges;
    bool operator==(const StoryManifest&) const = default;
};

struct PortableStoryEntry {
    std::size_t root_index = 0;
    std::string folder;
    StoryManifest manifest;
};

enum class PortableStoryScanStatus { Complete, NoDirectory, Unavailable, EntryBudget };
enum class PortableStoryIssueKind { Malformed, Missing, DuplicateId };
struct PortableStoryIssue {
    std::size_t root_index;
    std::string folder, id, reason;
    PortableStoryIssueKind kind;
};

struct PortableStoryCatalog {
    PortableStoryScanStatus status = PortableStoryScanStatus::Unavailable;
    std::vector<PortableStoryEntry> discovered;
    std::vector<PortableStoryIssue> issues;

    const PortableStoryEntry* find(std::string_view id) const {
        const auto it = std::lower_bound(discovered.begin(), discovered.end(), id,
            [](const auto& entry, std::string_view key) { return entry.manifest.id < key; });
        return it != discovered.end() && it->manifest.id == id ? &*it : nullptr;
    }
};

namespace story_detail {
inline bool fail(std::string* error, std::string_view why) {
    if(error) *error = "Story manifest: " + std::string(why);
    return false;
}

inline bool label_key_valid(std::string_view value) {
    auto alnum = [](char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'); };
    if(value.empty() || value.size() > 96 || !alnum(value.front())) return false;
    for(char c : value) if(!alnum(c) && c != '-' && c != '_' && c != '.') return false;
    return true;
}

inline bool map_coordinate(std::string_view text, unsigned* out) {
    if(text.empty()) return false;
    const auto end = text.data() + text.size();
    const auto result = std::from_chars(text.data(), end, *out);
    return result.ec == std::errc{} && result.ptr == end && *out <= kStoryMapScale;
}

inline constexpr std::size_t no_node = kStoryNodes;
struct GraphLinks {
    std::size_t start = no_node;
    // Slot 0 is win; slot 1 is loss. Only validated graph indices are used.
    std::array<std::array<std::size_t, 2>, kStoryNodes> next;
};

inline bool link_graph(const StoryManifest& manifest, GraphLinks* out, std::string* error) {
    if(!borg_package_id_valid(manifest.id) || !display_name_valid(manifest.name, 96) ||
       !borg_catalog_variant_id_valid(manifest.start) ||
       (!manifest.map_art.empty() && !borg_package_asset_path_valid(manifest.map_art)))
        return fail(error, "missing or invalid id, name, start, or map-art");
    if(manifest.nodes.empty() || manifest.nodes.size() > kStoryNodes ||
       manifest.edges.size() > kStoryEdges)
        return fail(error, "node or edge count outside bounds");

    auto find_node = [&](std::string_view id) {
        for(std::size_t i=0; i<manifest.nodes.size(); ++i)
            if(manifest.nodes[i].id == id) return i;
        return no_node;
    };
    for(std::size_t i=0; i<manifest.nodes.size(); ++i) {
        const auto& node = manifest.nodes[i];
        if(!borg_catalog_variant_id_valid(node.id) || !label_key_valid(node.label_key) ||
           node.x > kStoryMapScale || node.y > kStoryMapScale)
            return fail(error, "invalid node ID, label key, or coordinates");
        if(find_node(node.id) != i) return fail(error, "duplicate node ID");
        if(node.encounter &&
           (!borg_package_id_valid(node.encounter->challenge_id) ||
            !borg_catalog_variant_id_valid(node.encounter->encounter_id)))
            return fail(error, "invalid encounter reference");
    }

    GraphLinks links;
    links.start = find_node(manifest.start);
    if(links.start == no_node) return fail(error, "start is not a node");
    links.next.fill({no_node, no_node});
    for(const auto& edge : manifest.edges) {
        const auto source = find_node(edge.from), target = find_node(edge.to);
        if(source == no_node || target == no_node) return fail(error, "edge has an unknown endpoint");
        if(!manifest.nodes[source].encounter) return fail(error, "presentation node has a result edge");
        if(edge.outcome != StoryEdgeOutcome::Win && edge.outcome != StoryEdgeOutcome::Loss)
            return fail(error, "unsupported edge outcome");
        auto& next = links.next[source][edge.outcome == StoryEdgeOutcome::Win ? 0 : 1];
        if(next != no_node) return fail(error, "duplicate edge for node and outcome");
        next = target;
    }

    // Mark on enqueue so self-loops and longer cycles cannot exceed the queue.
    std::array<bool, kStoryNodes> reached{};
    std::array<std::size_t, kStoryNodes> queue{};
    queue[0] = links.start;
    reached[links.start] = true;
    std::size_t size = 1;
    for(std::size_t i=0; i<size; ++i) {
        for(const auto target : links.next[queue[i]]) {
            if(target == no_node || reached[target]) continue;
            reached[target] = true;
            queue[size++] = target;
        }
    }
    if(size != manifest.nodes.size()) return fail(error, "unreachable node");
    if(out) *out = links;
    if(error) error->clear();
    return true;
}
}

// Structural validation only. Named encounter dependencies and art are not
// loaded by this function, and valid data does not imply battle admission.
inline bool story_validate_manifest(const StoryManifest& manifest, std::string* error = nullptr) {
    return story_detail::link_graph(manifest, nullptr, error);
}

// No filesystem access. Leaves the caller's prior manifest untouched on failure.
inline bool story_read_manifest(std::string_view text, StoryManifest* out, std::string* error = nullptr) {
    using borg_package_detail::line;
    using borg_package_detail::split_fields;
    using story_detail::fail;
    if(!out || text.size() > kStoryManifestBytes) return fail(error, "manifest exceeds bound or output is missing");
    if(line(text) != "GFSTORY 1") return fail(error, "expected GFSTORY 1 header");

    StoryManifest next;
    while(!text.empty()) {
        const auto row = line(text);
        if(row.empty()) continue;
        // Validate even comments; no control characters inside a text row.
        if(!display_name_valid(row, kStoryManifestBytes)) return fail(error, "invalid UTF-8 or control character");
        if(row.starts_with('#')) continue;
        const auto equals = row.find('=');
        if(equals == std::string_view::npos) return fail(error, "expected key=value");
        const auto key = row.substr(0, equals), value = row.substr(equals + 1);

        if(key == "node") {
            std::array<std::string_view, 6> fields;
            StoryNode node;
            if(next.nodes.size() == kStoryNodes || !split_fields(value, &fields) ||
               !borg_catalog_variant_id_valid(fields[0]) || !story_detail::label_key_valid(fields[1]) ||
               !story_detail::map_coordinate(fields[2], &node.x) || !story_detail::map_coordinate(fields[3], &node.y))
                return fail(error, "malformed node row or too many nodes");
            node.id = fields[0];
            node.label_key = fields[1];
            if(fields[4] != "none" || fields[5] != "none") {
                if(!borg_package_id_valid(fields[4]) || !borg_catalog_variant_id_valid(fields[5]))
                    return fail(error, "malformed node encounter; use challenge ID and encounter ID or none|none");
                node.encounter = StoryEncounterRef{std::string(fields[4]), std::string(fields[5])};
            }
            next.nodes.push_back(std::move(node));
        } else if(key == "edge") {
            std::array<std::string_view, 3> fields;
            if(next.edges.size() == kStoryEdges || !split_fields(value, &fields) ||
               !borg_catalog_variant_id_valid(fields[0]) || !borg_catalog_variant_id_valid(fields[2]) ||
               (fields[1] != "win" && fields[1] != "loss"))
                return fail(error, "malformed edge row or too many edges");
            next.edges.push_back({std::string(fields[0]), std::string(fields[2]),
                fields[1] == "win" ? StoryEdgeOutcome::Win : StoryEdgeOutcome::Loss});
        } else {
            auto* target = key == "id" ? &next.id : key == "name" ? &next.name :
                key == "start" ? &next.start : key == "map-art" ? &next.map_art : nullptr;
            if(!target || !target->empty() || value.empty()) return fail(error, "unknown, duplicate, or empty scalar");
            if(!(key == "id" ? borg_package_id_valid(value) : key == "name" ? display_name_valid(value, 96) :
                 key == "start" ? borg_catalog_variant_id_valid(value) : borg_package_asset_path_valid(value)))
                return fail(error, "invalid scalar");
            *target = value;
        }
    }
    if(!story_validate_manifest(next, error)) return false;
    *out = std::move(next);
    return true;
}

} // namespace gf
