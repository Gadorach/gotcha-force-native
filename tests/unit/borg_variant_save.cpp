#include <cstdio>
#include <cstdlib>
#include "engine/save/borg_package_save.hpp"
#include "tests/unit/account_save_fixture.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"Borg variant save: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    PackageBorgRoster roster{{
        {4,{"lab:ant","Clockwork Ant"},false,SavedPackageVariant{"silver","Silver Edition"}},
        {7,{"lab:ant","Another Ant"}},
    },{{4,0,1},{7,0,2},{4,5,3}}};
    auto invalid=roster;invalid.owned[0].variant->id="../silver";
    check(package_borg_roster_valid(roster)&&!package_borg_roster_valid(invalid),"saved IDs and labels have an explicit data domain");
    invalid=roster;invalid.owned[0].variant->label="";
    check(!package_borg_roster_valid(invalid),"a named variant must retain a readable label");

    BorgPackageCatalog catalog;catalog.status=BorgPackageScanStatus::Complete;
    catalog.discovered.push_back({"ant",{{"lab:ant","Ant Deluxe"},{},{},{},{}}});
    std::array<BorgCatalogSnapshot,1> metadata;
    metadata[0].package=catalog.discovered.front();
    const std::string base="GFBORG-CATALOG 1\ndescription=Test Borg.\ntribe=none\ndefault-variant=normal\n"
        "variant=normal|Normal|200|unresolved\n";
    check(borg_catalog_read_profile(base,&metadata[0].profile),"metadata parses even without icons");
    check(package_borg_variant_status(roster.owned[0],catalog,{})==SavedPackageVariantStatus::CatalogueUnavailable&&
        package_borg_variant_status(roster.owned[0],catalog,metadata)==SavedPackageVariantStatus::Missing&&
        package_borg_variant_status(roster.owned[1],catalog,metadata)==SavedPackageVariantStatus::Unspecified,
        "missing variant, unavailable catalogue and legacy unspecified do not collapse");
    std::vector<MissingPackageBorg> notices;
    check(package_borg_missing(roster,catalog,&notices)&&notices.empty(),"catalogue not supplied cannot allege a missing variant");
    check(package_borg_missing(roster,catalog,&notices,metadata)&&notices.size()==1&&
        notices[0].reason==PackageBorgAvailability::MissingVariant&&notices[0].saved.variant==roster.owned[0].variant&&
        notices[0].affected==std::vector<PackageBorgPlacement>{{4,0,1},{4,5,3}},
        "keeps the saved variant name and all affected positions while the package remains installed");
    const auto missing=notices[0];
    auto stale=metadata;stale[0].package.folder="old-folder";
    PackageBorgRoster candidate;
    check(!package_borg_resolve_missing(roster,catalog,missing,MissingPackageChoice::DeleteFromSave,&candidate,stale),
        "a stale catalogue cannot authorize deletion");
    check(package_borg_resolve_missing(roster,catalog,missing,MissingPackageChoice::DisableInForces,&candidate,metadata)&&
        candidate.owned[0].disabled&&candidate.owned[0].variant==roster.owned[0].variant&&candidate.placements==roster.placements,
        "disable preserves the exact variant and both positions");
    const auto disabled=candidate;
    check(!package_borg_enable_restored(disabled,catalog,disabled.owned[0],&candidate,metadata),
        "installed package alone cannot re-enable a missing variant");
    check(borg_catalog_read_profile(base+"variant=silver|Renamed Silver|210|unresolved\n",&metadata[0].profile)&&
        !package_borg_resolve_missing(roster,catalog,missing,MissingPackageChoice::DeleteFromSave,&candidate,metadata)&&
        package_borg_enable_restored(disabled,catalog,disabled.owned[0],&candidate,metadata)&&candidate==roster,
        "restored ID permits re-enable, invalidates stale prompts, and keeps saved label");

    account_save_fixture::Storage storage;AccountSaveRepository repo(storage);
    AccountSaveEntry account,save,copy;std::unique_ptr<AccountSaveBackend> selected;
    check(repo.create_account(9,"Player",&account)==AccountResult::Ok&&repo.create_save(account,19,"Save",&save)==AccountResult::Ok&&
        repo.open(save,&selected)==AccountResult::Ok,"selected save fixture");
    PackageBorgSaveStore store(*selected);
    auto legacy=roster;legacy.owned[0].variant.reset();
    PackageBorgRoster loaded;
    std::vector<u8> bytes;
    check(store.load(&loaded)==PackageBorgLoad::Missing&&store.write(legacy)&&
        selected->load(kBorgPackageSaveSlot,bytes)&&load_be32(bytes.data()+kSaveHeaderBytes+4)==1&&
        store.load(&loaded)==PackageBorgLoad::Loaded&&loaded==legacy&&!loaded.owned[0].variant,
        "version one remains unchanged and never guesses the default variant");
    check(store.write(disabled)&&selected->load(kBorgPackageSaveSlot,bytes)&&load_be32(bytes.data()+kSaveHeaderBytes+4)==2&&
        repo.save_as(20,"Variant Copy",&copy)==AccountResult::Ok,"version two is written only for known variants and Save As copies it");
    repo.close();selected.reset();
    check(repo.open(copy,&selected)==AccountResult::Ok,"open copied save");
    PackageBorgSaveStore copied(*selected);
    check(copied.load(&loaded)==PackageBorgLoad::Loaded&&loaded==disabled&&loaded.owned[0].variant->label=="Silver Edition"&&
        !loaded.owned[1].variant,"mixed specified and legacy-unspecified variant data survives reload");
    // A bad variant flag in a valid framed record is not a migration hint.
    SaveWriter wrong;wrong.u32_field(0x4e425247u);wrong.u32_field(2);wrong.u32_field(1);wrong.u32_field(0);
    wrong.u64_field(4);wrong.u32_field(0);
    for(const auto* text:{&roster.owned[0].identity.id,&roster.owned[0].identity.name})
        wrong.blob(reinterpret_cast<const u8*>(text->data()),text->size());
    wrong.u32_field(2);
    SaveEngine codec(*selected);
    check(codec.write(kBorgPackageSaveSlot,wrong)&&copied.load(&loaded)==PackageBorgLoad::Invalid&&
        loaded==disabled&&!copied.write(roster),"malformed variant flag cannot erase or rewrite saved roster state");
    std::puts("Borg variant save: versioned identity, missing-variant recovery and exact restoration PASS");
}
