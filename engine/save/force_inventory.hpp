#pragma once
// Explicit endian codecs for the recovered box/warehouse images, enclosed in
// the native save framing. These images are NOT a GameCube memory-card file.
#include "engine/gameplay/force_inventory.hpp"
#include "engine/save/save_engine.hpp"
namespace gf {
inline constexpr std::size_t kForceBoxImageBytes=0x2190,kForceWarehouseImageBytes=0xabe4;
struct ForceInventoryImages {
    std::array<u8,kForceBoxImageBytes> box{};
    std::array<u8,kForceWarehouseImageBytes> warehouse{};
    std::array<u8,600> profile{};
};
namespace force_save_detail {
inline void put16(u8* p,u16 value){p[0]=u8(value>>8);p[1]=u8(value);}
inline void write_borg(u8* out,const ForceBorg& b){
    put16(out,std::bit_cast<u16>(b.model));out[2]=b.variant;out[3]=b.level;store_be32(out+4,b.acquired);store_be32(out+8,b.experience);out[12]=b.fresh;out[13]=b.reserved13;put16(out+14,b.reserved14);
}
inline ForceBorg read_borg(ByteView image,std::size_t at){
    ForceBorg b;b.model=std::bit_cast<s16>(image.be16(at));b.variant=image.data[at+2];b.level=image.data[at+3];b.acquired=image.be32(at+4);b.experience=image.be32(at+8);b.fresh=image.data[at+12];b.reserved13=image.data[at+13];b.reserved14=image.be16(at+14);return b;
}
}
inline bool force_inventory_encode(const ForceInventoryState& state,const ForceCatalog& catalog,ForceInventoryImages* out){
    if(!out||!ForceInventory::valid(state,catalog))return false;
    // Raw retail-shaped images cannot carry native tool provenance/overrides.
    for(const auto& b:state.box)if(b.value.edits!=BorgEdits{})return false;
    for(const auto& b:state.warehouse)if(b.edits!=BorgEdits{})return false;
    using namespace force_save_detail;
    for(unsigned i=0;i<kForceCount;++i){const auto& f=state.forces[i];auto* p=out->box.data()+i*0x6c;std::copy(f.name.begin(),f.name.end(),p);p[0x29]=f.count;put16(p+0x2a,f.reserved42);store_be32(p+0x2c,std::bit_cast<u32>(f.cost));for(unsigned j=0;j<kForceSlots;++j)put16(p+0x30+j*2,std::bit_cast<u16>(f.slots[j]));}
    for(unsigned i=0;i<kBoxBorgs;++i){auto* p=out->box.data()+0x870+i*0x20;const auto& b=state.box[i];write_borg(p,b.value);store_be32(p+0x10,b.memberships);for(unsigned j=0;j<3;++j)store_be32(p+0x14+j*4,b.extension[j]);}
    put16(out->box.data()+0x2170,state.force_count);put16(out->box.data()+0x2172,state.box_count);std::copy(state.box_name.begin(),state.box_name.end(),out->box.begin()+0x2174);std::copy(state.box_tail.begin(),state.box_tail.end(),out->box.begin()+0x2184);
    for(unsigned i=0;i<kWarehouseBorgs;++i)write_borg(out->warehouse.data()+i*0x10,state.warehouse[i]);
    for(unsigned i=0;i<kWarehouseParts;++i){const auto& part=state.parts[i];auto* p=out->warehouse.data()+32000+i*12;store_be32(p,part.key);store_be32(p+4,part.acquired);p[8]=part.fresh;std::copy(part.reserved.begin(),part.reserved.end(),p+9);}
    put16(out->warehouse.data()+0xabe0,state.warehouse_count);put16(out->warehouse.data()+0xabe2,state.part_count);out->profile=state.profile;return true;
}
inline bool force_inventory_decode(ByteView box,ByteView warehouse,ByteView profile,const ForceCatalog& catalog,ForceInventoryState* out){
    if(!out||!box.data||!warehouse.data||!profile.data||box.size!=kForceBoxImageBytes||warehouse.size!=kForceWarehouseImageBytes||profile.size!=600)return false;
    using namespace force_save_detail;auto next=std::make_unique<ForceInventoryState>();
    for(unsigned i=0;i<kForceCount;++i){auto& f=next->forces[i];const std::size_t p=i*0x6c;std::copy_n(box.data+p,41,f.name.begin());f.count=box.data[p+0x29];f.reserved42=box.be16(p+0x2a);f.cost=std::bit_cast<s32>(box.be32(p+0x2c));for(unsigned j=0;j<kForceSlots;++j)f.slots[j]=std::bit_cast<s16>(box.be16(p+0x30+j*2));}
    for(unsigned i=0;i<kBoxBorgs;++i){const std::size_t p=0x870+i*0x20;auto& b=next->box[i];b.value=read_borg(box,p);b.memberships=box.be32(p+0x10);for(unsigned j=0;j<3;++j)b.extension[j]=box.be32(p+0x14+j*4);}
    next->force_count=box.be16(0x2170);next->box_count=box.be16(0x2172);std::copy_n(box.data+0x2174,16,next->box_name.begin());std::copy_n(box.data+0x2184,12,next->box_tail.begin());
    for(unsigned i=0;i<kWarehouseBorgs;++i)next->warehouse[i]=read_borg(warehouse,i*0x10);
    for(unsigned i=0;i<kWarehouseParts;++i){auto& p=next->parts[i];const auto at=32000+i*12;p.key=warehouse.be32(at);p.acquired=warehouse.be32(at+4);p.fresh=warehouse.data[at+8];std::copy_n(warehouse.data+at+9,3,p.reserved.begin());}
    next->warehouse_count=warehouse.be16(0xabe0);next->part_count=warehouse.be16(0xabe2);std::copy_n(profile.data,600,next->profile.begin());
    if(!ForceInventory::valid(*next,catalog))return false;
    *out=std::move(*next);return true;
}
enum class ForceInventoryLoad {Loaded,Missing,Unavailable,Invalid};
// Slot0 is the native gameplay save; OPTION remains independently in slot1.
// An incompatible/failed read never authorizes replacing an existing save.
class ForceInventoryStore {
public:
    explicit ForceInventoryStore(ISaveBackend& backend,u32 slot=0):backend_(backend),save_(backend),slot_(slot){}
    ForceInventoryLoad load(const ForceCatalog& catalog,ForceInventory* out){
        writable_=false;if(!out||!catalog.ready()||slot_==1)return ForceInventoryLoad::Invalid;
        catalog_key_=catalog.fingerprint();
        std::vector<u8> record;const auto status=backend_.read_slot(slot_,record);
        if(status==SaveReadStatus::Missing){writable_=true;return ForceInventoryLoad::Missing;}
        if(status!=SaveReadStatus::Loaded)return ForceInventoryLoad::Unavailable;
        std::vector<u8> empty;SaveReader reader(empty);
        if(!save_.read_record(record,reader)||reader.u32_field()!=0x47464956u)return ForceInventoryLoad::Invalid;
        const auto version=reader.u32_field();if((version!=1&&version!=2)||reader.u32_field()!=catalog.fingerprint())return ForceInventoryLoad::Invalid;
        const auto profile=reader.blob(),box=reader.blob(),warehouse=reader.blob();
        const auto edits=version==2?reader.blob():std::vector<u8>{};if(!reader.consumed()||(version==2&&edits.size()!=(kBoxBorgs+kWarehouseBorgs)*18))return ForceInventoryLoad::Invalid;
        auto next=std::make_unique<ForceInventoryState>();
        if(!force_inventory_decode({box.data(),box.size()},{warehouse.data(),warehouse.size()},{profile.data(),profile.size()},catalog,next.get()))return ForceInventoryLoad::Invalid;
        if(version==2){const ByteView e{edits.data(),edits.size()};for(unsigned i=0;i<kBoxBorgs+kWarehouseBorgs;++i){auto& b=i<kBoxBorgs?next->box[i].value:next->warehouse[i-kBoxBorgs];const auto at=i*18;if(e.data[at]>1||e.data[at+1])return ForceInventoryLoad::Invalid;b.edits.modified=e.data[at]!=0;for(unsigned j=0;j<8;++j)b.edits.values[j]=std::bit_cast<s16>(e.be16(at+2+j*2));}
            for(auto& force:next->forces){force.cost=0;for(auto slot:force.slots)if(slot>=0)force.cost+=force_borg_cost(catalog,next->box[unsigned(slot)].value);}}
        if(!out->load_state(catalog,*next))return ForceInventoryLoad::Invalid;
        writable_=true;return ForceInventoryLoad::Loaded;
    }
    bool write(const ForceInventory& owner){
        if(!writable_||!owner.ready()||owner.catalog()->fingerprint()!=catalog_key_)return false;
        auto plain=std::make_unique<ForceInventoryState>(*owner.state());std::vector<u8> edits((kBoxBorgs+kWarehouseBorgs)*18);bool extended=false;
        for(unsigned i=0;i<kBoxBorgs+kWarehouseBorgs;++i){auto& b=i<kBoxBorgs?plain->box[i].value:plain->warehouse[i-kBoxBorgs];extended|=b.edits.modified;const auto at=i*18;edits[at]=u8(b.edits.modified);for(unsigned j=0;j<8;++j)store_be16(edits.data()+at+2+j*2,std::bit_cast<u16>(b.edits.values[j]));b.edits={};}
        for(auto& force:plain->forces){force.cost=0;for(auto slot:force.slots)if(slot>=0)force.cost+=force_borg_cost(*owner.catalog(),plain->box[unsigned(slot)].value);}
        auto images=std::make_unique<ForceInventoryImages>();if(!force_inventory_encode(*plain,*owner.catalog(),images.get()))return false;
        SaveWriter writer;writer.u32_field(0x47464956u);writer.u32_field(extended?2:1);writer.u32_field(owner.catalog()->fingerprint());writer.blob(images->profile.data(),images->profile.size());writer.blob(images->box.data(),images->box.size());writer.blob(images->warehouse.data(),images->warehouse.size());if(extended)writer.blob(edits.data(),edits.size());return save_.write(slot_,writer);
    }
    bool writable()const{return writable_;}
    //Logical native namespace identity only; no read/write or physical-card claim.
    bool same_namespace(const ForceInventoryStore& other)const{return &backend_==&other.backend_&&slot_==other.slot_;}
private:
    ISaveBackend& backend_;SaveEngine save_;u32 slot_;u32 catalog_key_=0;bool writable_=false;
};
} // namespace gf
