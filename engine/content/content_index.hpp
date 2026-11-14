#pragma once
// Content index: a cheap, bounded classifier over a validated AFS directory.
//
// Why this exists: the rewrite can open an AFS container long before it can
// decode anything inside it. Looking at a handful of leading bytes gives humans
// and tools a rough map of an archive without parsing or trusting any payload.
//
// What this is NOT:
//   * Not a format checker. A classification is a hint, never authority. No
//     decoder may accept, reject or route data because of it.
//   * Not exhaustive. RARC/U8/BNR magics are unambiguous, but HSDF meshes have
//     no magic at all, so "HsdfMesh" only means "the entry starts 0x00 0x00".
//   * Not a texture test. "LikelyTexture" is a weak heuristic, not evidence
//     that the entry is an image (see the rule below).
//
// Bounded by construction: at most kProbeBytes (8) leading bytes are read per
// entry, well inside the 32-byte ceiling this index is allowed to touch. The
// index owns no file handle; callers hand it an already-open AfsArchive.
//
// The brief grouped the banner kinds as "Afn/Bnr". No AFN magic exists in the
// observed data, so a single Bnr kind covers BNR1/BNR2 rather than inventing a
// branch that could never fire.
#include <array>
#include <cstddef>
#include <string>
#include <vector>

#include "engine/assets/afs.hpp"
#include "engine/core/types.hpp"

namespace gf {

// Classification is a hint for humans/tools, not a format claim.
// Precedence (documented so results are predictable):
//   Empty > LikelyTexture > Rarc/U8/Bnr > HsdfMesh > Unknown
// LikelyTexture deliberately wins over HsdfMesh: an all-zero prefix would
// otherwise be claimed by the mesh rule, even though zero padding is at least
// as common as a zero-prefixed mesh.
enum class ContentKind : u8 {
    Empty,         // no payload bytes (unreachable via a valid AFS header)
    Rarc,          // first 4 bytes are "RARC"
    U8,            // first 4 bytes are 0x55AA382D (big-endian "U8\x1D")
    Bnr,           // first 4 bytes are "BNR1" or "BNR2" (banner)
    HsdfMesh,      // first 2 bytes are 0x00 0x00 (heuristic, no magic exists)
    Unknown,       // no rule matched
    LikelyTexture, // size >= 64 and the first 8 bytes are all 0x00 or all 0xFF
};

// Countable, contiguous kinds. Keep this in sync with the enum above.
inline constexpr std::size_t kContentKindCount = 7;
static_assert(static_cast<std::size_t>(ContentKind::LikelyTexture) + 1 == kContentKindCount,
              "ContentKind must stay contiguous and match kContentKindCount");

constexpr std::size_t content_kind_index(ContentKind kind) {
    return static_cast<std::size_t>(kind);
}

// Stable lower-case spelling, useful for reports and test output.
constexpr const char* content_kind_name(ContentKind kind) {
    switch (kind) {
    case ContentKind::Empty: return "empty";
    case ContentKind::Rarc: return "rarc";
    case ContentKind::U8: return "u8";
    case ContentKind::Bnr: return "bnr";
    case ContentKind::HsdfMesh: return "hsdf_mesh";
    case ContentKind::Unknown: return "unknown";
    case ContentKind::LikelyTexture: return "likely_texture";
    }
    return "unknown";
}

struct ContentEntry {
    u32 index = 0;  // position in the AFS directory (archive order)
    u32 offset = 0;
    u32 size = 0;
    ContentKind kind = ContentKind::Unknown;

    friend constexpr bool operator==(const ContentEntry&, const ContentEntry&) = default;
};

class ContentIndex {
public:
    // Hard ceiling on bytes inspected per entry. The task allows up to 32; the
    // widest rule (the texture heuristic) needs only 8.
    static constexpr std::size_t kProbeBytes = 8;
    static constexpr std::size_t kTextureMinBytes = 64;
    static constexpr std::size_t kUniformProbeBytes = 8;

    // Classify one payload window. Empty/null windows are Empty. This is the
    // only place the heuristic lives; build() calls it exactly once per entry.
    static ContentKind classify(const ByteView& bytes) {
        if (bytes.data == nullptr || bytes.size == 0) return ContentKind::Empty;

        // Weak heuristic: a uniform leading run is what a texture's header
        // often looks like, but it is equally what cleared/padded data looks
        // like. Only ever a "worth a look" hint.
        if (bytes.size >= kTextureMinBytes &&
            (uniform_prefix(bytes, 0x00) || uniform_prefix(bytes, 0xFF))) {
            return ContentKind::LikelyTexture;
        }

        if (matches(bytes, "RARC")) return ContentKind::Rarc;
        // U8 stores its magic big-endian, unlike the AFS header.
        if (bytes.size >= 4 && bytes.data[0] == 0x55 && bytes.data[1] == 0xAA &&
            bytes.data[2] == 0x38 && bytes.data[3] == 0x2D) {
            return ContentKind::U8;
        }
        if (matches(bytes, "BNR1") || matches(bytes, "BNR2")) return ContentKind::Bnr;

        // HSDF meshes carry no magic; a zero word is merely suggestive.
        if (bytes.size >= 2 && bytes.data[0] == 0x00 && bytes.data[1] == 0x00) {
            return ContentKind::HsdfMesh;
        }
        return ContentKind::Unknown;
    }

    // Validates the directory and fills entries in archive order. Fails closed:
    // on any error the index is left empty and carries counts of zero.
    bool build(const AfsArchive& archive, std::string* error) {
        entries_.clear();
        counts_.fill(0);

        const std::size_t n = archive.size();
        if (n == 0) return fail(error, "content: archive is empty");
        if (n > std::size_t(kMaxAfsEntries)) {
            return fail(error, "content: archive exceeds the entry cap");
        }

        entries_.reserve(n);
        for (std::size_t i = 0; i < n; ++i) {
            const AfsEntry& raw = archive.entry(i);
            const ByteView view = archive.file(i);
            // The directory and the byte window must agree; a mismatch means a
            // partially-opened archive, which must not be indexed.
            if (view.data == nullptr || view.size == 0 || view.size != raw.size) {
                return fail(error, "content: entry view does not match the directory");
            }
            const ContentKind kind = classify(view);
            counts_[content_kind_index(kind)] += 1;
            entries_.push_back(ContentEntry{u32(i), raw.offset, raw.size, kind});
        }
        return true;
    }

    const std::vector<ContentEntry>& entries() const { return entries_; }
    std::size_t size() const { return entries_.size(); }
    bool empty() const { return entries_.empty(); }

    std::size_t count(ContentKind kind) const {
        const std::size_t slot = content_kind_index(kind);
        return slot < kContentKindCount ? counts_[slot] : 0;
    }

    // Archive-indexed lookup; nullptr when the index is out of range.
    const ContentEntry* at(u32 index) const {
        if (index >= entries_.size()) return nullptr;
        return &entries_[index];
    }

    // Largest entry of a kind, first in archive order on a tie. nullptr when no
    // entry of that kind exists. Deterministic for a deterministic archive.
    const ContentEntry* find_largest(ContentKind kind) const {
        const ContentEntry* best = nullptr;
        for (const ContentEntry& entry : entries_) {
            if (entry.kind != kind) continue;
            if (best == nullptr || entry.size > best->size) best = &entry;
        }
        return best;
    }

private:
    static bool matches(const ByteView& bytes, const char* magic) {
        if (bytes.size < 4) return false;
        for (std::size_t i = 0; i < 4; ++i) {
            if (bytes.data[i] != static_cast<u8>(magic[i])) return false;
        }
        return true;
    }

    static bool uniform_prefix(const ByteView& bytes, u8 fill) {
        if (bytes.size < kUniformProbeBytes) return false;
        for (std::size_t i = 0; i < kUniformProbeBytes; ++i) {
            if (bytes.data[i] != fill) return false;
        }
        return true;
    }

    bool fail(std::string* error, const char* message) {
        entries_.clear();
        counts_.fill(0);
        if (error) *error = message;
        return false;
    }

    std::vector<ContentEntry> entries_;
    std::array<std::size_t, kContentKindCount> counts_{};
};

} // namespace gf
