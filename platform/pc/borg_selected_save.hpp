#pragma once
#include "engine/save/account_save_repository.hpp"
#include "engine/gameplay/borg_package_roster.hpp"
#include "platform/pc/account_save_storage.hpp"
#include "platform/pc/borg_catalog_load.hpp"

namespace gf::pc {
struct BorgSelectedSave {
    std::unique_ptr<IAccountSaveStorage> storage;
    std::unique_ptr<AccountSaveRepository> repository;
    std::unique_ptr<AccountSaveBackend> backend;
    AccountSaveEntry entry;
};
inline std::unique_ptr<BorgSelectedSave> open_borg_selected_save(const std::string& root,u64 account_id,u64 save_id,
                                                                std::string* error){
    auto fail=[&](const char* why)->std::unique_ptr<BorgSelectedSave>{if(error)*error=why;return {};};
    if(!account_id||!save_id)return fail("explicit account and save IDs are required");
    auto next=std::make_unique<BorgSelectedSave>();
    next->storage=create_account_save_storage(root,false,error);
    if(!next->storage)return {};
    next->repository=std::make_unique<AccountSaveRepository>(*next->storage);
    AccountSavePage page;
    if(next->repository->accounts(account_id-1,1,&page)!=AccountResult::Ok||
       page.entries.empty()||page.entries.front().key!=AccountSaveKey{account_id,0})
        return fail("requested account unavailable");
    const auto account=page.entries.front();
    if(next->repository->saves(account,save_id-1,1,&page)!=AccountResult::Ok||
       page.entries.empty()||page.entries.front().key!=AccountSaveKey{account_id,save_id})
        return fail("requested save unavailable");
    next->entry=page.entries.front();
    if(next->repository->open(next->entry,&next->backend)!=AccountResult::Ok)return fail("could not open the requested save");
    if(error)error->clear();
    return next;
}
struct BorgOwnedMetadata {
    std::vector<BorgCatalogSnapshot> snapshots;
    unsigned unavailable=0;
};
inline BorgOwnedMetadata load_borg_owned_metadata(const std::filesystem::path& root,const BorgPackageCatalog& catalog,
                                                  const PackageBorgRoster& roster){
    std::vector<std::string> ids;
    for(const auto& owned:roster.owned)
        if(owned.variant&&catalog.find(owned.identity.id))ids.push_back(owned.identity.id);
    std::sort(ids.begin(),ids.end());ids.erase(std::unique(ids.begin(),ids.end()),ids.end());
    BorgOwnedMetadata result;
    for(const auto& id:ids){
        BorgCatalogSnapshot current;
        if(load_borg_catalog_snapshot(root,catalog,id,&current,nullptr))result.snapshots.push_back(std::move(current));
        else ++result.unavailable;
    }
    return result;
}
}
