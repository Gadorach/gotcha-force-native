#pragma once
// Flat, index-based scene graph for the native rewrite.
//
// Design notes:
//   * Entities are dense indices held in one vector; nothing outside the world
//     stores a host pointer to an entity, so a saved id stays meaningful and the
//     whole scene is trivially serialisable and testable.
//   * Ids start at 1 and never change; entity 0 is "no entity". Because ids are
//     dense and never freed, id - 1 is the array slot.
//   * No OS, GPU, emulator or game headers. Maths comes from the portable
//     animation kernels (row-major affine 3x4, three-float vectors).
//   * No globals, no singletons, no I/O. Entity storage only grows in create()
//     and the traversal order only grows when the hierarchy changes, both
//     bounded by kMaxEntities.
//
// Dirty handling is deliberately simpler than gf_hierarchy_plan: that API
// models guest tree traversal with costs and flags pulled from a reviewed
// binding, while this world only needs "recompute this subtree once per frame".
// mark_dirty() records the invalidation on one node; update_matrices() walks
// parent-before-child and a recomputed parent forces every descendant to
// recompute in the same pass. That produces the same result as eagerly marking
// the whole subtree, without an O(entity count) scan per moved entity.
#include <cmath>
#include <cstddef>
#include <vector>

#include "engine/animation/native_transform.h"
#include "engine/core/types.hpp"

namespace gf {

using EntityId = u32;

// Entity 0 is reserved so a zero-initialised id is never a live entity.
inline constexpr EntityId kInvalidEntity = 0;

// Hard caps. 1<<16 entities keeps ids in u16-friendly ranges for scene files
// and bounds every traversal in this file.
inline constexpr u32 kMaxEntities = 1u << 16;
// Maximum parent-chain length. A chain longer than this is refused so a broken
// or hostile hierarchy cannot turn a walk into an unbounded loop.
inline constexpr u32 kMaxEntityDepth = 64;

// Euler angles in radians, X then Y then Z, applied by gf_transform_srt.
struct Transform {
    f32 position[3] = {0.0f, 0.0f, 0.0f};
    f32 rotation[3] = {0.0f, 0.0f, 0.0f};
    f32 scale[3] = {1.0f, 1.0f, 1.0f};
};

class World {
public:
    static constexpr u32 kMaxEntities = gf::kMaxEntities;
    static constexpr u32 kMaxDepth = gf::kMaxEntityDepth;

    World() = default;

    // ---- structure ----------------------------------------------------

    // Creates a root (or a child of `parent`). Returns kInvalidEntity when the
    // entity cap is reached, when `parent` does not exist, when the parent
    // chain is corrupt/too long, or when the new depth would exceed kMaxDepth.
    EntityId create(EntityId parent = kInvalidEntity) {
        if (nodes_.size() >= kMaxEntities) return kInvalidEntity;
        if (parent != kInvalidEntity) {
            if (!alive(parent)) return kInvalidEntity;
            // A cycle is impossible for a brand-new id, but the chain is still
            // walked bounded so a corrupted node can never spin a loop.
            if (contains_self(parent, EntityId(nodes_.size() + 1))) return kInvalidEntity;
            if (depth_of(parent) + 1u > kMaxDepth) return kInvalidEntity;
        }
        Node node;
        node.parent = parent;
        node.dirty = true;
        nodes_.push_back(node);
        order_dirty_ = true;
        return EntityId(nodes_.size());
    }

    // Re-parents a node. Refuses unknown ids, self-parenting, a parent that is
    // a descendant of `id` (cycle), and a move that would push any node in the
    // subtree past kMaxDepth.
    bool set_parent(EntityId id, EntityId parent) {
        Node* node = at(id);
        if (node == nullptr) return false;
        if (parent == id) return false;
        if (parent != kInvalidEntity) {
            if (!alive(parent)) return false;
            if (contains_self(parent, id)) return false;
            const u32 new_depth = depth_of(parent) + 1u;
            if (new_depth > kMaxDepth) return false;
            // Longest distance below `id`: the deepest descendant ends up at
            // new_depth + distance, which must still fit inside the cap.
            u32 extension = 0;
            for (std::size_t i = 0; i < nodes_.size(); ++i) {
                const EntityId other = EntityId(i + 1);
                if (other == id) continue;
                const u32 distance = distance_below(id, other);
                if (distance > extension) extension = distance;
            }
            if (new_depth + extension > kMaxDepth) return false;
        }
        if (node->parent == parent) return true;
        node->parent = parent;
        node->dirty = true;
        order_dirty_ = true;
        return true;
    }

    bool alive(EntityId id) const { return at(id) != nullptr; }
    std::size_t size() const { return nodes_.size(); }

    // kInvalidEntity for roots and for unknown ids.
    EntityId parent_of(EntityId id) const {
        const Node* node = at(id);
        return node ? node->parent : kInvalidEntity;
    }

    // Root depth is 0. kMaxDepth + 1 signals an unknown/unusable chain.
    u32 depth_of(EntityId id) const {
        u32 depth = 0;
        const Node* node = at(id);
        if (node == nullptr) return kMaxDepth + 1u;
        EntityId cursor = node->parent;
        while (cursor != kInvalidEntity) {
            const Node* parent = at(cursor);
            if (parent == nullptr) return kMaxDepth + 1u;
            if (++depth > kMaxDepth) return kMaxDepth + 1u;
            cursor = parent->parent;
        }
        return depth;
    }

    void clear() {
        nodes_.clear();
        order_.clear();
        depths_.clear();
        order_dirty_ = false;
        pass_ = 0;
    }

    // ---- per-entity state ---------------------------------------------

    const Transform* transform(EntityId id) const {
        const Node* node = at(id);
        return node ? &node->transform : nullptr;
    }

    // Rejects unknown ids and any non-finite component; a refused transform
    // leaves the previous one untouched and never marks the node dirty.
    bool set_transform(EntityId id, const Transform& value) {
        Node* node = at(id);
        if (node == nullptr) return false;
        if (!finite3(value.position) || !finite3(value.rotation) || !finite3(value.scale)) {
            return false;
        }
        node->transform = value;
        node->dirty = true;
        return true;
    }

    bool visible(EntityId id) const {
        const Node* node = at(id);
        return node ? node->visible : false;
    }

    bool set_visible(EntityId id, bool value) {
        Node* node = at(id);
        if (node == nullptr) return false;
        node->visible = value;
        return true;
    }

    // ---- matrices ------------------------------------------------------

    // Invalidates this node's world matrix. Descendants are implied by the
    // parent-before-child pass in update_matrices(); see the file header.
    void mark_dirty(EntityId id) {
        Node* node = at(id);
        if (node != nullptr) node->dirty = true;
    }

    // True when this node or any ancestor carries a pending invalidation, i.e.
    // the next update_matrices() will rewrite its world matrix.
    bool is_dirty(EntityId id) const {
        const Node* node = at(id);
        if (node == nullptr) return false;
        if (node->dirty) return true;
        u32 depth = 0;
        EntityId cursor = node->parent;
        while (cursor != kInvalidEntity && depth < kMaxDepth) {
            const Node* parent = at(cursor);
            if (parent == nullptr) return false;
            if (parent->dirty) return true;
            cursor = parent->parent;
            ++depth;
        }
        return false;
    }

    // Recomputes only the subtrees that need it, parents before children, so a
    // shared parent matrix is always current when a child composes with it.
    // Repeated calls with nothing dirty leave every matrix bit-identical.
    void update_matrices() {
        if (order_dirty_) rebuild_order();
        if (++pass_ == 0) {
            // Wrapped: clear stale "computed this pass" markers.
            pass_ = 1;
            for (Node& node : nodes_) node.updated_pass = 0;
        }
        f32 local[12];
        for (const EntityId id : order_) {
            Node& node = nodes_[id - 1u];
            const bool parent_rebuilt =
                node.parent != kInvalidEntity &&
                nodes_[node.parent - 1u].updated_pass == pass_;
            if (!node.dirty && !parent_rebuilt) continue;
            if (!gf_transform_srt(node.transform.scale, node.transform.rotation,
                                  node.transform.position, nullptr, 0, local)) {
                // set_transform validates every input, so this is unreachable
                // for data that entered through the public API. Leave the last
                // good matrix in place rather than publishing garbage.
                node.dirty = false;
                continue;
            }
            f32 result[12];
            if (node.parent == kInvalidEntity) {
                for (unsigned i = 0; i < 12; ++i) result[i] = local[i];
            } else if (!gf_math_evaluate(GF_MATH_CONCAT, nodes_[node.parent - 1u].world,
                                         local, 0.0f, result)) {
                node.dirty = false;
                continue;
            }
            for (unsigned i = 0; i < 12; ++i) node.world[i] = result[i];
            node.dirty = false;
            node.updated_pass = pass_;
        }
    }

    // Row-major affine 3x4, row stride 4. nullptr for unknown ids. Before the
    // first update a freshly created entity reports its (identity) default.
    const f32* world_matrix(EntityId id) const {
        const Node* node = at(id);
        return node ? node->world : nullptr;
    }

private:
    struct Node {
        EntityId parent = kInvalidEntity;
        Transform transform{};
        f32 world[12] = {1.0f, 0.0f, 0.0f, 0.0f,
                         0.0f, 1.0f, 0.0f, 0.0f,
                         0.0f, 0.0f, 1.0f, 0.0f};
        u32 updated_pass = 0;
        bool visible = true;
        bool dirty = true;
    };

    static bool finite3(const f32* v) {
        return std::isfinite(v[0]) && std::isfinite(v[1]) && std::isfinite(v[2]);
    }

    Node* at(EntityId id) {
        if (id == kInvalidEntity || id > nodes_.size()) return nullptr;
        return &nodes_[id - 1u];
    }
    const Node* at(EntityId id) const {
        if (id == kInvalidEntity || id > nodes_.size()) return nullptr;
        return &nodes_[id - 1u];
    }

    // Walks up from `start` (inclusive) looking for `target`. Bounded: an
    // over-long or broken chain counts as "found" so callers refuse the edit
    // instead of trusting it.
    bool contains_self(EntityId start, EntityId target) const {
        if (start == target) return true;
        const Node* node = at(start);
        if (node == nullptr) return true;
        u32 steps = 0;
        EntityId cursor = node->parent;
        while (cursor != kInvalidEntity) {
            if (cursor == target) return true;
            const Node* parent = at(cursor);
            if (parent == nullptr) return true;
            if (++steps > kMaxDepth) return true;
            cursor = parent->parent;
        }
        return false;
    }

    // Number of edges from `ancestor` down to `node`, or 0 when `node` is not
    // inside that subtree.
    u32 distance_below(EntityId ancestor, EntityId node) const {
        u32 distance = 0;
        const Node* cursor = at(node);
        if (cursor == nullptr) return 0;
        EntityId parent = cursor->parent;
        while (parent != kInvalidEntity && distance < kMaxDepth) {
            if (parent == ancestor) return distance + 1u;
            const Node* next = at(parent);
            if (next == nullptr) return 0;
            parent = next->parent;
            ++distance;
        }
        return 0;
    }

    // Parent-before-child order: stable counting sort of ids by chain depth.
    // Depth is derived from the parent chain (never stored), so the order is a
    // function of the hierarchy alone and cannot depend on insertion order.
    // Rebuild only runs when create()/set_parent() invalidated it; the vectors
    // reuse their capacity, so a steady-state frame does not allocate.
    void rebuild_order() {
        const std::size_t count = nodes_.size();
        order_.clear();
        if (order_.capacity() < count) order_.reserve(count);
        if (depths_.capacity() < count) depths_.reserve(count);
        depths_.resize(count);
        u32 buckets[kMaxDepth + 1] = {};
        u32 max_depth = 0;
        for (std::size_t i = 0; i < count; ++i) {
            const u32 depth = depth_of(EntityId(i + 1));
            // A chain past the cap cannot occur through the public API; if it
            // somehow does, treat the node as a root so it is still visited.
            depths_[i] = depth > kMaxDepth ? 0u : depth;
            ++buckets[depths_[i]];
            if (depths_[i] > max_depth) max_depth = depths_[i];
        }
        u32 offset[kMaxDepth + 1] = {};
        for (u32 depth = 1; depth <= max_depth; ++depth) {
            offset[depth] = offset[depth - 1u] + buckets[depth - 1u];
        }
        order_.resize(count);
        for (std::size_t i = 0; i < count; ++i) {
            order_[offset[depths_[i]]++] = EntityId(i + 1);
        }
        order_dirty_ = false;
    }

    std::vector<Node> nodes_;
    std::vector<EntityId> order_;
    std::vector<u32> depths_;   // scratch for rebuild_order, reused
    u32 pass_ = 0;              // monotonic update counter; 0 after clear()
    bool order_dirty_ = false;
};

} // namespace gf
