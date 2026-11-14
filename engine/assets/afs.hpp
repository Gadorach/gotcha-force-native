#pragma once
// AFS archive reader.
//
// The game's files live in AFS containers. The format is simple and fully
// self-describing, which makes it the first real content path the rewrite can
// support without knowing anything about the game itself:
//
//   u32 magic 'AFS\0' | u32 file_count        (all fields little-endian)
//   file_count * { u32 offset, u32 size }
//   ... file data, 2048-byte aligned ...
//
// Note the byte order: the console is big-endian but this container stores its
// header little-endian, so the reader is explicit rather than assuming either.
//
// The reader takes a byte window and never owns the archive: a caller can map or
// read the file however its platform prefers, then hand the bytes over.
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "engine/core/types.hpp"

namespace gf {

inline constexpr u32 kAfsMagic = 0x00534641u;   // 'A','F','S',0 read as LE u32
inline constexpr std::size_t kAfsHeaderBytes = 8;
inline constexpr u32 kMaxAfsEntries = 1u << 16;

struct AfsEntry {
    u32 offset = 0;
    u32 size = 0;
};

// Little-endian readers. Kept local because AFS is the only little-endian
// format in the codebase; everything else is big-endian on purpose.
inline u32 load_le32(const u8* p) {
    return u32(p[0]) | (u32(p[1]) << 8) | (u32(p[2]) << 16) | (u32(p[3]) << 24);
}

class AfsArchive {
public:
    // Validates the whole header before any entry is exposed: a truncated or
    // hand-corrupted archive must not yield plausible-looking offsets.
    bool open(const ByteView& buffer, std::string* error) {
        buffer_ = buffer;
        entries_.clear();
        if (!buffer.data || buffer.size < kAfsHeaderBytes) {
            return fail(error, "afs: buffer shorter than the header");
        }
        if (load_le32(buffer.data) != kAfsMagic) {
            return fail(error, "afs: bad magic");
        }
        const u32 count = load_le32(buffer.data + 4);
        if (count == 0) return fail(error, "afs: archive declares no files");
        if (count > kMaxAfsEntries) return fail(error, "afs: file count exceeds the cap");
        const std::size_t header = kAfsHeaderBytes + std::size_t(count) * 8;
        if (header > buffer.size) return fail(error, "afs: header runs past the buffer");

        entries_.reserve(count);
        for (u32 i = 0; i < count; ++i) {
            const std::size_t at = kAfsHeaderBytes + std::size_t(i) * 8;
            const u32 offset = load_le32(buffer.data + at);
            const u32 size = load_le32(buffer.data + at + 4);
            if (size == 0) return fail(error, "afs: zero-length entry");
            if (offset < header) return fail(error, "afs: entry overlaps the header");
            if (std::size_t(offset) + size > buffer.size) {
                return fail(error, "afs: entry runs past the buffer");
            }
            entries_.push_back({offset, size});
        }
        return true;
    }

    std::size_t size() const { return entries_.size(); }
    const AfsEntry& entry(std::size_t index) const { return entries_[index]; }

    // Zero-copy view of one entry. Returns an empty view for an unknown index.
    ByteView file(std::size_t index) const {
        if (index >= entries_.size()) return {};
        const AfsEntry& e = entries_[index];
        return ByteView{buffer_.data + e.offset, e.size};
    }

    const ByteView& buffer() const { return buffer_; }

    // ---- optional name directory -------------------------------------
    //
    // AFS stores a pointer to its name table in the word immediately before the
    // first payload, and the table's byte size in the next word. Each record is
    // 48 bytes and the name is the first 32 bytes, NUL padded. Names are
    // optional and are not needed to read payloads, so this is a separate step:
    // a bad name table must never make a valid archive unusable.
    static constexpr std::size_t kNoEntry = static_cast<std::size_t>(-1);
    static constexpr std::size_t kNameRecordBytes = 48;
    static constexpr std::size_t kNameBytes = 32;

    bool open_names(std::string* error) {
        names_.clear();
        if (!buffer_.data || entries_.empty()) {
            return name_fail(error, "afs: names require a parsed archive");
        }
        std::size_t first = entries_[0].offset;
        for (const AfsEntry& entry : entries_) {
            if (entry.offset < first) first = entry.offset;
        }
        if (first < 8) return name_fail(error, "afs: no room for a name table pointer");
        const u32 directory = load_le32(buffer_.data + first - 8);
        const u32 bytes = load_le32(buffer_.data + first - 4);
        const std::size_t header = kAfsHeaderBytes + entries_.size() * 8;
        if (directory < header) return name_fail(error, "afs: name table overlaps the header");
        if (std::size_t(directory) + bytes > buffer_.size) {
            return name_fail(error, "afs: name table runs past the buffer");
        }
        if (bytes < entries_.size() * kNameRecordBytes) {
            return name_fail(error, "afs: name table is shorter than the entry count");
        }
        names_.reserve(entries_.size());
        for (std::size_t i = 0; i < entries_.size(); ++i) {
            const u8* record =
                buffer_.data + directory + i * kNameRecordBytes;
            std::size_t length = 0;
            while (length < kNameBytes && record[length] != 0) ++length;
            for (std::size_t c = 0; c < length; ++c) {
                if (record[c] < 0x20 || record[c] > 0x7e) {
                    names_.clear();
                    return name_fail(error, "afs: name table holds a non-ASCII name");
                }
            }
            names_.emplace_back(reinterpret_cast<const char*>(record), length);
        }
        return true;
    }

    bool names_loaded() const { return !names_.empty(); }

    // Index of a member by exact name, or kNoEntry. A linear scan over a few
    // thousand records is cheaper than a map for the one lookup a caller does.
    std::size_t find(std::string_view name) const {
        for (std::size_t i = 0; i < names_.size(); ++i) {
            if (names_[i] == name) return i;
        }
        return kNoEntry;
    }

    std::string_view name_of(std::size_t index) const {
        if (index >= names_.size()) return {};
        return names_[index];
    }

private:
    bool name_fail(std::string* error, const char* message) {
        if (error) *error = message;
        return false;
    }

    // A failed open must never leave entries from a partially parsed header
    // visible, so clearing is part of failing.
    bool fail(std::string* error, const char* message) {
        entries_.clear();
        if (error) *error = message;
        return false;
    }

    ByteView buffer_{};
    std::vector<AfsEntry> entries_;
    std::vector<std::string> names_;
};

} // namespace gf
