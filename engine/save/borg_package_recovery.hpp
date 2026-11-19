#pragma once
#include "engine/save/borg_package_save.hpp"

namespace gf {
enum class BorgRecoveryPhase {Closed,List,Notice,ConfirmDisable,ConfirmDelete,NeedsReload};
enum class BorgRecoveryOpen {Ready,NoRoster,ScanUnavailable,SaveUnavailable,InvalidSave};
enum class BorgRecoveryApply {Saved,Unchanged,NotConfirmed,StaleNotice,SaveUnconfirmed};

// Presentation-facing transaction owner for an explicitly selected save. It
// never opens a different save, installs packages, or grants battle admission.
class BorgPackageRecoverySession {
public:
    explicit BorgPackageRecoverySession(ISaveBackend& backend):store_(backend){}
    BorgPackageRecoverySession(const BorgPackageRecoverySession&)=delete;
    BorgPackageRecoverySession& operator=(const BorgPackageRecoverySession&)=delete;
    BorgRecoveryOpen open(const BorgPackageCatalog& catalog,std::span<const BorgCatalogSnapshot> metadata={}){
        close();
        if(catalog.status!=BorgPackageScanStatus::Complete&&catalog.status!=BorgPackageScanStatus::NoDirectory)
            return BorgRecoveryOpen::ScanUnavailable;
        PackageBorgRoster roster;
        switch(store_.load(&roster)){
        case PackageBorgLoad::Missing:return BorgRecoveryOpen::NoRoster;
        case PackageBorgLoad::Unavailable:return BorgRecoveryOpen::SaveUnavailable;
        case PackageBorgLoad::Invalid:return BorgRecoveryOpen::InvalidSave;
        case PackageBorgLoad::Loaded:break;
        }
        std::vector<MissingPackageBorg> notices;
        if(!package_borg_missing(roster,catalog,&notices,metadata))return BorgRecoveryOpen::InvalidSave;
        roster_=std::move(roster);notices_=std::move(notices);phase_=BorgRecoveryPhase::List;
        return BorgRecoveryOpen::Ready;
    }
    void close(){phase_=BorgRecoveryPhase::Closed;roster_.reset();selected_.reset();notices_.clear();}
    BorgRecoveryPhase phase()const{return phase_;}
    const PackageBorgRoster* roster()const{return roster_?&*roster_:nullptr;}
    std::span<const MissingPackageBorg> notices()const{return notices_;}
    const MissingPackageBorg* selected()const{return selected_?&*selected_:nullptr;}
    bool inspect(u64 instance){
        if(phase_!=BorgRecoveryPhase::List)return false;
        for(const auto& notice:notices_)if(notice.saved.instance==instance){
            selected_=notice;phase_=BorgRecoveryPhase::Notice;return true;
        }
        return false;
    }
    bool choose(MissingPackageChoice choice){
        if(phase_!=BorgRecoveryPhase::Notice||!selected_)return false;
        const auto reason=selected_->reason;
        if(reason!=PackageBorgAvailability::Missing&&reason!=PackageBorgAvailability::Incomplete&&
           reason!=PackageBorgAvailability::MissingVariant)return false;
        if(choice==MissingPackageChoice::DisableInForces)phase_=BorgRecoveryPhase::ConfirmDisable;
        else if(choice==MissingPackageChoice::DeleteFromSave)phase_=BorgRecoveryPhase::ConfirmDelete;
        else return false;
        return true;
    }
    bool cancel(){
        if(phase_==BorgRecoveryPhase::ConfirmDisable||phase_==BorgRecoveryPhase::ConfirmDelete){
            phase_=BorgRecoveryPhase::Notice;return true;
        }
        if(phase_==BorgRecoveryPhase::Notice){
            selected_.reset();phase_=BorgRecoveryPhase::List;return true;
        }
        return false;
    }
    // Confirmation requires fresh catalogue evidence and the still-current
    // selected save record. Neither a stale notice nor an unconfirmed write
    // can publish a new roster, and an uncertain write is never retried here.
    BorgRecoveryApply confirm(const BorgPackageCatalog& catalog,std::span<const BorgCatalogSnapshot> metadata={}){
        if(!roster_||!selected_||(phase_!=BorgRecoveryPhase::ConfirmDisable&&phase_!=BorgRecoveryPhase::ConfirmDelete))
            return BorgRecoveryApply::NotConfirmed;
        const auto choice=phase_==BorgRecoveryPhase::ConfirmDisable?MissingPackageChoice::DisableInForces:MissingPackageChoice::DeleteFromSave;
        PackageBorgRoster next;
        std::vector<MissingPackageBorg> notices;
        if(!package_borg_resolve_missing(*roster_,catalog,*selected_,choice,&next,metadata)||
           !package_borg_missing(next,catalog,&notices,metadata)){
            phase_=BorgRecoveryPhase::NeedsReload;return BorgRecoveryApply::StaleNotice;
        }
        const bool changed=next!=*roster_;
        if(!(changed?store_.write(next):store_.current())){
            phase_=BorgRecoveryPhase::NeedsReload;return BorgRecoveryApply::SaveUnconfirmed;
        }
        roster_=std::move(next);notices_=std::move(notices);selected_.reset();phase_=BorgRecoveryPhase::List;
        return changed?BorgRecoveryApply::Saved:BorgRecoveryApply::Unchanged;
    }
private:
    PackageBorgSaveStore store_;
    BorgRecoveryPhase phase_=BorgRecoveryPhase::Closed;
    std::optional<PackageBorgRoster> roster_;
    std::vector<MissingPackageBorg> notices_;
    std::optional<MissingPackageBorg> selected_;
};
}
