#include <cstdio>
#include <cstdlib>
#include "engine/core/headless_backends.hpp"
#include "engine/gameplay/borg_force_browser_view.hpp"
#include "tests/unit/force_inventory_fixture.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"Force browser: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    auto catalogue=force_inventory_fixture::catalog();ForceInventory legacy;
    check(legacy.create(catalogue),"fixture retail record");
    PackageForceView package;package.force=0;
    PackageForceSlotView one;one.slot=0;one.saved={7,{"fixture:bridge","Saved Borg"},false,SavedPackageVariant{"normal","Saved Normal"}};
    one.status=PackageForceSlotStatus::MetadataReady;one.declared_cost=42;
    auto missing=one;missing.slot=2;missing.saved.instance=8;missing.status=PackageForceSlotStatus::MissingVariant;
    missing.declared_cost.reset();
    package.slots={one,missing};
    MixedBorgForceView force;
    check(borg_mixed_force_view(package,&legacy,&force)&&force.find_slot(0)&&force.find_slot(0)->overlap()&&
        !force.find_slot(1)&&force.find_slot(2)&&force.find_slot(2)->packaged&&!force.declared_cost,
        "exact overlap stays two claims; the gap is empty and the cost is withheld");

    BorgForceBrowser cursor;
    check(cursor.open(0,0)&&!cursor.open(kForceCount,0)&&!cursor.open(0,kForceSlots)&&cursor.force()==0&&cursor.slot()==0,
        "out of range requests do not change the cursor");
    GamePadState up,down,left,back;up.dpad_up=down.dpad_down=left.dpad_left=back.b=true;
    check(cursor.step(up)==BorgForceBrowseChange::Slot&&cursor.slot()==kForceSlots-1&&
        cursor.step(up)==BorgForceBrowseChange::None,"slot navigation wraps without held repetition");
    check(cursor.step(down)==BorgForceBrowseChange::Slot&&cursor.slot()==0,"opposite move wraps to the first slot");
    auto together=left;together.dpad_up=true;
    check(cursor.step(together)==BorgForceBrowseChange::Force&&cursor.force()==kForceCount-1&&cursor.slot()==0,
        "Force move takes precedence and starts at slot one");
    check(cursor.step(back)==BorgForceBrowseChange::Close,"close is an edge");

    headless::Render device;RenderEngine render(device);std::string error;
    check(render.initialize({640,480,false,true,"Force browser"},nullptr,&error),"shared render backend");
    BorgForceBrowserView view;
    auto draw=[&](const MixedBorgForceView* snapshot){
        render.begin_frame({0,.05f,.07f,.09f});render.reset_arena();
        const auto draws=view.draw(render,640,480,cursor,snapshot,"Saved Fixture");render.end_frame();
        return draws;
    };
    check(cursor.open(0,0)&&draw(&force)>0,"shared view draws both claims for the selected position");
    check(cursor.open(0,1)&&draw(&force)>0,"empty slot is inspectable without a claim");
    check(draw(nullptr)>0,"stale selection draws a reload-only view");
    std::puts("Force browser: exact slots, overlap, held-key isolation, and shared rendering PASS");
}
