#include <cstdio>
#include <cstdlib>
#include "engine/core/headless_backends.hpp"
#include "engine/gameplay/borg_installed_browser_view.hpp"
#include "engine/gameplay/borg_placement_confirmation.hpp"
#include "engine/save/borg_package_placement.hpp"
#include "tests/unit/account_save_fixture.hpp"
#include "tests/unit/force_inventory_fixture.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"Borg package placement: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    using R=BorgPlacementResult;
    account_save_fixture::Storage storage;AccountSaveRepository repo(storage);
    AccountSaveEntry account,save,other;std::unique_ptr<AccountSaveBackend> backend;
    check(repo.create_account(6,"Player",&account)==AccountResult::Ok&&repo.create_save(account,7,"Selected",&save)==AccountResult::Ok&&
        repo.create_save(account,8,"Other",&other)==AccountResult::Ok&&repo.open(save,&backend)==AccountResult::Ok,
        "selected fixture save, no other context");
    auto retail_catalogue=force_inventory_fixture::catalog();ForceInventoryStore legacy(*backend);ForceInventory retail;
    check(legacy.load(retail_catalogue,&retail)==ForceInventoryLoad::Missing&&retail.create(retail_catalogue)&&legacy.write(retail),
        "real retail slot zero written to selected fixture");
    const PackageBorgRoster original{{
        {1,{"lab:ant","Clockwork Ant"},false,SavedPackageVariant{"silver","Silver Ant"}},
        {2,{"lab:ant","Second Ant"},false,SavedPackageVariant{"silver","Silver Ant"}},
        {3,{"lab:ant","Disabled Ant"},true,SavedPackageVariant{"silver","Silver Ant"}},
        {4,{"lab:ant","Legacy Ant"}},
    },{{2,0,3}}};
    PackageBorgSaveStore seed(*backend);PackageBorgRoster ignored;
    check(seed.load(&ignored)==PackageBorgLoad::Missing&&seed.write(original),"owned fixture instances");
    BorgPackageCatalog catalog;catalog.status=BorgPackageScanStatus::Complete;
    catalog.discovered.push_back({"ant",{{"lab:ant","Clockwork Ant"},{},{},{},{}}});
    BorgCatalogSnapshot ant;ant.package=catalog.discovered[0];
    const std::string prefix="GFBORG-CATALOG 1\ndescription=Test.\ntribe=none\ndefault-variant=normal\n"
        "variant=normal|Normal|100|unresolved\n";
    check(borg_catalog_read_profile(prefix+"variant=silver|Silver|225|unresolved\n",&ant.profile),"known variant metadata");
    const std::span<const BorgCatalogSnapshot> metadata(&ant,1);
    BorgPackagePlacementSession placement(*backend);
    const auto writes=storage.write_calls;const auto other_state=storage.objects.at(other.key);
    check(placement.prepare(1,0,0,&retail_catalogue,catalog,metadata)==R::Occupied&&
        placement.prepare(1,0,3,&retail_catalogue,catalog,metadata)==R::Occupied&&
        placement.prepare(2,0,1,&retail_catalogue,catalog,metadata)==R::AlreadyMember&&
        placement.prepare(1,0,1,nullptr,catalog,metadata)==R::RetailUnverified&&
        placement.prepare(3,0,1,&retail_catalogue,catalog,metadata)==R::Disabled&&
        placement.prepare(4,0,1,&retail_catalogue,catalog,metadata)==R::VariantUnavailable&&
        placement.prepare(99,0,1,&retail_catalogue,catalog,metadata)==R::OwnedUnavailable&&
        !placement.offer()&&storage.write_calls==writes,"retail/package occupancy, duplicate membership and unready instances all refuse without writes");
    auto prepare=[&](){return placement.prepare(1,0,1,&retail_catalogue,catalog,metadata);};
    check(prepare()==R::Ready&&placement.offer()->owned.variant->label=="Silver Ant"&&
        placement.offer()->declared_cost==225&&storage.write_calls==writes,"offer retains exact owned variant and position without writing");
    BorgPlacementConfirmation input;GamePadState accept,down,both,back;
    accept.a=true;down.dpad_down=true;both=down;both.a=true;back.b=back.a=true;
    using Request=BorgPlacementRequest;
    input.begin(accept);
    check(!input.confirm_selected()&&input.step(accept)==Request::None&&input.step({})==Request::None&&
        input.step(accept)==Request::Cancel,"held accept cannot advance; cancellation is first");
    input.begin(accept);input.step({});
    check(input.step(both)==Request::None&&input.confirm_selected()&&input.step(both)==Request::None&&
        input.step(back)==Request::Cancel,"navigation isn't consent and back dominates accept");
    input.begin(accept);input.step({});input.step(down);input.step({});
    check(input.step(accept)==Request::Confirm&&storage.write_calls==writes,"separate confirm edge only requests a write");
    headless::Render renderer;RenderEngine render(renderer);
    check(render.initialize({640,480,false,true,"Package Placement"},nullptr,nullptr),"shared renderer");
    BorgInstalledBrowser browser;BorgInstalledBrowserView view;
    check(browser.open_owned(catalog,original),"owned fixture for placement presentation");
    BorgPlacementPanel panel{0,1,{},&placement.offer()->owned,placement.offer()->declared_cost,false,false,false};
    render.begin_frame({0,.05f,.07f,.09f});render.reset_arena();
    const auto draws=view.draw(render,640,480,browser,{},false,false,"Selected Save",&panel);render.end_frame();
    check(draws>0,"renders named offer and cancellation-first choices");
    placement.cancel();
    check(placement.confirm(&retail_catalogue,catalog,metadata)==R::NotConfirmed&&storage.write_calls==writes,
        "cancellation destroys the offer");
    check(prepare()==R::Ready&&retail.place(0,1,1)==ForceChange::Applied&&legacy.write(retail),
        "retail owner fills the proposed slot after planning");
    auto current_writes=storage.write_calls;
    check(placement.confirm(&retail_catalogue,catalog,metadata)==R::Stale&&!placement.offer()&&
        storage.write_calls==current_writes&&seed.load(&ignored)==PackageBorgLoad::Loaded&&ignored==original,
        "changed retail bytes invalidate confirmation without overwriting either claimant");
    check(retail.remove(0,1)==ForceChange::Applied&&legacy.write(retail)&&prepare()==R::Ready,
        "explicitly empty retail slot can be planned again");
    auto changed_catalogue=ant;
    check(borg_catalog_read_profile(prefix+"variant=silver|Silver|230|unresolved\n",&changed_catalogue.profile),
         "changed exact variant metadata");
    current_writes=storage.write_calls;
    check(placement.confirm(&retail_catalogue,catalog,{&changed_catalogue,1})==R::Stale&&
        storage.write_calls==current_writes,"confirmation does not use a changed variant cost");
    check(prepare()==R::Ready,"fresh valid proposal");
    std::vector<u8> retail_before,retail_after;
    check(backend->load(0,retail_before)&&placement.confirm(&retail_catalogue,catalog,metadata)==R::Saved&&
        !placement.offer()&&backend->load(0,retail_after)&&retail_before==retail_after&&
        seed.load(&ignored)==PackageBorgLoad::Loaded&&ignored.owned==original.owned&&
        ignored.placements==std::vector<PackageBorgPlacement>{{1,0,1},{2,0,3}},
        "confirmation writes only the new package position and keeps retail bytes and ownership");
    check(storage.objects.at(other.key).revision==other_state.revision&&storage.objects.at(other.key).payload==other_state.payload,
        "different save untouched");

    repo.close();backend.reset();
    check(repo.open(other,&backend)==AccountResult::Ok,"explicitly switch to another save");
    PackageBorgSaveStore only_packages(*backend);
    check(only_packages.load(&ignored)==PackageBorgLoad::Missing&&only_packages.write(original),"fixture without retail record");
    BorgPackagePlacementSession absent(*backend);
    check(absent.prepare(1,0,1,nullptr,catalog,metadata)==R::Ready&&absent.confirm(nullptr,catalog,metadata)==R::Saved,
        "a verified absent retail record does not require a retail catalogue");
    check(absent.prepare(1,1,1,nullptr,catalog,metadata)==R::Ready,"another scoped proposal");
    storage.next_write=AccountWrite::Uncertain;storage.commit_uncertain=true;
    current_writes=storage.write_calls;
    check(absent.confirm(nullptr,catalog,metadata)==R::SaveUnconfirmed&&!absent.offer()&&
        absent.confirm(nullptr,catalog,metadata)==R::NotConfirmed&&!backend->current()&&storage.write_calls==current_writes+1,
        "unacknowledged write is not retried or treated as acknowledged even if bytes reached storage");
    std::puts("Borg placement: cancel-first input, exact ownership, stale refusal and one-shot package commit PASS");
}
