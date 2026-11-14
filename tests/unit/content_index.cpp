// Content-index tests: every classifier branch gets a synthetic payload, the
// index must preserve archive order, and the real local archive is indexed
// read-only when present.
//
// The classifier is a hint, so these tests pin the documented rules rather than
// claiming any payload really is a texture or a mesh. No GPU, OS or emulator
// headers involved; fixtures are built in memory.
#include "engine/content/content_index.hpp"

#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {

void check(bool ok, const char* what) {
    if (!ok) {
        std::fprintf(stderr, "content index failure: %s\n", what);
        std::abort();
    }
}

void put_le32(std::vector<gf::u8>& out, std::size_t at, gf::u32 value) {
    // Fixture corruption should abort, not overrun.
    if (at + 4 > out.size()) std::abort();
    out[at + 0] = gf::u8(value);
    out[at + 1] = gf::u8(value >> 8);
    out[at + 2] = gf::u8(value >> 16);
    out[at + 3] = gf::u8(value >> 24);
}

// Same layout the AFS reader documents: little-endian header, then payloads at
// 2048-byte alignment. Payload bytes are supplied verbatim so the classifier is
// what gets exercised, not a filler pattern.
std::vector<gf::u8> make_archive(const std::vector<std::vector<gf::u8>>& payloads) {
    const std::size_t header = 8 + payloads.size() * 8;
    std::size_t offset = (header + 2047) / 2048 * 2048;
    std::vector<gf::u8> out(offset, 0);
    put_le32(out, 0, gf::kAfsMagic);
    put_le32(out, 4, gf::u32(payloads.size()));
    for (std::size_t i = 0; i < payloads.size(); ++i) {
        const std::vector<gf::u8>& payload = payloads[i];
        out.resize(offset + payload.size(), 0);
        for (std::size_t b = 0; b < payload.size(); ++b) out[offset + b] = payload[b];
        put_le32(out, 8 + i * 8, gf::u32(offset));
        put_le32(out, 8 + i * 8 + 4, gf::u32(payload.size()));
        offset = (offset + payload.size() + 2047) / 2048 * 2048;
        out.resize(offset, 0);
    }
    return out;
}

std::vector<gf::u8> magic_payload(const char* magic, std::size_t size) {
    std::vector<gf::u8> bytes(size, 0x5A);  // non-uniform tail on purpose
    for (std::size_t i = 0; i < 4; ++i) bytes[i] = static_cast<gf::u8>(magic[i]);
    return bytes;
}

std::vector<gf::u8> filled(std::size_t size, gf::u8 byte) {
    return std::vector<gf::u8>(size, byte);
}

using Counts = std::array<std::size_t, gf::kContentKindCount>;

Counts counts_of(const gf::ContentIndex& index) {
    Counts counts{};
    for (const gf::ContentEntry& entry : index.entries()) {
        counts[gf::content_kind_index(entry.kind)] += 1;
    }
    return counts;
}

void print_kinds(const Counts& counts) {
    std::printf("{\"empty\":%zu,\"rarc\":%zu,\"u8\":%zu,\"bnr\":%zu,"
                "\"hsdf_mesh\":%zu,\"unknown\":%zu,\"likely_texture\":%zu}",
                counts[gf::content_kind_index(gf::ContentKind::Empty)],
                counts[gf::content_kind_index(gf::ContentKind::Rarc)],
                counts[gf::content_kind_index(gf::ContentKind::U8)],
                counts[gf::content_kind_index(gf::ContentKind::Bnr)],
                counts[gf::content_kind_index(gf::ContentKind::HsdfMesh)],
                counts[gf::content_kind_index(gf::ContentKind::Unknown)],
                counts[gf::content_kind_index(gf::ContentKind::LikelyTexture)]);
}

struct SyntheticResult {
    int kind_cases = 0;
    int cases = 0;
    std::size_t entries = 0;
    Counts counts{};
};

// Payload order is fixed; the expected kinds below must line up with it.
std::vector<std::vector<gf::u8>> synthetic_payloads() {
    std::vector<std::vector<gf::u8>> payloads;
    payloads.push_back(magic_payload("RARC", 64));              // 0
    payloads.push_back({0x55, 0xAA, 0x38, 0x2D, 0x01, 0x02});   // 1 U8
    payloads.push_back(magic_payload("BNR1", 64));              // 2
    payloads.push_back(magic_payload("BNR2", 128));             // 3
    payloads.push_back(filled(16, 0x00));                       // 4 zero, < 64
    payloads.push_back(filled(64, 0x00));                       // 5 texture hint
    payloads.push_back(filled(64, 0xFF));                       // 6 texture hint
    payloads.push_back({0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03, 0x04}); // 7
    payloads.push_back(filled(8, 0x00));                        // 8 zero, < 64
    payloads.push_back({0x00, 0x00, 0x7F});                     // 9 short zero
    payloads.push_back({0x00});                                 // 10 too short
    return payloads;
}

SyntheticResult synthetic_cases() {
    SyntheticResult result;
    int& cases = result.cases;
    auto expect = [&](bool ok, const char* what) {
        check(ok, what);
        ++cases;
    };

    const std::vector<std::vector<gf::u8>> payloads = synthetic_payloads();
    const std::vector<gf::u8> bytes = make_archive(payloads);

    gf::AfsArchive archive;
    std::string error = "sentinel";
    expect(archive.open({bytes.data(), bytes.size()}, &error), "synthetic archive opens");
    expect(archive.size() == payloads.size(), "synthetic archive exposes every entry");

    gf::ContentIndex index;
    expect(index.build(archive, &error), "content index builds");
    expect(error == "sentinel", "a successful build leaves the caller's error untouched");
    expect(index.size() == payloads.size(), "one content entry per AFS entry");
    expect(index.entries().size() == payloads.size(), "entries() mirrors size()");
    result.entries = index.size();

    const gf::ContentKind expected_kinds[] = {
        gf::ContentKind::Rarc,
        gf::ContentKind::U8,
        gf::ContentKind::Bnr,
        gf::ContentKind::Bnr,
        gf::ContentKind::HsdfMesh,
        gf::ContentKind::LikelyTexture,
        gf::ContentKind::LikelyTexture,
        gf::ContentKind::Unknown,
        gf::ContentKind::HsdfMesh,
        gf::ContentKind::HsdfMesh,
        gf::ContentKind::Unknown,
    };
    for (std::size_t i = 0; i < payloads.size(); ++i) {
        expect(index.entries()[i].kind == expected_kinds[i], "entry kind matches the rule");
        ++result.kind_cases;
    }

    // Archive order, directory mirroring and index numbering.
    for (std::size_t i = 0; i < payloads.size(); ++i) {
        const gf::ContentEntry& entry = index.entries()[i];
        const gf::AfsEntry& raw = archive.entry(i);
        expect(entry.index == gf::u32(i), "index numbering follows archive order");
        expect(entry.offset == raw.offset, "offset mirrors the directory");
        expect(entry.size == raw.size, "size mirrors the directory");
    }
    expect(index.entries().front().kind == gf::ContentKind::Rarc, "first entry is first in order");
    expect(index.entries().back().kind == gf::ContentKind::Unknown, "last entry is last in order");

    // Counts, both the aggregate snapshot and the count() accessor.
    Counts expected{};
    expected[gf::content_kind_index(gf::ContentKind::Rarc)] = 1;
    expected[gf::content_kind_index(gf::ContentKind::U8)] = 1;
    expected[gf::content_kind_index(gf::ContentKind::Bnr)] = 2;
    expected[gf::content_kind_index(gf::ContentKind::HsdfMesh)] = 3;
    expected[gf::content_kind_index(gf::ContentKind::LikelyTexture)] = 2;
    expected[gf::content_kind_index(gf::ContentKind::Unknown)] = 2;
    result.counts = counts_of(index);
    expect(result.counts == expected, "counts by kind");
    for (std::size_t k = 0; k < gf::kContentKindCount; ++k) {
        expect(index.count(static_cast<gf::ContentKind>(k)) == expected[k],
               "count(ContentKind) matches the aggregate");
    }
    expect(index.count(gf::ContentKind::Empty) == 0, "a valid AFS header cannot yield Empty");

    // find_largest: biggest of each kind, first on a tie, nullptr when absent.
    const gf::ContentEntry* largest = index.find_largest(gf::ContentKind::Rarc);
    expect(largest != nullptr && largest->index == 0, "find_largest(Rarc)");
    largest = index.find_largest(gf::ContentKind::U8);
    expect(largest != nullptr && largest->index == 1, "find_largest(U8)");
    largest = index.find_largest(gf::ContentKind::Bnr);
    expect(largest != nullptr && largest->index == 3 && largest->size == 128, "find_largest(Bnr)");
    largest = index.find_largest(gf::ContentKind::HsdfMesh);
    expect(largest != nullptr && largest->index == 4 && largest->size == 16, "find_largest(HsdfMesh)");
    largest = index.find_largest(gf::ContentKind::Unknown);
    expect(largest != nullptr && largest->index == 7, "find_largest(Unknown)");
    largest = index.find_largest(gf::ContentKind::LikelyTexture);
    expect(largest != nullptr && largest->index == 5 && largest->size == 64,
           "find_largest(LikelyTexture) breaks ties by archive order");
    expect(index.find_largest(gf::ContentKind::Empty) == nullptr, "find_largest on an absent kind is null");

    // at(): bounds checked on both ends.
    const gf::ContentEntry* first = index.at(0);
    expect(first != nullptr && first->kind == gf::ContentKind::Rarc, "at(0)");
    const gf::ContentEntry* last = index.at(gf::u32(payloads.size() - 1));
    expect(last != nullptr && last->index == gf::u32(payloads.size() - 1), "at(last)");
    expect(index.at(gf::u32(payloads.size())) == nullptr, "at(size) is out of range");
    expect(index.at(0xFFFFFFFFu) == nullptr, "at(huge) is out of range");

    // The classifier itself, including the Empty branch a validated AFS cannot
    // reach and the size floor on the texture heuristic.
    expect(gf::ContentIndex::classify(gf::ByteView{}) == gf::ContentKind::Empty,
           "classify(null window) is Empty");
    const gf::u8 zero = 0x00;
    expect(gf::ContentIndex::classify(gf::ByteView{&zero, 0}) == gf::ContentKind::Empty,
           "classify(zero-size window) is Empty");
    expect(gf::ContentIndex::classify(gf::ByteView{&zero, 1}) == gf::ContentKind::Unknown,
           "a single zero byte is not a mesh");
    const std::vector<gf::u8> short_ff = filled(8, 0xFF);
    expect(gf::ContentIndex::classify({short_ff.data(), short_ff.size()}) == gf::ContentKind::Unknown,
           "the texture heuristic requires at least 64 bytes");

    // Determinism / idempotency: rebuilding must neither duplicate nor reorder.
    const std::vector<gf::ContentEntry> snapshot = index.entries();
    const Counts snapshot_counts = counts_of(index);
    expect(index.build(archive, &error), "second build succeeds");
    expect(index.entries() == snapshot, "second build is idempotent");
    expect(counts_of(index) == snapshot_counts, "second build keeps counts");
    expect(index.size() == payloads.size(), "second build does not duplicate entries");

    // Negative: an unopened archive and failed opens must be rejected closed.
    gf::ContentIndex fresh;
    gf::AfsArchive missing;
    error = "sentinel";
    expect(!fresh.build(missing, &error), "build on an unopened archive fails");
    expect(!error.empty() && error != "sentinel", "failed build reports an error");
    expect(fresh.empty() && fresh.size() == 0, "failed build leaves no entries");
    expect(fresh.count(gf::ContentKind::Unknown) == 0, "failed build leaves zero counts");
    expect(fresh.at(0) == nullptr, "an empty index has no entries to look up");

    // A well-formed header that declares zero files is still an empty archive.
    const std::vector<std::vector<gf::u8>> none;
    const std::vector<gf::u8> none_bytes = make_archive(none);
    gf::AfsArchive zero_files;
    expect(!zero_files.open({none_bytes.data(), none_bytes.size()}, &error),
           "an archive declaring zero files fails to open");
    expect(!fresh.build(zero_files, &error), "a zero-file archive cannot be indexed");
    expect(fresh.empty(), "the zero-file rejection leaves no entries");

    std::vector<gf::u8> corrupt = bytes;
    corrupt[0] = 'X';
    gf::AfsArchive broken;
    expect(!broken.open({corrupt.data(), corrupt.size()}, &error), "corrupt archive fails to open");
    error = "sentinel";
    expect(!fresh.build(broken, &error), "build on a failed archive is rejected");
    expect(!error.empty() && error != "sentinel", "failed archive build reports an error");
    expect(fresh.empty(), "failed archive build exposes no entries");

    // A previously good index must be cleared by a later failed build.
    gf::ContentIndex reused;
    expect(reused.build(archive, &error), "index builds before the failure check");
    expect(reused.size() == payloads.size(), "reused index starts populated");
    expect(!reused.build(missing, &error), "rebuild on an unopened archive fails");
    expect(reused.empty() && reused.count(gf::ContentKind::Rarc) == 0,
           "a failed rebuild clears prior entries and counts");

    // Zero-length entries are rejected before they can ever become Empty.
    const std::vector<gf::u8> with_empty = make_archive({{0x01, 0x02}, {}});
    gf::AfsArchive empty_entry;
    expect(!empty_entry.open({with_empty.data(), with_empty.size()}, &error),
           "the AFS reader rejects a zero-length entry");
    expect(!fresh.build(empty_entry, &error), "a zero-length entry cannot be indexed");

    return result;
}

// Real archive: read-only and skipped when the local file is absent, so the
// test stays portable. No counts are asserted: the file may vary.
struct RealResult {
    bool present = false;
    bool ok = false;
    std::size_t entries = 0;
    Counts counts{};
};

RealResult real_archive() {
    RealResult result;
    std::ifstream file("original/extracted/files/afs_data.afs", std::ios::binary);
    if (!file) return result;
    result.present = true;
    const std::vector<gf::u8> data((std::istreambuf_iterator<char>(file)), {});
    gf::AfsArchive archive;
    std::string error;
    if (!archive.open({data.data(), data.size()}, &error)) return result;
    gf::ContentIndex index;
    if (!index.build(archive, &error)) return result;
    result.entries = index.size();
    result.counts = counts_of(index);
    result.ok = result.entries == archive.size();
    return result;
}

} // namespace

int main() {
    const SyntheticResult synthetic = synthetic_cases();
    const RealResult real = real_archive();
    if (real.present) {
        check(real.ok, "the local archive indexes fully");
        check(real.entries > 0, "the local archive contributes entries");
    }

    std::printf("{\"kind_cases\":%d,\"synthetic_cases\":%d,\"synthetic_entries\":%zu,\"kinds\":",
                synthetic.kind_cases, synthetic.cases, synthetic.entries);
    print_kinds(synthetic.counts);
    std::printf(",\"real_present\":%s", real.present ? "true" : "false");
    if (real.present) {
        std::printf(",\"real_entries\":%zu,\"real_kinds\":", real.entries);
        print_kinds(real.counts);
    }
    std::printf(",\"all_passed\":true}\n");
    return 0;
}
