#pragma once
#include <array>
#include <vector>
#include "engine/core/types.hpp"
namespace gf {
//A physical arena/index relationship, never an encoded guest or host pointer.
//Index-only comparisons are inspection conveniences; traversal uses BOTH fields.
struct ScenePhysicalLink {
    u8 physical=255;s32 index=-1;
    bool present()const{return physical!=255&&index>=0;}
    bool operator==(const ScenePhysicalLink&)const=default;
    bool operator==(s32 value)const{return present()?index==value:value==-1;}
};
struct ScenePhysicalRecord {
    u8 active00=0;s8 group01=0;u8 class02=0,physical03=0;
    //Source +4 is free-next while free, previous-group while allocated.
    ScenePhysicalLink free_next4,group_next8;u32 update0c_key=0;
    ScenePhysicalLink render_next110;
    u32 callback100_key=0;ScenePhysicalLink callback_previous104,callback_next108;
    u8 kind83=0,registration85=0;bool allocated=false;u64 generation=0;
    bool operator==(const ScenePhysicalRecord&)const=default;
};
struct ScenePhysicalBankSnapshot {
    s8 runtime_class=-1;s32 free_head=-1,free_tail=-1;u16 free_count=0;
    std::vector<ScenePhysicalRecord> records;
    bool operator==(const ScenePhysicalBankSnapshot&)const=default;
};
} // namespace gf
