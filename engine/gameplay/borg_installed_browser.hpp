#pragma once
#include "engine/assets/borg_catalog_profile.hpp"
#include "engine/input/input_map.hpp"

namespace gf {
struct BorgBrowserSelection {
    BorgIdentity identity;
    std::string variant,label;
    bool operator==(const BorgBrowserSelection&)const=default;
};
enum class BorgBrowserChange {None,Package,Variant,Confirm,Preview};

// Read-only navigation of an installation snapshot. A selection is neither
// a saved instance nor a permission to construct an actor.
class BorgInstalledBrowser {
public:
    void open(BorgPackageCatalog catalog){
        if(catalog.status!=BorgPackageScanStatus::Complete)catalog.discovered.clear();
        catalog_=std::move(catalog);entry_=variant_=0;metadata_.reset();previous_={};
    }
    const BorgPackageCatalog& catalog()const{return catalog_;}
    std::size_t package_index()const{return entry_;}
    std::size_t variant_index()const{return variant_;}
    const BorgPackageEntry* entry()const{
        return entry_<catalog_.discovered.size()?&catalog_.discovered[entry_]:nullptr;
    }
    const BorgCatalogSnapshot* metadata()const{return metadata_?&*metadata_:nullptr;}
    const BorgCatalogVariant* variant()const{
        return metadata_&&variant_<metadata_->profile.variants.size()?&metadata_->profile.variants[variant_]:nullptr;
    }
    bool bind(BorgCatalogSnapshot snapshot){
        const auto* current=entry();
        if(!current||current->folder!=snapshot.package.folder||current->manifest!=snapshot.package.manifest)return false;
        const auto* initial=snapshot.profile.find(snapshot.profile.default_variant);
        if(!initial)return false;
        variant_=std::size_t(initial-snapshot.profile.variants.data());
        metadata_=std::move(snapshot);return true;
    }
    std::optional<BorgBrowserSelection> selection()const{
        const auto* value=variant();const auto* selected=entry();
        if(!value||!selected)return {};
        return BorgBrowserSelection{selected->manifest.identity,value->id,value->label};
    }
    void sync_input(const GamePadState& pad){previous_=pad;}
    BorgBrowserChange step(const GamePadState& pad){
        const bool up=pad.dpad_up&&!previous_.dpad_up,down=pad.dpad_down&&!previous_.dpad_down;
        const bool left=pad.dpad_left&&!previous_.dpad_left,right=pad.dpad_right&&!previous_.dpad_right;
        const bool confirm=pad.a&&!previous_.a,preview=pad.x&&!previous_.x;previous_=pad;
        const auto entries=catalog_.discovered.size();
        if(entries>1&&up!=down){
            entry_=up?(entry_+entries-1)%entries:(entry_+1)%entries;
            variant_=0;metadata_.reset();
            return BorgBrowserChange::Package;
        }
        if(metadata_&&metadata_->profile.variants.size()>1&&left!=right){
            const auto count=metadata_->profile.variants.size();
            variant_=left?(variant_+count-1)%count:(variant_+1)%count;
            return BorgBrowserChange::Variant;
        }
        if(!variant())return BorgBrowserChange::None;
        if(preview)return BorgBrowserChange::Preview;
        return confirm?BorgBrowserChange::Confirm:BorgBrowserChange::None;
    }
private:
    BorgPackageCatalog catalog_;
    std::optional<BorgCatalogSnapshot> metadata_;
    std::size_t entry_=0,variant_=0;
    GamePadState previous_{};
};
}
