#pragma once

#include <cstdio>
#include "engine/gameplay/portable_challenge.hpp"
#include "platform/pc/borg_force_browse.hpp"
#include "platform/pc/portable_challenge_scan.hpp"

namespace gf::pc {

struct PortableChallengeSelection {
    std::string id,encounter,save_root;
    AccountSaveKey save;
    PortableBattleSavedForce force;
};

struct PortableChallengeLaunch {
    std::unique_ptr<PortableChallengeRun> run;
    std::unique_ptr<BorgForceBrowseSnapshot> saved_force;
};

inline int list_portable_challenges(const std::filesystem::path& root){
    const auto challenges=scan_portable_challenges(root);
    if(challenges.status==PortableChallengeScanStatus::NoDirectory){
        std::puts("No challenges directory");return 0;
    }
    if(challenges.status!=PortableChallengeScanStatus::Complete){
        std::fputs("Challenge discovery unavailable\n",stderr);return 2;
    }
    for(const auto& entry:challenges.discovered){
        const auto& manifest=entry.manifest;
        std::printf("%s | %s | %zu encounter(s); battle not validated\n",manifest.id.c_str(),manifest.name.c_str(),
            manifest.encounters.size());
        for(const auto& encounter:manifest.encounters)std::printf("  %s\n",encounter.id.c_str());
    }
    for(const auto& issue:challenges.issues){
        const auto* reason=issue.kind==PortableChallengeIssueKind::DuplicateId?"duplicate Challenge ID":
            issue.kind==PortableChallengeIssueKind::Missing?"missing challenge.manifest":"malformed or linked Challenge package";
        std::fprintf(stderr,"Challenge package %s%s%s: %s\n",
            display_name_valid(issue.folder,255)?issue.folder.c_str():"<unprintable folder>",
            issue.id.empty()?"":" | ",issue.id.c_str(),reason);
    }
    return 0;
}

// Before opening a renderer, pin the exact Challenge and selected save lease.
// This hosted slice intentionally refuses multi-encounter manifests; the
// in-memory Challenge runner still retains every declared encounter.
inline PortableBattleCheck load_portable_challenge_launch(const PortableChallengeSelection& choice,
                                                            const BorgPackageCatalog& borgs,ForceCatalog retail,
                                                            PortableChallengeLaunch* out){
    using Status=PortableBattleStatus;
    if(!out||!borg_package_id_valid(choice.id)||!borg_catalog_variant_id_valid(choice.encounter)||
       choice.save_root.empty()||!choice.save.account||
       !choice.save.save||choice.force.index>=kForceCount)
        return {Status::Malformed,"explicit Challenge, encounter, account, save and Force are required"};
    const auto challenges=scan_portable_challenges("challenges");
    if(challenges.status==PortableChallengeScanStatus::NoDirectory)return {Status::Missing,"challenges directory is missing"};
    if(challenges.status!=PortableChallengeScanStatus::Complete)return {Status::Unavailable,"Challenge discovery unavailable"};
    const auto* entry=challenges.find(choice.id);
    if(!entry){
        for(const auto& issue:challenges.issues)if(issue.id==choice.id&&issue.kind==PortableChallengeIssueKind::DuplicateId)
            return {Status::Malformed,"duplicate Challenge ID: "+choice.id};
        return {Status::Missing,"Challenge ID is missing: "+choice.id};
    }
    if(entry->manifest.encounters.size()!=1)return {Status::Unsupported,"this host requires exactly one encounter"};
    if(!entry->manifest.find(choice.encounter))return {Status::Missing,"Challenge encounter ID is missing: "+choice.encounter};
    std::string error;
    auto saved_force=BorgForceBrowseSnapshot::open(choice.save_root,choice.save,"borgs",borgs,std::move(retail),&error);
    if(!saved_force)return {Status::Unavailable,std::move(error)};
    *out={std::make_unique<PortableChallengeRun>(entry->manifest,choice.force),std::move(saved_force)};
    return {Status::Ready,{}};
}

} // namespace gf::pc
