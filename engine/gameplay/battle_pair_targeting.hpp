#pragma once
// Recovered 8002DB58: the pair loop selects either every valid pair (direct)
// or the single nearest valid target, then triggers the reaction.
//
// This seam owns the recovered selection order, the sphere gate and the
// direct/nearest switch. The retail relation filter (8002FD7C) and category
// dispatch (80030490) stay with the caller as `BattlePairFilter`, so engine
// code does not grow branches on retail model ids.
#include <array>
#include <cmath>
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include "engine/core/types.hpp"

namespace gf {

struct BattlePairFields {
    bool declared = false;
    bool exists = false;
    bool active = false;
    std::array<f32, 3> position{};
    f32 radius = 0;
};

struct BattlePairProfile {
    bool loaded = false;
    f32 minimum_radius = 0.1f;            // 80039F6C: both radii must exceed
    f32 initial_best_squared = 400000000.0f; // 8002DB58 sentinel
};

// A true result means the pair passes the caller's relation/dispatch tests.
using BattlePairFilter = std::function<bool(const BattlePairFields&,
                                            const BattlePairFields&)>;

struct BattlePairSelection {
    bool direct = false;
    bool found = false;
    std::vector<u8> slots;
    f32 best_squared = 0;
    bool operator==(const BattlePairSelection&) const = default;
};

inline bool battle_pair_profile_valid(const BattlePairProfile& p) {
    return p.loaded && std::isfinite(p.minimum_radius) && p.minimum_radius >= 0 &&
           std::isfinite(p.initial_best_squared) && p.initial_best_squared >= 0;
}

namespace battle_pair_detail {
inline bool finite_vec(const std::array<f32, 3>& v) {
    return std::isfinite(v[0]) && std::isfinite(v[1]) && std::isfinite(v[2]);
}
inline f32 squared_distance(const std::array<f32, 3>& a,
                            const std::array<f32, 3>& b) {
    const f32 dx = a[0] - b[0];
    const f32 dy = a[1] - b[1];
    const f32 dz = a[2] - b[2];
    return dx * dx + dy * dy + dz * dz;
}
} // namespace battle_pair_detail

// 80039F6C: both radii above 0.1 and centre distance within the radius sum.
inline bool battle_pair_sphere_overlap(const BattlePairFields& a,
                                       const BattlePairFields& b,
                                       const BattlePairProfile& p,
                                       bool* out, std::string* error) {
    if (out) *out = false;
    auto fail = [&]() {
        if (error) *error = "battle pair sphere: invalid profile/fields";
        return false;
    };
    if (!out || !battle_pair_profile_valid(p) || !a.declared || !b.declared)
        return fail();
    if (!std::isfinite(a.radius) || !std::isfinite(b.radius) ||
        !battle_pair_detail::finite_vec(a.position) ||
        !battle_pair_detail::finite_vec(b.position))
        return fail();
    if (a.radius <= p.minimum_radius || b.radius <= p.minimum_radius) {
        if (error) error->clear();
        return true;
    }
    const f32 sum = a.radius + b.radius;
    const f32 squared = battle_pair_detail::squared_distance(a.position,
                                                             b.position);
    if (!std::isfinite(sum) || !std::isfinite(squared)) return fail();
    *out = squared <= sum * sum;
    if (error) error->clear();
    return true;
}

// 8002DB58 selection. `direct` is the caller's argument: true for the
// actor+88 == 2 / record+16 bit 0 path, false for the nearest-target path.
// Ties keep the LAST candidate, matching the source `distance <= best` test.
inline bool battle_pair_select(const std::vector<BattlePairFields>& pool,
                               u8 owner_slot, bool direct,
                               const BattlePairProfile& p,
                               const BattlePairFilter& filter,
                               BattlePairSelection* out, std::string* error) {
    if (out) *out = {};
    auto fail = [&](const char* why) {
        if (error) *error = std::string("battle pair select: ") + why;
        return false;
    };
    if (!out || !battle_pair_profile_valid(p) || !filter)
        return fail("profile/filter required");
    if (pool.size() > 256u) return fail("pool exceeds one-byte slots");
    if (owner_slot >= pool.size()) return fail("owner slot outside pool");
    const auto& owner = pool[owner_slot];
    if (!owner.declared || !owner.exists || !owner.active)
        return fail("owner declaration/existence/activity required");
    if (!std::isfinite(owner.radius) ||
        !battle_pair_detail::finite_vec(owner.position))
        return fail("owner geometry");

    BattlePairSelection next;
    next.direct = direct;
    next.best_squared = p.initial_best_squared;
    for (unsigned i = 0; i < pool.size(); ++i) {
        if (i == owner_slot) continue;
        const auto& peer = pool[i];
        if (!peer.declared) return fail("candidate declaration required");
        if (!peer.exists || !peer.active) continue;
        if (!std::isfinite(peer.radius) ||
            !battle_pair_detail::finite_vec(peer.position))
            return fail("candidate geometry");
        if (!filter(owner, peer)) continue;
        bool overlap = false;
        if (!battle_pair_sphere_overlap(owner, peer, p, &overlap, error))
            return false;
        if (!overlap) continue;
        if (direct) {
            next.slots.push_back(u8(i));
            next.found = true;
            continue;
        }
        const f32 squared = battle_pair_detail::squared_distance(owner.position,
                                                                 peer.position);
        if (!std::isfinite(squared)) return fail("distance domain");
        if (squared <= next.best_squared) {
            next.best_squared = squared;
            next.slots.assign(1, u8(i));
            next.found = true;
        }
    }
    *out = std::move(next);
    if (error) error->clear();
    return true;
}

} // namespace gf
