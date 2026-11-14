#pragma once
// Loads the game's AFS archive once and keeps a summary for screens to display.
//
// Failing to find or parse an archive is not fatal: menu screens still run, they
// just report that no content is available. That keeps the rewrite runnable
// without a game dump while making the real-data path the default when present.
#include <string>
#include <string_view>
#include <vector>

#include "engine/assets/afs.hpp"
#include "engine/core/backends.hpp"
#include "engine/content/content_index.hpp"

namespace gf {

class ArchiveSummary {
public:
    bool load(IFileSystem& files, const std::string& path, std::string* error) {
        std::vector<u8> bytes;
        loaded_ = false;
        if (!files.read(path, bytes)) {
            if (error) *error = "archive not readable: " + path;
            return false;
        }
        bytes_ = std::move(bytes);
        if (!archive_.open({bytes_.data(), bytes_.size()}, error)) return false;
        // Names are optional: an archive without a usable name table still
        // reads by index, so a name failure is reported, not fatal.
        std::string name_error;
        named_ = archive_.open_names(&name_error);
        if (!index_.build(archive_, error)) return false;
        loaded_ = true;
        return true;
    }

    bool loaded() const { return loaded_; }
    std::size_t files() const { return archive_.size(); }
    std::size_t count(ContentKind kind) const { return index_.count(kind); }
    const ContentIndex& index() const { return index_; }
    // Zero-copy view of one member, for callers that decode it (textures,
    // models). Empty when the archive is not loaded or the index is invalid.
    ByteView file(std::size_t index) const { return archive_.file(index); }

    // Name directory: present only when the archive carries one. Loading by name
    // keeps callers off hard-coded member indices.
    bool named() const { return named_; }
    std::size_t find(std::string_view name) const { return archive_.find(name); }
    std::string_view name_of(std::size_t index) const { return archive_.name_of(index); }

private:
    std::vector<u8> bytes_;
    AfsArchive archive_{};
    ContentIndex index_{};
    bool loaded_ = false;
    bool named_ = false;
};

} // namespace gf
