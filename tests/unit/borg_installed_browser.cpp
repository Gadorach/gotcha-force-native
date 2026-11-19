#include <cstdio>
#include <cstdlib>
#include "engine/core/headless_backends.hpp"
#include "engine/gameplay/borg_installed_browser_view.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"Installed Borg picker: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    BorgPackageCatalog scanned;scanned.status=BorgPackageScanStatus::Complete;
    scanned.discovered={{"alpha",{{"example:alpha","Clockwork Ant"},{},{},{},{}}},
                        {"beta",{{"example:beta","Beta Borg"},{},{},{},{}}}};
    BorgCatalogSnapshot metadata;
    metadata.package=scanned.discovered[0];
    check(borg_catalog_read_profile("GFBORG-CATALOG 1\ndescription=Test.\ntribe=none\ndefault-variant=normal\n"
        "variant=bronze|Bronze|220|unresolved\nvariant=normal|Standard|240|placeholder\n",&metadata.profile),"real catalogue parser");
    BorgInstalledBrowser browser;browser.open(scanned);
    check(browser.entry()&&browser.entry()->manifest.identity.id=="example:alpha"&&!browser.selection(),
        "discovery is visible without inventing variant metadata");
    check(browser.bind(metadata)&&browser.selection()->variant=="normal"&&browser.variant_index()==1,
        "default comes from the package, not scan or variant order");
    GamePadState pad;pad.dpad_right=true;
    check(browser.step(pad)==BorgBrowserChange::Variant&&browser.selection()->variant=="bronze"&&
        browser.step(pad)==BorgBrowserChange::None,"edge navigation wraps and held input does not auto-repeat");
    browser.step({});pad={};pad.a=true;
    check(browser.step(pad)==BorgBrowserChange::Confirm&&browser.selection()->label=="Bronze"&&
        browser.step(pad)==BorgBrowserChange::None,"confirmation yields an exact read-only selection");
    browser.step({});pad={};pad.dpad_down=true;pad.a=true;
    check(browser.step(pad)==BorgBrowserChange::Package&&browser.entry()->manifest.identity.id=="example:beta"&&
        !browser.variant()&&!browser.selection()&&!browser.bind(metadata),"package movement wins over confirm and clears stale metadata");
    browser.step({});pad={};pad.a=true;
    check(browser.step(pad)==BorgBrowserChange::None&&!browser.selection(),"a discovery-only entry cannot confirm a guessed variant");
    headless::Render device;RenderEngine render(device);std::string error;
    check(render.initialize({640,480,false,true,"Installed Borg picker"},nullptr,&error),"shared renderer initializes");
    BorgInstalledBrowserView view;
    render.begin_frame({0,.05f,.07f,.09f});render.reset_arena();
    const auto unavailable=view.draw(render,640,480,browser,{},false);render.end_frame();
    check(unavailable>0,"metadata-unavailable entries remain visible");
    auto failed=scanned;failed.status=BorgPackageScanStatus::EntryBudget;
    browser.open(std::move(failed));check(!browser.entry()&&!browser.selection(),"failed scan never exposes a partial picker roster");
    render.begin_frame({1,.05f,.07f,.09f});render.reset_arena();
    const auto refused=view.draw(render,640,480,browser,{},false);render.end_frame();
    check(refused>0,"failed scan still draws its state without inventing entries");
    std::puts("Installed Borg picker: parsed identities, variant navigation, stale rejection and rendering PASS");
}
