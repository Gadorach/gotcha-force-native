#pragma once
#include "engine/save/account_save_repository.hpp"
#include "engine/save/force_inventory.hpp"

namespace gf {
// An empty snapshot means the selected save proved that slot 0 was absent,
// not that an unavailable or mismatched record was ignored.
inline bool scoped_retail_force_record_current(AccountSaveBackend& backend,const std::vector<u8>& snapshot){
    std::vector<u8> current;const auto status=backend.read_slot(0,current);
    return snapshot.empty()?status==SaveReadStatus::Missing:(status==SaveReadStatus::Loaded&&current==snapshot);
}
inline bool read_scoped_retail_force_record(AccountSaveBackend& backend,const ForceCatalog* catalogue,
                                           std::vector<u8>* record,ForceInventory* inventory){
    if(!record||!inventory)return false;
    std::vector<u8> bytes;ForceInventory next;
    const auto status=backend.read_slot(0,bytes);
    if(status==SaveReadStatus::Loaded){
        if(!catalogue||bytes.empty()||bytes.size()>kAccountObjectBytes)return false;
        ForceInventoryStore reader(backend);
        if(reader.load(*catalogue,&next)!=ForceInventoryLoad::Loaded)return false;
    }else if(status!=SaveReadStatus::Missing)return false;
    if(!scoped_retail_force_record_current(backend,bytes))return false;
    *record=std::move(bytes);*inventory=std::move(next);return true;
}
}
