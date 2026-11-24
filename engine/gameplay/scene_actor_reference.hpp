#pragma once
#include <memory>
namespace gf {
class BattleActorConstruction;struct ScenePhysicalControlFields;
namespace scene_actor_reference_detail {
struct Scope {
    mutable bool live=true;mutable const BattleActorConstruction* owner=nullptr;
    //Queued scene actors also borrow the fixed physical arena's lifetime.
    //Holding a C++ construction object cannot keep a retired arena alive.
    mutable std::weak_ptr<ScenePhysicalControlFields> arena;mutable bool arena_bound=false;
    bool arena_live()const{return !arena_bound||!arena.expired();}
};
}
//Lifetime/alias identity for an owned construction body. It is NOT proof that
//the parent has completed construction, a pose, an update or a world frame.
class SceneActorReference {
public:
    bool live()const{const auto scope=scope_.lock();return scope&&scope->live&&scope->owner&&scope->arena_live();}
    bool operator==(const SceneActorReference& other)const{return !scope_.owner_before(other.scope_)&&!other.scope_.owner_before(scope_);}
private:
    friend class BattleActorConstruction;
    friend class BattlePreparationBootstrap;
    std::weak_ptr<const scene_actor_reference_detail::Scope> scope_;
};
enum class SceneActorFieldSlot {Flag168,Flag169,Flag16a,Point150,Point15c};
struct SceneActorFieldReference {
    SceneActorReference owner;SceneActorFieldSlot slot=SceneActorFieldSlot::Flag168;
    bool live()const{return owner.live();}
    bool operator==(const SceneActorFieldReference&)const=default;
};
} // namespace gf
