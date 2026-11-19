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
    GamePadState up,down,left,back,practice;up.dpad_up=down.dpad_down=left.dpad_left=back.b=practice.y=true;
    check(cursor.step(up)==BorgForceBrowseChange::Slot&&cursor.slot()==kForceSlots-1&&
        cursor.step(up)==BorgForceBrowseChange::None,"slot navigation wraps without held repetition");
    check(cursor.step(down)==BorgForceBrowseChange::Slot&&cursor.slot()==0,"opposite move wraps to the first slot");
    auto together=left;together.dpad_up=true;
    check(cursor.step(together)==BorgForceBrowseChange::Force&&cursor.force()==kForceCount-1&&cursor.slot()==0,
        "Force move takes precedence and starts at slot one");
    check(cursor.step(back)==BorgForceBrowseChange::Close,"close is an edge");
    check(cursor.step(practice)==BorgForceBrowseChange::None,"practice is opt-in");
    cursor.step({});
    auto move_and_practice=practice;move_and_practice.dpad_down=true;
    check(cursor.step(move_and_practice,true)==BorgForceBrowseChange::Slot&&
        cursor.step(move_and_practice,true)==BorgForceBrowseChange::None,"a movement edge wins over practice");
    cursor.step({});
    check(cursor.step(practice,true)==BorgForceBrowseChange::Practice&&cursor.step(practice,true)==BorgForceBrowseChange::None,
        "practice requests are single edges and cannot modify positions");
    GamePadState lane;lane.l=true;
    const auto force_before=cursor.force(),slot_before=cursor.slot();
    check(cursor.step(lane)==BorgForceBrowseChange::None&&cursor.practice_route()==GRedPickerPracticeRoute::Standard,
        "route switching is opt-in");
    cursor.step({});lane.y=true;
    check(cursor.step(lane,true)==BorgForceBrowseChange::PracticeRoute&&cursor.practice_route()==GRedPickerPracticeRoute::ExpiryPush&&
        cursor.step(lane,true)==BorgForceBrowseChange::None,"route switch wraps and consumes a simultaneous launch edge");
    cursor.step({});
    check(cursor.step(lane,true)==BorgForceBrowseChange::PracticeRoute&&cursor.practice_route()==GRedPickerPracticeRoute::ExpiryAirDash&&
        cursor.force()==force_before&&cursor.slot()==slot_before,"expiry air dash selection does not change the slot");
    cursor.step({});
    check(cursor.step(lane,true)==BorgForceBrowseChange::PracticeRoute&&cursor.practice_route()==GRedPickerPracticeRoute::ExpiryHandoff&&
        cursor.force()==force_before&&cursor.slot()==slot_before,"third Left shoulder reaches the expiry handoff");
    cursor.step({});
    check(cursor.step(lane,true)==BorgForceBrowseChange::PracticeRoute&&cursor.practice_route()==GRedPickerPracticeRoute::TimedHandoff&&
        cursor.force()==force_before&&cursor.slot()==slot_before,"fourth Left shoulder reaches the timed handoff");
    GamePadState next_route;next_route.r=true;
    check(cursor.open(force_before,slot_before)&&cursor.step(next_route,true)==BorgForceBrowseChange::PracticeRoute&&
        cursor.practice_route()==GRedPickerPracticeRoute::RecoverySeam&&cursor.force()==force_before&&cursor.slot()==slot_before,
        "one Right shoulder from Standard selects Recovery Seam without moving either cursor");
    cursor.step({});
    check(cursor.step(next_route,true)==BorgForceBrowseChange::PracticeRoute&&
        cursor.practice_route()==GRedPickerPracticeRoute::TimedRelaunchFloor&&cursor.practice_route_name()=="TIMED RELAUNCH FLOOR"&&
        cursor.step(next_route,true)==BorgForceBrowseChange::None,"second Right shoulder reaches the timed floor guide once");
    cursor.step({});
    check(cursor.step(next_route,true)==BorgForceBrowseChange::PracticeRoute&&
        cursor.practice_route()==GRedPickerPracticeRoute::InclineDashJump&&cursor.practice_route_name()=="INCLINE DASH JUMP"&&
        cursor.force()==force_before&&cursor.slot()==slot_before,"third Right shoulder reaches incline dash jump without moving either cursor");
    cursor.step({});
    check(cursor.step(next_route,true)==BorgForceBrowseChange::PracticeRoute&&
        cursor.practice_route()==GRedPickerPracticeRoute::TimedLandingDash&&cursor.practice_route_name()=="TIMED LANDING DASH",
        "fourth Right shoulder reaches the timed landing dash");
    cursor.step({});
    check(cursor.step(next_route,true)==BorgForceBrowseChange::PracticeRoute&&
        cursor.practice_route()==GRedPickerPracticeRoute::DirectExpiryRelaunch&&cursor.practice_route_name()=="DIRECT EXPIRY RELAUNCH"&&
        cursor.force()==force_before&&cursor.slot()==slot_before,"fifth Right shoulder reaches direct expiry relaunch without moving either cursor");
    for(auto route:{GRedPickerPracticeRoute::PositiveFloorHandoff,GRedPickerPracticeRoute::ExpiredFloorHandoff,
        GRedPickerPracticeRoute::TimedSnapDash}){
        cursor.step({});
        check(cursor.step(next_route,true)==BorgForceBrowseChange::PracticeRoute&&cursor.practice_route()==route&&
            cursor.step(next_route,true)==BorgForceBrowseChange::None&&cursor.force()==force_before&&cursor.slot()==slot_before,
            "sixth through eighth Right shoulder select floor-snap guides without moving the Force or slot");
    }
    for(auto route:{GRedPickerPracticeRoute::ExpiryStepHandoff,GRedPickerPracticeRoute::DirectTimedRelaunch,
        GRedPickerPracticeRoute::DirectTimedUpRelaunch,GRedPickerPracticeRoute::ActiveSnapRelaunch,
        GRedPickerPracticeRoute::ExpiringDirectRelaunch,GRedPickerPracticeRoute::ExpiringDirectRelaunchPush,
        GRedPickerPracticeRoute::TimedReleasePush,GRedPickerPracticeRoute::TimedReleaseTwoPushes}){
        cursor.step({});
        check(cursor.step(next_route,true)==BorgForceBrowseChange::PracticeRoute&&cursor.practice_route()==route&&
            cursor.step(next_route,true)==BorgForceBrowseChange::None&&cursor.force()==force_before&&cursor.slot()==slot_before,
            "ninth through sixteenth Right shoulder select the timed guides without moving either cursor");
    }
    check(cursor.practice_route_name()=="TWO TIMED RELEASE PUSHES","two-push guide has its own route label");
    lane.y=false;
    cursor.step({});
    check(cursor.step(lane,true)==BorgForceBrowseChange::PracticeRoute&&cursor.practice_route_name()=="TIMED RELEASE PUSH"&&
        cursor.force()==force_before&&cursor.slot()==slot_before,"previous selects the single-push guide without moving either cursor");
    for(auto label:{"ENTRY PUSH ON EXPIRY","RELAUNCH ON EXPIRY"}){
        cursor.step({});
        check(cursor.step(lane,true)==BorgForceBrowseChange::PracticeRoute&&cursor.practice_route_name()==label&&
            cursor.force()==force_before&&cursor.slot()==slot_before,
            "the two expiry-step labels are distinct and neither changes the Force or slot");
    }

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
