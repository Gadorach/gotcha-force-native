#include <cstdio>
#include <cstdlib>
#include "engine/save/force_inventory.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
namespace {
using namespace gf;unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"force save: %s\n",why);std::abort();}}
struct Store final:ISaveBackend {
    std::vector<u8> record;bool unavailable=false,fail_write=false;unsigned writes=0;u32 last_slot=99;
    bool load(u32,std::vector<u8>& out)override{out=record;return !record.empty()&&!unavailable;}
    SaveReadStatus read_slot(u32,std::vector<u8>& out)override{if(unavailable)return SaveReadStatus::Error;if(record.empty())return SaveReadStatus::Missing;out=record;return SaveReadStatus::Loaded;}
    bool store(u32 slot,const u8* p,std::size_t n)override{++writes;last_slot=slot;if(fail_write)return false;record.assign(p,p+n);return true;}
    bool erase(u32)override{return false;}
};
}
int main(){
    using namespace gf;auto lib=force_inventory_fixture::catalog();ForceInventory inv;check(inv.create(lib),"owned source-shaped starter initialization");
    auto state=std::make_unique<ForceInventoryState>(*inv.state());state->box[0].value.reserved13=0xfe;state->box[0].value.reserved14=0x9234;state->box[0].extension={0xdeadbeef,0x76543210,0xabcdef01};state->forces[0].reserved42=0x1234;state->forces[0].name[40]=0x67;state->box_tail.fill(0xb1);state->profile[0x40]=0xb2;check(inv.load_state(lib,*state),"preserved unknown fields");
    auto images=std::make_unique<ForceInventoryImages>();check(force_inventory_encode(*inv.state(),lib,images.get()),"source image encode");check(images->box[0x870]==0x10&&images->box[0x871]==1&&images->box[0x880]==0&&images->box[0x883]==1,"model and membership big endian");
    auto decoded=std::make_unique<ForceInventoryState>();auto decode=[&](){return force_inventory_decode({images->box.data(),images->box.size()},{images->warehouse.data(),images->warehouse.size()},{images->profile.data(),images->profile.size()},lib,decoded.get());};
    check(decode(),"source image decode");auto roundtrip=std::make_unique<ForceInventoryImages>();check(force_inventory_encode(*decoded,lib,roundtrip.get())&&images->box==roundtrip->box&&images->warehouse==roundtrip->warehouse&&images->profile==roundtrip->profile,"every source-layout byte retained");
    auto old=images->box;images->box[0x31]=200;check(!decode(),"out of range member");images->box=old;images->box[0x883]=0;check(!decode(),"membership cache mismatch");images->box=old;images->box[0x29]=2;check(!decode(),"member count mismatch");images->box=old;images->box[0x2f]^=1;check(!decode(),"cached cost mismatch");images->box=old;images->box[0x872]=6;check(!decode(),"invalid variant");images->box=old;
    Store backend;ForceInventoryStore save(backend);check(!save.write(inv)&&backend.writes==0,"read before write");check(save.load(lib,&inv)==ForceInventoryLoad::Missing&&backend.writes==0,"missing status does not invent load or save");check(save.write(inv)&&backend.last_slot==0,"native gameplay slot");auto record=backend.record;
    ForceInventory loaded;check(save.load(lib,&loaded)==ForceInventoryLoad::Loaded,"actual saved load");check(force_inventory_encode(*loaded.state(),lib,roundtrip.get())&&images->box==roundtrip->box&&images->warehouse==roundtrip->warehouse&&images->profile==roundtrip->profile,"saved typed state exact");
    backend.fail_write=true;check(loaded.place(1,0,1)==ForceChange::Applied&&!save.write(loaded)&&backend.record==record,"failed store retains prior saved data");backend.fail_write=false;
    backend.record.back()^=1;check(save.load(lib,&loaded)==ForceInventoryLoad::Invalid,"checksum corruption");auto writes=backend.writes;check(!save.write(loaded)&&backend.writes==writes,"invalid file never overwritten");backend.record=record;backend.unavailable=true;check(save.load(lib,&loaded)==ForceInventoryLoad::Unavailable&&!save.write(loaded),"I/O failure distinct and protected");backend.unavailable=false;
    Store prefs;ForceInventoryStore wrong_slot(prefs,1);check(wrong_slot.load(lib,&loaded)==ForceInventoryLoad::Invalid&&!wrong_slot.write(loaded)&&prefs.writes==0,"OPTION slot reserved");
    // Valid native save framing with an unrelated payload is not a force save.
    SaveEngine framing(backend);SaveWriter unrelated;unrelated.u32_field(1);check(framing.write(0,unrelated),"unrelated owned native save");check(save.load(lib,&loaded)==ForceInventoryLoad::Invalid&&!save.write(loaded),"legacy Session payload not overwritten or relabeled");
    std::printf("force inventory save: %u owned checks PASS\n",checks);
}
