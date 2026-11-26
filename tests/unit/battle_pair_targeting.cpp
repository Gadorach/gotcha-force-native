// Recovered 8002DB58 pair targeting: direct-vs-nearest, sphere gate, order.
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include <vector>
#include "engine/gameplay/battle_pair_targeting.hpp"

namespace {
using namespace gf;

void check(bool ok, const char* name) {
    if (!ok) {
        std::fprintf(stderr, "Battle pair targeting: %s\n", name);
        std::abort();
    }
}

BattlePairFields actor(bool active, f32 x = 0, f32 radius = 1) {
    BattlePairFields f;
    f.declared = true;
    f.exists = true;
    f.active = active;
    f.position = {x, 0, 0};
    f.radius = radius;
    return f;
}

bool accept_all(const BattlePairFields&, const BattlePairFields&) {
    return true;
}

BattlePairFields related(u32 key) {
    auto f = actor(true, 0, 1);
    f.actor_key = key;
    f.owner_key = key;
    f.team = 0;
    f.kind = 1;
    f.attack_key = 7;
    return f;
}
} // namespace

int main() {
    BattlePairProfile profile;
    profile.loaded = true;
    bool overlap = false;
    std::string error;

    check(!battle_pair_profile_valid({}),
          "unloaded profile is refused");
    check(battle_pair_profile_valid(profile), "loaded profile is valid");

    {
        auto a = actor(true, 0, 1);
        auto b = actor(true, 1.5f, 1);
        check(battle_pair_sphere_overlap(a, b, profile, &overlap, &error) &&
                  overlap,
              "distance 1.5 within radius sum 2");
        b.position[0] = 2.5f;
        check(battle_pair_sphere_overlap(a, b, profile, &overlap, &error) &&
                  !overlap,
              "distance 2.5 outside radius sum 2");
        b.position[0] = 1.5f;
        b.radius = 0.1f;
        check(battle_pair_sphere_overlap(a, b, profile, &overlap, &error) &&
                  !overlap,
              "radius at the 0.1 epsilon is rejected");
    }

    {
        std::vector<BattlePairFields> pool{
            actor(true, 0, 10), actor(true, 1, 10), actor(true, 1, 10),
            actor(false, 1, 10)};
        BattlePairSelection result;
        check(battle_pair_select(pool, 0, true, profile, accept_all, &result,
                                 &error),
              "direct selection succeeds");
        check(result.found && result.slots == std::vector<u8>({1, 2}),
              "direct collects every active valid pair");
        check(battle_pair_select(pool, 0, false, profile, accept_all, &result,
                                 &error),
              "nearest selection succeeds");
        check(result.found && result.slots == std::vector<u8>({2}),
              "nearest keeps the last equal-distance candidate");
    }

    {
        std::vector<BattlePairFields> pool{
            actor(true, 0, 10), actor(true, 1, 10), actor(true, 3, 10)};
        auto reject_first = [](const BattlePairFields& owner,
                               const BattlePairFields& peer) {
            return peer.position[0] != owner.position[0] + 1;
        };
        BattlePairSelection result;
        check(battle_pair_select(pool, 0, false, profile, reject_first, &result,
                                 &error),
              "filtered nearest succeeds");
        check(result.found && result.slots == std::vector<u8>({2}),
              "filter removes the closer candidate");

        auto reject_all = [](const BattlePairFields&, const BattlePairFields&) {
            return false;
        };
        check(battle_pair_select(pool, 0, true, profile, reject_all, &result,
                                 &error) &&
                  !result.found && result.slots.empty(),
              "no accepted pair means no target");
    }

    {
        std::vector<BattlePairFields> pool{actor(true, 0, 10),
                                           actor(true, 100, 10)};
        BattlePairSelection result;
        check(battle_pair_select(pool, 0, false, profile, accept_all, &result,
                                 &error) &&
                  !result.found,
              "out-of-range pair is not selected");
        result.best_squared = 0;
        check(battle_pair_select(pool, 0, false, profile, accept_all, &result,
                                 &error) &&
                  !result.found &&
                  result.best_squared == profile.initial_best_squared,
              "sentinel is preserved when nothing is selected");
    }

    {
        std::vector<BattlePairFields> pool{actor(true, 0, 10), actor(true, 1, 10)};
        pool[1].declared = false;
        BattlePairSelection result;
        check(!battle_pair_select(pool, 0, true, profile, accept_all, &result,
                                  &error),
              "undeclared candidate refuses");
        pool[1].declared = true;
        pool[1].position[0] = std::numeric_limits<f32>::quiet_NaN();
        check(!battle_pair_select(pool, 0, true, profile, accept_all, &result,
                                  &error),
              "nonfinite candidate refuses");
        pool[1].position[0] = 1;
        pool[0].active = false;
        check(!battle_pair_select(pool, 0, true, profile, accept_all, &result,
                                  &error),
              "inactive owner refuses");
        pool[0].active = true;
        check(!battle_pair_select(pool, 9, true, profile, accept_all, &result,
                                  &error),
              "owner outside the pool refuses");
    }

    {
        bool ok = true;
        auto a = related(1);
        auto b = related(2);
        check(battle_pair_relation_ok(a, b, 2, &ok, &error) && ok,
              "distinct actors and owners pass");

        b.actor_key = 1;
        check(battle_pair_relation_ok(a, b, 2, &ok, &error) && !ok,
              "same actor is rejected");
        b.actor_key = 2;

        b.owner_key = 1;
        check(battle_pair_relation_ok(a, b, 2, &ok, &error) && !ok,
              "same owner with clear flags is rejected");
        a.record_flags10 = 0x40;
        b.record_flags10 = 0x40;
        check(battle_pair_relation_ok(a, b, 2, &ok, &error) && ok,
              "same owner with both flags set passes");
        a.record_flags10 = 0;
        b.record_flags10 = 0;
        b.owner_key = 2;

        a.record_flags10 = 0x80;
        check(battle_pair_relation_ok(a, b, 2, &ok, &error) && !ok,
              "same team with flag 0x80 is rejected");
        a.record_flags10 = 0;

        b.record_flags12 = 0x40;
        check(battle_pair_relation_ok(a, b, 2, &ok, &error) && !ok,
              "target attack class rejects a non-0x10 kind");
        a.kind = 0x10;
        check(battle_pair_relation_ok(a, b, 2, &ok, &error) && ok,
              "kind 0x10 bypasses the target class");
        a.kind = 1;
        b.record_flags12 = 0;

        a.record_flags12 = 0x400;
        check(battle_pair_relation_ok(a, b, 2, &ok, &error) && !ok,
              "source class rejects a normal target kind");
        b.kind = 0x20;
        check(battle_pair_relation_ok(a, b, 2, &ok, &error) && ok,
              "kind 0x20 bypasses the source class");
        b.kind = 1;
        a.record_flags12 = 0;

        b.immunity[10] = 7;
        check(battle_pair_relation_ok(a, b, 2, &ok, &error) && !ok,
              "target immunity key is rejected");
        b.immunity[10] = 0;

        b.owner_flags2 = 0x40;
        b.owner_immunity[3] = 7;
        check(battle_pair_relation_ok(a, b, 2, &ok, &error) && !ok,
              "owner immunity key is rejected");
        b.owner_flags2 = 0;
        b.owner_immunity[3] = 0;

        b.target_mask = 2;
        check(battle_pair_relation_ok(a, b, 2, &ok, &error) && !ok,
              "target mask bit is rejected");
        check(battle_pair_relation_ok(a, b, 4, &ok, &error) && ok,
              "unrelated mask bit passes");

        a.kind = 0;
        b.kind = 0;
        b.actor_6fc = true;
        check(battle_pair_relation_ok(a, b, 2, &ok, &error) && !ok,
              "same-team zero-kind 6FC rule is rejected");
        b.actor_6fc = false;

        b.declared = false;
        check(!battle_pair_relation_ok(a, b, 2, &ok, &error),
              "undeclared relation fields refuse");
    }

    std::puts("Battle pair targeting: direct/nearest, sphere, relation and refusals PASS");
    return 0;
}
