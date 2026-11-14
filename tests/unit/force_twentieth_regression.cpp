#include <cstdio>
#include <cstdlib>
#include <tuple>
#include "engine/gameplay/force_editor_grid.hpp"
#include "engine/save/force_inventory.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
namespace {
using namespace gf;unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"20th force regression: %s\n",why);std::abort();}}
auto attributes(const ForceBorg& b){return std::tuple{b.model,b.variant,b.level,b.acquired,b.experience,b.reserved13,b.reserved14};}
}
int main(){
    using namespace gf;using namespace force_inventory_fixture;const auto library=catalog();
    // Owned Borg metadata substitutes for original assets. Slots/force sequences
    // reproduce the owner's supplied guide; displayed indices are one-based.
    std::vector<u32> sequences={0,2u|8u|32u|512u|1024u,521,0xfffffu};
    for(unsigned bit=0;bit<20;++bit)sequences.push_back(u32(1)<<bit);
    unsigned scenarios=0;
    for(auto memberships:sequences)for(unsigned variation=0;variation<2;++variation)for(bool near_full:{false,true}){
        ForceInventory owner;check(owner.create(library,false),"fresh regression owner");
        for(unsigned i=0;i<3;++i){auto b=borg(i);b.variant=u8((i+variation)%6);b.level=u8(i+7);b.experience=1000+i;check(owner.acquire(b)==ForceChange::Applied,"distinct source attributes");}
        for(unsigned f=0;f<20;++f)if(memberships&(u32(1)<<f))check(owner.place(f,0,1)==ForceChange::Applied,"target force-sequence memberships");
        check(owner.place(19,22,0)==ForceChange::Applied&&owner.place(19,14,2)==ForceChange::Applied,"force20 slots23 and15");
        auto state=std::make_unique<ForceInventoryState>(*owner.state());
        if(near_full){
            for(unsigned i=3;i<199;++i){state->box[i].value=borg(i%3);state->box[i].value.acquired=i;}
            state->box_count=199;
            for(unsigned i=0;i<1999;++i){state->warehouse[i]=borg(i%3);state->warehouse[i].acquired=200+i;}
            state->warehouse_count=1999;store_be32(state->profile.data()+0x50,2200);
        }
        state->box[1].extension={0x80000001u,0x92345678u,0xaabbccddu};state->box[1].value.reserved13=0x81;state->box[1].value.reserved14=0x9234;
        check(owner.load_state(library,*state),"capacity/tail fixture valid");
        std::array<ForceBorg,3> initial;for(unsigned i=0;i<3;++i)initial[i]=owner.state()->box[i].value;
        ForceEditorGrid editor;ForceEditorEvents events;check(editor.open(owner,19),"20th force editor");
        auto step=[&](ForceEditorHit hit,u32 pressed){return editor.step(owner,hit,0,pressed,&events);};
        auto empty_pick=[&](){auto before=std::make_unique<ForceInventoryImages>();auto after=std::make_unique<ForceInventoryImages>();const auto revision=owner.revision();return force_inventory_encode(*owner.state(),library,before.get())&&step({s16(near_full?199:3),-1},0x100)&&editor.held()==-1&&events.sound_count==0&&owner.revision()==revision&&force_inventory_encode(*owner.state(),library,after.get())&&before->box==after->box&&before->warehouse==after->warehouse&&before->profile==after->profile;};
        check(empty_pick(),"first empty-cell A: no sound, held item, write or revision");
        check(step({1,-1},0x100)&&owner.state()->forces[19].count<=3,"target A cannot fabricate a full force");
        check(step({},0x200)&&editor.held()==-1,"release target safely");
        check(step({-1,23},0x100)&&editor.held()==-1&&events.sound_count==0,"24th force slot remains genuinely empty");
        check(step({-1,s8(variation?15:14)},0x100)&&step({},0x200),"guide's next member action is ordinary bounded editing");
        check(empty_pick(),"second empty-cell A still cannot fabricate a held Borg");
        check(step({1,-1},0x100)&&step({},0x200),"second target A/B cannot change attribute words");
        for(unsigned i=0;i<3;++i)check(attributes(owner.state()->box[i].value)==attributes(initial[i]),"identity/variant/level/experience/serial/tails unchanged");
        check(owner.state()->box[1].extension==state->box[1].extension,"membership is not an attribute or extension-byte write");
        for(unsigned f=0;f<20;++f){const auto result=owner.delete_force(f);check(result==ForceChange::Applied||result==ForceChange::Empty,"delete all forces retains owned instances");}
        check(owner.state()->box_count==(near_full?199:3)&&owner.state()->warehouse_count==(near_full?1999:0)&&owner.state()->force_count==0&&ForceInventory::valid(*owner.state(),library),"complete recipe preserves ownership and invariants");
        for(unsigned i=0;i<3;++i)check(attributes(owner.state()->box[i].value)==attributes(initial[i]),"final attributes unchanged after all forces deleted");
        ++scenarios;
    }
    ForceInventory owner;check(owner.create(library),"boundary owner");ForceEditorGrid editor;ForceEditorEvents events;check(editor.open(owner,19),"last legal force");const auto revision=owner.revision();
    for(ForceEditorHit hit:std::array<ForceEditorHit,5>{{{200,-1},{-2,-1},{-1,30},{-1,-2},{0,0}}})check(!editor.step(owner,hit,0,0x100,&events)&&owner.revision()==revision,"invalid cursor domain has no mutation");
    check(owner.place(20,0,0)==ForceChange::Invalid&&owner.place(19,30,0)==ForceChange::Invalid&&owner.place(19,0,200)==ForceChange::Invalid&&owner.swap_box(unsigned(-1),0)==ForceChange::Invalid&&owner.revision()==revision,"no force21/member31/box201/negative-index alias");
    auto invalid=std::make_unique<ForceInventoryState>(*owner.state());invalid->box[0].value.variant=6;check(!owner.load_state(library,*invalid),"invalid color import refused");invalid->box[0].value.variant=0;invalid->box[0].value.level=0x80;check(!owner.load_state(library,*invalid),"mutant-level import cannot index beyond normal level data");invalid->box[0].value.level=0;invalid->box[0].value.model=0x7ffe;check(!owner.load_state(library,*invalid),"unknown model import refused");
    std::printf("20th force regression: %u checks / %u guide-sequence scenarios PASS\n",checks,scenarios);
}
