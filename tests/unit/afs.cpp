// AFS reader tests: synthetic fixtures for every validation rule, plus a
// read-only check against the real archive when one is present locally.
#include "engine/assets/afs.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

namespace {

void check(bool ok, const char* what) {
    if (!ok) {
        std::fprintf(stderr, "afs failure: %s\n", what);
        std::abort();
    }
}

void put_le32(std::vector<gf::u8>& out, std::size_t at, gf::u32 value) {
    // Explicit bounds check: fixture corruption should abort, not overrun.
    if (at + 4 > out.size()) std::abort();
    out[at + 0] = gf::u8(value);
    out[at + 1] = gf::u8(value >> 8);
    out[at + 2] = gf::u8(value >> 16);
    out[at + 3] = gf::u8(value >> 24);
}

// Builds an archive with the given payload sizes, 2048-byte aligned like the real
// format.
std::vector<gf::u8> make_archive(const std::vector<gf::u32>& sizes) {
    const std::size_t header = 8 + sizes.size() * 8;
    std::size_t offset = (header + 2047) / 2048 * 2048;
    std::vector<gf::u8> out(offset);
    put_le32(out, 0, gf::kAfsMagic);
    put_le32(out, 4, gf::u32(sizes.size()));
    for (std::size_t i = 0; i < sizes.size(); ++i) {
        out.resize(offset + sizes[i], 0);
        for (gf::u32 b = 0; b < sizes[i]; ++b) out[offset + b] = gf::u8(i + 1);
        put_le32(out, 8 + i * 8, gf::u32(offset));
        put_le32(out, 8 + i * 8 + 4, sizes[i]);
        offset = (offset + sizes[i] + 2047) / 2048 * 2048;
        out.resize(offset, 0);
    }
    return out;
}

// Builds an archive with a name table: the pointer word sits immediately before
// the first payload and points at `sizes.size()` records of 48 bytes.
std::vector<gf::u8> make_named_archive(const std::vector<std::string>& names,
                                       const std::vector<gf::u32>& sizes) {
    std::vector<gf::u8> out = make_archive(sizes);
    const std::size_t first = gf::load_le32(out.data() + 8);
    const std::size_t table = out.size();
    const std::size_t table_bytes = names.size() * 48;
    out.resize(table + table_bytes, 0);
    for (std::size_t i = 0; i < names.size(); ++i) {
        if (names[i].size() >= 32) std::abort();
        for (std::size_t c = 0; c < names[i].size(); ++c) {
            out[table + i * 48 + c] = gf::u8(names[i][c]);
        }
    }
    put_le32(out, first - 8, gf::u32(table));
    put_le32(out, first - 4, gf::u32(table_bytes));
    return out;
}

int name_cases() {
    int cases = 0;
    const std::vector<gf::u8> archive =
        make_named_archive({"alpha.bin", "two.bin", "three.tpl"}, {16, 32, 64});
    gf::AfsArchive afs;
    std::string error;
    check(afs.open({archive.data(), archive.size()}, &error), "named archive opens");
    check(!afs.names_loaded(), "names are not loaded until asked for");
    check(afs.open_names(&error), "name table parses");
    check(afs.names_loaded(), "names are loaded");
    check(afs.name_of(0) == "alpha.bin", "first name");
    check(afs.name_of(1) == "two.bin", "second name");
    check(afs.name_of(2) == "three.tpl", "third name");
    check(afs.name_of(3).empty(), "name past the end is empty");
    check(afs.find("two.bin") == 1, "find by name");
    check(afs.find("three.tpl") == 2, "find the last name");
    check(afs.find("missing.bin") == gf::AfsArchive::kNoEntry, "missing name yields kNoEntry");
    check(afs.find("") == gf::AfsArchive::kNoEntry, "empty name yields kNoEntry");
    cases += 8;

    // Payload access is unaffected by loading names.
    check(afs.file(1).size == 32 && afs.file(1).data[0] == 2, "payloads still readable");
    ++cases;

    // Names must not make a valid archive unusable: every rejection leaves the
    // payload entries intact.
    auto rejects_names = [&](const std::vector<gf::u8>& data, const char* what) {
        gf::AfsArchive bad;
        std::string message;
        check(bad.open({data.data(), data.size()}, &message), "archive itself remains valid");
        check(!bad.open_names(&message), what);
        check(!message.empty(), "name rejection carries a message");
        check(!bad.names_loaded(), "failed name parse exposes no names");
        check(bad.size() == 3 && bad.file(0).size == 16, "payloads survive a bad name table");
        ++cases;
    };

    {   // directory pointer past the end of the buffer
        std::vector<gf::u8> data = archive;
        put_le32(data, gf::load_le32(data.data() + 8) - 8, gf::u32(data.size() - 4));
        rejects_names(data, "name table pointer past the buffer rejected");
    }
    {   // table claims fewer bytes than the entry count needs
        std::vector<gf::u8> data = archive;
        put_le32(data, gf::load_le32(data.data() + 8) - 4, gf::u32(48));
        rejects_names(data, "short name table rejected");
    }
    {   // name with a non-printable byte
        std::vector<gf::u8> data = archive;
        const std::size_t table = gf::load_le32(data.data() + gf::load_le32(data.data() + 8) - 8);
        data[table + 1] = 0x01;
        rejects_names(data, "non-ASCII name rejected");
    }

    // open_names before open() is a refusal, not a crash.
    gf::AfsArchive empty;
    check(!empty.open_names(&error) && !error.empty(), "names need a parsed archive");
    ++cases;
    return cases;
}

int synthetic_cases() {
    int cases = 0;
    const std::vector<gf::u8> archive = make_archive({16, 32, 64});
    gf::AfsArchive afs;
    std::string error;
    check(afs.open({archive.data(), archive.size()}, &error), "valid archive opens");
    check(afs.size() == 3, "entry count");
    for (std::size_t i = 0; i < 3; ++i) {
        const gf::ByteView view = afs.file(i);
        check(view.size == (16u << i), "entry size matches");
        check(view.data[0] == gf::u8(i + 1), "entry payload is the expected file");
        ++cases;
    }
    check(afs.file(9).data == nullptr, "out-of-range index yields an empty view");
    ++cases;

    // Every rejection path must fail closed with a message.
    auto reject = [&](std::vector<gf::u8> data, const char* what) {
        gf::AfsArchive bad;
        std::string message;
        check(!bad.open({data.data(), data.size()}, &message), what);
        check(!message.empty(), "rejection carries a message");
        check(bad.size() == 0, "failed open exposes no entries");
        ++cases;
    };
    reject({}, "empty buffer rejected");
    reject(std::vector<gf::u8>(4, 0), "short buffer rejected");

    auto bad_magic = archive; bad_magic[0] = 'X';
    reject(bad_magic, "bad magic rejected");

    auto no_files = archive; put_le32(no_files, 4, 0);
    reject(no_files, "zero file count rejected");

    auto huge = archive; put_le32(huge, 4, 0x7FFFFFFF);
    reject(huge, "absurd file count rejected");

    auto overlap = archive; put_le32(overlap, 8, 4);
    reject(overlap, "entry overlapping the header rejected");

    auto zero = archive; put_le32(zero, 8 + 4, 0);
    reject(zero, "zero-length entry rejected");

    auto past_end = archive; put_le32(past_end, 8 + 4, 0xFFFFFF00);
    reject(past_end, "entry past the buffer rejected");

    // Cut into the last file's payload, not the alignment padding after it.
    auto truncated = archive;
    const std::size_t last_at = 8 + 2 * 8;
    const std::size_t last_offset = gf::load_le32(archive.data() + last_at);
    truncated.resize(last_offset + 32);   // last entry declares 64 bytes
    reject(truncated, "truncated payload rejected");
    return cases;
}

// Real archive: read-only, skipped when the local file is absent so the test is
// portable. Reports into the JSON line.
struct RealResult { bool present = false; bool ok = false; unsigned files = 0; unsigned long long bytes = 0;
                    bool names = false; unsigned named = 0; bool menu_member = false; };

RealResult real_archive() {
    RealResult result;
    std::ifstream file("original/extracted/files/afs_data.afs", std::ios::binary);
    if (!file) return result;
    result.present = true;
    const std::vector<gf::u8> data((std::istreambuf_iterator<char>(file)), {});
    gf::AfsArchive afs;
    std::string error;
    if (!afs.open({data.data(), data.size()}, &error)) return result;
    result.files = static_cast<unsigned>(afs.size());
    for (std::size_t i = 0; i < afs.size(); ++i) result.bytes += afs.file(i).size;
    result.ok = result.files > 0;
    if (!afs.open_names(&error)) return result;
    result.names = true;
    for (std::size_t i = 0; i < afs.size(); ++i) {
        if (!afs.name_of(i).empty()) ++result.named;
    }
    // The menu room archive, found by name. An HSD archive starts with its own
    // declared file size (big-endian), not with the 0x0020AF30 magic: that magic
    // belongs to TPL files and to data inside these archives.
    const std::size_t menu = afs.find("tl00_mdl.arc");
    if (menu != gf::AfsArchive::kNoEntry) {
        const gf::ByteView view = afs.file(menu);
        const gf::u32 declared = view.size >= 0x20
                                     ? (gf::u32(view.data[0]) << 24) | (gf::u32(view.data[1]) << 16) |
                                           (gf::u32(view.data[2]) << 8) | gf::u32(view.data[3])
                                     : 0;
        result.menu_member = view.size >= 0x20 && declared >= 0x20 && declared <= view.size;
    }
    return result;
}

} // namespace

int main() {
    const int cases = synthetic_cases() + name_cases();
    const RealResult real = real_archive();
    if (real.present) {
        check(real.ok, "the local archive parses");
        check(real.names, "the local archive has a name table");
        check(real.named == real.files, "every member has a name");
        check(real.menu_member, "tl00_mdl.arc is found by name and is an HSD archive");
    }

    std::printf("{\"afs_synthetic_cases\":%d,\"real_archive_present\":%s,"
                "\"real_archive_files\":%u,\"real_archive_bytes\":%llu,"
                "\"real_names\":%s,\"real_named\":%u,\"all_passed\":true}\n",
                cases, real.present ? "true" : "false", real.files, real.bytes,
                real.names ? "true" : "false", real.named);
    return 0;
}
