#include <cstdio>
#include <cstdlib>
#include "engine/core/headless_backends.hpp"
#include "engine/gameplay/borg_installed_browser_view.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"Owned Borg picker: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    BorgPackageCatalog catalog;catalog.status=BorgPackageScanStatus::Complete;
    catalog.discovered={{"renamed",{{"test:ant","New Name"},{},{},{},{}}},
        {"unowned",{{"test:unowned","Not Owned"},{},{},{},{}}}};
    BorgCatalogSnapshot metadata;metadata.package=catalog.discovered.front();
    check(borg_catalog_read_profile("GFBORG-CATALOG 1\ndescription=Test.\ntribe=none\ndefault-variant=normal\n"
        "variant=normal|Normal|180|unresolved\nvariant=silver|New Silver|225|unresolved\n",&metadata.profile),"real profile");
    const PackageBorgRoster roster{{
        {3,{"test:ant","Old Name"},false,SavedPackageVariant{"silver","Old Silver"}},
        {7,{"test:ant","Old Name"},true,SavedPackageVariant{"silver","Old Silver"}},
        {8,{"test:ant","Bronze Ant"},false,SavedPackageVariant{"bronze","Bronze"}},
        {9,{"test:ant","Legacy Ant"}},
        {10,{"test:absent","Missing Borg"},false,SavedPackageVariant{"normal","Normal"}},
    },{{3,0,1},{3,2,4}}};
    BorgInstalledBrowser browser;
    check(browser.open_owned(catalog,roster)&&browser.size()==5&&browser.owned_entry()->instance==3&&
        browser.owned_entry()->identity.name=="Old Name"&&browser.owned_entry()->variant->label=="Old Silver",
        "each saved copy is exposed, not all installed packages or live names");
    check(browser.bind(metadata)&&browser.selection()->variant=="silver"&&browser.owned_status()=="EXACT VARIANT RESOLVED",
        "folder and names may change, but only the saved variant ID binds");
    GamePadState right,down,x;right.dpad_right=down.dpad_down=x.x=true;
    auto tap=[&](const GamePadState& pad){browser.step({});return browser.step(pad);};
    check(tap(right)==BorgBrowserChange::None&&tap(x)==BorgBrowserChange::Preview&&browser.selection()->variant=="silver",
        "owned variants cannot cycle; preview request is exact");
    GamePadState q;q.y=true;
    check(tap(q)==BorgBrowserChange::None,"practice is not a default picker action");
    browser.step({},true);
    auto route_and_launch=q;route_and_launch.dpad_left=true;
    check(browser.practice_route()==GRedPickerPracticeRoute::Standard&&
        browser.step(route_and_launch,true)==BorgBrowserChange::PracticeRoute&&
        browser.practice_route()==GRedPickerPracticeRoute::ExpiryPush&&
        browser.step(route_and_launch,true)==BorgBrowserChange::None&&
        browser.selection()->variant=="silver"&&browser.owned_entry()->instance==3,
        "route wraps separately from the owned variant and wins over a simultaneous launch");
    browser.step({},true);
    check(browser.step(route_and_launch,true)==BorgBrowserChange::PracticeRoute&&
        browser.practice_route()==GRedPickerPracticeRoute::ExpiryAirDash,"second Left reaches the verified expiry air dash");
    browser.step({},true);
    check(browser.step(route_and_launch,true)==BorgBrowserChange::PracticeRoute&&
        browser.practice_route()==GRedPickerPracticeRoute::ExpiryHandoff,"third Left reaches the expiry handoff");
    browser.step({},true);
    check(browser.step(route_and_launch,true)==BorgBrowserChange::PracticeRoute&&
        browser.practice_route()==GRedPickerPracticeRoute::TimedHandoff&&browser.owned_entry()->instance==3,
        "fourth Left reaches the timed handoff without changing the exact copy");
    browser.step({},true);
    check(browser.step(right,true)==BorgBrowserChange::PracticeRoute&&browser.practice_route()==GRedPickerPracticeRoute::ExpiryHandoff,
        "Right returns to the expiry handoff");
    browser.step({},true);
    check(browser.step(right,true)==BorgBrowserChange::PracticeRoute&&browser.practice_route()==GRedPickerPracticeRoute::ExpiryAirDash,
        "Right returns to the earlier expiry air dash");
    browser.step({},true);
    check(browser.step(right,true)==BorgBrowserChange::PracticeRoute&&browser.practice_route()==GRedPickerPracticeRoute::ExpiryPush,
        "Right returns to the expiry push route");
    browser.step({},true);
    check(browser.step(q,true)==BorgBrowserChange::Practice&&browser.step(q,true)==BorgBrowserChange::None&&
        browser.owned_entry()->instance==3,"an opted-in practice request is a single edge on the exact copy");
    browser.step({},true);
    auto together=q;together.x=true;
    check(browser.step(together,true)==BorgBrowserChange::Preview,"a simultaneous preview never launches practice");
    browser.sync_input(down);
    check(browser.step(down)==BorgBrowserChange::None&&browser.owned_entry()->instance==3&&
        browser.practice_route()==GRedPickerPracticeRoute::ExpiryPush,
        "held navigation after preview cannot change the owned copy or its route");
    browser.step({},true);
    check(browser.step(right,true)==BorgBrowserChange::PracticeRoute&&browser.practice_route()==GRedPickerPracticeRoute::Standard,
        "Right wraps expiry push back to Standard");
    browser.step({},true);
    check(browser.step(right,true)==BorgBrowserChange::PracticeRoute&&browser.practice_route()==GRedPickerPracticeRoute::RecoverySeam&&
        browser.owned_entry()->instance==3&&browser.selection()->variant=="silver",
        "Recovery Seam selects a route, not a different copy or a different variant");
    browser.step({},true);
    check(browser.step(right,true)==BorgBrowserChange::PracticeRoute&&
        browser.practice_route()==GRedPickerPracticeRoute::TimedRelaunchFloor&&browser.practice_route_name()=="TIMED RELAUNCH FLOOR"&&
        browser.step(right,true)==BorgBrowserChange::None,
        "second Right reaches the timed floor guide once per press");
    browser.step({},true);
    check(browser.step(right,true)==BorgBrowserChange::PracticeRoute&&
        browser.practice_route()==GRedPickerPracticeRoute::InclineDashJump&&browser.practice_route_name()=="INCLINE DASH JUMP"&&
        browser.owned_entry()->instance==3&&browser.selection()->variant=="silver",
        "third Right selects the incline jump without changing saved contents");
    check(tap(down)==BorgBrowserChange::Package&&browser.owned_entry()->instance==7&&!browser.selection()&&
        browser.bind(metadata)&&browser.variant()&&!browser.selection()&&tap(x)==BorgBrowserChange::None,
        "disabled copy stays visible but cannot request preview");
    check(tap(down)==BorgBrowserChange::Package&&browser.bind(metadata)&&!browser.selection()&&
        browser.owned_status()=="SAVED VARIANT MISSING","missing variant is never replaced by default");
    check(tap(down)==BorgBrowserChange::Package&&browser.bind(metadata)&&!browser.selection()&&
        browser.owned_status()=="SAVED VARIANT UNSPECIFIED","legacy unspecified path is separate");
    check(tap(down)==BorgBrowserChange::Package&&!browser.entry()&&!browser.bind(metadata)&&
        browser.owned_status()=="PACKAGE MISSING"&&!browser.open_owned(catalog,roster,99)&&browser.owned_entry()->instance==10,
        "missing package stays visible, and invalid initial selection does not mutate the browser");
    headless::Render backend;RenderEngine render(backend);std::string error;
    check(render.initialize({640,480,false,true,"Owned picker"},nullptr,&error),"shared render backend");
    BorgInstalledBrowserView view;
    render.begin_frame({0,.05f,.07f,.09f});render.reset_arena();
    const auto draws=view.draw(render,640,480,browser,{},false,false,"Fixture Save");render.end_frame();
    check(draws>0,"same view renders unavailable owned copies");
    catalog.status=BorgPackageScanStatus::EntryBudget;
    check(browser.open_owned(catalog,roster,7)&&browser.owned_entry()->instance==7&&browser.size()==5&&!browser.entry()&&
        browser.owned_status()=="PACKAGE SCAN UNAVAILABLE","failed scan doesn't hide ownership or fabricate absence");
    check(browser.open_owned(catalog,{})&&browser.owned()&&!browser.size(),"empty selected roster remains an owned view");
    check(browser.step(q,true)==BorgBrowserChange::None,"an empty owned view cannot request practice");
    browser.open(catalog);
    check(browser.step(q,true)==BorgBrowserChange::None,"an installed-only picker cannot request owned practice");
    std::puts("Owned Borg picker: exact variants, distinct copies, unavailable states and shared rendering PASS");
}
