#pragma once
#include <memory>
#include "engine/core/types.hpp"
namespace gf {
class BattleViews;class SceneObjectPool;struct ScenePhysicalControlFields;
namespace scene_object_pool_detail {struct Storage;}
namespace scene_view_reference_detail {
struct Scope {
    bool live=true,bound=false;std::weak_ptr<ScenePhysicalControlFields> arena;
    std::weak_ptr<scene_object_pool_detail::Storage> objects;
};
}
//Identity/lifetime of one fixed view record. It is not an active-camera,
//matrix-evaluation, projection or collider-registration acknowledgement.
class SceneViewReference {
public:
    bool live()const{const auto s=scope_.lock();return s&&s->live&&s->bound&&!s->arena.expired()&&!s->objects.expired()&&index_<13;}
    u8 index()const{return index_;}
    bool operator==(const SceneViewReference& other)const{return index_==other.index_&&!scope_.owner_before(other.scope_)&&!other.scope_.owner_before(scope_);}
private:friend class BattleViews;friend class SceneObjectPool;std::weak_ptr<const scene_view_reference_detail::Scope> scope_;u8 index_=255;
};
}
