#pragma once
#include "engine/gameplay/borg_collection.hpp"
#include "engine/gameplay/gred_picker_practice_route.hpp"
#include "engine/input/input_map.hpp"

namespace gf {
struct BorgBrowserSelection {
    BorgIdentity identity;
    std::string variant,label;
    bool operator==(const BorgBrowserSelection&)const=default;
};
enum class BorgBrowserChange {None,Package,Variant,Confirm,Preview,Practice,PracticeRoute};

// Read-only navigation of installed packages or a selected save snapshot.
// Body preview uses package identity, never owned-instance or actor authority.
class BorgInstalledBrowser {
public:
    void open(BorgPackageCatalog catalog){
        if(catalog.status!=BorgPackageScanStatus::Complete)catalog.discovered.clear();
        catalog_=std::move(catalog);entry_=variant_=0;metadata_.reset();owned_.reset();previous_={};
        collection_.reset();snapshots_.clear();
        practice_route_=GRedPickerPracticeRoute::Standard;
    }
    bool open_owned(BorgPackageCatalog catalog,PackageBorgRoster roster,u64 initial_instance=0){
        if(!package_borg_roster_valid(roster))return false;
        const auto* initial=initial_instance?roster.find(initial_instance):nullptr;
        if(initial_instance&&!initial)return false;
        const auto index=initial?std::size_t(initial-roster.owned.data()):0;
        open(std::move(catalog));owned_=std::move(roster);entry_=index;return true;
    }
    // Installed, owned and Arrange share one exact row model. The legacy
    // practice screen retains its explicit opt-in action.
    bool open_collection(BorgPackageCatalog catalog,std::vector<BorgCatalogSnapshot> snapshots,
                         std::span<const BorgLibraryCatalogIssue> issues={},const PackageBorgRoster* roster=nullptr,
                         u64 initial_instance=0){
        if(roster&&!package_borg_roster_valid(*roster))return false;
        if(initial_instance&&(!roster||!roster->find(initial_instance)))return false;
        std::optional<PackageBorgRoster> save;
        if(roster)save=*roster;
        BorgLibrary library(catalog,snapshots,issues);
        open(std::move(catalog));snapshots_=std::move(snapshots);
        if(save){
            owned_=std::move(save);
            collection_.emplace(BorgLibrary(BorgPackageCatalog{}));
            collection_->refresh(std::move(library),&*owned_);
            if(initial_instance)collection_->select_instance(initial_instance);
        }else collection_.emplace(std::move(library));
        return true;
    }
    const BorgCollection* collection()const{return collection_?&*collection_:nullptr;}
    bool select_collection_row(std::size_t index){return collection_&&collection_->select(index);}
    const BorgPackageCatalog& catalog()const{return catalog_;}
    const PackageBorgRoster* owned()const{return owned_?&*owned_:nullptr;}
    const OwnedPackageBorg* owned_entry()const{
        if(collection_){
            const auto* row=collection_->owned_row(collection_->selected());
            return row?&row->saved:nullptr;
        }
        return owned_&&entry_<owned_->owned.size()?&owned_->owned[entry_]:nullptr;
    }
    std::size_t size()const{return collection_?collection_->size():owned_?owned_->owned.size():catalog_.discovered.size();}
    std::string_view owned_status()const{
        const auto* saved=owned_entry();
        if(!saved)return {};
        if(collection_)return borg_library_state_label(collection_->owned_row(collection_->selected())->state);
        switch(package_borg_availability(catalog_,saved->identity.id)){
        case PackageBorgAvailability::Missing:return "PACKAGE MISSING";
        case PackageBorgAvailability::Incomplete:return "PACKAGE INCOMPLETE";
        case PackageBorgAvailability::Conflict:return "CONFLICTING PACKAGES";
        case PackageBorgAvailability::ScanUnavailable:return "PACKAGE SCAN UNAVAILABLE";
        case PackageBorgAvailability::MissingVariant:return "SAVED VARIANT MISSING";
        case PackageBorgAvailability::Discovered:break;
        }
        if(!saved->variant)return "SAVED VARIANT UNSPECIFIED";
        if(!metadata_)return "CATALOGUE UNAVAILABLE";
        return variant()?"EXACT VARIANT RESOLVED":"SAVED VARIANT MISSING";
    }
    std::size_t package_index()const{return collection_?collection_->selected():entry_;}
    std::size_t variant_index()const{return variant_;}
    GRedPickerPracticeRoute practice_route()const{return practice_route_;}
    std::string_view practice_route_name()const{return gred_picker_practice_route_name(practice_route_);}
    const BorgPackageEntry* entry()const{
        if(collection_){
            if(const auto* saved=owned_entry())return catalog_.find(saved->identity.id);
            const auto* row=collection_->installed_row(collection_->selected());
            return row?catalog_.find(row->identity.id):nullptr;
        }
        if(owned_){
            const auto* saved=owned_entry();
            return saved?catalog_.find(saved->identity.id):nullptr;
        }
        return entry_<catalog_.discovered.size()?&catalog_.discovered[entry_]:nullptr;
    }
    const BorgCatalogSnapshot* metadata()const{
        if(collection_){
            const auto* row=collection_->installed_row(collection_->selected());
            if(!row||!row->key)return nullptr;
            for(const auto& snapshot:snapshots_)if(snapshot.package.manifest.identity.id==row->key->package_id)return &snapshot;
            return nullptr;
        }
        return metadata_?&*metadata_:nullptr;
    }
    const BorgCatalogVariant* variant()const{
        if(collection_){
            const auto* row=collection_->installed_row(collection_->selected());
            const auto* snapshot=metadata();
            return snapshot?snapshot->profile.find(row->key->variant_id):nullptr;
        }
        return metadata_&&variant_<metadata_->profile.variants.size()?&metadata_->profile.variants[variant_]:nullptr;
    }
    bool bind(BorgCatalogSnapshot snapshot){
        if(collection_)return false;
        const auto* current=entry();
        if(!current||current->folder!=snapshot.package.folder||current->manifest!=snapshot.package.manifest)return false;
        const auto* fallback=snapshot.profile.find(snapshot.profile.default_variant);
        if(!fallback)return false;
        const auto* saved=owned_entry();
        const auto* initial=saved?(saved->variant?snapshot.profile.find(saved->variant->id):nullptr):fallback;
        variant_=initial?std::size_t(initial-snapshot.profile.variants.data()):snapshot.profile.variants.size();
        metadata_=std::move(snapshot);return true;
    }
    std::optional<BorgBrowserSelection> selection()const{
        const auto* value=variant();const auto* selected=entry();
        if(!value||!selected)return {};
        if(const auto* saved=owned_entry()){
            if(saved->disabled)return {};
            if(collection_)return BorgBrowserSelection{saved->identity,value->id,saved->variant->label};
        }
        return BorgBrowserSelection{selected->manifest.identity,value->id,value->label};
    }
    void sync_input(const GamePadState& pad){previous_=pad;}
    BorgBrowserChange step(const GamePadState& pad,bool allow_practice=false,std::size_t page_size=1,bool allow_arrange=false){
        const bool up=pad.dpad_up&&!previous_.dpad_up,down=pad.dpad_down&&!previous_.dpad_down;
        const bool left=pad.dpad_left&&!previous_.dpad_left,right=pad.dpad_right&&!previous_.dpad_right;
        const bool confirm=pad.a&&!previous_.a,preview=pad.x&&!previous_.x;
        const bool practice=allow_practice&&pad.y&&!previous_.y&&!pad.b&&!pad.start;
        const bool sort=pad.l&&!previous_.l,filter=pad.r&&!previous_.r;
        previous_=pad;
        if(collection_){
            if(sort||filter){
                collection_->configure(!sort?collection_->sort():collection_->sort()==BorgCollectionSort::Identity?
                    BorgCollectionSort::Name:BorgCollectionSort::Identity,
                    !filter?collection_->filter():collection_->filter()==BorgCollectionFilter::All?
                    BorgCollectionFilter::Exact:BorgCollectionFilter::All);
                return BorgBrowserChange::Package;
            }
            if(up!=down&&collection_->move(up))return BorgBrowserChange::Package;
            if(left!=right&&collection_->move(left,std::max(std::size_t{1},page_size)))return BorgBrowserChange::Package;
            if(preview&&selection())return BorgBrowserChange::Preview;
            if(confirm&&((allow_arrange&&owned_entry())||selection()))return BorgBrowserChange::Confirm;
            return BorgBrowserChange::None;
        }
        const auto entries=size();
        if(entries>1&&up!=down){
            entry_=up?(entry_+entries-1)%entries:(entry_+1)%entries;
            variant_=0;metadata_.reset();
            return BorgBrowserChange::Package;
        }
        if(!owned_&&metadata_&&metadata_->profile.variants.size()>1&&left!=right){
            const auto count=metadata_->profile.variants.size();
            variant_=left?(variant_+count-1)%count:(variant_+1)%count;
            return BorgBrowserChange::Variant;
        }
        if(allow_practice&&owned_entry()&&left!=right){
            practice_route_=cycle_gred_picker_practice_route(practice_route_,left);
            return BorgBrowserChange::PracticeRoute;
        }
        // This is a request to check the exact owned copy, not a readiness
        // receipt. Preview and inspect win if pressed on the same sample.
        if(practice&&!preview&&!confirm&&owned_entry())return BorgBrowserChange::Practice;
        if(const auto* saved=owned_entry();saved&&saved->disabled)return BorgBrowserChange::None;
        if(!variant())return BorgBrowserChange::None;
        if(preview)return BorgBrowserChange::Preview;
        return confirm?BorgBrowserChange::Confirm:BorgBrowserChange::None;
    }
private:
    BorgPackageCatalog catalog_;
    std::optional<PackageBorgRoster> owned_;
    std::optional<BorgCatalogSnapshot> metadata_;
    std::optional<BorgCollection> collection_;
    std::vector<BorgCatalogSnapshot> snapshots_;
    std::size_t entry_=0,variant_=0;
    GRedPickerPracticeRoute practice_route_=GRedPickerPracticeRoute::Standard;
    GamePadState previous_{};
};
}
