// Scene world tests: flat index-based entities, bounded creation, dirty-only
// matrix updates with parent-before-child composition.
// No SDL, GPU, emulator or game data involved.
#include "engine/scene/world.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace {

int g_cases = 0;

void check(bool ok, const char* what) {
    if (!ok) {
        std::fprintf(stderr, "scene world failure: %s\n", what);
        std::abort();
    }
    ++g_cases;
}

gf::f32 nan_value() { return std::numeric_limits<gf::f32>::quiet_NaN(); }
gf::f32 inf_value() { return std::numeric_limits<gf::f32>::infinity(); }

// Translation-only transform helper; scale stays 1 and rotation stays 0.
gf::Transform at(gf::f32 x, gf::f32 y, gf::f32 z) {
    gf::Transform t;
    t.position[0] = x;
    t.position[1] = y;
    t.position[2] = z;
    return t;
}

bool near(gf::f32 a, gf::f32 b, gf::f32 tolerance = 1e-5f) {
    return std::fabs(a - b) <= tolerance;
}

bool matrices_near(const gf::f32* a, const gf::f32* b, gf::f32 tolerance = 1e-5f) {
    if (a == nullptr || b == nullptr) return false;
    for (unsigned i = 0; i < 12; ++i) {
        if (!near(a[i], b[i], tolerance)) return false;
    }
    return true;
}

bool matrices_identical(const gf::f32* a, const gf::f32* b) {
    if (a == nullptr || b == nullptr) return false;
    return std::memcmp(a, b, 12 * sizeof(gf::f32)) == 0;
}

// Compares two worlds node by node through a logical id mapping.
bool same_through(const gf::World& a, const gf::World& b,
                  const gf::EntityId* a_ids, const gf::EntityId* b_ids, std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        if (!matrices_near(a.world_matrix(a_ids[i]), b.world_matrix(b_ids[i]))) return false;
    }
    return true;
}

} // namespace

int main() {
    // ---- 1. creation, ids and parent validity ---------------------------
    {
        gf::World world;
        check(world.size() == 0, "new world is empty");
        check(world.create(7) == gf::kInvalidEntity, "create with an unknown parent is refused");
        const gf::EntityId root = world.create();
        check(root == 1, "ids start at 1");
        const gf::EntityId child = world.create(root);
        check(child == 2, "second id is 2");
        check(world.parent_of(child) == root, "child records its parent");
        check(world.parent_of(root) == gf::kInvalidEntity, "roots report no parent");
        check(!world.alive(gf::kInvalidEntity), "entity 0 is never alive");
        check(!world.alive(99), "unknown id is not alive");
        check(world.world_matrix(gf::kInvalidEntity) == nullptr, "no matrix for entity 0");
        check(world.world_matrix(99) == nullptr, "no matrix for an unknown id");
        check(world.depth_of(root) == 0 && world.depth_of(child) == 1, "depths follow the chain");
        check(world.world_matrix(child) != nullptr, "valid ids expose a matrix");
        check(world.visible(child), "entities start visible");
        check(world.set_visible(child, false) && !world.visible(child), "visibility flips");
    }

    // ---- 2. cycle rejection and reparenting -----------------------------
    {
        gf::World world;
        const gf::EntityId a = world.create();
        const gf::EntityId b = world.create(a);
        check(!world.set_parent(a, b), "reparenting under a descendant is refused");
        check(world.parent_of(a) == gf::kInvalidEntity, "refused reparent leaves the parent unchanged");
        check(world.set_parent(b, a), "idempotent reparent is accepted");
        check(!world.set_parent(a, a), "self parenting is refused");
        check(!world.set_parent(a, 1234), "unknown parent is refused");
        check(!world.set_parent(999, a), "unknown child is refused");
        check(world.set_parent(b, gf::kInvalidEntity), "a child can become a root");
        check(world.parent_of(b) == gf::kInvalidEntity, "re-rooting takes effect");
        check(world.depth_of(b) == 0, "re-rooted node depth is 0");
        // Put the pair back for a multi-node cycle attempt: c -> b -> a.
        check(world.set_parent(b, a), "reparent back to a");
        const gf::EntityId c = world.create(b);
        check(!world.set_parent(a, c), "deep descendant parent is refused");
        check(world.set_parent(c, gf::kInvalidEntity), "deep node can still be re-rooted");
    }

    // ---- 3. entity cap ---------------------------------------------------
    {
        gf::World world;
        bool grew = true;
        for (gf::u32 i = 0; i < gf::World::kMaxEntities; ++i) {
            if (world.create() == gf::kInvalidEntity) {
                grew = false;
                break;
            }
        }
        check(grew, "every entity up to the cap is created");
        check(world.size() == gf::World::kMaxEntities, "world holds exactly the cap");
        check(world.create() == gf::kInvalidEntity, "creation past the cap is refused");
        check(!world.alive(gf::World::kMaxEntities + 1u), "refused id is not alive");
    }

    // ---- 4. depth cap ----------------------------------------------------
    {
        gf::World world;
        gf::EntityId parent = world.create();
        bool chained = true;
        for (gf::u32 i = 1; i <= gf::World::kMaxDepth; ++i) {
            parent = world.create(parent);
            if (parent == gf::kInvalidEntity) {
                chained = false;
                break;
            }
        }
        check(chained, "a chain up to the depth cap is accepted");
        check(world.depth_of(parent) == gf::World::kMaxDepth, "deepest node reports the cap");
        check(world.create(parent) == gf::kInvalidEntity, "a chain past the depth cap is refused");
    }

    // ---- 5. transform validation ----------------------------------------
    {
        gf::World world;
        const gf::EntityId id = world.create();
        gf::Transform base;
        base.position[0] = 1.0f; base.position[1] = 2.0f; base.position[2] = 3.0f;
        base.rotation[0] = 0.5f;
        base.scale[2] = 2.0f;
        check(world.set_transform(id, base), "finite transform is accepted");
        check(!world.set_transform(999, base), "unknown id is refused");

        for (unsigned i = 0; i < 3; ++i) {
            gf::Transform t = base;
            t.position[i] = nan_value();
            check(!world.set_transform(id, t), "NaN position is refused");
            t = base;
            t.position[i] = inf_value();
            check(!world.set_transform(id, t), "infinite position is refused");
            t = base;
            t.rotation[i] = nan_value();
            check(!world.set_transform(id, t), "NaN rotation is refused");
            t = base;
            t.rotation[i] = inf_value();
            check(!world.set_transform(id, t), "infinite rotation is refused");
            t = base;
            t.scale[i] = nan_value();
            check(!world.set_transform(id, t), "NaN scale is refused");
            t = base;
            t.scale[i] = inf_value();
            check(!world.set_transform(id, t), "infinite scale is refused");
        }
        const gf::Transform* kept = world.transform(id);
        check(kept != nullptr && kept->position[0] == 1.0f && kept->position[1] == 2.0f &&
                  kept->position[2] == 3.0f,
              "refused transforms leave the stored value untouched");
        check(world.transform(999) == nullptr, "no transform for an unknown id");
    }

    // ---- 6. a root matrix is its own SRT --------------------------------
    {
        gf::World world;
        const gf::EntityId id = world.create();
        gf::Transform t;
        t.position[0] = 1.5f; t.position[1] = -2.0f; t.position[2] = 0.25f;
        t.rotation[0] = 0.3f; t.rotation[1] = -0.2f; t.rotation[2] = 0.7f;
        t.scale[0] = 2.0f; t.scale[1] = 3.0f; t.scale[2] = 4.0f;
        check(world.set_transform(id, t), "SRT transform accepted");
        world.update_matrices();
        gf::f32 expected[12] = {};
        check(gf_transform_srt(t.scale, t.rotation, t.position, nullptr, 0, expected) == 1,
              "reference SRT kernel accepts the transform");
        check(matrices_near(world.world_matrix(id), expected), "root matrix equals its SRT");
        check(world.world_matrix(id)[3] == expected[3] &&
                  world.world_matrix(id)[7] == expected[7] &&
                  world.world_matrix(id)[11] == expected[11],
              "root translation column matches");
    }

    // ---- 7. child composition, translation only, by hand ----------------
    {
        gf::World world;
        const gf::EntityId parent = world.create();
        const gf::EntityId child = world.create(parent);
        check(world.set_transform(parent, at(3.0f, 0.0f, 0.0f)), "parent translate set");
        check(world.set_transform(child, at(0.0f, 4.0f, 0.0f)), "child translate set");
        world.update_matrices();
        const gf::f32 expected_parent[12] = {1, 0, 0, 3, 0, 1, 0, 0, 0, 0, 1, 0};
        const gf::f32 expected_child[12] = {1, 0, 0, 3, 0, 1, 0, 4, 0, 0, 1, 0};
        check(matrices_near(world.world_matrix(parent), expected_parent),
              "root translation matrix is a hand-checked identity");
        check(matrices_near(world.world_matrix(child), expected_child),
              "child composes translation by hand (3,4,0)");
    }

    // ---- 8. rotation composition, by hand -------------------------------
    {
        gf::World world;
        const gf::EntityId parent = world.create();
        const gf::EntityId child = world.create(parent);
        gf::Transform rotation;
        rotation.rotation[2] = 1.5707963267948966f;  // +90 degrees about Z
        check(world.set_transform(parent, rotation), "parent rotation set");
        check(world.set_transform(child, at(1.0f, 0.0f, 0.0f)), "child offset set");
        world.update_matrices();
        const gf::f32* m = world.world_matrix(child);
        check(near(m[3], 0.0f) && near(m[7], 1.0f) && near(m[11], 0.0f),
              "child of a Z-rotation lands at (0,1,0)");
        check(near(m[0], 0.0f) && near(m[1], -1.0f) && near(m[4], 1.0f) && near(m[5], 0.0f),
              "child rotation block is the parent rotation");
    }

    // ---- 9. grandchild chain --------------------------------------------
    {
        gf::World world;
        const gf::EntityId a = world.create();
        const gf::EntityId b = world.create(a);
        const gf::EntityId c = world.create(b);
        check(world.set_transform(a, at(1.0f, 0.0f, 0.0f)), "chain root set");
        check(world.set_transform(b, at(0.0f, 2.0f, 0.0f)), "chain mid set");
        check(world.set_transform(c, at(0.0f, 0.0f, 3.0f)), "chain leaf set");
        world.update_matrices();
        const gf::f32* m = world.world_matrix(c);
        check(near(m[3], 1.0f) && near(m[7], 2.0f) && near(m[11], 3.0f),
              "grandchild sums all ancestor translations");
        check(near(m[0], 1.0f) && near(m[5], 1.0f) && near(m[10], 1.0f),
              "grandchild rotation block stays identity");
    }

    // ---- 10. dirty-only update equals a full recompute ------------------
    {
        gf::World live;
        const gf::EntityId root = live.create();
        const gf::EntityId left = live.create(root);
        const gf::EntityId leaf = live.create(left);
        const gf::EntityId right = live.create(root);
        // Initial state on both worlds.
        gf::Transform root_t = at(2.0f, 1.0f, 0.0f);
        root_t.rotation[2] = 0.4f;
        gf::Transform left_t = at(1.0f, 0.0f, 0.0f);
        gf::Transform leaf_t;
        leaf_t.scale[0] = 2.0f;
        leaf_t.scale[1] = 2.0f;
        gf::Transform right_t = at(0.0f, -3.0f, 1.0f);
        check(live.set_transform(root, root_t), "live root set");
        check(live.set_transform(left, left_t), "live left set");
        check(live.set_transform(leaf, leaf_t), "live leaf set");
        check(live.set_transform(right, right_t), "live right set");
        live.update_matrices();
        check(!live.is_dirty(root) && !live.is_dirty(leaf), "update clears dirty state");

        // Move a subset only; the untouched branch must stay correct.
        gf::Transform moved_left = at(1.0f, 2.5f, 0.0f);
        moved_left.rotation[1] = 0.25f;
        gf::Transform moved_right = at(4.0f, 0.0f, 0.0f);
        check(live.set_transform(left, moved_left), "live left moved");
        check(live.set_transform(right, moved_right), "live right moved");
        check(live.is_dirty(left) && live.is_dirty(leaf), "moving a parent dirties descendants");
        live.update_matrices();

        gf::World fresh;
        const gf::EntityId froot = fresh.create();
        const gf::EntityId fleft = fresh.create(froot);
        const gf::EntityId fleaf = fresh.create(fleft);
        const gf::EntityId fright = fresh.create(froot);
        check(fresh.set_transform(froot, root_t), "fresh root set");
        check(fresh.set_transform(fleft, moved_left), "fresh left set");
        check(fresh.set_transform(fleaf, leaf_t), "fresh leaf set");
        check(fresh.set_transform(fright, moved_right), "fresh right set");
        fresh.update_matrices();

        const gf::EntityId live_ids[4] = {root, left, leaf, right};
        const gf::EntityId fresh_ids[4] = {froot, fleft, fleaf, fright};
        check(same_through(live, fresh, live_ids, fresh_ids, 4),
              "dirty-only update equals a full recompute");
    }

    // ---- 11. moving a parent updates descendants ------------------------
    {
        gf::World world;
        const gf::EntityId root = world.create();
        const gf::EntityId mid = world.create(root);
        const gf::EntityId leaf = world.create(mid);
        check(world.set_transform(root, at(1.0f, 0.0f, 0.0f)), "root placed");
        check(world.set_transform(mid, at(0.0f, 1.0f, 0.0f)), "mid placed");
        check(world.set_transform(leaf, at(0.0f, 0.0f, 1.0f)), "leaf placed");
        world.update_matrices();
        check(near(world.world_matrix(leaf)[3], 1.0f) &&
                  near(world.world_matrix(leaf)[7], 1.0f) &&
                  near(world.world_matrix(leaf)[11], 1.0f),
              "leaf starts at (1,1,1)");
        check(world.set_transform(root, at(10.0f, 0.0f, 0.0f)), "root moved");
        check(world.is_dirty(mid) && world.is_dirty(leaf), "descendants report dirty");
        world.update_matrices();
        check(near(world.world_matrix(leaf)[3], 10.0f) &&
                  near(world.world_matrix(leaf)[7], 1.0f) &&
                  near(world.world_matrix(leaf)[11], 1.0f),
              "moving the root moved the whole chain");
    }

    // ---- 12. determinism across insertion order ------------------------
    {
        // Forward: parents created before children.
        gf::World forward;
        const gf::EntityId f_root = forward.create();
        const gf::EntityId f_mid = forward.create(f_root);
        const gf::EntityId f_leaf = forward.create(f_mid);
        const gf::EntityId f_side = forward.create(f_root);

        // Backward: children first, then reparented into the same shape.
        gf::World backward;
        const gf::EntityId b_leaf = backward.create();
        const gf::EntityId b_mid = backward.create();
        const gf::EntityId b_root = backward.create();
        const gf::EntityId b_side = backward.create(b_root);
        check(backward.set_parent(b_mid, b_root), "backward mid reparented");
        check(backward.set_parent(b_leaf, b_mid), "backward leaf reparented");

        gf::Transform mid_t;
        mid_t.rotation[2] = 0.6f;
        mid_t.position[0] = 1.5f;
        mid_t.scale[2] = 0.5f;
        for (gf::World* w : {&forward, &backward}) {
            const gf::EntityId root = w == &forward ? f_root : b_root;
            const gf::EntityId mid = w == &forward ? f_mid : b_mid;
            const gf::EntityId leaf = w == &forward ? f_leaf : b_leaf;
            const gf::EntityId side = w == &forward ? f_side : b_side;
            check(w->set_transform(root, at(2.0f, 0.0f, -1.0f)), "det root set");
            check(w->set_transform(mid, mid_t), "det mid set");
            check(w->set_transform(leaf, at(0.0f, 0.0f, 3.0f)), "det leaf set");
            check(w->set_transform(side, at(0.0f, 4.0f, 0.0f)), "det side set");
            w->update_matrices();
        }
        const gf::EntityId forward_ids[4] = {f_root, f_mid, f_leaf, f_side};
        const gf::EntityId backward_ids[4] = {b_root, b_mid, b_leaf, b_side};
        check(same_through(forward, backward, forward_ids, backward_ids, 4),
              "insertion order does not change world matrices");
    }

    // ---- 13. update with nothing dirty is a bit-identical no-op --------
    {
        gf::World world;
        const gf::EntityId root = world.create();
        const gf::EntityId child = world.create(root);
        gf::Transform t = at(1.0f, 2.0f, 3.0f);
        t.rotation[0] = 0.125f;
        t.scale[1] = 0.75f;
        check(world.set_transform(root, t), "no-op root set");
        check(world.set_transform(child, at(-1.0f, 0.5f, 2.0f)), "no-op child set");
        world.update_matrices();
        gf::f32 root_before[12];
        gf::f32 child_before[12];
        std::memcpy(root_before, world.world_matrix(root), sizeof(root_before));
        std::memcpy(child_before, world.world_matrix(child), sizeof(child_before));
        check(!world.is_dirty(root) && !world.is_dirty(child), "nothing is dirty after an update");
        world.update_matrices();
        world.update_matrices();
        check(matrices_identical(world.world_matrix(root), root_before),
              "root is unchanged by idle updates");
        check(matrices_identical(world.world_matrix(child), child_before),
              "child is unchanged by idle updates");
        check(world.set_visible(child, false), "visibility change does not need a matrix rebuild");
        world.update_matrices();
        check(matrices_identical(world.world_matrix(child), child_before),
              "visibility change leaves the matrix untouched");
    }

    std::printf("{\"cases\":%d,\"max_entities\":%u,\"all_passed\":true}\n", g_cases,
                gf::World::kMaxEntities);
    return 0;
}
