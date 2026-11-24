#pragma once
#include <memory>
#include <optional>
#include "engine/save/account_save_format.hpp"

namespace gf {

enum class AccountResult {Ok,Missing,Invalid,Unavailable,Exists,Conflict,Busy,Uncertain};
enum class AccountEntryStatus {Ready,Invalid,Unavailable};
struct AccountRepositoryScope {};
struct AccountSaveEntry {
    AccountSaveKey key;u64 revision=0,parent_revision=0;std::string name;
    AccountEntryStatus status=AccountEntryStatus::Unavailable;
    std::weak_ptr<const AccountRepositoryScope> repository;
};
struct AccountSavePage {std::vector<AccountSaveEntry> entries;u64 next=0;bool more=false;};
namespace account_save_detail {struct Lease;}
class AccountSaveRevision {
public:
    AccountSaveKey key()const{return key_;}
    u64 revision()const{return revision_;}
    u64 parent_revision()const{return parent_revision_;}
    bool operator==(const AccountSaveRevision& other)const{
        return key_==other.key_&&revision_==other.revision_&&parent_revision_==other.parent_revision_&&
            !lease_.owner_before(other.lease_)&&!other.lease_.owner_before(lease_);
    }
private:
    friend class AccountSaveBackend;
    AccountSaveKey key_;u64 revision_=0,parent_revision_=0;
    std::weak_ptr<const account_save_detail::Lease> lease_;
};

namespace account_save_detail {
struct Snapshot {u64 revision=0;AccountSaveDescription description;AccountSaveRecords records;};
inline AccountResult load(IAccountSaveStorage& storage,AccountSaveKey key,u64 expected,Snapshot* out){
    if(!out)return AccountResult::Invalid;
    AccountObjectRead object;const auto status=storage.read(key,&object);
    if(status==SaveReadStatus::Missing)return AccountResult::Missing;
    if(status!=SaveReadStatus::Loaded)return AccountResult::Unavailable;
    Snapshot next;next.revision=object.revision;
    if(!next.revision||!account_description_decode(object.metadata,key,&next.description)||!account_records_decode(object.payload,key,&next.records))return AccountResult::Invalid;
    if(expected&&next.revision!=expected)return AccountResult::Conflict;
    if(next.description.retired)return AccountResult::Missing;
    *out=std::move(next);return AccountResult::Ok;
}
inline AccountResult current(IAccountSaveStorage& storage,AccountSaveKey key,u64 expected){
    AccountMetadataRead object;const auto status=storage.metadata(key,&object);
    if(status==SaveReadStatus::Missing)return AccountResult::Missing;
    if(status!=SaveReadStatus::Loaded)return AccountResult::Unavailable;
    AccountSaveDescription description;
    if(!object.revision||!account_description_decode(object.bytes,key,&description))return AccountResult::Invalid;
    if(object.revision!=expected)return AccountResult::Conflict;
    return description.retired?AccountResult::Missing:AccountResult::Ok;
}
inline AccountResult result(AccountWrite value){
    switch(value){case AccountWrite::Stored:return AccountResult::Ok;case AccountWrite::Exists:return AccountResult::Exists;case AccountWrite::Conflict:return AccountResult::Conflict;case AccountWrite::Uncertain:return AccountResult::Uncertain;case AccountWrite::Error:return AccountResult::Unavailable;}return AccountResult::Invalid;
}
struct Lease {
    IAccountSaveStorage* storage=nullptr;std::weak_ptr<const AccountRepositoryScope> repository;
    Snapshot account,save;bool failed=false;
    bool current(){
        if(failed||repository.expired()||!storage)return false;
        if(account_save_detail::current(*storage,account.description.key,account.revision)!=AccountResult::Ok||account_save_detail::current(*storage,save.description.key,save.revision)!=AccountResult::Ok){failed=true;return false;}
        return true;
    }
};
}

//An immutable selected account/save identity. Closing/switching the repository
//revokes this lease instead of redirecting existing editor/store references.
class AccountSaveBackend final:public ISaveBackend {
public:
    bool load(u32 slot,std::vector<u8>& out)override{return read_slot(slot,out)==SaveReadStatus::Loaded;}
    SaveReadStatus read_slot(u32 slot,std::vector<u8>& out)override{
        out.clear();const auto lease=lease_.lock();if(slot>kBorgPackageSaveSlot||!lease||!lease->current())return SaveReadStatus::Error;
        const auto& record=slot==1?lease->account.records.records[1]:lease->save.records.records[slot];
        if(record.empty())return SaveReadStatus::Missing;
        out=record;return SaveReadStatus::Loaded;
    }
    bool store(u32 slot,const u8* data,std::size_t size)override{
        return store_impl(slot,data,size,nullptr);
    }
    bool store_if_save_revision(u32 slot,const u8* data,std::size_t size,const AccountSaveRevision& expected){
        // This CAS predicate is for save-local categories, not account options.
        return slot!=1&&store_impl(slot,data,size,&expected);
    }
    //Individual category deletion is not account/save management. Retire the
    //whole selected object through a separately confirmed UI, never erase slot0.
    bool erase(u32)override{return false;}
    bool current(){const auto lease=lease_.lock();return lease&&lease->current();}
    // Bytes alone cannot detect a same-lease write to another category or a
    // changed record restored to its old bytes. Bind staged UI to the revision.
    std::optional<AccountSaveRevision> revision(){
        const auto lease=lease_.lock();if(!lease||!lease->current())return {};
        return revision_of(lease);
    }
private:
    static AccountSaveRevision revision_of(const std::shared_ptr<account_save_detail::Lease>& lease){
        AccountSaveRevision next;
        next.key_=lease->save.description.key;next.revision_=lease->save.revision;next.parent_revision_=lease->account.revision;
        next.lease_=lease;return next;
    }
    bool store_impl(u32 slot,const u8* data,std::size_t size,const AccountSaveRevision* expected){
        const auto lease=lease_.lock();if(slot>kBorgPackageSaveSlot||!data||size>kAccountObjectBytes||!lease||!lease->current())return false;
        if(expected&&revision_of(lease)!=*expected)return false;
        std::vector<u8> record(data,data+size);if(!account_save_detail::framed(record))return false;
        auto next=slot==1?lease->account:lease->save;next.records.records[slot]=std::move(record);
        std::vector<u8> metadata,payload;
        if(!account_description_encode(next.description,&metadata)||!account_records_encode(next.description.key,next.records,&payload))return false;
        u64 revision=0;const auto written=lease->storage->replace(next.description.key,next.revision,slot==1?0:lease->account.revision,metadata,payload,&revision);
        if(written!=AccountWrite::Stored){if(written==AccountWrite::Uncertain||written==AccountWrite::Conflict)lease->failed=true;return false;}
        if(!revision||revision<=next.revision){lease->failed=true;return false;}
        next.revision=revision;if(slot==1)lease->account=std::move(next);else lease->save=std::move(next);return true;
    }
    friend class AccountSaveRepository;
    explicit AccountSaveBackend(std::weak_ptr<account_save_detail::Lease> lease):lease_(std::move(lease)){}
    std::weak_ptr<account_save_detail::Lease> lease_;
};

class AccountSaveRepository {
public:
    explicit AccountSaveRepository(IAccountSaveStorage& storage):storage_(storage),scope_(std::make_shared<const AccountRepositoryScope>()){}
    AccountSaveRepository(const AccountSaveRepository&)=delete;
    AccountSaveRepository& operator=(const AccountSaveRepository&)=delete;
    AccountSaveRepository(AccountSaveRepository&&)=delete;
    AccountSaveRepository& operator=(AccountSaveRepository&&)=delete;

    AccountResult accounts(u64 after,u32 limit,AccountSavePage* out){return page(0,0,after,limit,out);}
    AccountResult saves(const AccountSaveEntry& account,u64 after,u32 limit,AccountSavePage* out){
        if(!out)return AccountResult::Invalid;
        *out={};if(!valid(account)||account.key.save)return AccountResult::Invalid;
        const auto status=account_save_detail::current(storage_,account.key,account.revision);if(status!=AccountResult::Ok)return status;
        return page(account.key.account,account.revision,after,limit,out);
    }
    AccountResult create_account(u64 id,const std::string& name,AccountSaveEntry* out){return create({id,0},0,name,{},out);}
    AccountResult create_save(const AccountSaveEntry& account,u64 id,const std::string& name,AccountSaveEntry* out){
        if(!valid(account)||account.key.save||!id)return AccountResult::Invalid;
        const auto status=account_save_detail::current(storage_,account.key,account.revision);if(status!=AccountResult::Ok)return status;
        return create({account.key.account,id},account.revision,name,{},out);
    }
    AccountResult rename(const AccountSaveEntry& entry,const std::string& name,AccountSaveEntry* out){return change_description(entry,name,false,out);}
    //Logical deletion preserves a reserved tombstone/ID. Permanent removal and
    //its explicit confirmation/trash policy belong to the later management UI.
    AccountResult retire(const AccountSaveEntry& entry){return change_description(entry,entry.name,true,nullptr);}
    AccountResult open(const AccountSaveEntry& entry,std::unique_ptr<AccountSaveBackend>* out){
        if(!out||!valid(entry)||!entry.key.save)return AccountResult::Invalid;
        if(uncertain_)return AccountResult::Uncertain;
        if(active_||*out)return AccountResult::Busy;
        auto lease=std::make_shared<account_save_detail::Lease>();lease->storage=&storage_;lease->repository=scope_;
        auto result=account_save_detail::load(storage_,{entry.key.account,0},entry.parent_revision,&lease->account);if(result!=AccountResult::Ok)return result;
        result=account_save_detail::load(storage_,entry.key,entry.revision,&lease->save);if(result!=AccountResult::Ok)return result;
        if(!lease->current())return AccountResult::Conflict;
        active_=std::move(lease);out->reset(new AccountSaveBackend(active_));return AccountResult::Ok;
    }
    void close(){active_.reset();}
    bool selected(AccountSaveKey* out)const{if(out)*out={};if(!out||!active_||active_->failed)return false;*out=active_->save.description.key;return true;}
    //Save As copies the exact per-save records into a new exclusive object.
    //The old context remains selected until the UI explicitly closes/rebinds it.
    AccountResult save_as(u64 id,const std::string& name,AccountSaveEntry* out){
        if(!active_||!active_->current()||!id)return AccountResult::Invalid;
        return create({active_->save.description.key.account,id},active_->account.revision,name,active_->save.records,out);
    }
private:
    bool valid(const AccountSaveEntry& value)const{return value.status==AccountEntryStatus::Ready&&value.key.valid()&&value.revision&&value.repository.lock()==scope_&&(!value.key.save||value.parent_revision);}
    AccountSaveEntry entry(const account_save_detail::Snapshot& snapshot,u64 parent)const{return {snapshot.description.key,snapshot.revision,parent,snapshot.description.name,AccountEntryStatus::Ready,scope_};}
    AccountResult create(AccountSaveKey key,u64 parent,const std::string& name,const AccountSaveRecords& records,AccountSaveEntry* out){
        if(uncertain_)return AccountResult::Uncertain;
        AccountSaveDescription description{key,name,false};std::vector<u8> metadata,payload;
        if(!key.valid()||(key.save&&!parent)||!account_description_encode(description,&metadata)||!account_records_encode(key,records,&payload))return AccountResult::Invalid;
        u64 revision=0;const auto written=storage_.create(key,parent,metadata,payload,&revision);
        if(written==AccountWrite::Uncertain||(written==AccountWrite::Stored&&!revision)){uncertain_=true;return AccountResult::Uncertain;}
        if(written!=AccountWrite::Stored)return account_save_detail::result(written);
        if(out)*out={key,revision,parent,name,AccountEntryStatus::Ready,scope_};
        return AccountResult::Ok;
    }
    AccountResult change_description(const AccountSaveEntry& value,const std::string& name,bool retired,AccountSaveEntry* out){
        if(!valid(value)||!account_save_detail::name_valid(name))return AccountResult::Invalid;
        if(uncertain_)return AccountResult::Uncertain;
        if(active_&&active_->account.description.key.account==value.key.account)return AccountResult::Busy;
        if(value.key.save){const auto parent=account_save_detail::current(storage_,{value.key.account,0},value.parent_revision);if(parent!=AccountResult::Ok)return parent;}
        account_save_detail::Snapshot next;const auto status=account_save_detail::load(storage_,value.key,value.revision,&next);if(status!=AccountResult::Ok)return status;
        next.description.name=name;next.description.retired=retired;std::vector<u8> metadata,payload;
        if(!account_description_encode(next.description,&metadata)||!account_records_encode(value.key,next.records,&payload))return AccountResult::Invalid;
        u64 revision=0;const auto written=storage_.replace(value.key,value.revision,value.parent_revision,metadata,payload,&revision);
        if(written==AccountWrite::Uncertain||(written==AccountWrite::Stored&&revision<=value.revision)){uncertain_=true;return AccountResult::Uncertain;}
        if(written!=AccountWrite::Stored)return account_save_detail::result(written);
        next.revision=revision;if(out)*out=entry(next,value.parent_revision);return AccountResult::Ok;
    }
    AccountResult page(u64 account,u64 parent,u64 after,u32 limit,AccountSavePage* out){
        if(!out)return AccountResult::Invalid;
        *out={};if(!limit||limit>kAccountCataloguePage)return AccountResult::Invalid;
        std::vector<u64> ids;bool more=false;if(!storage_.list(account,after,limit,&ids,&more))return AccountResult::Unavailable;
        if(ids.size()>limit||(more&&ids.empty()))return AccountResult::Invalid;
        AccountSavePage next;next.next=after;next.more=more;
        for(auto id:ids){if(!id||id<=next.next)return AccountResult::Invalid;next.next=id;
            const AccountSaveKey key=account?AccountSaveKey{account,id}:AccountSaveKey{id,0};AccountMetadataRead metadata;const auto read=storage_.metadata(key,&metadata);AccountSaveDescription description;
            if(read==SaveReadStatus::Missing)continue;
            const bool valid_metadata=read==SaveReadStatus::Loaded&&metadata.revision&&account_description_decode(metadata.bytes,key,&description);
            if(valid_metadata&&description.retired)continue;
            next.entries.push_back({key,metadata.revision,parent,valid_metadata?description.name:std::string{},valid_metadata?AccountEntryStatus::Ready:read==SaveReadStatus::Loaded?AccountEntryStatus::Invalid:AccountEntryStatus::Unavailable,scope_});
        }
        *out=std::move(next);return AccountResult::Ok;
    }
    IAccountSaveStorage& storage_;std::shared_ptr<const AccountRepositoryScope> scope_;
    std::shared_ptr<account_save_detail::Lease> active_;bool uncertain_=false;
};

} //namespace gf
