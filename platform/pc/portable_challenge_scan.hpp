#pragma once

#include <filesystem>
#include <memory>
#include <span>
#include "engine/assets/portable_challenge.hpp"
#include "platform/pc/stage_package_scan.hpp"

namespace gf::pc {

inline constexpr std::size_t kPortableChallengeScanEntries=2048;
inline constexpr std::size_t kPortableChallengeScanBytes=8u<<20;
inline constexpr std::size_t kPortableChallengeScanRoots=32;

// Scan all roots together so a packaged retail import cannot take precedence
// over a custom package of the same ID (or vice versa). One level, no assets
// decoded; the returned catalogue owns its parsed data.
inline PortableChallengeCatalog scan_portable_challenges(std::span<const std::filesystem::path> roots){
    using Status=PortableChallengeScanStatus;
    const auto failure=[](Status status){
        PortableChallengeCatalog empty;
        empty.status=status;
        return empty;
    };
    if(roots.size()>kPortableChallengeScanRoots)return failure(Status::EntryBudget);
#if defined(__linux__)
    // Reuse the stage loader's bounded, descriptor-relative package reader.
    // No link can redirect a package or manifest open during discovery.
    using namespace stage_package_scan_detail;
    using Issue=PortableChallengeIssueKind;
    PortableChallengeCatalog result;
    result.status=Status::NoDirectory;
    std::vector<PortableChallengeEntry> candidates;
    std::size_t entries=0,bytes=0;
    for(const auto& root:roots){
        Fd directory;
        if(const auto code=open_root(root,directory);code){
            if(code==ENOENT)continue;
            return failure(Status::Unavailable);
        }
        result.status=Status::Complete;
        const int iterator=::openat(directory.value,".",dir_flags);
        if(iterator<0)return failure(Status::Unavailable);
        DIR* raw=::fdopendir(iterator);
        if(!raw){::close(iterator);return failure(Status::Unavailable);}
        const std::unique_ptr<DIR,int(*)(DIR*)> stream(raw,::closedir);
        for(;;){
            errno=0;
            const auto* entry=::readdir(stream.get());
            if(!entry){if(errno)return failure(Status::Unavailable);break;}
            const std::string_view name=entry->d_name;
            if(name=="."||name=="..")continue;
            if(++entries>kPortableChallengeScanEntries)return failure(Status::EntryBudget);
            struct stat status{};
            if(::fstatat(directory.value,entry->d_name,&status,AT_SYMLINK_NOFOLLOW)!=0)return failure(Status::Unavailable);
            if(S_ISLNK(status.st_mode)){
                result.issues.push_back({std::string(name),{},Issue::Malformed});
                continue;
            }
            if(!S_ISDIR(status.st_mode))continue;
            Fd folder(::openat(directory.value,entry->d_name,dir_flags));
            std::vector<u8> text;
            const auto issue=folder.value<0?open_error(errno):
                local_asset(folder.value,"challenge.manifest",kPortableChallengeManifestBytes,&text);
            if(issue!=StagePackageIssue::None){
                if(issue==StagePackageIssue::Unavailable||issue==StagePackageIssue::Changed)return failure(Status::Unavailable);
                result.issues.push_back({std::string(name),{},issue==StagePackageIssue::Missing?Issue::Missing:Issue::Malformed});
                continue;
            }
            if(text.size()>kPortableChallengeScanBytes-bytes)return failure(Status::EntryBudget);
            bytes+=text.size();
            PortableChallengeManifest manifest;
            if(!portable_challenge_read_manifest({reinterpret_cast<const char*>(text.data()),text.size()},&manifest)){
                result.issues.push_back({std::string(name),{},Issue::Malformed});
                continue;
            }
            candidates.push_back({std::string(name),std::move(manifest)});
        }
    }
    std::sort(candidates.begin(),candidates.end(),[](const auto& a,const auto& b){return a.manifest.id<b.manifest.id;});
    for(std::size_t first=0;first<candidates.size();){
        auto last=first+1;
        const auto& id=candidates[first].manifest.id;
        while(last<candidates.size()&&candidates[last].manifest.id==id)++last;
        if(last==first+1)result.discovered.push_back(std::move(candidates[first]));
        else for(auto i=first;i<last;++i)result.issues.push_back({candidates[i].folder,id,Issue::DuplicateId});
        first=last;
    }
    std::sort(result.issues.begin(),result.issues.end(),[](const auto& a,const auto& b){
        if(a.folder!=b.folder)return a.folder<b.folder;
        if(a.id!=b.id)return a.id<b.id;
        return a.kind<b.kind;
    });
    return result;
#else
    return failure(Status::Unavailable); // No safe no-follow package reader.
#endif
}

inline PortableChallengeCatalog scan_portable_challenges(const std::filesystem::path& root){
    return scan_portable_challenges(std::span(&root,1));
}

} // namespace gf::pc
