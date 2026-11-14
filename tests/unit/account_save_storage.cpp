#include <atomic>
#include <barrier>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <thread>
#include "platform/pc/account_save_storage.hpp"
#include "engine/save/account_save_repository.hpp"
#include "engine/gameplay/option_settings.hpp"
#if defined(__linux__)
#include <sys/stat.h>
#endif

namespace {
using namespace gf;unsigned checks=0;std::atomic<int> fail_after{-1};
void check(bool ok,const char* what){++checks;if(!ok){std::fprintf(stderr,"account filesystem: %s\n",what);std::abort();}}
std::vector<u8> read_file(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
AccountSaveEntry account(AccountSaveRepository& r,u64 id){AccountSavePage p;check(r.accounts(0,64,&p)==AccountResult::Ok,"account page");for(const auto& e:p.entries)if(e.key.account==id)return e;check(false,"expected account");return {};}
AccountSaveEntry save(AccountSaveRepository& r,u64 owner,u64 id){auto a=account(r,owner);AccountSavePage p;check(r.saves(a,0,64,&p)==AccountResult::Ok,"save page");for(const auto& e:p.entries)if(e.key.save==id)return e;check(false,"expected save");return {};}
}
extern "C" int __real_fsync(int);
extern "C" int __wrap_fsync(int fd){const auto n=fail_after.load();if(n>=0&&fail_after.fetch_sub(1)==0){errno=EIO;return -1;}return __real_fsync(fd);}

int main(int argc,char** argv){
#if !defined(__linux__)
    (void)argc;(void)argv;return 77;
#else
    using namespace gf;if(argc!=2)return 2;const std::filesystem::path root=argv[1];check(std::filesystem::is_directory(root)&&std::filesystem::is_empty(root),"exclusive empty runner-owned temporary root");std::string error;pc::AccountSaveIoStats io;auto disk=pc::create_account_save_storage(root.string(),false,&error,&io);check(bool(disk),"explicit Linux store, no SDL/device initialization");
    {std::ofstream sentinel(root/"slot0.sav",std::ios::binary);sentinel<<"UNRELATED LEGACY SENTINEL";}const auto legacy=read_file(root/"slot0.sav");AccountSaveRepository repository(*disk);AccountSaveEntry a;
    for(u64 id=1;id<=4;++id){check(repository.create_account(id,"Player "+std::to_string(id),&a)==AccountResult::Ok,"persist distinct account marker");for(u64 s=1;s<=12;++s)check(repository.create_save(a,s,"Save "+std::to_string(s),nullptr)==AccountResult::Ok,"persist more than two save files per account");}
    check(read_file(root/"slot0.sav")==legacy,"no implicit legacy import or overwrite");const auto payload_before=io.payload_bytes,full_before=io.full_reads;a=account(repository,2);AccountSavePage page;u64 cursor=0;unsigned count=0;
    do{check(repository.saves(a,cursor,5,&page)==AccountResult::Ok,"bounded real directory metadata page");for(const auto& e:page.entries){check(e.key.account==2&&e.key.save>cursor&&e.status==AccountEntryStatus::Ready,"stable scoped IDs and readable metadata");++count;}cursor=page.next;}while(page.more);check(count==12&&io.payload_bytes==payload_before&&io.full_reads==full_before,"scrolling does not read full save payloads");
    auto s=save(repository,2,1);std::unique_ptr<AccountSaveBackend> selected;check(repository.open(s,&selected)==AccountResult::Ok,"open actual persisted save context");OptionSettings preferences;preferences.mono=1;preferences.volume={1,2,3};OptionSettingsStore options(*selected);check(options.write(preferences),"existing OPTION codec writes account preferences");repository.close();selected.reset();disk.reset();disk=pc::create_account_save_storage(root.string(),false,&error,&io);check(bool(disk),"fresh storage instance opens existing marker/files");AccountSaveRepository reopened(*disk);s=save(reopened,2,2);check(reopened.open(s,&selected)==AccountResult::Ok,"same account other save after reopen");OptionSettings loaded;OptionSettingsStore reloaded_options(*selected);check(reloaded_options.load(&loaded)==OptionSettingsLoad::Loaded&&loaded==preferences,"account preferences survive process-storage reconstruction");reopened.close();selected.reset();
    auto target=save(reopened,2,3);AccountSaveEntry renamed;const auto path=root/"s_0000000000000002_0000000000000003.gfs";const auto prior=read_file(path);fail_after=2;check(reopened.rename(target,"must not commit",nullptr)==AccountResult::Unavailable,"prepublication fsync failure is known-no-write");fail_after=-1;check(read_file(path)==prior,"failed replacement preserves exact prior save bytes");check(reopened.rename(target,"Renamed",&renamed)==AccountResult::Ok&&renamed.key==target.key,"fresh requested replacement succeeds, identity retained");
    auto other_disk=pc::create_account_save_storage(root.string(),false,&error);check(bool(other_disk),"separate store instance");AccountSaveRepository other(*other_disk);auto x=save(reopened,2,4),y=save(other,2,4);std::barrier ready(2);AccountResult rx=AccountResult::Invalid,ry=AccountResult::Invalid;
    std::thread one([&](){ready.arrive_and_wait();rx=reopened.rename(x,"Writer X",nullptr);});std::thread two([&](){ready.arrive_and_wait();ry=other.rename(y,"Writer Y",nullptr);});one.join();two.join();check(unsigned(rx==AccountResult::Ok)+unsigned(ry==AccountResult::Ok)==1,"two stores cannot commit same stale revision twice");
    auto uncertain=save(reopened,2,5);fail_after=3;check(reopened.rename(uncertain,"Published but uncertain",nullptr)==AccountResult::Uncertain,"postpublication directory fsync failure is uncertain, not success");fail_after=-1;const auto commits=io.commits;check(reopened.rename(uncertain,"never replay",nullptr)==AccountResult::Uncertain&&io.commits==commits,"uncertain repository refuses automatic repeated write");AccountObjectRead actual;check(disk->read(uncertain.key,&actual)==SaveReadStatus::Loaded,"uncertain result can be inspected read-only");AccountSaveDescription description;check(account_description_decode(actual.metadata,uncertain.key,&description)&&description.name=="Published but uncertain","inspection preserves actual outcome, never rollback/replay");
    AccountSaveRepository corruption(*disk);auto bad=save(corruption,3,1);const auto bad_path=root/"s_0000000000000003_0000000000000001.gfs";{std::fstream f(bad_path,std::ios::binary|std::ios::in|std::ios::out);f.seekg(-1,std::ios::end);char byte=0;f.read(&byte,1);byte^=1;f.seekp(-1,std::ios::end);f.write(&byte,1);}AccountMetadataRead metadata;check(disk->metadata(bad.key,&metadata)==SaveReadStatus::Loaded,"metadata-only scan remains independent of payload corruption");check(corruption.open(bad,&selected)==AccountResult::Unavailable&&!selected,"full load refuses corrupted payload without writable defaults");
    const auto missing=root.string()+"-not-created";check(!pc::create_account_save_storage(missing,false,&error)&&!std::filesystem::exists(missing),"read-only root open does not create missing directories");const auto link=root.string()+"-link";std::filesystem::create_directory_symlink(root,link);check(!pc::create_account_save_storage(link,false,&error),"root symlink refused");
    std::filesystem::create_symlink(root/"slot0.sav",root/"a_00000000000003e7.gfac");check(disk->metadata({999,0},&metadata)==SaveReadStatus::Error,"object symlink cannot escape selected storage root");std::vector<u64> ids;bool more=false;check(!disk->list(0,0,64,&ids,&more),"unsafe catalogue object is not silently accepted");check(read_file(root/"slot0.sav")==legacy,"all management/fault cases retain legacy sentinel");
    std::printf("Linux account/save storage: %u owned checks PASS; metadata%llu payload%llu\n",checks,static_cast<unsigned long long>(io.metadata_bytes),static_cast<unsigned long long>(io.payload_bytes));return 0;
#endif
}
