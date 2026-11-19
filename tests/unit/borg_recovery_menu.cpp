#include <cstdio>
#include <cstdlib>
#include "engine/core/headless_backends.hpp"
#include "engine/gameplay/borg_recovery_menu_view.hpp"
#include "tests/unit/account_save_fixture.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"Borg recovery menu: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    account_save_fixture::Storage storage;AccountSaveRepository repo(storage);
    AccountSaveEntry account,save;std::unique_ptr<AccountSaveBackend> chosen;
    check(repo.create_account(1,"Player",&account)==AccountResult::Ok&&repo.create_save(account,2,"Named Save",&save)==AccountResult::Ok&&
        repo.open(save,&chosen)==AccountResult::Ok,"explicit selected save");
    PackageBorgSaveStore seed(*chosen);PackageBorgRoster discarded;
    PackageBorgRoster original{{
        {8,{"lab:ant","Clockwork Ant"},false,SavedPackageVariant{"silver","Silver Edition"}},
        {9,{"lab:bee","Bee"}},
    },{{8,0,1},{9,0,2},{8,4,8}}};
    check(seed.load(&discarded)==PackageBorgLoad::Missing&&seed.write(original),"test save seeding");
    BorgPackageCatalog packages;packages.status=BorgPackageScanStatus::NoDirectory;
    BorgPackageRecoverySession session(*chosen);BorgRecoveryMenu menu;
    check(session.open(packages)==BorgRecoveryOpen::Ready&&session.notices().size()==2,"two named notices");
    const auto writes=storage.write_calls;
    auto tap=[&](const GamePadState& pad){
        menu.step({},session);
        return menu.step(pad,session);
    };
    GamePadState accept,down,back;accept.a=true;down.dpad_down=true;back.b=true;
    check(tap(accept)==BorgRecoveryMenuRequest::None&&session.phase()==BorgRecoveryPhase::Notice&&
        session.selected()->saved.variant->label=="Silver Edition"&&session.selected()->affected.size()==2&&
        menu.action_index()==0&&menu.step(accept,session)==BorgRecoveryMenuRequest::None,
        "opening a named notice does not turn a held button into recovery consent");
    check(tap(accept)==BorgRecoveryMenuRequest::None&&session.phase()==BorgRecoveryPhase::List&&
        storage.write_calls==writes,"default notice action cancels");
    tap(accept);tap(down);tap(down);tap(accept);
    check(session.phase()==BorgRecoveryPhase::ConfirmDelete&&menu.action_index()==0&&
        tap(accept)==BorgRecoveryMenuRequest::None&&session.phase()==BorgRecoveryPhase::Notice&&storage.write_calls==writes,
        "default delete confirmation also cancels");
    tap(back);tap(accept);tap(down);tap(down);tap(accept);
    check(session.phase()==BorgRecoveryPhase::ConfirmDelete,"explicitly enter destructive confirmation again");
    GamePadState together=down;together.a=true;
    check(tap(together)==BorgRecoveryMenuRequest::None&&menu.action_index()==1&&storage.write_calls==writes,
        "navigation edge cannot double as confirmation");
    check(tap(accept)==BorgRecoveryMenuRequest::Confirm&&storage.write_calls==writes,"menu emits a request, not a write");
    headless::Render backend;RenderEngine render(backend);std::string error;
    check(render.initialize({640,480,false,true,"Recovery test"},nullptr,&error),"recovery uses shared render backend");
    BorgRecoveryMenuView view;render.begin_frame({0,.05f,.07f,.10f});render.reset_arena();
    const auto draws=view.draw(render,640,480,session,menu,"Save: Named Save","",false);render.end_frame();
    check(draws>0,"actual menu view draws the confirmation");
    check(session.confirm(packages)==BorgRecoveryApply::Saved&&!session.roster()->find(8)&&session.roster()->find(9)&&
        session.roster()->placements==std::vector<PackageBorgPlacement>{{9,0,2}}&&storage.write_calls==writes+1,
        "only the scoped recovery session commits the separately confirmed loss");
    std::puts("Borg recovery menu: named notices, cancellation-first confirmation, and shared rendering PASS");
}
