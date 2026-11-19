#pragma once
#include <filesystem>
#include <fstream>
#include "engine/assets/borg_package.hpp"

namespace gf::pc {
namespace borg_package_scan_detail {
inline bool local_file(const std::filesystem::path& folder,const std::filesystem::path& path){
    std::error_code error;auto current=folder;
    for(auto it=path.begin();it!=path.end();){
        current/=*it++;
        const auto status=std::filesystem::symlink_status(current,error);
        if(error||(it==path.end()?!std::filesystem::is_regular_file(status):!std::filesystem::is_directory(status)))return false;
    }
    return true;
}
inline bool read(const std::filesystem::path& folder,BorgPackageManifest* out){
    constexpr auto filename="borg.manifest";
    if(!local_file(folder,filename))return false;
    std::ifstream file(folder/filename,std::ios::binary|std::ios::ate);
    if(!file)return false;
    const auto size=file.tellg();
    if(size<=0||size>std::streamoff(kBorgManifestBytes))return false;
    std::string contents(static_cast<std::size_t>(size),'\0');
    file.seekg(0,std::ios::beg);
    return bool(file.read(contents.data(),std::streamsize(size)))&&borg_package_read_manifest(contents,out);
}
}

// Read-only, one package directory deep. A failed root scan never produces a
// partial roster that could be mistaken for many missing Borgs.
inline BorgPackageCatalog scan_borg_packages(const std::filesystem::path& root){
    using Status=BorgPackageScanStatus;
    auto failure=[](Status status){BorgPackageCatalog result;result.status=status;return result;};
    std::error_code error;const auto root_status=std::filesystem::symlink_status(root,error);
    if(error==std::errc::no_such_file_or_directory||root_status.type()==std::filesystem::file_type::not_found)
        return failure(Status::NoDirectory);
    if(error||!std::filesystem::is_directory(root_status))return failure(Status::Unavailable);
    BorgPackageCatalog result;result.status=Status::Complete;
    struct Candidate {BorgPackageEntry entry;bool has_files=false;};
    std::vector<Candidate> candidates;
    std::size_t scanned=0;
    std::filesystem::directory_iterator it(root,error),end;
    for(;!error&&it!=end;it.increment(error)){
        if(++scanned>2048)return failure(Status::EntryBudget);
        const auto status=it->symlink_status(error);
        if(error)break;
        if(!std::filesystem::is_directory(status))continue;
        const auto& folder=it->path();const auto name=folder.filename().string();
        BorgPackageManifest manifest;
        if(!borg_package_scan_detail::read(folder,&manifest)){
            result.issues.push_back({name,{},BorgPackageIssueKind::InvalidManifest});continue;
        }
        const bool has_files=borg_package_scan_detail::local_file(folder,manifest.model)&&
            borg_package_scan_detail::local_file(folder,manifest.animations)&&borg_package_scan_detail::local_file(folder,manifest.rules)&&
            (manifest.animation_map.empty()||borg_package_scan_detail::local_file(folder,manifest.animation_map));
        candidates.push_back({{name,std::move(manifest)},has_files});
    }
    if(error)return failure(Status::Unavailable);
    std::sort(candidates.begin(),candidates.end(),[](const auto& a,const auto& b){
        return a.entry.manifest.identity.id<b.entry.manifest.identity.id;
    });
    for(std::size_t i=0;i<candidates.size();){
        std::size_t last=i+1;const auto& id=candidates[i].entry.manifest.identity.id;
        while(last<candidates.size()&&candidates[last].entry.manifest.identity.id==id)++last;
        if(last-i>1){
            for(auto j=i;j<last;++j)result.issues.push_back({candidates[j].entry.folder,id,BorgPackageIssueKind::DuplicateId});
        }else if(!candidates[i].has_files)result.issues.push_back({candidates[i].entry.folder,id,BorgPackageIssueKind::MissingAsset});
        else result.discovered.push_back(std::move(candidates[i].entry));
        i=last;
    }
    std::sort(result.issues.begin(),result.issues.end(),[](const auto& a,const auto& b){return a.folder<b.folder;});
    return result;
}
}
