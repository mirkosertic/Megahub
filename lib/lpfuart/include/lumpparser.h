#ifndef LUMPPARSER_H
#define LUMPPARSER_H

#include <cstdint>
#include <cstring>

// Forward declaration
class LegoDevice;

// ---------------------------------------------------------------------------
// Observability counters — all uint32_t, never reset autonomously
// ---------------------------------------------------------------------------
struct LumpParserStats {
    uint32_t framesOk;          // successfully dispatched frames
    uint32_t checksumErrors;    // frames rejected by checksum
    uint32_t bytesDiscarded;    // bytes consumed during re-sync
    uint32_t syncRecoveries;    // transitions: bad streak -> first good frame
    uint32_t bufferOverflows;   // bytes dropped because ring buffer was full
    uint32_t unknownSysBytes;   // unrecognised system bytes (type==0, not SYNC/ACK/NACK)
    uint32_t invalidSizeBytes;  // header bytes with reserved size encoding
};

// ---------------------------------------------------------------------------
// LumpParser — sliding-window streaming parser for the LEGO UART Message
// Protocol (LUMP). Replaces the polymorphic state-machine approach.
//
// Design goals:
//   - Zero heap allocation in the hot path
//   - Self-synchronising: slides by 1 byte on checksum mismatch
//   - No virtual calls in processBuffer() / dispatchFrame()
// ---------------------------------------------------------------------------
class LumpParser {
public:
    static constexpr int      ringBufSize            = 128;
    // Consecutive checksum errors before triggering device reset
    static constexpr uint32_t syncLossResetThreshold = 300;

    explicit LumpParser(LegoDevice *device);

    // Feed one byte from the UART into the parser.
    // May dispatch zero or more frames as a side effect.
    void feedByte(uint8_t byte);

    // Feed a buffer of bytes (convenience wrapper).
    void feedBytes(const uint8_t *data, int len);

    // Retrieve current statistics snapshot.
    auto stats() const -> const LumpParserStats &;

    // Reset statistics counters (e.g. after logging them).
    void resetStats();

    // Reset parser state (ring buffer, sync-loss flags, extModeOffset).
    // Called from LegoDevice::reset().
    void reset();

    // Flush ring buffer without clearing statistics.
    // Call after a baud-rate switch to discard stale bytes from the previous speed.
    void clearBuffer();

private:
    // Decode the payload size from a header byte.
    // Returns payload byte count (1/2/4/8/16/32), or -1 for reserved encodings.
    static auto decodePayloadSize(uint8_t header) -> int;

    // Inner loop: consume as many frames as possible from the head of the buffer.
    void processBuffer();

    // Dispatch a fully validated frame (header + payload already verified by checksum).
    void dispatchFrame(uint8_t header, const uint8_t *payload, int payloadSize);

    // Dispatch a single-byte system message.
    void dispatchSystemByte(uint8_t sysByte);

    // Ring buffer storage
    uint8_t  buf_[ringBufSize];
    uint16_t head_;     // read index (mod ringBufSize)
    uint16_t count_;    // number of bytes currently in the buffer

    // Extended-mode offset: set by CMD_EXT_MODE, consumed by next DATA frame
    uint8_t  extModeOffset_;

    // Sync-loss tracking
    bool     inSyncLoss_;
    uint32_t consecutiveErrors_;

    // Sync-loss per-episode tracking
    uint32_t syncLossDiscardStart_;          // bytesDiscarded value at sync-loss entry
    static constexpr uint8_t discardCapSize = 64;
    uint8_t  discardCap_[discardCapSize];    // captures first N discarded bytes per episode
    uint8_t  discardCapCount_;               // how many bytes captured this episode

    LumpParserStats stats_;
    LegoDevice     *device_;
};

#endif // LUMPPARSER_H
