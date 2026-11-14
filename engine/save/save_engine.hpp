#pragma once
// Save engine: a portable, versioned save format on top of a swappable store.
//
// The on-disk/on-device representation is explicit: magic, version, payload
// length and a checksum, all big-endian, followed by a field-encoded payload.
// No struct is written directly, so padding, alignment and host endianness
// cannot leak into save files.
#include <algorithm>
#include <bit>
#include <string>
#include <vector>

#include "engine/core/backends.hpp"
#include "engine/core/types.hpp"

namespace gf {

inline constexpr u32 kSaveMagic = 0x47465356u;   // "GFSV"
inline constexpr u32 kSaveVersion = 1;
inline constexpr std::size_t kSaveHeaderBytes = 16;
inline constexpr std::size_t kMaxSavePayload = 1u << 20;

inline u64 fnv1a64(const u8* data, std::size_t size) {
    u64 hash = 1469598103934665603ull;
    for (std::size_t i = 0; i < size; ++i) {
        hash ^= data[i];
        hash *= 1099511628211ull;
    }
    return hash;
}

// Field-encoded payload writer. Every field is length-prefixed or fixed width.
class SaveWriter {
public:
    void u32_field(u32 value) {
        u8 buffer[4];
        store_be32(buffer, value);
        bytes_.insert(bytes_.end(), buffer, buffer + 4);
    }
    void u64_field(u64 value) {
        u32_field(u32(value >> 32));
        u32_field(u32(value));
    }
    void f32_field(f32 value) { u32_field(f32_to_bits(value)); }
    void f64_field(f64 value) { u64_field(std::bit_cast<u64>(value)); }
    void blob(const u8* data, std::size_t size) {
        u32_field(u32(size));
        bytes_.insert(bytes_.end(), data, data + size);
    }
    const std::vector<u8>& bytes() const { return bytes_; }
private:
    std::vector<u8> bytes_;
};

// Symmetric reader. Any short or malformed field marks the reader failed and
// leaves the remaining reads as no-ops, so callers check once at the end.
class SaveReader {
public:
    explicit SaveReader(const std::vector<u8>& bytes) : bytes_(&bytes) {}
    u32 u32_field() {
        if (!ok_ || position_ + 4 > bytes_->size()) { ok_ = false; return 0; }
        const u32 value = load_be32(bytes_->data() + position_);
        position_ += 4;
        return value;
    }
    u64 u64_field() {
        const u64 high = u32_field();
        const u64 low = u32_field();
        return (high << 32) | low;
    }
    f32 f32_field() { return bits_to_f32(u32_field()); }
    f64 f64_field() { return std::bit_cast<f64>(u64_field()); }
    std::vector<u8> blob() {
        const u32 size = u32_field();
        if (!ok_ || size > kMaxSavePayload || position_ + size > bytes_->size()) {
            ok_ = false;
            return {};
        }
        std::vector<u8> out(bytes_->begin() + position_, bytes_->begin() + position_ + size);
        position_ += size;
        return out;
    }
    bool ok() const { return ok_; }
    bool consumed() const { return ok_ && position_ == bytes_->size(); }
private:
    const std::vector<u8>* bytes_;
    std::size_t position_ = 0;
    bool ok_ = true;
};

class SaveEngine {
public:
    explicit SaveEngine(ISaveBackend& backend) : backend_(backend) {}

    bool write(u32 slot, const SaveWriter& writer) {
        const std::vector<u8>& payload = writer.bytes();
        if (payload.size() > kMaxSavePayload) return false;
        std::vector<u8> record(kSaveHeaderBytes + payload.size());
        store_be32(record.data() + 0, kSaveMagic);
        store_be32(record.data() + 4, kSaveVersion);
        store_be32(record.data() + 8, u32(payload.size()));
        store_be32(record.data() + 12, u32(fnv1a64(payload.data(), payload.size())));
        std::copy(payload.begin(), payload.end(), record.begin() + kSaveHeaderBytes);
        return backend_.store(slot, record.data(), record.size());
    }

    // Unknown versions, bad magic, truncation and checksum mismatch all fail
    // closed: the caller sees no partially decoded state.
    bool read(u32 slot, SaveReader& reader) {
        std::vector<u8> record;
        if (!backend_.load(slot, record)) return false;
        return read_record(record,reader);
    }
    // Shared framing validation for callers that already performed a
    // status-bearing backend read. No second I/O and no duplicated format.
    bool read_record(const std::vector<u8>& record, SaveReader& reader) {
        if (record.size() < kSaveHeaderBytes) return false;
        if (load_be32(record.data()) != kSaveMagic) return false;
        if (load_be32(record.data() + 4) != kSaveVersion) return false;
        const u32 length = load_be32(record.data() + 8);
        if (length > kMaxSavePayload || record.size() != kSaveHeaderBytes + length) return false;
        const u32 expected = load_be32(record.data() + 12);
        const u8* payload = record.data() + kSaveHeaderBytes;
        if (u32(fnv1a64(payload, length)) != expected) return false;
        payload_.assign(payload, payload + length);
        reader = SaveReader(payload_);
        return true;
    }

    bool erase(u32 slot) { return backend_.erase(slot); }

private:
    ISaveBackend& backend_;
    std::vector<u8> payload_;
};

} // namespace gf
