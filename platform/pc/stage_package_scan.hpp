#pragma once
#include <filesystem>
#include <memory>
#include "engine/assets/stage_package.hpp"
#if defined(__linux__)
#include <cerrno>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace gf::pc {
#if defined(__linux__)
namespace stage_package_scan_detail {
struct Fd {
    int value=-1;
    explicit Fd(int fd=-1):value(fd){}
    Fd(const Fd&)=delete;
    Fd& operator=(const Fd&)=delete;
    ~Fd(){if(value>=0)::close(value);}
    void reset(int fd){if(value>=0)::close(value);value=fd;}
};
inline constexpr int dir_flags=O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW;

// Anchor reads at verified directory descriptors. No symlink can redirect a
// component, even if it is swapped between discovery and admission.
inline int open_root(const std::filesystem::path& root,Fd& directory){
    if(root.empty())return ENOENT;
    directory.reset(::open(root.is_absolute()?"/":".",dir_flags));
    if(directory.value<0)return errno;
    for(const auto& component:root.relative_path()){
        if(component.empty()||component==".")continue;
        if(component==".."||component.native().find('\0')!=std::string::npos)return EINVAL;
        const int child=::openat(directory.value,component.c_str(),dir_flags);
        if(child<0)return errno;
        directory.reset(child);
    }
    return 0;
}
inline StagePackageIssue open_error(int error){
    if(error==ENOENT)return StagePackageIssue::Missing;
    if(error==ELOOP||error==ENOTDIR)return StagePackageIssue::Malformed;
    return StagePackageIssue::Unavailable;
}
inline StagePackageIssue local_asset(int folder,std::string_view path,std::size_t cap,std::vector<u8>* out){
    using Issue=StagePackageIssue;
    if(!borg_package_asset_path_valid(path))return Issue::Malformed;
    Fd subfolder;
    for(auto slash=path.find('/');slash!=std::string_view::npos;slash=path.find('/')){
        const int child=::openat(folder,std::string(path.substr(0,slash)).c_str(),dir_flags);
        if(child<0)return open_error(errno);
        subfolder.reset(child);folder=child;path.remove_prefix(slash+1);
    }
    const std::string name(path);
    struct stat status{};
    if(::fstatat(folder,name.c_str(),&status,AT_SYMLINK_NOFOLLOW)!=0)return open_error(errno);
    if(!S_ISREG(status.st_mode)||status.st_nlink!=1||status.st_size<=0||u64(status.st_size)>cap)return Issue::Malformed;
    if(!out)return Issue::None;
    Fd file(::openat(folder,name.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK));
    if(file.value<0)return open_error(errno);
    struct stat opened{};
    if(::fstat(file.value,&opened)!=0)return Issue::Unavailable;
    if(!S_ISREG(opened.st_mode)||opened.st_nlink!=1||status.st_dev!=opened.st_dev||status.st_ino!=opened.st_ino||
       status.st_size!=opened.st_size)return Issue::Changed;
    std::vector<u8> bytes(std::size_t(status.st_size));
    std::size_t at=0;
    while(at<bytes.size()){
        const auto count=::read(file.value,bytes.data()+at,bytes.size()-at);
        if(count<0&&errno==EINTR)continue;
        if(count<=0)return count?Issue::Unavailable:Issue::Changed;
        at+=std::size_t(count);
    }
    struct stat after{};
    if(::fstat(file.value,&after)!=0)return Issue::Unavailable;
    if(after.st_nlink!=1||status.st_size!=after.st_size||
       opened.st_mtim.tv_sec!=after.st_mtim.tv_sec||opened.st_mtim.tv_nsec!=after.st_mtim.tv_nsec||
       opened.st_ctim.tv_sec!=after.st_ctim.tv_sec||opened.st_ctim.tv_nsec!=after.st_ctim.tv_nsec)return Issue::Changed;
    *out=std::move(bytes);return Issue::None;
}
inline const char* issue_name(StagePackageIssue issue){
    switch(issue){
    case StagePackageIssue::Malformed:return "malformed, linked or oversized";
    case StagePackageIssue::Missing:return "missing";
    case StagePackageIssue::DuplicateId:return "duplicate stage ID";
    case StagePackageIssue::Changed:return "changed; rescan stages";
    case StagePackageIssue::Unavailable:return "unavailable";
    default:return "";
    }
}
inline StagePackageIssue read_manifest(int folder,StagePackageManifest* manifest,std::string* error){
    std::vector<u8> bytes;
    const auto issue=local_asset(folder,"stage.manifest",kStageManifestBytes,&bytes);
    if(issue!=StagePackageIssue::None){
        if(error)*error=std::string("stage.manifest: ")+issue_name(issue);
        return issue;
    }
    return stage_package_read_manifest({reinterpret_cast<const char*>(bytes.data()),bytes.size()},manifest,error)?
        StagePackageIssue::None:StagePackageIssue::Malformed;
}
}

inline StagePackageCatalog scan_stage_packages(const std::filesystem::path& root){
    using namespace stage_package_scan_detail;
    using Status=StagePackageScanStatus;
    using Issue=StagePackageIssue;
    auto failure=[](Status status){StagePackageCatalog result;result.status=status;return result;};
    Fd directory;
    if(const auto error=open_root(root,directory);error)return failure(error==ENOENT?Status::NoDirectory:Status::Unavailable);
    const int iterator=::openat(directory.value,".",dir_flags);
    if(iterator<0)return failure(Status::Unavailable);
    DIR* raw=::fdopendir(iterator);
    if(!raw){::close(iterator);return failure(Status::Unavailable);}
    std::unique_ptr<DIR,int(*)(DIR*)> stream(raw,::closedir);
    StagePackageCatalog result;result.status=Status::Complete;
    std::vector<StagePackageEntry> candidates;
    for(unsigned scanned=0;;){
        errno=0;const auto* dir=::readdir(stream.get());
        if(!dir){if(errno)return failure(Status::Unavailable);break;}
        const std::string_view name=dir->d_name;
        if(name=="."||name=="..")continue;
        if(++scanned>2048)return failure(Status::EntryBudget);
        struct stat status{};
        if(::fstatat(directory.value,dir->d_name,&status,AT_SYMLINK_NOFOLLOW)!=0)return failure(Status::Unavailable);
        if(S_ISLNK(status.st_mode)){
            result.issues.push_back({std::string(name),{},Issue::Malformed,"linked stage package entry"});continue;
        }
        if(!S_ISDIR(status.st_mode))continue;
        StagePackageEntry entry;entry.folder=name;
        Fd folder(::openat(directory.value,dir->d_name,dir_flags));
        if(folder.value<0){entry.issue=open_error(errno);entry.reason=issue_name(entry.issue);}
        else entry.issue=read_manifest(folder.value,&entry.manifest,&entry.reason);
        if(entry.issue!=Issue::None){result.issues.push_back(std::move(entry));continue;}
        for(unsigned i=0;i<kStageAssetCount;++i){
            const auto& path=entry.manifest.asset_paths[i];
            if(path.empty())continue;
            const auto issue=local_asset(folder.value,path,kStageAssetLimits[i],nullptr);
            if(issue!=Issue::None){
                entry.issue=issue;entry.reason=path+": "+issue_name(issue);break;
            }
        }
        candidates.push_back(std::move(entry));
    }
    std::sort(candidates.begin(),candidates.end(),[](const auto& left,const auto& right){return left.manifest.id<right.manifest.id;});
    for(std::size_t i=0;i<candidates.size();){
        std::size_t last=i+1;
        while(last<candidates.size()&&candidates[last].manifest.id==candidates[i].manifest.id)++last;
        if(last==i+1)result.entries.push_back(std::move(candidates[i]));
        else for(auto j=i;j<last;++j){
            auto& entry=candidates[j];entry.issue=Issue::DuplicateId;entry.reason=issue_name(entry.issue);
            result.issues.push_back(std::move(entry));
        }
        i=last;
    }
    std::sort(result.issues.begin(),result.issues.end(),[](const auto& left,const auto& right){return left.folder<right.folder;});
    return result;
}

inline bool pin_stage_package(const std::filesystem::path& root,const StagePackageCatalog& catalog,std::string_view id,
                              std::shared_ptr<const StagePackageSnapshot>* out,std::string* error){
    using namespace stage_package_scan_detail;
    using Issue=StagePackageIssue;
    const auto fail=[&](std::string message){if(error)*error="stage "+std::string(id)+": "+std::move(message);return false;};
    if(!out||!borg_package_id_valid(id))return fail("malformed stage selection");
    if(catalog.status!=StagePackageScanStatus::Complete)
        return fail(catalog.status==StagePackageScanStatus::NoDirectory?"missing stages root":"stage discovery unavailable");
    const auto* entry=catalog.find(id);
    if(!entry){
        for(const auto& issue:catalog.issues)if(issue.manifest.id==id)return fail(issue.reason);
        return fail("missing stage ID");
    }
    if(entry->issue!=Issue::None)return fail(entry->reason);
    if(entry->folder.empty()||entry->folder=="."||entry->folder==".."||
       entry->folder.find_first_of(std::string_view("/\0",2))!=std::string::npos)return fail("malformed package directory");
    Fd directory;
    if(const auto code=open_root(root,directory);code)return fail(issue_name(open_error(code)));
    Fd folder(::openat(directory.value,entry->folder.c_str(),dir_flags));
    if(folder.value<0)return fail(issue_name(open_error(errno)));
    auto next=std::make_shared<StagePackageSnapshot>();
    std::string reason;
    if(read_manifest(folder.value,&next->manifest,&reason)!=Issue::None)return fail(reason);
    if(next->manifest!=entry->manifest)return fail(issue_name(Issue::Changed));
    for(unsigned i=0;i<kStageAssetCount;++i){
        const auto& path=next->manifest.asset_paths[i];
        if(path.empty())continue;
        const auto issue=local_asset(folder.value,path,kStageAssetLimits[i],&next->bytes[i]);
        if(issue!=Issue::None)return fail(path+": "+issue_name(issue));
    }
    StagePackageManifest confirmed;
    if(read_manifest(folder.value,&confirmed,&reason)!=Issue::None)return fail(reason);
    if(confirmed!=next->manifest)return fail(issue_name(Issue::Changed));
    *out=std::move(next);if(error)error->clear();return true;
}
#else
inline StagePackageCatalog scan_stage_packages(const std::filesystem::path&){return {};}
inline bool pin_stage_package(const std::filesystem::path&,const StagePackageCatalog&,std::string_view,
                              std::shared_ptr<const StagePackageSnapshot>*,std::string* error){
    if(error)*error="unsupported host stage discovery: no no-follow file adapter";
    return false;
}
#endif
} // namespace gf::pc
