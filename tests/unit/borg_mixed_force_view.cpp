#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borg_mixed_force_view.hpp"
#include "tests/unit/force_inventory_fixture.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"Mixed Borg Force view: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    auto catalogue=force_inventory_fixture::catalog();ForceInventory retail;
    check(retail.create(catalogue)&&retail.place(0,3,1)==ForceChange::Applied&&
        retail.edit_borg({true,true},false,0,retail.revision(),BorgEditField::Cost,137)==ForceChange::Applied,
        "two real retail force positions including native edit provenance");
    const auto revision=retail.revision();
    PackageForceView packages;
    PackageForceSlotView package;package.slot=0;package.saved={19,{"example:ant","Clockwork Ant"},false,SavedPackageVariant{"silver","Silver Edition"}};
    package.status=PackageForceSlotStatus::MetadataReady;package.declared_cost=225;
    packages.slots.push_back(package);
    MixedBorgForceView mixed;
    check(borg_mixed_force_view(packages,&retail,&mixed)&&mixed.retail_record_present&&mixed.slots.size()==2&&
        mixed.overlaps==1&&!mixed.declared_cost&&mixed.slots[0].overlap(),"same position retains two claims and withholds total");
    const auto& collision=mixed.slots[0];
    check(collision.retail->box_slot==0&&collision.retail->value.model==0x1001&&collision.retail->value.edits.modified&&
        collision.retail->declared_cost==137&&collision.packaged->saved==package.saved,
        "retail edit provenance and exact packaged variant both survive overlap");
    packages.slots[0].slot=2;
    check(borg_mixed_force_view(packages,&retail,&mixed)&&mixed.slots.size()==3&&!mixed.overlaps&&mixed.declared_cost==612&&
        mixed.slots[0].slot==0&&mixed.slots[1].slot==2&&mixed.slots[2].slot==3,
        "complete mixed view preserves position order and sums only declared costs");
    packages.slots[0].saved.disabled=true;
    check(borg_mixed_force_view(packages,&retail,&mixed)&&!mixed.declared_cost&&mixed.slots[1].packaged->saved.disabled,
        "a disabled package remains in the view but prevents a complete total");
    packages.slots[0].saved.disabled=false;
    check(borg_mixed_force_view(packages,nullptr,&mixed)&&!mixed.retail_record_present&&mixed.slots.size()==1&&mixed.declared_cost==225,
        "a known absent retail record is explicitly distinguished");
    packages.slots.push_back(packages.slots[0]);
    check(!borg_mixed_force_view(packages,&retail,&mixed)&&mixed.slots.size()==1&&retail.revision()==revision&&
        retail.state()->forces[0].slots[0]==0&&retail.state()->forces[0].slots[3]==1,
        "invalid view refuses atomically and retail owner is never modified");
    std::puts("Mixed Borg Force view: ordered sources, overlaps, declared cost and edit provenance PASS");
}
