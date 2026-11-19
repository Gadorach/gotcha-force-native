#include <cstdio>
#include <cstdlib>
#include "engine/save/borg_package_save.hpp"
#include "engine/gameplay/cheat_settings.hpp"
#include "tests/unit/account_save_fixture.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"Borg package save: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    PackageBorgRoster roster{{
        {1,{"lab:ant","Ant"}},
        {2,{"lab:crane","Clockwork Crane"}},
        {3,{"lab:not-crane","Clockwork Crane"}},
        {4,{"lab:bee","Bee"}},
        {5,{"lab:dragon","Dragon"}},
    },{{2,0,0},{1,0,1},{2,5,3},{3,5,4},{4,8,0}}};
    check(package_borg_roster_valid(roster),"valid owned IDs and stable Force positions");
    auto malformed=roster;malformed.placements[0].instance=999;
    check(!package_borg_roster_valid(malformed),"dangling Force slot cannot serialize");
    malformed=roster;malformed.placements[1].instance=2;
    check(!package_borg_roster_valid(malformed),"one instance cannot occupy two positions in a Force");
    malformed=roster;malformed.owned[1].instance=1;
    check(!package_borg_roster_valid(malformed),"duplicate owned ID cannot serialize");
    BorgPackageCatalog catalog;catalog.status=BorgPackageScanStatus::Complete;
    catalog.discovered={{"renamed-ant",{{"lab:ant","Ant Deluxe"},{},{},{},{}}},
        {"different-crane",{{"lab:not-crane","Clockwork Crane"},{},{},{},{}}}};
    catalog.issues={{"bee-a","lab:bee",BorgPackageIssueKind::DuplicateId},{"dragon","lab:dragon",BorgPackageIssueKind::MissingAsset}};
    std::vector<MissingPackageBorg> notices;
    check(package_borg_missing(roster,catalog,&notices)&&notices.size()==3&&
        notices[0].saved.identity==BorgIdentity{"lab:crane","Clockwork Crane"}&&
        notices[0].affected==std::vector<PackageBorgPlacement>{{2,0,0},{2,5,3}}&&
        notices[1].reason==PackageBorgAvailability::Conflict&&notices[2].reason==PackageBorgAvailability::Incomplete,
        "reload uses saved names, exact IDs, separate conflict reasons and all affected slots");
    const auto crane_notice=notices[0];
    PackageBorgRoster next;
    check(!package_borg_resolve_missing(roster,catalog,notices[1],MissingPackageChoice::DeleteFromSave,&next)&&
        next.owned.empty(),"a duplicate claim does not authorize missing-Borg deletion");
    auto scan_failed=catalog;scan_failed.status=BorgPackageScanStatus::Unavailable;
    check(!package_borg_missing(roster,scan_failed,&notices)&&notices[0]==crane_notice&&
        !package_borg_resolve_missing(roster,scan_failed,crane_notice,MissingPackageChoice::DeleteFromSave,&next),
        "failed scan cannot turn roster entries into missing notices or authorize a new save");
    auto restored=catalog;restored.discovered.insert(restored.discovered.begin()+1,
        {"new-folder",{{"lab:crane","Crane Returns"},{},{},{},{}}});
    check(!package_borg_resolve_missing(roster,restored,crane_notice,MissingPackageChoice::DisableInForces,&next),
        "restore invalidates a stale missing notice even if folder and name changed");
    auto changed_slots=roster;changed_slots.placements[2].slot=2;
    check(!package_borg_resolve_missing(changed_slots,catalog,crane_notice,MissingPackageChoice::DeleteFromSave,&next),
        "changed Force membership invalidates prior consent");
    check(package_borg_resolve_missing(roster,catalog,crane_notice,MissingPackageChoice::DisableInForces,&next)&&
        next.find(2)->disabled&&!roster.find(2)->disabled&&next.placements==roster.placements&&next.find(3)&&
        next.find(2)->identity==roster.find(2)->identity,"disabling preserves ownership and both Force positions in a candidate");
    const auto disabled=next;
    check(!package_borg_enable_restored(disabled,catalog,*disabled.find(2),&next)&&
        package_borg_enable_restored(disabled,restored,*disabled.find(2),&next)&&next==roster,
        "restoring the correct ID permits an explicit re-enable, not automatic substitution");
    check(package_borg_missing(disabled,catalog,&notices)&&notices[0].saved.disabled&&
        package_borg_resolve_missing(disabled,catalog,notices[0],MissingPackageChoice::DeleteFromSave,&next)&&
        !next.find(2)&&next.find(3)&&next.placements==std::vector<PackageBorgPlacement>{{1,0,1},{3,5,4},{4,8,0}},
        "confirmed delete affects only the selected instance and removes all its references without renumbering slots");
    const auto deleted=next;

    account_save_fixture::Storage storage;AccountSaveRepository repo(storage);
    AccountSaveEntry account,save,copy;std::unique_ptr<AccountSaveBackend> selected;
    check(repo.create_account(4,"Player",&account)==AccountResult::Ok&&repo.create_save(account,8,"Game",&save)==AccountResult::Ok&&
        repo.open(save,&selected)==AccountResult::Ok,"open selected save through account lease");
    check(load_be32(storage.objects.at(save.key).payload.data()+kSaveHeaderBytes+4)==1,
        "ordinary account records still use existing version one layout");
    PackageBorgSaveStore writer(*selected),stale(*selected);
    auto loaded=deleted;auto untouched=loaded;
    check(!writer.write(roster)&&writer.load(&loaded)==PackageBorgLoad::Missing&&loaded==untouched&&
        stale.load(&loaded)==PackageBorgLoad::Missing,"legacy save reports missing extension without replacing existing data");
    CheatSettingsStore cheats(*selected);CheatSettings flags;
    check(cheats.load(&flags)==CheatSettingsLoad::Missing&&cheats.write({true,true})&&writer.write(roster)&&
        !stale.write(deleted),"append package category atomically and refuse stale store overwrite");
    check(load_be32(storage.objects.at(save.key).payload.data()+kSaveHeaderBytes+4)==2&&
        writer.write(disabled)&&repo.save_as(9,"Copy",&copy)==AccountResult::Ok,
        "new category versions the outer record and Save As copies it");
    repo.close();selected.reset();
    check(repo.open(copy,&selected)==AccountResult::Ok,"reload copied save independently");
    PackageBorgSaveStore reloaded(*selected);CheatSettingsStore copy_cheats(*selected);
    check(reloaded.load(&loaded)==PackageBorgLoad::Loaded&&loaded==disabled&&
        copy_cheats.load(&flags)==CheatSettingsLoad::Loaded&&flags==CheatSettings{true,true},
        "last known name, disabled flag and Force slots survive reload without touching cheats");
    check(reloaded.write(deleted)&&reloaded.load(&loaded)==PackageBorgLoad::Loaded&&loaded==deleted,
        "confirmed candidate can persist atomically and reload");
    const auto before_failure=storage.objects.at(copy.key);const auto writes=storage.write_calls;
    check(!reloaded.write(malformed)&&storage.write_calls==writes&&storage.objects.at(copy.key).payload==before_failure.payload,
        "invalid candidate cannot rewrite the stored save");
    SaveWriter wrong;wrong.u32_field(123);SaveEngine framed(*selected);
    check(framed.write(kBorgPackageSaveSlot,wrong),"install wrong framed record for refusal test");
    const auto invalid_writes=storage.write_calls;
    check(reloaded.load(&loaded)==PackageBorgLoad::Invalid&&loaded==deleted&&!reloaded.write(disabled)&&
        storage.write_calls==invalid_writes,"incompatible extension refuses without corrupting prior roster or authorizing overwrite");
    std::puts("Borg package save: stable named identity, Force recovery, scope, compatibility and reload PASS");
}
