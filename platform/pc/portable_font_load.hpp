#pragma once
#include "engine/graphics/portable_text.hpp"
#include "platform/pc/borg_package_scan.hpp"

namespace gf::pc {
// Only the explicitly selected directory is searched. No startup discovery,
// save relationship, implicit second atlas or platform font lookup.
inline bool load_local_portable_font(std::filesystem::path root,RenderEngine& render,u32 texture,PortableFont* out,
                                     std::string* error=nullptr){
    auto fail=[&](const char* why){if(error)*error=why;return false;};
    if(!out||root.empty())return fail("no font directory selected");
    if(root.native().size()>4096)return fail("font directory path exceeds budget");
    while(root!=root.root_path()&&!root.has_filename())root=root.parent_path();
    std::error_code ec;
    // Validate CLI-selected root components too; e.g. `linked/sub` must
    // not evade the link rule merely because `sub` is a real directory.
    std::filesystem::path current=root.root_path();
    unsigned components=0;
    for(const auto& part:root.relative_path()){
        if(++components>64)return fail("font directory path exceeds component budget");
        if(part=="..")return fail("font directory must not contain parent traversal");
        current/=part;
        if(!std::filesystem::is_directory(std::filesystem::symlink_status(current,ec))||ec)
            return fail("font directory unavailable or linked");
    }
    using borg_package_scan_detail::read_local;
    std::string manifest,atlas;
    if(!read_local(root,"font.manifest",kPortableFontManifestBytes,&manifest))return fail("font manifest unavailable, linked or oversized");
    PortableFontMetrics metrics;
    if(!PortableFontMetrics::parse(manifest,&metrics,error))return false;
    if(!read_local(root,metrics.atlas_path(),metrics.atlas_bytes(),&atlas)||atlas.size()!=metrics.atlas_bytes())
        return fail("font atlas unavailable, linked or the wrong length");
    return out->bind(render,texture,metrics,{reinterpret_cast<const u8*>(atlas.data()),atlas.size()},error);
}
}
