#pragma once
#include "engine/gameplay/force_specification.hpp"
namespace gf {
class ForceEditorSession;
class ForceSpecPresentation;
class ForceStatsUiPresentation;
enum class ForceSpecPresentationKind {ActorPreview,StatisticsUiOnly};
struct ForceSpecEditScope {};
// One editor-entry identity. A new request is issued even for the same stored
// instance/revision; a completed old presentation cannot close a later entry.
class ForceSpecRequest {
public:
    const ForceSpecificationSnapshot& snapshot()const{return snapshot_;}
    ForceSpecPresentationKind kind()const{return kind_;}
private:
    ForceSpecificationSnapshot snapshot_;
    ForceSpecPresentationKind kind_=ForceSpecPresentationKind::ActorPreview;
    friend class ForceEditorSession;
};
class ForceSpecCompletion {
private:
    std::weak_ptr<const ForceSpecRequest> request_;
    std::weak_ptr<const ForceSpecEditScope> edit_scope_;bool for_edit_=false,ui_only_=false;
    friend class ForceSpecPresentation;
    friend class ForceStatsUiPresentation;
    friend class ForceEditorSession;
};
class ForceSpecEditRequest {
private:
    std::weak_ptr<const ForceSpecRequest> request_;std::weak_ptr<const ForceSpecEditScope> scope_;
    friend class ForceEditorSession;friend class ForceSpecPresentation;friend class ForceStatsUiPresentation;
};
} // namespace gf
