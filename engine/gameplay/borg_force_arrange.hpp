#pragma once
#include "engine/gameplay/borg_placement_confirmation.hpp"
#include "engine/save/borg_package_placement.hpp"

namespace gf {
enum class BorgForceArrangeOpen {Ready,InvalidPosition,SaveUnavailable,RetailUnverified,Stale};
enum class BorgForceArrangeState {Closed,Browse,Confirm,ReopenRequired};

// Controller for the selected-save Arrange page. The host owns the backend
// for the lifetime of this object and renders with its own adaptive layout.
// Positions and owned selections are stable IDs, not pointers or hit indices.
class BorgForceArrange {
public:
    explicit BorgForceArrange(AccountSaveBackend& backend):backend_(backend),store_(backend),placement_(backend){}
    BorgForceArrange(const BorgForceArrange&)=delete;
    BorgForceArrange& operator=(const BorgForceArrange&)=delete;

    BorgForceArrangeOpen open(unsigned force,unsigned slot,const ForceCatalog* retail_catalogue,
                              BorgPackageCatalog packages,std::vector<BorgCatalogSnapshot> metadata,
                              const BorgPlacementPolicy& policy={},std::span<const BorgLibraryCatalogIssue> issues={}){
        close();
        using Result=BorgForceArrangeOpen;
        if(force>=kForceCount||slot>=kForceSlots)return Result::InvalidPosition;
        const auto revision=backend_.revision();
        if(!revision)return Result::SaveUnavailable;
        PackageBorgRoster roster;
        const auto loaded=store_.load(&roster);
        if(loaded!=PackageBorgLoad::Loaded&&loaded!=PackageBorgLoad::Missing)return Result::SaveUnavailable;
        ForceInventory retail;std::vector<u8> record;
        if(!read_scoped_retail_force_record(backend_,retail_catalogue,&record,&retail))return Result::RetailUnverified;
        BorgLibrary library(packages,metadata,issues);
        auto owned=library.project_owned(roster);
        MixedBorgForceView view;
        if(!owned||!project(roster,packages,metadata,retail,force,&view))return Result::SaveUnavailable;
        if(!store_.current()||backend_.revision()!=revision)return Result::Stale;
        opened_.emplace(Opened{*revision,std::move(packages),std::move(metadata),std::move(library),
            std::move(roster),std::move(retail),std::move(*owned),std::move(view),slot,0,policy});
        state_=BorgForceArrangeState::Browse;
        return Result::Ready;
    }

    void close(){
        placement_.cancel();opened_.reset();consent_=false;state_=BorgForceArrangeState::Closed;
    }
    BorgForceArrangeState state()const{return state_;}
    // Views are borrowed only until the next mutating call. On lost or
    // uncertain save scope, exposed data disappear until explicit reopen.
    const AccountSaveRevision* save_revision()const{return opened_?&opened_->revision:nullptr;}
    const MixedBorgForceView* force_view()const{return opened_?&opened_->view:nullptr;}
    const PackageBorgRoster* roster()const{return opened_?&opened_->roster:nullptr;}
    const BorgLibrary* library()const{return opened_?&opened_->library:nullptr;}
    std::span<const BorgLibraryOwnedRow> owned_rows()const{
        return opened_?std::span<const BorgLibraryOwnedRow>(opened_->owned):std::span<const BorgLibraryOwnedRow>{};
    }
    unsigned slot()const{return opened_?opened_->slot:0;}
    u64 selected_instance()const{return opened_?opened_->selected:0;}
    const BorgPlacementOffer* offer()const{return placement_.offer();}
    bool confirm_selected()const{return state_==BorgForceArrangeState::Confirm&&input_.confirm_selected();}

    bool current(){
        if(!opened_)return false;
        if(backend_.revision()!=opened_->revision){invalidate();return false;}
        return true;
    }
    bool select_position(unsigned force,unsigned slot){
        if(state_!=BorgForceArrangeState::Browse||!current()||force>=kForceCount||slot>=kForceSlots)return false;
        if(force!=opened_->view.force&&!project(opened_->roster,opened_->packages,opened_->metadata,opened_->retail,
                force,&opened_->view))return false;
        opened_->slot=slot;return true;
    }
    bool select_owned(u64 instance){
        if(state_!=BorgForceArrangeState::Browse||!current()||!opened_->roster.find(instance))return false;
        opened_->selected=instance;return true;
    }

    BorgPlacementResult stage(const GamePadState& opening_input={}){
        using Result=BorgPlacementResult;
        if(state_!=BorgForceArrangeState::Browse)return Result::NotConfirmed;
        if(!current())return Result::Stale;
        if(!opened_->selected)return Result::OwnedUnavailable;
        const auto result=placement_.prepare(opened_->selected,opened_->view.force,opened_->slot,
            opened_->retail.catalog(),opened_->packages,opened_->metadata,opened_->policy);
        if(!current())return Result::Stale;
        if(result==Result::Ready){
            input_.begin(opening_input);consent_=false;state_=BorgForceArrangeState::Confirm;
        }
        return result;
    }

    void cancel(){
        placement_.cancel();consent_=false;
        if(state_==BorgForceArrangeState::Confirm)state_=BorgForceArrangeState::Browse;
    }
    // The host only rescans metadata after this returns Confirm. Back and
    // quit consume the offer; the opening input sample cannot acknowledge it.
    BorgPlacementRequest step_confirmation(const GamePadState& pad){
        if(state_!=BorgForceArrangeState::Confirm)return BorgPlacementRequest::None;
        const auto request=pad.quit_requested?BorgPlacementRequest::Cancel:input_.step(pad);
        if(request==BorgPlacementRequest::Cancel)cancel();
        else if(request==BorgPlacementRequest::Confirm){
            if(!current())return BorgPlacementRequest::None;
            consent_=true;
        }
        return request;
    }
    BorgPlacementResult confirm(const ForceCatalog* retail_catalogue,const BorgPackageCatalog& current_packages,
                                std::span<const BorgCatalogSnapshot> current_metadata,
                                const BorgPlacementPolicy& current_policy={}){
        using Result=BorgPlacementResult;
        if(state_!=BorgForceArrangeState::Confirm||!consent_)return Result::NotConfirmed;
        if(!current())return Result::Stale;
        consent_=false;
        // Copy exact consent before the transaction consumes the offer.
        const auto placed=*placement_.offer();
        const auto result=placement_.confirm(retail_catalogue,current_packages,current_metadata,current_policy);
        if(result!=Result::Saved){invalidate();return result;}
        // The existing repo lease adopts only acknowledged revisions. Update
        // the view by reloading, not by reapplying the mutation to any record.
        const auto revision=backend_.revision();
        if(!revision||store_.load(&opened_->roster)!=PackageBorgLoad::Loaded||
           !project(opened_->roster,opened_->packages,opened_->metadata,opened_->retail,placed.force,&opened_->view)||
           !store_.current()||backend_.revision()!=revision){
            invalidate();return Result::Saved; // acknowledged write, refresh failed
        }
        opened_->revision=*revision;state_=BorgForceArrangeState::Browse;
        return Result::Saved;
    }
private:
    struct Opened {
        AccountSaveRevision revision;
        BorgPackageCatalog packages;
        std::vector<BorgCatalogSnapshot> metadata;
        BorgLibrary library;
        PackageBorgRoster roster;
        ForceInventory retail;
        std::vector<BorgLibraryOwnedRow> owned;
        MixedBorgForceView view;
        unsigned slot;
        u64 selected;
        BorgPlacementPolicy policy;
    };
    static bool project(const PackageBorgRoster& roster,const BorgPackageCatalog& packages,
                        std::span<const BorgCatalogSnapshot> metadata,const ForceInventory& retail,
                        unsigned force,MixedBorgForceView* view){
        PackageForceView package;
        return package_borg_force_view(roster,packages,metadata,force,&package)&&
               borg_mixed_force_view(package,retail.ready()?&retail:nullptr,view);
    }
    void invalidate(){close();state_=BorgForceArrangeState::ReopenRequired;}

    AccountSaveBackend& backend_;
    PackageBorgSaveStore store_;
    BorgPackagePlacementSession placement_;
    std::optional<Opened> opened_;
    BorgPlacementConfirmation input_;
    BorgForceArrangeState state_=BorgForceArrangeState::Closed;
    bool consent_=false;
};
}
