#pragma once
#include <map>
#include "engine/save/account_save_repository.hpp"

namespace account_save_fixture {
using namespace gf;
//Owned serialized storage with real exclusive create/parent-CAS/object-CAS.
//No filesystem, credentials, user saves, native assets or platform initialization.
class Storage final:public IAccountSaveStorage {
public:
    std::map<AccountSaveKey,AccountObjectRead> objects;
    u64 sequence=1;unsigned metadata_reads=0,full_reads=0,write_calls=0,commits=0;
    bool read_error=false,list_error=false,bad_order=false,commit_uncertain=false;
    AccountWrite next_write=AccountWrite::Stored;
    SaveReadStatus metadata(AccountSaveKey key,AccountMetadataRead* out)override{
        ++metadata_reads;if(!out)return SaveReadStatus::Error;*out={};if(read_error)return SaveReadStatus::Error;
        const auto i=objects.find(key);if(i==objects.end())return SaveReadStatus::Missing;
        out->revision=i->second.revision;out->bytes=i->second.metadata;return SaveReadStatus::Loaded;
    }
    SaveReadStatus read(AccountSaveKey key,AccountObjectRead* out)override{
        ++full_reads;if(!out)return SaveReadStatus::Error;*out={};if(read_error)return SaveReadStatus::Error;
        const auto i=objects.find(key);if(i==objects.end())return SaveReadStatus::Missing;
        *out=i->second;return SaveReadStatus::Loaded;
    }
    bool list(u64 account,u64 after,u32 limit,std::vector<u64>* out,bool* more)override{
        if(!out||!more)return false;
        out->clear();*more=false;if(list_error)return false;
        for(const auto& [key,object]:objects){(void)object;const bool match=account?key.account==account&&key.save!=0:key.save==0;const auto id=account?key.save:key.account;if(!match||id<=after)continue;if(out->size()==limit){*more=true;break;}out->push_back(id);}
        if(bad_order&&!out->empty())out->push_back(out->back());
        return true;
    }
    AccountWrite create(AccountSaveKey key,u64 parent,const std::vector<u8>& metadata,const std::vector<u8>& payload,u64* revision)override{return write(key,0,parent,metadata,payload,revision,true);}
    AccountWrite replace(AccountSaveKey key,u64 expected,u64 parent,const std::vector<u8>& metadata,const std::vector<u8>& payload,u64* revision)override{return write(key,expected,parent,metadata,payload,revision,false);}
private:
    AccountWrite write(AccountSaveKey key,u64 expected,u64 parent,const std::vector<u8>& metadata,const std::vector<u8>& payload,u64* revision,bool create){
        ++write_calls;if(!revision)return AccountWrite::Error;*revision=0;
        if(!key.valid()||metadata.size()>kAccountMetadataBytes||payload.size()>kAccountObjectBytes||sequence==~u64(0))return AccountWrite::Error;
        if(key.save){const auto p=objects.find({key.account,0});if(p==objects.end()||p->second.revision!=parent)return AccountWrite::Conflict;}else if(parent)return AccountWrite::Error;
        const auto old=objects.find(key);if(create&&old!=objects.end())return AccountWrite::Exists;if(!create&&(old==objects.end()||old->second.revision!=expected))return AccountWrite::Conflict;
        const auto result=next_write;next_write=AccountWrite::Stored;
        if(result!=AccountWrite::Stored&&(result!=AccountWrite::Uncertain||!commit_uncertain))return result;
        const auto version=sequence++;objects[key]={version,metadata,payload};++commits;
        if(result==AccountWrite::Uncertain)return result;
        *revision=version;return AccountWrite::Stored;
    }
};
}
