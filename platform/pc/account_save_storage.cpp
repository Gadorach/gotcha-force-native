#include "platform/pc/account_save_storage.hpp"
#include "engine/save/save_engine.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <mutex>
#if defined(__linux__)
#include <dirent.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace gf::pc {
#if defined(__linux__)
namespace {
constexpr std::size_t header_bytes=52;
constexpr const char* sequence_name=".gf-profile-sequence";
struct Fd {
    int value=-1;explicit Fd(int v=-1):value(v){}~Fd(){if(value>=0)::close(value);}
    Fd(const Fd&)=delete;Fd& operator=(const Fd&)=delete;
};
bool private_regular(int fd,struct stat* out){struct stat s{};if(::fstat(fd,&s)||!S_ISREG(s.st_mode)||s.st_uid!=::geteuid()||s.st_nlink!=1||(s.st_mode&077))return false;if(out)*out=s;return true;}
bool read_at(int fd,u8* data,std::size_t size,std::size_t offset){
    while(size){const auto n=::pread(fd,data,size,off_t(offset));if(n<0&&errno==EINTR)continue;if(n<=0)return false;data+=n;size-=std::size_t(n);offset+=std::size_t(n);}return true;
}
bool write_all(int fd,const u8* data,std::size_t size){
    while(size){const auto n=::write(fd,data,size);if(n<0&&errno==EINTR)continue;if(n<=0)return false;data+=n;size-=std::size_t(n);}return true;
}
void put64(u8* p,u64 value){store_be32(p,u32(value>>32));store_be32(p+4,u32(value));}
u64 get64(const u8* p){return (u64(load_be32(p))<<32)|load_be32(p+4);}
std::string hex(u64 id){char out[17];std::snprintf(out,sizeof(out),"%016llx",static_cast<unsigned long long>(id));return out;}
bool parse_hex(const std::string& s,u64* out){if(!out||s.size()!=16)return false;u64 id=0;for(char c:s){const unsigned n=c>='0'&&c<='9'?unsigned(c-'0'):c>='a'&&c<='f'?unsigned(c-'a'+10):16;if(n==16)return false;id=(id<<4)|n;}*out=id;return id!=0;}
std::string filename(AccountSaveKey key){return key.save?"s_"+hex(key.account)+"_"+hex(key.save)+".gfs":"a_"+hex(key.account)+".gfac";}
bool parse_filename(const std::string& name,AccountSaveKey* key){
    if(name.size()==23&&name.compare(0,2,"a_")==0&&name.compare(18,5,".gfac")==0){key->save=0;return parse_hex(name.substr(2,16),&key->account);}
    if(name.size()==39&&name.compare(0,2,"s_")==0&&name[18]=='_'&&name.compare(35,4,".gfs")==0)return parse_hex(name.substr(2,16),&key->account)&&parse_hex(name.substr(19,16),&key->save);
    return false;
}
bool object_name(const std::string& name){return name.starts_with("a_")||name.starts_with("s_");}

class Storage final:public IAccountSaveStorage {
public:
    Storage(int root,int lock,AccountSaveIoStats* stats):root_(root),lock_(lock),stats_(stats){}
    ~Storage()override{::close(lock_);::close(root_);}
    SaveReadStatus metadata(AccountSaveKey key,AccountMetadataRead* out)override{
        if(!out)return SaveReadStatus::Error;
        *out={};Guard guard(*this,false);if(!guard.ok)return SaveReadStatus::Error;
        AccountObjectRead object;const auto status=read_unlocked(key,false,&object);if(status==SaveReadStatus::Loaded){out->revision=object.revision;out->bytes=std::move(object.metadata);}if(stats_)++stats_->metadata_reads;return status;
    }
    SaveReadStatus read(AccountSaveKey key,AccountObjectRead* out)override{
        if(!out)return SaveReadStatus::Error;
        *out={};Guard guard(*this,false);if(!guard.ok)return SaveReadStatus::Error;
        const auto status=read_unlocked(key,true,out);if(stats_)++stats_->full_reads;return status;
    }
    bool list(u64 account,u64 after,u32 limit,std::vector<u64>* out,bool* more)override{
        if(!out||!more)return false;
        out->clear();*more=false;if(!limit||limit>kAccountCataloguePage)return false;
        Guard guard(*this,false);if(!guard.ok)return false;
        Fd fd(::openat(root_,".",O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW));if(fd.value<0)return false;
        DIR* directory=::fdopendir(fd.value);if(!directory)return false;fd.value=-1;
        std::vector<u64> ids;bool ok=true;errno=0;
        while(const auto* item=::readdir(directory)){
            const std::string name=item->d_name;AccountSaveKey key;if(!parse_filename(name,&key)){if(object_name(name)){ok=false;break;}continue;}
            struct stat info{};if(::fstatat(root_,name.c_str(),&info,AT_SYMLINK_NOFOLLOW)||!S_ISREG(info.st_mode)||info.st_uid!=::geteuid()||info.st_nlink!=1||(info.st_mode&077)){ok=false;break;}
            const bool match=account?key.account==account&&key.save:key.save==0;const auto id=account?key.save:key.account;if(!match||id<=after)continue;
            const auto at=std::lower_bound(ids.begin(),ids.end(),id);if(at!=ids.end()&&*at==id){ok=false;break;}ids.insert(at,id);if(ids.size()>std::size_t(limit)+1)ids.pop_back();errno=0;
        }
        if(errno)ok=false;
        ::closedir(directory);if(!ok)return false;
        *more=ids.size()>limit;if(*more)ids.pop_back();*out=std::move(ids);return true;
    }
    AccountWrite create(AccountSaveKey key,u64 parent,const std::vector<u8>& metadata,const std::vector<u8>& payload,u64* revision)override{return write(key,0,parent,metadata,payload,revision,true);}
    AccountWrite replace(AccountSaveKey key,u64 expected,u64 parent,const std::vector<u8>& metadata,const std::vector<u8>& payload,u64* revision)override{return write(key,expected,parent,metadata,payload,revision,false);}
private:
    struct Guard {
        Storage& owner;std::unique_lock<std::mutex> mutex;bool ok=false;
        Guard(Storage& s,bool exclusive):owner(s),mutex(s.mutex_){ok=::flock(s.lock_,(exclusive?LOCK_EX:LOCK_SH)|LOCK_NB)==0;}
        ~Guard(){if(ok)::flock(owner.lock_,LOCK_UN);}
    };
    SaveReadStatus read_unlocked(AccountSaveKey key,bool full,AccountObjectRead* out){
        *out={};if(!key.valid())return SaveReadStatus::Error;
        Fd fd(::openat(root_,filename(key).c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC));if(fd.value<0)return errno==ENOENT?SaveReadStatus::Missing:SaveReadStatus::Error;
        struct stat info{};std::array<u8,header_bytes> header{};
        if(!private_regular(fd.value,&info)||info.st_size<off_t(header_bytes)||!read_at(fd.value,header.data(),header.size(),0))return SaveReadStatus::Error;
        if(stats_)stats_->metadata_bytes+=header.size();
        if(load_be32(header.data())!=0x4746504fu||load_be32(header.data()+4)!=1||get64(header.data()+8)!=key.account||get64(header.data()+16)!=key.save||!get64(header.data()+24)||u32(fnv1a64(header.data(),48))!=load_be32(header.data()+48))return SaveReadStatus::Error;
        const auto metadata_size=load_be32(header.data()+32),payload_size=load_be32(header.data()+36);
        if(metadata_size>kAccountMetadataBytes||payload_size>kAccountObjectBytes||u64(info.st_size)!=header_bytes+u64(metadata_size)+payload_size)return SaveReadStatus::Error;
        AccountObjectRead next;next.revision=get64(header.data()+24);next.metadata.resize(metadata_size);
        if(!read_at(fd.value,next.metadata.data(),next.metadata.size(),header_bytes)||u32(fnv1a64(next.metadata.data(),next.metadata.size()))!=load_be32(header.data()+40))return SaveReadStatus::Error;
        if(stats_)stats_->metadata_bytes+=next.metadata.size();
        if(full){next.payload.resize(payload_size);if(!read_at(fd.value,next.payload.data(),next.payload.size(),header_bytes+metadata_size)||u32(fnv1a64(next.payload.data(),next.payload.size()))!=load_be32(header.data()+44))return SaveReadStatus::Error;if(stats_)stats_->payload_bytes+=next.payload.size();}
        *out=std::move(next);return SaveReadStatus::Loaded;
    }
    bool existing_objects(bool* found){
        *found=false;Fd fd(::openat(root_,".",O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW));if(fd.value<0)return false;DIR* directory=::fdopendir(fd.value);if(!directory)return false;fd.value=-1;errno=0;
        while(const auto* item=::readdir(directory)){const std::string name=item->d_name;if(object_name(name)||name.starts_with(".gf-profile-tmp-")){*found=true;break;}}
        const bool ok=errno==0;::closedir(directory);return ok;
    }
    AccountWrite publish(const std::string& target,const std::vector<u8>& bytes,bool exclusive,const std::string& tag){
        const auto temporary=".gf-profile-tmp-"+tag+"-"+hex(u64(::getpid()))+"-"+hex(++temporary_serial_);
        Fd fd(::openat(root_,temporary.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600));if(fd.value<0)return AccountWrite::Error;
        if(!write_all(fd.value,bytes.data(),bytes.size())||::fsync(fd.value)){::unlinkat(root_,temporary.c_str(),0);return AccountWrite::Error;}
        const int published=exclusive ? ::linkat(root_,temporary.c_str(),root_,target.c_str(),0) : ::renameat(root_,temporary.c_str(),root_,target.c_str());
        if(published){const int why=errno;if(why!=EIO&&why!=EINTR)::unlinkat(root_,temporary.c_str(),0);return why==EIO||why==EINTR?AccountWrite::Uncertain:why==EEXIST?AccountWrite::Exists:AccountWrite::Error;}
        if(exclusive&&::unlinkat(root_,temporary.c_str(),0))return AccountWrite::Uncertain;
        if(::fsync(root_))return AccountWrite::Uncertain;
        return AccountWrite::Stored;
    }
    AccountWrite reserve_revision(u64* revision){
        u64 next=1;bool exists=false;Fd fd(::openat(root_,sequence_name,O_RDONLY|O_NOFOLLOW|O_CLOEXEC));
        if(fd.value<0){if(errno!=ENOENT)return AccountWrite::Error;bool found=false;if(!existing_objects(&found)||found)return AccountWrite::Error;}
        else{exists=true;std::array<u8,20> bytes{};struct stat info{};if(!private_regular(fd.value,&info)||info.st_size!=20||!read_at(fd.value,bytes.data(),bytes.size(),0)||load_be32(bytes.data())!=0x47465351u||load_be32(bytes.data()+4)!=1||u32(fnv1a64(bytes.data(),16))!=load_be32(bytes.data()+16))return AccountWrite::Error;next=get64(bytes.data()+8);}
        if(!next||next==~u64(0))return AccountWrite::Error;
        std::vector<u8> bytes(20);store_be32(bytes.data(),0x47465351u);store_be32(bytes.data()+4,1);put64(bytes.data()+8,next+1);store_be32(bytes.data()+16,u32(fnv1a64(bytes.data(),16)));
        const auto status=publish(sequence_name,bytes,!exists,"sequence");if(status==AccountWrite::Stored)*revision=next;return status;
    }
    AccountWrite write(AccountSaveKey key,u64 expected,u64 parent,const std::vector<u8>& metadata,const std::vector<u8>& payload,u64* revision,bool create){
        if(!revision)return AccountWrite::Error;
        *revision=0;
        if(!key.valid()||metadata.size()>kAccountMetadataBytes||payload.size()>kAccountObjectBytes||temporary_serial_==~u64(0))return AccountWrite::Error;
        Guard guard(*this,true);if(!guard.ok)return AccountWrite::Error;
        if(key.save){AccountObjectRead account;if(read_unlocked({key.account,0},false,&account)!=SaveReadStatus::Loaded||account.revision!=parent)return AccountWrite::Conflict;}else if(parent)return AccountWrite::Error;
        struct stat existing{};const auto name=filename(key);const bool present=::fstatat(root_,name.c_str(),&existing,AT_SYMLINK_NOFOLLOW)==0;
        if(create&&present)return AccountWrite::Exists;
        if(!present&&errno!=ENOENT)return AccountWrite::Error;
        if(!create){AccountObjectRead old;if(read_unlocked(key,false,&old)!=SaveReadStatus::Loaded||old.revision!=expected)return AccountWrite::Conflict;}
        u64 version=0;const auto reserved=reserve_revision(&version);if(reserved!=AccountWrite::Stored)return reserved;
        std::vector<u8> bytes(header_bytes+metadata.size()+payload.size());store_be32(bytes.data(),0x4746504fu);store_be32(bytes.data()+4,1);put64(bytes.data()+8,key.account);put64(bytes.data()+16,key.save);put64(bytes.data()+24,version);store_be32(bytes.data()+32,u32(metadata.size()));store_be32(bytes.data()+36,u32(payload.size()));store_be32(bytes.data()+40,u32(fnv1a64(metadata.data(),metadata.size())));store_be32(bytes.data()+44,u32(fnv1a64(payload.data(),payload.size())));store_be32(bytes.data()+48,u32(fnv1a64(bytes.data(),48)));std::copy(metadata.begin(),metadata.end(),bytes.begin()+header_bytes);std::copy(payload.begin(),payload.end(),bytes.begin()+header_bytes+metadata.size());
        const auto stored=publish(name,bytes,create,"object-"+hex(version));if(stored==AccountWrite::Stored){*revision=version;if(stats_)++stats_->commits;}return stored;
    }
    int root_=-1,lock_=-1;AccountSaveIoStats* stats_=nullptr;std::mutex mutex_;u64 temporary_serial_=0;
};
}
#endif

std::unique_ptr<IAccountSaveStorage> create_account_save_storage(const std::string& root,bool create,std::string* error,AccountSaveIoStats* stats){
#if defined(__linux__)
    auto fail=[&](const char* message)->std::unique_ptr<IAccountSaveStorage>{if(error)*error=std::string("account storage: ")+message;return {};};
    if(root.empty()||root.find('\0')!=std::string::npos)return fail("invalid explicit root");
    if(create&&::mkdir(root.c_str(),0700)&&errno!=EEXIST)return fail("cannot create explicit root");
    Fd directory(::open(root.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));if(directory.value<0)return fail("root missing, unreadable or symlink");
    struct stat info{};if(::fstat(directory.value,&info)||!S_ISDIR(info.st_mode)||info.st_uid!=::geteuid()||(info.st_mode&0022))return fail("root ownership/permissions");
    Fd lock(::openat(directory.value,".gf-profile.lock",O_RDWR|O_CREAT|O_NOFOLLOW|O_CLOEXEC,0600));if(lock.value<0||!private_regular(lock.value,nullptr))return fail("lock ownership/permissions");
    auto result=std::make_unique<Storage>(directory.value,lock.value,stats);directory.value=lock.value=-1;if(error)error->clear();return result;
#else
    (void)root;(void)create;(void)stats;if(error)*error="account storage: Linux adapter unavailable on this platform";return {};
#endif
}
}
