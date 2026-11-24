#pragma once
#include <memory>
#include <new>
#include "engine/gameplay/scene_descriptor_pool.hpp"
namespace gf {
//Pool2 stays unexposed. Only the exact positive bounded06DC8 pool3 consumer
//is added; these native fields never encode a guest or host pointer.
struct SceneSmallDescriptorRecord {
    s32 link0=-1,link4=-1;u8 pool08=0,allocated09=0,flag0a=0,flag0b=0;u32 unresolved_header0c=0;
    u64 generation=0;bool header0c_nonnull=false,buffer_owned=false;
    u32 requested_bytes=0,rounded_bytes=0,initialized_bytes=0,consumers=0;
    bool operator==(const SceneSmallDescriptorRecord&)const=default;
};
struct SceneSmallDescriptorPoolSnapshot {s32 free_head=-1,live_head=-1;s16 free_count=0;std::vector<SceneSmallDescriptorRecord> records;bool operator==(const SceneSmallDescriptorPoolSnapshot&)const=default;};
class SceneScratchAllocation {
    struct Delete {void operator()(u8* p)const{::operator delete[](p,std::align_val_t(32));}};
public:
    static SceneScratchAllocation allocate(u32 bytes){
        SceneScratchAllocation block;block.data_.reset(static_cast<u8*>(::operator new[](bytes,std::align_val_t(32),std::nothrow)));
        if(block.data_)block.bytes_=bytes;
        return block;
    }
    bool allocated()const{return bool(data_);}
    u32 bytes()const{return allocated()?bytes_:0;}
    bool aligned()const{return !data_||(reinterpret_cast<std::uintptr_t>(data_.get())&31u)==0;}
    void reset(){data_.reset();bytes_=0;}
    //Opaque, unwritten storage: no read or implicit zero/identity capability.
private:std::unique_ptr<u8,Delete> data_;u32 bytes_=0;
};
class ISceneScratchHeap {
public:virtual ~ISceneScratchHeap()=default;virtual SceneScratchAllocation allocate(u32 bytes)=0;
};
class SceneScratchHeap final:public ISceneScratchHeap {
public:SceneScratchAllocation allocate(u32 bytes)override{return SceneScratchAllocation::allocate(bytes);}
};
namespace scene_small_descriptor_detail {
struct Storage {
    bool live=true;u8 pool=0;SceneSmallDescriptorPoolSnapshot state;
    std::weak_ptr<SceneDescriptorPool> domain;const SceneDescriptorPoolSnapshot* identity=nullptr;
    std::vector<SceneScratchAllocation> buffers;
    bool current()const{const auto p=domain.lock();return live&&p&&p->ready()&&p->snapshot()==identity;}
};
}
class SceneDescriptorBootstrap;
class SceneScratchLease {
public:
    bool current()const{
        const auto s=owner_.lock();if(!s||!s->current()||s->pool!=3||index_>=s->state.records.size())return false;
        const auto& r=s->state.records[index_];return r.allocated09&&r.generation==generation_&&r.buffer_owned&&r.header0c_nonnull&&s->buffers[index_].allocated();
    }
    u16 index()const{return index_;}u64 generation()const{return generation_;}
    u32 requested_bytes()const{const auto s=owner_.lock();return current()?s->state.records[index_].requested_bytes:0;}
    u32 rounded_bytes()const{const auto s=owner_.lock();return current()?s->state.records[index_].rounded_bytes:0;}
private:friend class SceneDescriptorBootstrap;std::weak_ptr<scene_small_descriptor_detail::Storage> owner_;u16 index_=65535;u64 generation_=0;
};
enum class SceneScratchAcquire {Allocated,PoolExhausted,HeapFailure,Refused};
struct SceneScratchReference {
    u8 pool=3;u16 index=65535;u64 generation=0;u32 bytes=0;
    bool operator==(const SceneScratchReference&)const=default;
};
}
