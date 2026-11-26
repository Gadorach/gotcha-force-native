#pragma once

#include "engine/gameplay/portable_battle_duel.hpp"
#include "platform/pc/borg_force_browse.hpp"
#include "platform/pc/borg_gameplay_load.hpp"
#include "platform/pc/stage_package_scan.hpp"

namespace gf::pc {

// The owning mode may impose a budget; GFCHALLENGE 1 declares none. This
// policy is not a manifest value or a caller-provided replacement roster.
struct PortableForcePolicy {
    std::optional<u32> budget;
};

struct PortableBattleHostReport {
    PortableDuelEnd end=PortableDuelEnd::Cancelled;
    u32 ticks=0;
};

inline PortableBattleCheck portable_single_saved_force(const MixedBorgForceView& force,u64 revision,
                                                        PortableForcePolicy policy,PortableBattleForceSnapshot* out){
    using Status=PortableBattleStatus;
    if(!out||!revision)return {Status::Unavailable,"selected Force has no current save revision"};
    if(force.overlaps)return {Status::Denied,"selected Force has conflicting positions"};
    if(force.slots.empty())return {Status::Denied,"selected Force is empty"};
    if(force.slots.size()!=1)return {Status::Unsupported,"duel adapter cannot run the full selected Force"};
    const auto& slot=force.slots.front();
    if(slot.overlap())return {Status::Denied,"selected Force has conflicting positions"};
    if(slot.retail)return {Status::Unsupported,"retail Force members are not admitted to the portable duel"};
    if(!slot.packaged)return {Status::Malformed,"selected Force position has no member"};
    const auto& packaged=*slot.packaged;
    if(packaged.saved.disabled)return {Status::Denied,"selected Force contains a disabled copy"};
    if(packaged.status==PackageForceSlotStatus::Conflict)
        return {Status::Denied,"selected Force Borg package has a duplicate ID"};
    if(!packaged.selectable_metadata()||!packaged.saved.variant||!packaged.declared_cost)
        return {Status::Missing,"selected Force copy has no validated exact catalogue variant"};
    if(policy.budget&&*packaged.declared_cost>*policy.budget)
        return {Status::Denied,"selected Force exceeds the mode budget"};
    *out={revision,{{packaged.saved.identity.id,packaged.saved.variant->id}}};
    return {Status::Ready,{}};
}

// Shared host handoff for one accepted request. It binds content using the
// existing view and loaders, but never starts a world or produces a result.
// Callers must use the same view in the actual world and keep this object
// alive through the run. Its cached bytes are never reread on acceptance.
class PortableBattleHost {
public:
    PortableBattleHost(const BorgPackageCatalog& borgs,BorgForceBrowseSnapshot& selection,
                       NativeBattleMvpView& view,RenderEngine& render,PortableForcePolicy policy={},
                       std::filesystem::path borg_root="borgs",std::filesystem::path stage_root="stages"):
        borgs_(borgs),selection_(selection),view_(view),render_(render),policy_(policy),
        borg_root_(std::move(borg_root)),stage_root_(std::move(stage_root)){}
    PortableBattleHost(const PortableBattleHost&)=delete;
    PortableBattleHost& operator=(const PortableBattleHost&)=delete;

    // Works with PortableChallengeRun and PortableStoryRun. The receipt is
    // acquired directly from that run using this exact set of pinned assets.
    template<class Run> PortableBattleCheck admit(Run& run){
        if(attempted_)return {Status::Busy,"this host handoff was already used"};
        attempted_=true;
        const auto checks=resolver();
        if(const auto check=run.prepare(checks);!check)return check;
        if(const auto check=run.accept(checks,&receipt_);!check)return check;
        if(!receipt_.accepted()||!request_||receipt_.accepted()->request!=*request_)
            return {Status::Unavailable,"run did not return the prepared battle receipt"};
        if(!selection_.current()){
            run.cancel(receipt_);
            return {Status::Stale,"selected save changed before the handoff"};
        }
        active_=true;
        return {Status::Ready,{}};
    }

    const BattleMvpConfig& config()const{return config_;}
    const std::array<BorgBattlePackageFiles,2>& packages()const{return packages_;}
    NativeBattleMvpView& view(){return view_;}
    const PortableBattlePrepared* accepted()const{return active_?receipt_.accepted():nullptr;}

    // Only called by the host after execution (or to cancel a failed start).
    template<class Run> bool finish(Run& run,const PortableBattleHostReport& report){
        if(!active_)return false;
        const auto end=report.end;
        const auto outcome=portable_duel_campaign_outcome(end);
        if(!outcome&&end!=PortableDuelEnd::Draw&&end!=PortableDuelEnd::Timeout&&end!=PortableDuelEnd::Cancelled)
            return false;
        if(end!=PortableDuelEnd::Cancelled&&!report.ticks)return false;
        const auto limit=receipt_.accepted()->request.tick_limit;
        if((limit&&report.ticks>limit)||(end==PortableDuelEnd::Timeout&&(!limit||report.ticks!=limit)))return false;
        const bool accepted=outcome?run.complete(receipt_,*outcome):run.cancel(receipt_);
        if(accepted)active_=false;
        return accepted;
    }

private:
    using Status=PortableBattleStatus;
    PortableBattleResolver resolver(){
        return {
            [this](const auto& stage,auto player,auto enemy){return bind_stage(stage,player.front(),enemy.front());},
            [this](const auto& borg,PortableBattleSide side){return bind_borg(borg,side);},
            [this](PortableBattleSavedForce force,PortableBattleUse use,PortableBattleForceSnapshot* out){
                if(use!=PortableBattleUse::Campaign)return PortableBattleCheck{Status::Denied,"only campaign requests use this handoff"};
                MixedBorgForceView projection;
                if(!selection_.project(borgs_,force.index,&projection))
                    return PortableBattleCheck{Status::Stale,"could not confirm the current selected Force"};
                return portable_single_saved_force(projection,selection_.save().revision,policy_,out);
            },
            [this](const auto& request){return bind_capability(request);}
        };
    }
    PortableBattleCheck bind_stage(const PortableBattleStage& stage,const PortableBattleSpawn& player,const PortableBattleSpawn& enemy){
        const std::array markers{player,enemy};
        if(stage_){
            if(stage_->manifest.id!=stage.id||markers!=markers_)return {Status::Stale,"pinned stage selection changed"};
            return {Status::Ready,{}};
        }
        const auto catalog=scan_stage_packages(stage_root_);
        if(catalog.status==StagePackageScanStatus::NoDirectory)return {Status::Missing,"stages directory is missing"};
        if(catalog.status!=StagePackageScanStatus::Complete)return {Status::Unavailable,"stage discovery unavailable"};
        const auto* chosen=catalog.find(stage.id);
        if(!chosen){
            for(const auto& issue:catalog.issues)
                if(issue.manifest.id==stage.id)return {Status::Malformed,"stage "+stage.id+": "+issue.reason};
            return {Status::Missing,"stage ID is missing: "+stage.id};
        }
        for(const auto& marker:markers)if(!chosen->manifest.spawn(marker.id))
            return {Status::Missing,"stage spawn marker is missing: "+marker.id};
        if(chosen->issue!=StagePackageIssue::None)return {
            chosen->issue==StagePackageIssue::Missing?Status::Missing:Status::Malformed,chosen->reason};
        std::shared_ptr<const StagePackageSnapshot> next;
        std::string error;
        if(!pin_stage_package(stage_root_,catalog,stage.id,&next,&error))return {Status::Unavailable,std::move(error)};
        if(!view_.bind_stage(*next,render_,&error)||
           !view_.configure_stage_duel(stage.id,{player.id,enemy.id},&config_,&error))
            return {Status::Unsupported,std::move(error)};
        stage_=std::move(next);
        markers_=markers;
        return {Status::Ready,{}};
    }

    PortableBattleCheck bind_borg(const PortableBattleBorg& borg,PortableBattleSide side){
        const auto i=side==PortableBattleSide::Player?0u:1u;
        if(borg_keys_[i]){
            if(*borg_keys_[i]!=borg)return {Status::Stale,"pinned exact Borg selection changed"};
            return {Status::Ready,{}};
        }
        if(!portable_battle_borg_valid(borg))return {Status::Malformed,"expected exact Borg package and variant IDs"};
        if(borgs_.status!=BorgPackageScanStatus::Complete&&borgs_.status!=BorgPackageScanStatus::NoDirectory)
            return {Status::Unavailable,"Borg package discovery unavailable"};
        const auto* package=borgs_.find(borg.package_id);
        if(!package){
            for(const auto& issue:borgs_.issues)if(issue.id==borg.package_id&&issue.kind==BorgPackageIssueKind::DuplicateId)
                return {Status::Malformed,"duplicate Borg package ID: "+borg.package_id};
            return {Status::Missing,"Borg package is missing: "+borg.package_id};
        }
        std::string error;
        BorgCatalogSnapshot catalogue;
        const auto label=borg.package_id+"/"+borg.variant_id+": ";
        if(!load_borg_catalog_snapshot(borg_root_,borgs_,borg.package_id,&catalogue,&error))
            return {Status::Unavailable,label+error};
        if(!catalogue.profile.find(borg.variant_id))return {Status::Missing,label+"exact Borg variant is missing"};
        if(package->manifest.gameplay.empty()||catalogue.profile.default_variant!=borg.variant_id)
            return {Status::Unsupported,label+"exact variant has no executable base-variant gameplay admission"};
        BorgBattlePackageFiles next;
        if(!load_borg_battle_package(borg_root_,borgs_,borg.package_id,borg.variant_id,&next,&error))
            return {Status::Unsupported,label+error};
        if(next.identity.id!=borg.package_id||!next.definition||next.definition->variant!=borg.variant_id)
            return {Status::Unavailable,label+"loader did not return the exact Borg definition"};
        if(next.definition->movement==BorgBattleMovement::NormalGRed)
            return {Status::Unsupported,label+"normal G-Red requires its qualified legacy stage"};
        config_.packages[i]=next.definition;
        packages_[i]=std::move(next);
        borg_keys_[i]=borg;
        return {Status::Ready,{}};
    }

    PortableBattleCheck bind_capability(const PortableBattleRequest& request){
        if(request_){
            if(*request_!=request)return {Status::Stale,"pinned battle request changed"};
            return {Status::Ready,{}};
        }
        if(request.use!=PortableBattleUse::Campaign||!std::holds_alternative<PortableBattleSavedForce>(request.player))
            return {Status::Denied,"portable host requires a campaign request with a selected saved Force"};
        std::string error;
        const std::array visuals{packages_[0].visuals(),packages_[1].visuals()};
        const std::array duel_packages{config_.packages[0],config_.packages[1]};
        if(!view_.load_packages(render_,duel_packages,visuals,&error))return {Status::Unsupported,std::move(error)};
        request_=request;
        return {Status::Ready,{}};
    }

    const BorgPackageCatalog& borgs_;
    BorgForceBrowseSnapshot& selection_;
    NativeBattleMvpView& view_;
    RenderEngine& render_;
    const PortableForcePolicy policy_;
    const std::filesystem::path borg_root_,stage_root_;
    std::shared_ptr<const StagePackageSnapshot> stage_;
    std::array<PortableBattleSpawn,2> markers_;
    std::array<std::optional<PortableBattleBorg>,2> borg_keys_;
    BattleMvpConfig config_;
    std::array<BorgBattlePackageFiles,2> packages_;
    std::optional<PortableBattleRequest> request_;
    PortableBattleReceipt receipt_;
    bool attempted_=false,active_=false;
};

} // namespace gf::pc
