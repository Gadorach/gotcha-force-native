#pragma once
#include <span>
#include "engine/assets/story_manifest.hpp"
#include "engine/assets/tpl.hpp"
#include "platform/pc/stage_package_scan.hpp"

namespace gf::pc {

inline constexpr std::size_t kPortableStoryScanEntries = 2048;
inline constexpr std::size_t kPortableStoryScanBytes = 8u << 20;
inline constexpr std::size_t kPortableStoryScanRoots = 32;
inline constexpr std::size_t kPortableStoryArtBytes = 8u << 20;

inline PortableStoryCatalog scan_portable_stories(std::span<const std::filesystem::path> roots) {
    using Status = PortableStoryScanStatus;
    const auto failure = [](Status status) {
        PortableStoryCatalog empty;
        empty.status = status;
        return empty;
    };
    if(roots.size() > kPortableStoryScanRoots) return failure(Status::EntryBudget);
#if defined(__linux__)
    // Share the stage loader's descriptor-relative reader. Do not probe,
    // resolve or decode optional art during discovery.
    using namespace stage_package_scan_detail;
    using Issue = PortableStoryIssueKind;
    PortableStoryCatalog result;
    result.status = Status::NoDirectory;
    std::vector<PortableStoryEntry> candidates;
    std::size_t entries = 0, bytes = 0;
    for(std::size_t root_index = 0; root_index < roots.size(); ++root_index) {
        Fd directory;
        if(const auto code = open_root(roots[root_index], directory); code) {
            if(code == ENOENT) continue;
            return failure(Status::Unavailable);
        }
        result.status = Status::Complete;
        const int iterator = ::openat(directory.value, ".", dir_flags);
        if(iterator < 0) return failure(Status::Unavailable);
        DIR* raw = ::fdopendir(iterator);
        if(!raw) {
            ::close(iterator);
            return failure(Status::Unavailable);
        }
        const std::unique_ptr<DIR, int(*)(DIR*)> stream(raw, ::closedir);
        for(;;) {
            errno = 0;
            const auto* entry = ::readdir(stream.get());
            if(!entry) {
                if(errno) return failure(Status::Unavailable);
                break;
            }
            const std::string_view name = entry->d_name;
            if(name == "." || name == "..") continue;
            if(++entries > kPortableStoryScanEntries) return failure(Status::EntryBudget);
            struct stat status{};
            if(::fstatat(directory.value, entry->d_name, &status, AT_SYMLINK_NOFOLLOW) != 0)
                return failure(Status::Unavailable);
            if(S_ISLNK(status.st_mode)) {
                result.issues.push_back({root_index, std::string(name), {}, "linked Story entry", Issue::Malformed});
                continue;
            }
            if(!S_ISDIR(status.st_mode)) continue;
            Fd folder(::openat(directory.value, entry->d_name, dir_flags));
            std::vector<u8> text;
            const auto access = folder.value < 0 ? open_error(errno) :
                local_asset(folder.value, "story.manifest", kStoryManifestBytes, &text);
            if(access != StagePackageIssue::None) {
                if(access == StagePackageIssue::Unavailable || access == StagePackageIssue::Changed)
                    return failure(Status::Unavailable);
                result.issues.push_back({root_index, std::string(name), {},
                    std::string("story.manifest: ") + issue_name(access),
                    access == StagePackageIssue::Missing ? Issue::Missing : Issue::Malformed});
                continue;
            }
            if(text.size() > kPortableStoryScanBytes - bytes) return failure(Status::EntryBudget);
            bytes += text.size();
            StoryManifest manifest;
            std::string error;
            if(!story_read_manifest({reinterpret_cast<const char*>(text.data()), text.size()}, &manifest, &error)) {
                result.issues.push_back({root_index, std::string(name), {}, std::move(error), Issue::Malformed});
                continue;
            }
            candidates.push_back({root_index, std::string(name), std::move(manifest)});
        }
    }
    std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) { return a.manifest.id < b.manifest.id; });
    for(std::size_t first = 0; first < candidates.size();) {
        auto last = first + 1;
        const auto& id = candidates[first].manifest.id;
        while(last < candidates.size() && candidates[last].manifest.id == id) ++last;
        if(last == first+1) result.discovered.push_back(std::move(candidates[first]));
        else for(auto i = first; i < last; ++i) result.issues.push_back({
            candidates[i].root_index, candidates[i].folder, id, "duplicate Story ID", Issue::DuplicateId});
        first = last;
    }
    std::sort(result.issues.begin(), result.issues.end(), [](const auto& a, const auto& b) {
        return a.root_index == b.root_index ? a.folder < b.folder : a.root_index < b.root_index;
    });
    return result;
#else
    return failure(Status::Unavailable); // No safe no-follow package reader.
#endif
}

inline PortableStoryCatalog scan_portable_stories(const std::filesystem::path& root) {
    return scan_portable_stories(std::span(&root, 1));
}

// Pin only the art for the selected Story. A changed manifest never binds art
// to an older graph, and an invalid image does not hide a valid graph.
inline bool load_portable_story_map_art(std::span<const std::filesystem::path> roots, const PortableStoryEntry& entry,
                                        TplImage* out, std::string* error) {
    const auto fail = [&](std::string why) {
        if(error) *error = std::move(why);
        return false;
    };
    if(!out || entry.root_index >= roots.size() || entry.manifest.map_art.empty() ||
       entry.folder.empty() || entry.folder == "." || entry.folder == ".." ||
       entry.folder.find_first_of(std::string_view("/\0", 2)) != std::string::npos)
        return fail("missing or invalid Story map art selection");
#if defined(__linux__)
    using namespace stage_package_scan_detail;
    Fd directory;
    if(const auto code = open_root(roots[entry.root_index], directory); code) return fail(issue_name(open_error(code)));
    Fd folder(::openat(directory.value, entry.folder.c_str(), dir_flags));
    if(folder.value < 0) return fail(issue_name(open_error(errno)));
    auto graph_matches = [&]() {
        std::vector<u8> text;
        StoryManifest manifest;
        return local_asset(folder.value, "story.manifest", kStoryManifestBytes, &text) == StagePackageIssue::None &&
            story_read_manifest({reinterpret_cast<const char*>(text.data()), text.size()}, &manifest) &&
            manifest == entry.manifest;
    };
    if(!graph_matches()) return fail("Story manifest changed; reopen to inspect art");
    std::vector<u8> bytes;
    const auto access = local_asset(folder.value, entry.manifest.map_art, kPortableStoryArtBytes, &bytes);
    if(access != StagePackageIssue::None) return fail(issue_name(access));
    if(!graph_matches()) return fail("Story manifest changed; reopen to inspect art");

    const ByteView file{bytes.data(), bytes.size()};
    u32 count = 0;
    if(!tpl_image_count(file, &count, error)) return false;
    if(count != 1) return fail("Story map art must be a single-image TPL");
    TplImage image;
    if(!tpl_decode(file, 0, &image, error)) return false;
    *out = std::move(image);
    return true;
#else
    return fail("unsupported host Story art load: no no-follow file adapter");
#endif
}

} // namespace gf::pc
