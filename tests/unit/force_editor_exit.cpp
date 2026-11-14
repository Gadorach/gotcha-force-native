#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/force_editor_session.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
namespace {
using namespace gf;unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"editor exit: %s\n",why);std::abort();}}
struct Save final:ISaveBackend {
    std::vector<u8> record;bool fail=false;unsigned attempts=0,writes=0;
    bool load(u32,std::vector<u8>& out)override{out=record;return !record.empty();}
    SaveReadStatus read_slot(u32 slot,std::vector<u8>& out)override{if(slot)return SaveReadStatus::Error;out=record;return record.empty()?SaveReadStatus::Missing:SaveReadStatus::Loaded;}
    bool store(u32 slot,const u8* p,std::size_t n)override{++attempts;if(fail||slot)return false;record.assign(p,p+n);++writes;return true;}
    bool erase(u32)override{return false;}
};
ForceSupplement supplement(const ForceCatalog& lib){ForceSupplement result;std::array<std::string,6> variants={"N","A","G","S","C","B"};std::array<std::string,10> digits;for(unsigned i=0;i<10;++i)digits[i]=std::to_string(i);std::string error;check(result.assign(lib,{},"PART ",variants,digits,{"A"},&error),"owned supplement");return result;}
}
int main(){
    using namespace gf;const auto lib=force_inventory_fixture::catalog();const auto names=supplement(lib);
    // Each exit surface uses the same normalizer/save handoff, no Yes/No prompt.
    for(unsigned surface=0;surface<3;++surface){Save save;ForceInventoryStore store(save);ForceInventory owner;check(store.load(lib,&owner)==ForceInventoryLoad::Missing&&owner.create(lib),"writable missing native save");ForceEditorSession session;check(session.open(owner,names,store),"standalone entry");
        if(surface)check(session.step({},0,surface==1?0x20:0x40)&&session.view()==ForceEditorView::List,"storage exit surface");
        check(session.step({},0,0x80)&&session.view()==ForceEditorView::Returned&&save.attempts==1&&save.writes==1,"Start saves and completes without A");ForceEditorReturn result;check(session.take_return(&result)&&result.saved&&result.destination==ForceEditorDestination::MainMenu,"main return carries successful save only");
        check(!session.take_return(&result)&&session.step({},0,0x180)&&save.attempts==1,"completed return cannot repeat save or delivery");ForceInventory loaded;check(store.load(lib,&loaded)==ForceInventoryLoad::Loaded&&loaded.state()->forces[0].count==owner.state()->forces[0].count,"native saved data reloads");
    }
    // Both contexts refuse all-empty exit before any inventory/save mutation.
    for(unsigned context=0;context<2;++context)for(unsigned surface=0;surface<3;++surface){Save save;ForceInventoryStore store(save);ForceInventory owner;check(store.load(lib,&owner)==ForceInventoryLoad::Missing&&owner.create(lib)&&owner.delete_force(0)==ForceChange::Applied,"all-empty fixture");ForceEditorSession session;
        check(context?session.open_for_selection(owner,names,store,0,100,ForceCostMode::Full):session.open(owner,names,store),"empty-context entry");if(surface)check(session.step({},0,surface==1?0x20:0x40),"empty storage entry");const auto previous=session.view();const auto revision=owner.revision();
        check(session.step({},0,0x80)&&session.view()==ForceEditorView::ExitBlocked&&session.notice_wait()==10&&owner.revision()==revision&&save.attempts==0,"empty force notice precedes normalization and save");
        for(unsigned i=0;i<10;++i)check(session.step({},0,0x300)&&session.view()==ForceEditorView::ExitBlocked,"notice wait consumes A/B without replay");
        check(session.notice_wait()==0&&session.step({},0,0)&&session.view()==ForceEditorView::ExitBlocked,"held acknowledgement is not synthesized");check(session.step({},0,0x200)&&session.view()==previous&&owner.revision()==revision&&save.attempts==0,"fresh B resumes exact prior view");
    }
    // Rebuild row0 means FIRST NONEMPTY, even if slot0 and requested slot are empty.
    for(unsigned requested=0;requested<20;++requested){Save save;ForceInventoryStore store(save);ForceInventory owner;check(owner.create(lib)&&owner.delete_force(0)==ForceChange::Applied&&owner.place(7,22,0)==ForceChange::Applied&&owner.place(19,29,1)==ForceChange::Applied,"sparse nonempty forces");ForceEditorSession session;
        check(session.open_for_selection(owner,names,store,requested,250,ForceCostMode::Full)&&session.step({},0,0x80),"selection exit with read-only/unloaded save store");const unsigned expected=requested==19?19:7;check(session.view()==ForceEditorView::Selection&&session.force()==expected&&save.attempts==0,"requested nonempty retained, absent requested maps to first nonempty");
        ForceEditorReturn result;check(session.take_return(&result)&&result.destination==ForceEditorDestination::Selection&&result.force==expected&&!result.saved&&!session.take_return(&result),"selection return identity and one-shot delivery");check(owner.state()->forces[7].slots[0]==0&&owner.state()->forces[19].slots[0]==1,"normalization compacts original references");
    }
    {
        Save save;ForceInventoryStore store(save);ForceInventory owner;check(store.load(lib,&owner)==ForceInventoryLoad::Missing&&owner.create(lib)&&store.write(owner),"existing save fixture");const auto previous=save.record;save.fail=true;ForceEditorSession session;check(session.open(owner,names,store)&&session.step({},0,0x80)&&session.view()==ForceEditorView::SaveError&&save.record==previous,"failed write retains previous native record");
        const auto revision=owner.revision();check(session.step({},0,0)&&save.attempts==2&&owner.revision()==revision,"save error does not retry during idle");check(session.step({},0,0x200)&&session.view()==ForceEditorView::Grid&&save.record==previous,"failed save returns to editable owner");
        save.fail=false;check(session.step({},0,0x80)&&session.view()==ForceEditorView::Returned&&save.attempts==3,"fresh Start retries once after return");
        check(!session.open(owner,names,store,20)&&!session.step({},0,0x80)&&save.attempts==3,"failed reopen cannot retain previous active session");
    }
    {
        Save save;ForceInventoryStore store(save);ForceInventory owner;check(owner.create(lib)&&owner.place(0,1,1)==ForceChange::Applied,"budget roster fixture");ForceEditorSession session;check(session.open_for_selection(owner,names,store,0,100,ForceCostMode::Full)&&session.step({},0,0x80),"budget-bearing selection context");
        check(session.step({},0,0x800)&&session.view()==ForceEditorView::Selection,"Y does not enter editor");check(session.step({},0,0x100),"confirm budget prefix");ForceBattleRoster roster;check(session.take_battle_roster(&roster)&&roster.members().size()==1&&roster.cost()==100&&!session.take_battle_roster(&roster),"caller budget equality and one-shot actual roster");
        check(session.step({},0,0x400)&&session.view()==ForceEditorView::Grid&&session.step({},0,0x80)&&session.view()==ForceEditorView::Selection,"X reentry and direct return");
        check(session.step({},0,0x100)&&session.step({},0,0x200)&&session.view()==ForceEditorView::SelectionExit&&!session.exit_yes(),"B opens default-No return-main modal");check(session.step({},0,0x100)&&session.view()==ForceEditorView::Selection&&!session.take_battle_roster(&roster),"No cancels return and old launch is cleared");
        check(session.step({},0,0x200)&&session.step({},8,0x100)&&session.view()==ForceEditorView::Returned&&save.attempts==0,"selection return-main does not save editor locally");ForceEditorReturn result;check(session.take_return(&result)&&result.destination==ForceEditorDestination::MainMenu&&!result.saved,"parent receives explicit unsaved context handoff");
        check(session.open_for_selection(owner,names,store,0,100,ForceCostMode::TwoThirds)&&session.step({},0,0x80)&&session.step({},0,0x100)&&session.take_battle_roster(&roster)&&roster.cost()==66,"two-thirds source cost mode survives editor roundtrip");
        check(session.step({},0,0x100)&&owner.rename_force(0,"CHANGED")==ForceChange::Applied&&!session.take_battle_roster(&roster),"post-confirm edits invalidate roster");
        check(session.open_for_selection(owner,names,store,0,100,ForceCostMode::Full)&&owner.create(lib)&&!session.step({},0,0x80),"same object reload invalidates session scope");
    }
    {
        Save save;ForceInventoryStore store(save);ForceInventory owner;check(owner.create(lib)&&owner.place(7,0,1)==ForceChange::Applied&&owner.place(19,0,2)==ForceChange::Applied,"nonempty navigation fixture");ForceEditorSession session;check(session.open_for_selection(owner,names,store,0,-1,ForceCostMode::Full)&&session.step({},0,0x80)&&session.choose_force(-1)&&session.force()==19&&session.choose_force(1)&&session.force()==0&&session.choose_force(1)&&session.force()==7,"selection navigation follows nonempty slot order with wrap");
    }
    std::printf("editor exit: %u owned checks PASS\n",checks);
}
