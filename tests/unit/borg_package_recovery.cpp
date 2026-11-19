#include <cstdio>
#include <cstdlib>
#include "engine/save/borg_package_recovery.hpp"
#include "tests/unit/account_save_fixture.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"Borg recovery session: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    account_save_fixture::Storage storage;AccountSaveRepository repo(storage);
    AccountSaveEntry account,save,other;std::unique_ptr<AccountSaveBackend> selected;
    check(repo.create_account(5,"Player",&account)==AccountResult::Ok&&repo.create_save(account,10,"Selected",&save)==AccountResult::Ok&&
        repo.create_save(account,11,"Other",&other)==AccountResult::Ok&&repo.open(save,&selected)==AccountResult::Ok,
        "explicit selected account/save fixture");
    const auto untouched=storage.objects.at(other.key);
    BorgPackageRecoverySession session(*selected);
    BorgPackageCatalog catalog;catalog.status=BorgPackageScanStatus::Complete;
    catalog.discovered.push_back({"ant",{{"lab:ant","Ant"},{},{},{},{}}});
    catalog.issues.push_back({"bee-a","lab:bee",BorgPackageIssueKind::DuplicateId});
    BorgCatalogSnapshot snapshot;snapshot.package=catalog.discovered[0];
    const std::string normal="GFBORG-CATALOG 1\ndescription=Test Borg.\ntribe=none\ndefault-variant=normal\n"
        "variant=normal|Normal|22|unresolved\n";
    check(borg_catalog_read_profile(normal,&snapshot.profile),"parsed metadata independent of unresolved art");
    const std::span<const BorgCatalogSnapshot> metadata(&snapshot,1);
    auto failed=catalog;failed.status=BorgPackageScanStatus::Unavailable;
    const auto create_writes=storage.write_calls;
    check(session.open(failed)==BorgRecoveryOpen::ScanUnavailable&&session.open(catalog,metadata)==BorgRecoveryOpen::NoRoster&&
        session.confirm(catalog,metadata)==BorgRecoveryApply::NotConfirmed&&storage.write_calls==create_writes,
        "absent roster or unavailable scan cannot initialize new inventory");
    const PackageBorgRoster original{{
        {1,{"lab:ant","Clockwork Ant"},false,SavedPackageVariant{"silver","Silver Edition"}},
        {2,{"lab:crane","Clockwork Crane"}},
        {3,{"lab:bee","Bee"}},
    },{{1,0,0},{2,0,1},{2,5,3}}};
    PackageBorgSaveStore seed(*selected);PackageBorgRoster ignored;
    check(seed.load(&ignored)==PackageBorgLoad::Missing&&seed.write(original),"install owned metadata in selected fixture only");
    const auto first_save=storage.objects.at(save.key);const auto initial_writes=storage.write_calls;
    check(session.open(catalog,metadata)==BorgRecoveryOpen::Ready&&session.notices().size()==3&&
        session.phase()==BorgRecoveryPhase::List&&storage.write_calls==initial_writes,"opening lists notices without writes");
    check(session.inspect(3)&&!session.choose(MissingPackageChoice::DeleteFromSave)&&session.cancel(),
        "conflict details cannot authorize deletion");
    check(session.inspect(1)&&session.selected()->saved.variant->label=="Silver Edition"&&
        session.choose(MissingPackageChoice::DeleteFromSave)&&session.phase()==BorgRecoveryPhase::ConfirmDelete&&
        session.cancel()&&session.phase()==BorgRecoveryPhase::Notice&&session.cancel()&&!session.selected()&&
        storage.write_calls==initial_writes&&storage.objects.at(save.key).payload==first_save.payload,
        "two steps may be cancelled without any write");
    check(session.inspect(1)&&session.choose(MissingPackageChoice::DeleteFromSave),"explicit delete request");
    auto restored=snapshot;
    check(borg_catalog_read_profile(normal+"variant=silver|Renamed Silver|22|unresolved\n",&restored.profile)&&
        session.confirm(catalog,{&restored,1})==BorgRecoveryApply::StaleNotice&&session.phase()==BorgRecoveryPhase::NeedsReload&&
        *session.roster()==original&&storage.write_calls==initial_writes,
        "reappearing exact variant invalidates a pending confirmation");
    check(session.open(catalog,metadata)==BorgRecoveryOpen::Ready&&session.inspect(1)&&
        session.choose(MissingPackageChoice::DisableInForces)&&session.phase()==BorgRecoveryPhase::ConfirmDisable&&
        session.confirm(catalog,metadata)==BorgRecoveryApply::Saved&&session.roster()->find(1)->disabled&&
        session.roster()->placements==original.placements,"confirmed disable preserves the exact variant and all Force positions");
    const auto disabled=*session.roster();const auto after_disable=storage.objects.at(save.key);const auto disable_writes=storage.write_calls;
    check(session.inspect(1)&&session.choose(MissingPackageChoice::DisableInForces)&&
        session.confirm(catalog,metadata)==BorgRecoveryApply::Unchanged&&storage.write_calls==disable_writes,
        "already disabled with a current save is a no-op");
    check(session.inspect(2)&&session.selected()->affected==std::vector<PackageBorgPlacement>{{2,0,1},{2,5,3}}&&
        session.choose(MissingPackageChoice::DeleteFromSave),"one missing package can name every affected Force position");
    storage.next_write=AccountWrite::Error;
    check(session.confirm(catalog,metadata)==BorgRecoveryApply::SaveUnconfirmed&&
        session.phase()==BorgRecoveryPhase::NeedsReload&&*session.roster()==disabled&&
        storage.objects.at(save.key).payload==after_disable.payload,"failed write preserves last accepted display and stored roster");
    const auto failed_writes=storage.write_calls;
    check(session.confirm(catalog,metadata)==BorgRecoveryApply::NotConfirmed&&storage.write_calls==failed_writes,
        "a failed confirmation is never retried in place");
    check(session.open(catalog,metadata)==BorgRecoveryOpen::Ready&&session.inspect(2)&&
        session.choose(MissingPackageChoice::DeleteFromSave)&&session.confirm(catalog,metadata)==BorgRecoveryApply::Saved&&
        !session.roster()->find(2)&&session.roster()->find(1)->disabled&&session.roster()->find(3)&&
        session.roster()->placements==std::vector<PackageBorgPlacement>{{1,0,0}},"acknowledged deletion removes only the selected instance and its positions");
    const auto last_accepted=*session.roster();
    check(session.inspect(1)&&session.choose(MissingPackageChoice::DeleteFromSave),"another explicit delete request");
    storage.next_write=AccountWrite::Uncertain;storage.commit_uncertain=true;
    check(session.confirm(catalog,metadata)==BorgRecoveryApply::SaveUnconfirmed&&*session.roster()==last_accepted&&
        session.phase()==BorgRecoveryPhase::NeedsReload,"uncertain persistence does not publish a new local roster");
    const auto uncertain_writes=storage.write_calls;
    check(session.confirm(catalog,metadata)==BorgRecoveryApply::NotConfirmed&&storage.write_calls==uncertain_writes&&
        storage.objects.at(other.key).revision==untouched.revision&&storage.objects.at(other.key).payload==untouched.payload,
        "unconfirmed writes cannot replay or touch a different save");
    std::puts("Borg recovery session: named notices, cancellation, confirmed edits and stale/refused writes PASS");
}
