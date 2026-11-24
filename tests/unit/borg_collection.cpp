#include <cstdio>
#include <cstdlib>
#include "engine/core/headless_backends.hpp"
#include "engine/gameplay/borg_installed_browser_view.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"Borg Collection: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    BorgPackageCatalog packages;
    packages.status=BorgPackageScanStatus::Complete;
    BorgPackageManifest manifest;
    check(borg_package_read_manifest("GFBORG 2\nid=lab:ant\nname=Very Long Multiword \xE5\xB7\xA5\xE6\x88\xBF Ant\ncatalog=card.txt\n",&manifest),
        "parse a long UTF-8 name");
    BorgCatalogSnapshot ant;
    ant.package={"ant",manifest};
    check(borg_catalog_read_profile("GFBORG-CATALOG 1\ndescription=Test.\ntribe=none\ndefault-variant=normal\n"
        "variant=silver|Silver \xC3\xA9|25|unresolved\nvariant=normal|Exact Base|20|placeholder\n",&ant.profile),"parse catalogue");
    packages.discovered.push_back(ant.package);
    std::vector<BorgCatalogSnapshot> snapshots{ant};
    for(char id='a';id<='z';++id){
        auto other=ant;
        other.package.folder=std::string("folder-")+id;
        other.package.manifest.identity={std::string("test:")+id,std::string("Borg ")+id};
        packages.discovered.push_back(other.package);
        snapshots.push_back(std::move(other));
    }
    auto bad=manifest;
    bad.identity={"lab:bad","Bad Borg"};
    packages.discovered.insert(packages.discovered.begin()+1,{"bad",bad});
    const BorgLibraryCatalogIssue diagnostics[]{{"lab:bad",BorgLibraryState::CatalogInvalid}};
    BorgCollection cursor(BorgLibrary(packages,snapshots,diagnostics));
    check(cursor.size()==55&&cursor.select(1)&&cursor.installed_row(cursor.selected())->key==BorgLibraryKey{"lab:ant","normal"},
        "one selectable row per exact variant rather than one mutable package selector");
    cursor.configure(BorgCollectionSort::Name,BorgCollectionFilter::Exact);
    check(cursor.size()==54&&cursor.selected()>10&&cursor.installed_row(cursor.selected())->key==BorgLibraryKey{"lab:ant","normal"},
        "name sort and exact filter keep the selected variant, not the index");
    cursor.configure(BorgCollectionSort::Identity,BorgCollectionFilter::All);
    check(cursor.select(2)&&!cursor.installed_row(cursor.selected())->key,"keyless diagnostic row scrolls without becoming an exact key");
    cursor.configure(BorgCollectionSort::Name,BorgCollectionFilter::Exact);
    cursor.configure(BorgCollectionSort::Name,BorgCollectionFilter::All);
    check(cursor.installed_row(cursor.selected())->identity.id=="lab:bad","hidden row returns by its diagnostic package key");
    check(cursor.select(cursor.size()-1),"select last row");
    const auto preferred=cursor.installed_row(cursor.selected())->key;
    snapshots[0].profile.variants[1].label="Renamed Exact Base";
    std::reverse(snapshots[0].profile.variants.begin(),snapshots[0].profile.variants.end());
    check(cursor.refresh(BorgLibrary(packages,snapshots,diagnostics))&&cursor.installed_row(cursor.selected())->key==preferred,
        "metadata refresh retains exact key across reordered variants and label updates");

    headless::Render device;RenderEngine render(device);std::string error;
    check(render.initialize({640,480,false,true,"Borg Collection"},nullptr,&error),"shared renderer starts");
    BorgInstalledBrowser browser;BorgInstalledBrowserView view;
    check(browser.open_collection(packages,snapshots,diagnostics)&&browser.select_collection_row(35),"browser opens exact installed rows");
    const auto initial=browser.selection();
    check(initial.has_value(),"selected row has an exact preview key");
    const unsigned viewports[][4]{{640,480,640,480},{1280,720,1280,720},{2240,620,2240,620},
        {300,570,300,570},{1920,1080,960,540},{228,150,228,150}};
    unsigned frame=0;
    for(const auto& v:viewports){
        const auto layout=BorgCollectionLayout::fit(v[0],v[1],v[2],v[3]);
        const auto& list=*browser.collection();
        const auto [first,last]=list.visible_range(layout.rows);
        check(layout.rows&&first<=list.selected()&&list.selected()<last&&last-first<=layout.rows,"selected row is visible after resize");
        check(layout.scale==(v[0]>v[2]?2:1)&&layout.stacked==(layout.width<480),"uniform scale and narrow stacking");
        const s32 x=layout.list_left+4,y=layout.list_y+s32(list.selected()-first)*layout.row_height+2;
        check(layout.hit_row(x,y,list)==list.selected()&&!layout.hit_row(0,y,list)&&
            !layout.hit_row(x,layout.list_y-1,list)&&!layout.hit_row(x,s32(v[3]),list),"hit-test matches visible highlights and safe area");
        check(browser.selection()==initial,"viewport changes never change the exact selection");
        render.begin_frame({frame++,.05f,.07f,.09f,true});render.reset_arena();
        const auto draws=view.draw(render,v[0],v[1],browser,{BorgBrowserTileStatus::Image,7,64,32},false,false,{},
            nullptr,nullptr,&layout);
        render.end_frame();
        check(draws>0,"shared renderer draws at each aspect and HiDPI scale");
    }
    GamePadState inspect,preview,down,practice;
    inspect.a=preview.x=down.dpad_down=practice.y=true;
    check(browser.step(inspect)==BorgBrowserChange::Confirm&&browser.step(inspect)==BorgBrowserChange::None,
        "inspection is an edge, not an actor request");
    check(browser.step(preview)==BorgBrowserChange::Preview&&browser.selection()==initial,"preview requests the same exact variant");
    check(browser.select_collection_row(2)&&!browser.selection()&&browser.step(inspect)==BorgBrowserChange::None,
        "clickable diagnostic row cannot request inspect or preview");

    const PackageBorgRoster roster{{
        {7,{"lab:ant","Cached Long Ant"},false,SavedPackageVariant{"silver","Cached Silver"}},
        {8,{"lab:ant","Cached Disabled Ant"},true,SavedPackageVariant{"normal","Cached Base"}},
        {9,{"lab:missing","Cached Missing Ant"},false,SavedPackageVariant{"normal","Cached Base"}},
    },{}};
    check(browser.open_collection(packages,snapshots,diagnostics,&roster,7)&&browser.selection()&&
        browser.selection()->identity.name=="Cached Long Ant"&&browser.selection()->label=="Cached Silver",
        "read-only owned selection keeps the saved names while resolving the exact key");
    check(browser.open_collection(packages,snapshots,diagnostics,&roster,8)&&browser.owned_entry()->instance==8&&
        browser.owned_entry()->identity.name=="Cached Disabled Ant"&&!browser.selection()&&browser.step(preview)==BorgBrowserChange::None,
        "owned disabled copy remains visible and cannot request preview");
    check(browser.step(down)==BorgBrowserChange::Package&&browser.owned_entry()->instance==9&&
        browser.owned_status()=="PACKAGE MISSING"&&!browser.selection()&&browser.step(practice,true)==BorgBrowserChange::None,
        "owned missing copy stays visible; Collection cannot launch practice even with the legacy input flag");
    BorgCollection copies(BorgLibrary(packages,snapshots,diagnostics));
    check(copies.refresh(BorgLibrary(packages,snapshots,diagnostics),&roster)&&copies.select_instance(8),"owned cursor binds by instance");
    copies.configure(BorgCollectionSort::Name,BorgCollectionFilter::Exact);
    copies.configure(BorgCollectionSort::Identity,BorgCollectionFilter::All);
    auto renamed=roster;renamed.owned[1].identity.name="Edited Display Name";
    check(copies.refresh(BorgLibrary(packages,snapshots,diagnostics),&renamed)&&copies.owned_row(copies.selected())->saved.instance==8,
        "filter and refresh retain exact owned instance despite disabled status or new name");
    std::puts("Borg Collection: exact scrolling, resize, hit-testing, rendering and no action authority PASS");
}
