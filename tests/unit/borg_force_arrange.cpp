#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borg_force_arrange.hpp"
#include "tests/unit/account_save_fixture.hpp"
#include "tests/unit/force_inventory_fixture.hpp"

namespace {
using namespace gf;
using R=BorgPlacementResult;
using Open=BorgForceArrangeOpen;
using State=BorgForceArrangeState;
using Request=BorgPlacementRequest;
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"portable Force arrangement: %s\n",why);std::abort();}
}
BorgCatalogSnapshot snapshot(const BorgPackageEntry& entry,std::string_view variants){
    BorgCatalogSnapshot next;next.package=entry;
    check(borg_catalog_read_profile("GFBORG-CATALOG 1\ndescription=Example\ntribe=none\n"
        "default-variant=normal\n"+std::string(variants),&next.profile),"fixture catalogue profile");
    return next;
}
void choose_confirm(BorgForceArrange& arrange){
    GamePadState accept,down;accept.a=true;down.dpad_down=true;
    check(arrange.step_confirmation({})==Request::None&&arrange.step_confirmation(down)==Request::None&&
        arrange.confirm_selected()&&arrange.step_confirmation({})==Request::None&&
        arrange.step_confirmation(accept)==Request::Confirm,"explicit separate confirmation edge");
}
}

int main(){
    using namespace gf;
    account_save_fixture::Storage storage;AccountSaveRepository repo(storage);
    AccountSaveEntry account,save,other;std::unique_ptr<AccountSaveBackend> backend;
    check(repo.create_account(6,"Player",&account)==AccountResult::Ok&&repo.create_save(account,7,"Selected",&save)==AccountResult::Ok&&
        repo.create_save(account,8,"Other",&other)==AccountResult::Ok&&repo.open(save,&backend)==AccountResult::Ok,"selected save");
    const auto retail_catalogue=force_inventory_fixture::catalog();ForceInventoryStore legacy(*backend);ForceInventory retail;
    check(legacy.load(retail_catalogue,&retail)==ForceInventoryLoad::Missing&&retail.create(retail_catalogue)&&
        retail.rename_force(0,"Saved Team")==ForceChange::Applied&&legacy.write(retail),"real retail record with name");
    const PackageBorgRoster original{{
        {11,{"lab:ant","Saved Ant"},false,SavedPackageVariant{"silver","Saved Silver"}},
        {22,{"lab:ant","Another Saved Ant"},false,SavedPackageVariant{"normal","Saved Normal"}},
        {33,{"lab:bug","Saved Bug"},false,SavedPackageVariant{"normal","Saved Bug Variant"}},
        {44,{"lab:lost","Disabled Missing Copy"},true,SavedPackageVariant{"gold","Saved Gold"}},
        {55,{"lab:lost","Active Missing Copy"},false,SavedPackageVariant{"gold","Saved Gold"}},
        {66,{"lab:ant","Old Unspecified Copy"}},
    },{{33,0,3},{44,0,7},{55,1,0},{22,2,0}}};
    PackageBorgSaveStore roster_store(*backend);PackageBorgRoster readback;
    check(roster_store.load(&readback)==PackageBorgLoad::Missing&&roster_store.write(original),"seed package roster");
    BorgPackageCatalog packages;packages.status=BorgPackageScanStatus::Complete;
    const auto entry=[](std::string folder,std::string id,std::string name){
        BorgPackageEntry package;package.folder=std::move(folder);
        package.manifest.identity={std::move(id),std::move(name)};package.manifest.catalog="catalog.txt";
        return package;
    };
    packages.discovered={entry("ant","lab:ant","Current Ant"),entry("bug","lab:bug","Current Bug")};
    const auto ant=snapshot(packages.discovered[0],"variant=normal|Current Normal|98|placeholder\n"
                                                  "variant=silver|Current Silver|225|unresolved\n");
    const auto bug=snapshot(packages.discovered[1],"variant=normal|Current Bug Variant|103|placeholder\n");
    const std::vector metadata{ant,bug};
    const BorgPlacementPolicy budget{284,ForceCostMode::TwoThirds,false};
    BorgForceArrange arrange(*backend);GamePadState accept,back;accept.a=true;back.b=true;
    auto open=[&](const BorgPlacementPolicy& policy=BorgPlacementPolicy{}){
        return arrange.open(0,1,&retail_catalogue,packages,metadata,policy);
    };
    const auto other_before=storage.objects.at(other.key);
    const auto writes=storage.write_calls;
    check(arrange.open(0,1,nullptr,packages,metadata)==Open::RetailUnverified&&arrange.state()==State::Closed&&
        open(budget)==Open::Ready&&arrange.save_revision()->key()==save.key&&arrange.force_view()->retail_source_name=="Saved Team"&&
        arrange.force_view()->find_slot(0)->retail&&arrange.force_view()->find_slot(3)->packaged&&
        arrange.force_view()->find_slot(7)->packaged->saved.disabled&&!arrange.force_view()->find_slot(1),"mixed view retains claims");
    const auto owned=arrange.owned_rows();
    check(owned.size()==6&&owned[0].saved.identity.name=="Saved Ant"&&owned[0].saved.variant->label=="Saved Silver"&&
        owned[0].installed->variant_label=="Current Silver"&&owned[1].installed->key->variant_id=="normal"&&
        owned[3].saved.disabled&&owned[3].state==BorgLibraryState::PackageMissing&&owned[5].state==BorgLibraryState::VariantUnspecified,
        "instance-keyed library rows preserve exact and missing/disabled saved data");
    check(arrange.select_owned(44)&&arrange.stage()==R::Disabled&&arrange.select_owned(55)&&arrange.stage()==R::VariantUnavailable&&
        arrange.select_owned(66)&&arrange.stage()==R::VariantUnavailable&&!arrange.select_owned(99)&&!arrange.offer(),
        "unavailable ownership remains inspectable but not arrangeable");
    check(arrange.select_owned(11)&&arrange.select_position(1,1)&&arrange.stage()==R::CostUnverified&&
        arrange.select_position(0,0)&&arrange.stage()==R::Occupied&&arrange.select_position(0,7)&&arrange.stage()==R::Occupied&&
        storage.write_calls==writes,"unresolved active costs and occupied slots refuse; disabled still reserves its slot");
    BorgPlacementPolicy low=budget;low.budget=283;
    check(open(low)==Open::Ready&&arrange.select_owned(11)&&arrange.stage()==R::OverBudget&&storage.write_calls==writes,
        "mode budget enforced with per-member rounding");
    check(open(budget)==Open::Ready&&arrange.select_owned(11)&&arrange.stage(accept)==R::Ready&&arrange.offer()&&
        arrange.offer()->retail_force_name=="Saved Team"&&arrange.offer()->owned==original.owned[0]&&
        arrange.offer()->force==0&&arrange.offer()->slot==1&&arrange.offer()->declared_cost==225&&
        arrange.offer()->before_cost==134&&arrange.offer()->after_cost==284,"offer shows exact saved copy and mixed cost basis");
    check(!arrange.select_position(2,4)&&!arrange.select_owned(22)&&arrange.step_confirmation(accept)==Request::None&&
        arrange.confirm(&retail_catalogue,packages,metadata,budget)==R::NotConfirmed&&storage.write_calls==writes,
        "modal cannot retarget and an open/held input is not consent");
    check(arrange.step_confirmation(back)==Request::Cancel&&!arrange.offer()&&arrange.state()==State::Browse&&
        arrange.confirm(&retail_catalogue,packages,metadata,budget)==R::NotConfirmed&&storage.write_calls==writes,"cancel consumes");
    auto changed=metadata;
    changed[1].profile.variants[0].force_cost=102; // Same 2/3 rounded cost as 103.
    check(arrange.stage()==R::Ready,"new explicit proposal");
    choose_confirm(arrange);
    check(arrange.confirm(&retail_catalogue,packages,changed,budget)==R::Stale&&!arrange.offer()&&!arrange.force_view()&&
        arrange.state()==State::ReopenRequired&&arrange.confirm(&retail_catalogue,packages,metadata,budget)==R::NotConfirmed&&
        storage.write_calls==writes,"any other active variant cost change consumes and locks the proposal");
    check(open(budget)==Open::Ready&&arrange.select_owned(11)&&arrange.stage()==R::Ready,"reopen for another proposal");
    choose_confirm(arrange);
    check(arrange.confirm(&retail_catalogue,packages,metadata,{285,ForceCostMode::TwoThirds,false})==R::Stale&&
        storage.write_calls==writes,"changed mode policy cannot broaden earlier consent");
    check(open(budget)==Open::Ready&&arrange.select_owned(11)&&arrange.stage()==R::Ready,"reopen after policy change");
    choose_confirm(arrange);
    const auto rev_before=*arrange.save_revision();
    // A same-lease write of identical bytes advances the save revision, even
    // though neither Force record changes.
    CheatSettingsStore options(*backend);CheatSettings flags;
    check(options.load(&flags)==CheatSettingsLoad::Missing&&options.write({})&&options.write({}),"same-lease revision changes");
    check(arrange.confirm(&retail_catalogue,packages,metadata,budget)==R::Stale&&arrange.state()==State::ReopenRequired&&
        !arrange.save_revision()&&storage.write_calls==writes+2&&backend->revision()!=rev_before,"revision change invalidates the displayed snapshot");
    check(open(budget)==Open::Ready&&arrange.select_owned(11)&&arrange.stage()==R::Ready,"explicit fresh proposal");
    choose_confirm(arrange);
    std::vector<u8> retail_bytes,after;
    check(backend->load(0,retail_bytes)&&arrange.confirm(&retail_catalogue,packages,metadata,budget)==R::Saved&&
        arrange.state()==State::Browse&&arrange.selected_instance()==11&&arrange.slot()==1&&
        arrange.force_view()->find_slot(1)->packaged->saved==original.owned[0]&&
        backend->load(0,after)&&after==retail_bytes&&roster_store.load(&readback)==PackageBorgLoad::Loaded&&
        readback.owned==original.owned&&readback.placements.size()==original.placements.size()+1&&
        readback.placements[0]==PackageBorgPlacement{11,0,1}&&storage.write_calls==writes+3,"one acknowledged placement, exact records preserved");
    check(storage.objects.at(other.key).revision==other_before.revision&&storage.objects.at(other.key).payload==other_before.payload,
        "other save was never touched");

    check(retail.place(2,0,0)==ForceChange::Applied&&legacy.write(retail)&&
        open()==Open::Ready&&arrange.select_owned(11)&&arrange.select_position(2,1)&&arrange.force_view()->find_slot(0)->overlap()&&
        arrange.stage()==R::ForceConflict,"existing overlap is visible and blocks arrangement elsewhere in the same Force");
    check(retail.edit_borg({false,true},false,0,retail.revision(),BorgEditField::Cost,200)==ForceChange::Applied&&legacy.write(retail)&&
        open()==Open::Ready&&arrange.select_position(0,2)&&arrange.select_owned(22)&&arrange.stage()==R::EditedRetail,
        "default arrangement policy does not overlook existing edited retail members");

    check(open()==Open::Ready&&arrange.select_position(3,1)&&arrange.select_owned(11)&&arrange.stage()==R::Ready,
        "new Force with verified empty position");
    choose_confirm(arrange);
    const auto attempt=storage.write_calls;
    storage.next_write=AccountWrite::Uncertain;storage.commit_uncertain=true;
    check(arrange.confirm(&retail_catalogue,packages,metadata)==R::SaveUnconfirmed&&arrange.state()==State::ReopenRequired&&
        arrange.confirm(&retail_catalogue,packages,metadata)==R::NotConfirmed&&!backend->current()&&
        storage.write_calls==attempt+1,"uncertain write is attempted once, even when storage actually committed");
    check(open()==Open::SaveUnavailable,"do not reuse a failed selected-save lease");
    std::puts("Portable Force arrangement: exact owned slot, mixed budgets, cancellation and revision-guarded commit PASS");
}
