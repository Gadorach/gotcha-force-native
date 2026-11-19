#pragma once
#include "engine/gameplay/borg_package_roster.hpp"
#include "engine/input/input_map.hpp"

namespace gf {
struct BorgBrowserSelection {
    BorgIdentity identity;
    std::string variant,label;
    bool operator==(const BorgBrowserSelection&)const=default;
};
enum class BorgBrowserChange {None,Package,Variant,Confirm,Preview,Practice,PracticeRoute};
// UI choices for the qualified normal-G-Red lab only. Choosing a route
// selects neither a package variant nor permission to run any copy.
enum class GRedPickerPracticeRoute : u8 {
    Standard,SlopeJoin,InclinedWalk,InclinedDash,LedgePush,HighAnglePush,ExpiryPush,Count
};
inline constexpr std::array<std::string_view,unsigned(GRedPickerPracticeRoute::Count)> kGRedPickerPracticeRouteNames{
    "STANDARD","SLOPE JOIN","INCLINED WALK","INCLINED DASH","WALK LEDGE PUSH","STEEPER LEDGE PUSH","LEDGE TIMER EXPIRY"
};

// Read-only navigation of installed packages or a selected save snapshot.
// Body preview uses package identity, never owned-instance or actor authority.
class BorgInstalledBrowser {
public:
    void open(BorgPackageCatalog catalog){
        if(catalog.status!=BorgPackageScanStatus::Complete)catalog.discovered.clear();
        catalog_=std::move(catalog);entry_=variant_=0;metadata_.reset();owned_.reset();previous_={};
        practice_route_=GRedPickerPracticeRoute::Standard;
    }
    bool open_owned(BorgPackageCatalog catalog,PackageBorgRoster roster,u64 initial_instance=0){
        if(!package_borg_roster_valid(roster))return false;
        const auto* initial=initial_instance?roster.find(initial_instance):nullptr;
        if(initial_instance&&!initial)return false;
        const auto index=initial?std::size_t(initial-roster.owned.data()):0;
        open(std::move(catalog));owned_=std::move(roster);entry_=index;return true;
    }
    const BorgPackageCatalog& catalog()const{return catalog_;}
    const PackageBorgRoster* owned()const{return owned_?&*owned_:nullptr;}
    const OwnedPackageBorg* owned_entry()const{
        return owned_&&entry_<owned_->owned.size()?&owned_->owned[entry_]:nullptr;
    }
    std::size_t size()const{return owned_?owned_->owned.size():catalog_.discovered.size();}
    std::string_view owned_status()const{
        const auto* saved=owned_entry();
        if(!saved)return {};
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
    std::size_t package_index()const{return entry_;}
    std::size_t variant_index()const{return variant_;}
    GRedPickerPracticeRoute practice_route()const{return practice_route_;}
    std::string_view practice_route_name()const{return kGRedPickerPracticeRouteNames[unsigned(practice_route_)];}
    const BorgPackageEntry* entry()const{
        if(owned_){
            const auto* saved=owned_entry();
            return saved?catalog_.find(saved->identity.id):nullptr;
        }
        return entry_<catalog_.discovered.size()?&catalog_.discovered[entry_]:nullptr;
    }
    const BorgCatalogSnapshot* metadata()const{return metadata_?&*metadata_:nullptr;}
    const BorgCatalogVariant* variant()const{
        return metadata_&&variant_<metadata_->profile.variants.size()?&metadata_->profile.variants[variant_]:nullptr;
    }
    bool bind(BorgCatalogSnapshot snapshot){
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
        if(const auto* saved=owned_entry();saved&&saved->disabled)return {};
        if(!value||!selected)return {};
        return BorgBrowserSelection{selected->manifest.identity,value->id,value->label};
    }
    void sync_input(const GamePadState& pad){previous_=pad;}
    BorgBrowserChange step(const GamePadState& pad,bool allow_practice=false){
        const bool up=pad.dpad_up&&!previous_.dpad_up,down=pad.dpad_down&&!previous_.dpad_down;
        const bool left=pad.dpad_left&&!previous_.dpad_left,right=pad.dpad_right&&!previous_.dpad_right;
        const bool confirm=pad.a&&!previous_.a,preview=pad.x&&!previous_.x;
        const bool practice=allow_practice&&pad.y&&!previous_.y&&!pad.b&&!pad.start;previous_=pad;
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
            constexpr auto count=unsigned(GRedPickerPracticeRoute::Count);
            const auto index=unsigned(practice_route_);
            practice_route_=GRedPickerPracticeRoute((index+(left?count-1:1))%count);
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
    std::size_t entry_=0,variant_=0;
    GRedPickerPracticeRoute practice_route_=GRedPickerPracticeRoute::Standard;
    GamePadState previous_{};
};
}
