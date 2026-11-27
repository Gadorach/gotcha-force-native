#pragma once

#include <optional>
#include "engine/assets/battle_roster_preset.hpp"
#include "platform/pc/stage_package_scan.hpp"

namespace gf::pc {

#if defined(__linux__)
namespace battle_roster_load_detail {
inline bool read_file(int folder,std::string_view name,BattleRosterPreset* out,std::string* error){
    using namespace stage_package_scan_detail;
    std::vector<u8> bytes;
    const auto issue=local_asset(folder,name,kBattleRosterPresetBytes,&bytes);
    if(issue!=StagePackageIssue::None){if(error)*error=issue_name(issue);return false;}
    return battle_roster_read_preset({reinterpret_cast<const char*>(bytes.data()),bytes.size()},out,error);
}
}
#endif

// Only the explicit file is read. Dependencies are resolved from the normal
// borgs/ and stages/ roots, never relative to arbitrary paths in the preset.
inline bool load_battle_roster_preset(const std::filesystem::path& file,BattleRosterPreset* out,std::string* error=nullptr){
#if defined(__linux__)
    using namespace stage_package_scan_detail;
    const auto fail=[&](const char* why){if(error)*error=why;return false;};
    if(!out||file.empty()||file.native().size()>4096)return fail("missing roster file or path exceeds budget");
    const auto filename=file.filename().string();
    if(!borg_package_asset_path_valid(filename))return fail("invalid roster filename");
    Fd folder;
    if(open_root(file.has_parent_path()?file.parent_path():std::filesystem::path("."),folder))
        return fail("roster parent directory unavailable or linked");
    return battle_roster_load_detail::read_file(folder.value,filename,out,error);
#else
    (void)file;(void)out;
    if(error)*error="roster preset file admission is unavailable on this host";
    return false;
#endif
}

struct BattleRosterListing {
    std::string filename;
    std::optional<BattleRosterPreset> preset;
    std::string error;
};

// Directory listing verifies metadata only. Leave output untouched on
// a root scan failure or entry-budget failure rather than list a subset.
inline bool scan_battle_roster_presets(const std::filesystem::path& root,std::vector<BattleRosterListing>* out,
                                      std::string* error=nullptr){
#if defined(__linux__)
    using namespace stage_package_scan_detail;
    const auto fail=[&](const char* why){if(error)*error=why;return false;};
    if(!out||root.empty()||root.native().size()>4096)return fail("missing roster directory or path exceeds budget");
    Fd directory;
    if(open_root(root,directory))return fail("roster directory unavailable or linked");
    const int iterator=::openat(directory.value,".",dir_flags);
    if(iterator<0)return fail("could not enumerate roster directory");
    DIR* raw=::fdopendir(iterator);
    if(!raw){::close(iterator);return fail("could not enumerate roster directory");}
    std::unique_ptr<DIR,int(*)(DIR*)> stream(raw,::closedir);
    std::vector<BattleRosterListing> entries;
    unsigned scanned=0;
    for(;;){
        errno=0;
        const auto* entry=::readdir(stream.get());
        if(!entry){if(errno)return fail("could not enumerate roster directory");break;}
        const std::string_view name=entry->d_name;
        if(name=="."||name=="..")continue;
        if(++scanned>256)return fail("roster directory exceeds the 256-entry scan budget");
        if(!name.ends_with(".roster"))continue;
        BattleRosterListing listing;
        listing.filename=name;
        BattleRosterPreset preset;
        if(battle_roster_load_detail::read_file(directory.value,name,&preset,&listing.error))
            listing.preset=std::move(preset);
        entries.push_back(std::move(listing));
    }
    std::sort(entries.begin(),entries.end(),[](const auto& a,const auto& b){return a.filename<b.filename;});
    *out=std::move(entries);if(error)error->clear();return true;
#else
    (void)root;(void)out;
    if(error)*error="roster preset discovery is unavailable on this host";
    return false;
#endif
}

} // namespace gf::pc
