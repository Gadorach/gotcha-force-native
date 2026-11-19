#pragma once
#include "engine/gameplay/borg_mixed_force_view.hpp"
#include "engine/save/borg_package_save.hpp"
#include "engine/save/scoped_retail_force_record.hpp"
#include "platform/pc/borg_selected_save.hpp"

namespace gf::pc {
// Hold both records from the same selected account/save lease. Practice and
// placement are separate consumers; this object cannot write or launch either.
class BorgForceBrowseSnapshot {
public:
    const AccountSaveEntry& save()const{return selected_->entry;}
    const ForceCatalog* retail_catalogue()const{return catalogue_.ready()?&catalogue_:nullptr;}
    bool current(){
        return package_record_.current()&&scoped_retail_force_record_current(*selected_->backend,retail_record_);
    }
    bool project(const BorgPackageCatalog& packages,unsigned force,MixedBorgForceView* out){
        if(!out||!current())return false;
        PackageForceView package;MixedBorgForceView next;
        if(!package_borg_force_view(roster_,packages,metadata_.snapshots,force,&package)||
           !borg_mixed_force_view(package,retail_.ready()?&retail_:nullptr,&next)||!current())return false;
        *out=std::move(next);return true;
    }
    static std::unique_ptr<BorgForceBrowseSnapshot> open(const std::string& root,AccountSaveKey key,
                                                         const std::filesystem::path& packages_root,
                                                         const BorgPackageCatalog& packages,ForceCatalog catalogue,
                                                         std::string* error){
        auto fail=[&](const char* why)->std::unique_ptr<BorgForceBrowseSnapshot>{if(error)*error=why;return {};};
        auto selected=open_borg_selected_save(root,key.account,key.save,error);
        if(!selected)return {};
        auto next=std::unique_ptr<BorgForceBrowseSnapshot>(new BorgForceBrowseSnapshot(std::move(selected),std::move(catalogue)));
        const auto loaded=next->package_record_.load(&next->roster_);
        if(loaded!=PackageBorgLoad::Loaded&&loaded!=PackageBorgLoad::Missing)return fail("package roster unavailable or invalid");
        if(!read_scoped_retail_force_record(*next->selected_->backend,next->catalogue_.ready()?&next->catalogue_:nullptr,
            &next->retail_record_,&next->retail_))return fail("retail Force record is unverified; supply its matching --retail-catalog");
        next->metadata_=load_borg_owned_metadata(packages_root,packages,next->roster_);
        if(!next->current())return fail("selected save changed while opening the Force browser");
        if(error)error->clear();
        return next;
    }
private:
    BorgForceBrowseSnapshot(std::unique_ptr<BorgSelectedSave> selected,ForceCatalog catalogue)
        :selected_(std::move(selected)),package_record_(*selected_->backend),catalogue_(std::move(catalogue)){}
    std::unique_ptr<BorgSelectedSave> selected_;
    PackageBorgSaveStore package_record_;
    const ForceCatalog catalogue_; // Owns the table pinned by retail_ until it is destroyed.
    ForceInventory retail_;
    std::vector<u8> retail_record_;
    PackageBorgRoster roster_;
    BorgOwnedMetadata metadata_;
};
}
