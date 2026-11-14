#pragma once
// Networking: portable message framing over a swappable transport.
//
// The engine owns framing, bounds and validation. A backend owns sockets,
// loopback, a null transport, or whatever a given platform provides. Game code
// only ever sees whole `NetPacket` values, so transport changes cannot alter
// game logic.
#include <cstddef>
#include <string>
#include <vector>

#include "engine/core/backends.hpp"
#include "engine/core/types.hpp"

namespace gf {

inline constexpr std::size_t kNetHeaderBytes = 8;   // type + payload length
inline constexpr u32 kMaxNetPayload = 1400;         // fits a single datagram

class NetEngine {
public:
    explicit NetEngine(INetworkBackend& transport, u32 max_payload = kMaxNetPayload)
        : transport_(transport), max_payload_(max_payload ? max_payload : kMaxNetPayload) {}

    bool open(const std::string& endpoint) { return transport_.open(endpoint); }
    void close() { transport_.close(); }
    bool connected() const { return transport_.connected(); }

    // Oversized messages are rejected rather than fragmented: a dropped packet
    // must never look like a truncated one.
    bool send(u32 type, const u8* payload, std::size_t size) {
        if (!transport_.connected()) return false;
        if (size > max_payload_) { ++rejected_; return false; }
        std::vector<u8> frame(kNetHeaderBytes + size);
        store_be32(frame.data(), type);
        store_be32(frame.data() + 4, u32(size));
        if (size) std::copy(payload, payload + size, frame.begin() + kNetHeaderBytes);
        if (!transport_.send(frame.data(), frame.size())) { ++send_failures_; return false; }
        ++sent_;
        return true;
    }

    // Returns false when no message is available. Malformed frames are counted
    // and skipped, never surfaced as valid packets.
    bool poll(NetPacket& out) {
        std::vector<u8> frame;
        if (!transport_.receive(frame)) return false;
        if (frame.size() < kNetHeaderBytes) { ++malformed_; return false; }
        const u32 declared = load_be32(frame.data() + 4);
        if (declared > max_payload_ || frame.size() != kNetHeaderBytes + declared) {
            ++malformed_;
            return false;
        }
        out.type = load_be32(frame.data());
        out.payload.assign(frame.begin() + kNetHeaderBytes, frame.end());
        ++received_;
        return true;
    }

    u64 sent() const { return sent_; }
    u64 received() const { return received_; }
    u64 rejected() const { return rejected_; }
    u64 malformed() const { return malformed_; }
    u64 send_failures() const { return send_failures_; }

private:
    INetworkBackend& transport_;
    u32 max_payload_;
    u64 sent_ = 0;
    u64 received_ = 0;
    u64 rejected_ = 0;
    u64 malformed_ = 0;
    u64 send_failures_ = 0;
};

} // namespace gf
