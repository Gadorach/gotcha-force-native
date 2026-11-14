#pragma once
#include "engine/core/backends.hpp"

namespace gf {

//Opt-in catalogue capability. The platform owns paths, enumeration and atomic
//object replacement; game code owns metadata, record formats and selection.
//Account keys have save=0. Save keys have both IDs nonzero. Names are never paths.
struct AccountSaveKey {
    u64 account=0,save=0;
    bool valid()const{return account!=0;}
    bool operator==(const AccountSaveKey&)const=default;
    bool operator<(const AccountSaveKey& other)const{return account!=other.account?account<other.account:save<other.save;}
};
inline constexpr std::size_t kAccountMetadataBytes=4096,kAccountObjectBytes=1u<<20;
inline constexpr u32 kAccountCataloguePage=64;
struct AccountMetadataRead {u64 revision=0;std::vector<u8> bytes;};
struct AccountObjectRead {u64 revision=0;std::vector<u8> metadata,payload;};
enum class AccountWrite {Stored,Exists,Conflict,Error,Uncertain};

class IAccountSaveStorage {
public:
    virtual ~IAccountSaveStorage()=default;
    //Metadata reads must not read/decode the potentially large save payload.
    virtual SaveReadStatus metadata(AccountSaveKey,AccountMetadataRead*)=0;
    virtual SaveReadStatus read(AccountSaveKey,AccountObjectRead*)=0;
    //account=0 enumerates account IDs; otherwise enumerates that account's save
    //IDs. Ascending unique nonzero IDs >after; <=limit results, more implies a
    //nonempty result. No two-card limit. Deleted IDs remain reserved tombstones.
    virtual bool list(u64 account,u64 after,u32 limit,std::vector<u64>*,bool* more)=0;
    //All writes replace ONE complete metadata+payload object. Create is exclusive.
    //For saves, parent_revision must atomically match the account marker. Replace
    //also compares expected_revision. Revisions are nonzero, never reused for the
    //same key; wrap/exhaustion refuses. Tombstones must not be overwritten by create.
    //Known Error/Conflict/Exists mean no write. Uncertain means outcome unknown:
    //the caller MUST latch, never automatically replay. Methods are serialized;
    //a backend unable to provide these guarantees must refuse, not fake success.
    virtual AccountWrite create(AccountSaveKey,u64 parent_revision,const std::vector<u8>& metadata,const std::vector<u8>& payload,u64* revision)=0;
    virtual AccountWrite replace(AccountSaveKey,u64 expected_revision,u64 parent_revision,const std::vector<u8>& metadata,const std::vector<u8>& payload,u64* revision)=0;
};

} //namespace gf
