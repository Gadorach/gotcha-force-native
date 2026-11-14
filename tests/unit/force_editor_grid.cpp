#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/force_editor_grid.hpp"
#include "engine/gameplay/force_selection.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"force editor grid: %s\n",why);std::abort();}}
}
int main(){
    using namespace gf;using namespace force_inventory_fixture;const auto lib=catalog();ForceInventory owner;check(owner.create(lib,false),"empty inventory");
    for(unsigned i=0;i<3;++i)check(owner.acquire(borg(i))==ForceChange::Applied,"owned box instances");
    check(owner.place(0,5,0)==ForceChange::Applied&&owner.place(0,8,1)==ForceChange::Applied,"sparse force");
    ForceEditorGrid editor;ForceEditorEvents event;check(editor.open(owner,0),"shared grid entry");
    auto step=[&](ForceEditorHit hit,u32 repeat,u32 pressed){return editor.step(owner,hit,repeat,pressed,&event);};
    auto wait=[&](){while(editor.wait_updates())if(!step({},0,0))return false;return true;};
    auto operations=[&](ForceEditorHit hit,unsigned row){if(!step(hit,0,0x800)||!wait())return false;for(unsigned i=0;i<row;++i)if(!step({},4,0))return false;return true;};
    check(step({-1,5},0,0x100)&&editor.held()==0&&owner.state()->forces[0].slots[5]==-1&&event.sounds[0]==122,"picking removes membership immediately");
    check(!editor.select_force(owner,1),"wheel cannot change force while holding");
    check(step({},0,0x300)&&editor.held()==-1&&owner.state()->forces[0].slots[5]==-1&&event.sounds[0]==123,"B beats A; cancel never reinserts removed member");
    check(step({2,-1},0,0x100)&&editor.held()==2,"pick box instance");
    check(step({-1,8},0,0x100)&&editor.held()==1&&owner.state()->forces[0].slots[8]==2,"replace hands displaced member to cursor");
    check(step({0,-1},0,0x100)&&editor.held()==1&&owner.state()->box[0].value.model==0x1002&&owner.state()->box[1].value.model==0x1001,"box swap keeps held slot and carries displaced instance");
    check(step({-1,1},0,0x100)&&editor.held()==-1&&owner.state()->forces[0].slots[1]==1,"place held instance in empty force slot");
    check(step({1,-1},0,0x100)&&editor.held()==-1&&event.sound_count==0,"already-member box pick ignored");
    check(step({-1,1},0,0x100)&&editor.held()==1,"pick again");
    check(step({2,-1},0,0x100)&&editor.held()==-1&&owner.state()->forces[0].slots[8]==1,"swapped-in active force member ends carry and retargets membership");
    check(step({},0,0x400)&&event.sound_count==1&&event.sounds[0]==125,"spec on empty cursor rejects with source error cue");
    check(step({0,-1},0,0x4e0)&&event.request==ForceEditorRequest::Exit&&event.sounds[0]==122,"idle Start beats shoulder and spec inputs");
    check(!step({},0,0)&&editor.resume(owner),"external view requires explicit return");
    check(step({},0,0x60)&&event.request==ForceEditorRequest::Components&&editor.resume(owner),"L before R");
    check(step({},0,0x20)&&event.request==ForceEditorRequest::Warehouse&&editor.resume(owner),"warehouse request");
    check(step({0,-1},0,0x100)&&step({},0,0x400)&&event.request==ForceEditorRequest::Spec&&event.spec_box==0&&editor.held()==0,"held spec uses carried instance");
    check(editor.resume(owner)&&step({},0,0x200),"spec returns carry then cancel");
    check(operations({0,-1},2),"copy menu ready");
    check(step({},0,0x100)&&editor.force()==1&&event.wheel_direction==-1&&owner.state()->forces[1].count==1,"copy selects first empty destination");
    check(wait()&&operations({},3)&&step({},0,0x100)&&owner.state()->forces[1].count==0&&owner.state()->box_count==3,"delete force keeps owned Borgs");
    check(wait()&&operations({},2)&&(editor.disabled_operations(owner)&0xc)==0xc,"empty force disables copy and delete");
    check(step({},0,0x100)&&event.sounds[0]==125&&editor.phase()==ForceEditorGridPhase::Operations,"disabled action rejects");
    check(step({},0,0x200)&&wait()&&operations({0,-1},4)&&step({},0,0x100)&&owner.state()->warehouse_count==1&&!owner.state()->box[0].value.present(),"move from grid into warehouse");
    check(wait()&&operations({2,-1},5)&&step({},0,0x100)&&editor.phase()==ForceEditorGridPhase::Discard&&editor.menu_row()==1,"discard defaults No");
    check(wait()&&step({},0,0x100)&&owner.state()->box[2].value.present(),"confirm default No keeps Borg");
    check(wait()&&operations({2,-1},5)&&step({},0,0x100)&&wait(),"reopen discard");
    check(step({},8,0x100)&&event.sound_count==2&&event.sounds[0]==119&&event.sounds[1]==122&&!owner.state()->box[2].value.present(),"navigate-before-confirm deletes on explicit Yes with ordered sounds");
    check(wait()&&operations({},0)&&step({},0,0x100)&&event.request==ForceEditorRequest::Search,"search operation");
    check(editor.resume(owner)&&operations({},1)&&step({},0,0x100)&&event.request==ForceEditorRequest::Name,"name operation");
    check(owner.rename_force(1,"RETURNED")==ForceChange::Applied&&editor.resume(owner)&&editor.current(owner),"same-owner naming view may change inventory on return");
    check(owner.acquire(borg())==ForceChange::Applied&&!step({},0,0),"external mutation invalidates active hit/held state");
    // Real protected identities with entirely owned metadata, not private assets.
    auto entries=std::vector<ForceBorgMetadata>(lib.entries().begin(),lib.entries().end());entries[0].model=0x615;ForceCatalog protected_lib;std::string error;
    check(protected_lib.assign(std::move(entries),{0x615},{},"OWNED","ABCDEFGHIJKLMNOPQRSTUA","OWNED",21,&error),"protected identity fixture");
    ForceInventory protected_owner;check(protected_owner.create(protected_lib)&&editor.open(protected_owner,0),"protected grid");
    check(editor.step(protected_owner,{0,-1},0,0x800,&event)&&(editor.disabled_operations(protected_owner)&0x20),"G Red discard disabled by editor policy");
    // Selection shares the owner, preserves source order and accepts the changed
    // force returned by editing, while launch delivery is current and one-shot.
    ForceSelection selection;ForceBattleRoster roster;check(selection.open(owner,0,100,ForceCostMode::Full),"selection context");
    check(selection.confirm(owner)&&selection.take(owner,&roster)&&roster.members().size()==1&&roster.members()[0].borg.model==0x1003,"budget selection original order");
    check(!selection.take(owner,&roster),"launch handed off once");
    check(selection.confirm(owner)&&owner.rename_force(0,"EDITED")==ForceChange::Applied&&!selection.take(owner,&roster),"editing after confirmation prevents stale battle launch");
    unsigned edit_force=99;check(selection.begin_edit(owner,&edit_force)&&edit_force==0&&selection.editing()&&!selection.confirm(owner),"editor entry holds launch");
    check(owner.place(1,10,0)==ForceChange::Applied&&owner.finish_edit()==ForceChange::Applied&&selection.return_from_edit(owner,1)&&selection.force()==1,"editor return preserves newly selected force");
    check(selection.confirm(owner)&&selection.take(owner,&roster)&&roster.force()==1&&roster.members()[0].force_slot==0,"battle receives edited compacted roster");
    ForceInventory neighbor;check(neighbor.load_state(lib,*owner.state())&&!selection.confirm(neighbor),"selection cannot silently switch inventory owner");
    check(owner.load_state(lib,*owner.state())&&!selection.confirm(owner),"reload replaces scope even when revision returns to one");
    std::printf("force editor grid: %u owned checks PASS\n",checks);
}
