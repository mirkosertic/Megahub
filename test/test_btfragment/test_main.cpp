// ---------------------------------------------------------------------------
// Unit tests for BT fragment reassembly — T3
// Uses Unity test framework (PlatformIO native environment)
//
// Run with: pio test -e native --filter test_btfragment
//
// This file reproduces the core fragment reassembly logic from
// BTRemote::handleFragment() inline, without FreeRTOS/BLE dependencies.
// Constants and types match lib/btremote/include/btremote.h exactly.
// ---------------------------------------------------------------------------

#include <unity.h>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <vector>
#include <map>

// ---------------------------------------------------------------------------
// millis() stub (controlled by tests)
// ---------------------------------------------------------------------------
static unsigned long g_millis = 0;
static unsigned long millis() { return g_millis; }

// ---------------------------------------------------------------------------
// Logging stubs
// ---------------------------------------------------------------------------
#define INFO(msg, ...)  do { printf("[INFO] " msg "\n", ##__VA_ARGS__); } while(0)
#define WARN(msg, ...)  do { printf("[WARN] " msg "\n", ##__VA_ARGS__); } while(0)
#define ERROR(msg, ...) do { printf("[ERRO] " msg "\n", ##__VA_ARGS__); } while(0)
#define DEBUG(msg, ...) do {} while(0)

// ---------------------------------------------------------------------------
// Protocol types and constants — from lib/btremote/include/btremote.h
// ---------------------------------------------------------------------------
enum class ProtocolMessageType : uint8_t {
    REQUEST    = 0x01,
    RESPONSE   = 0x02,
    EVENT      = 0x03,
    CONTROL    = 0x04,
    STREAM_DATA  = 0x05,
    STREAM_START = 0x06,
    STREAM_END   = 0x07
};

enum class ControlMessageType : uint8_t {
    ACK         = 0x01,
    NACK        = 0x02,
    RETRY       = 0x03,
    BUFFER_FULL = 0x04,
    RESET       = 0x05,
    MTU_INFO    = 0x06,
    STREAM_ACK  = 0x07,
    STREAM_ERROR = 0x09
};

static const size_t   FRAGMENT_HEADER_SIZE    = 5;
static const size_t   MAX_BUFFER_SIZE         = 65536;
static const uint32_t FRAGMENT_TIMEOUT_MS     = 50000;
static const size_t   MAX_CONCURRENT_MESSAGES = 3;
static const uint8_t  FLAG_LAST_FRAGMENT      = 0x01;

struct FragmentBuffer {
    std::vector<uint8_t> data;
    uint32_t lastFragmentTime;
    int receivedFragments;
    ProtocolMessageType protocolType;
};

// ---------------------------------------------------------------------------
// Result type returned by the test-friendly reassembly function
// ---------------------------------------------------------------------------
enum class AssemblyResult {
    PENDING,        // fragment accepted, message not yet complete
    COMPLETE,       // message fully assembled
    NACK,           // timeout or error
    BUFFER_FULL,    // message too large or too many concurrent
    STREAM_ROUTED,  // streaming protocol byte — not reassembled here
    TOO_SHORT       // packet shorter than header
};

// ---------------------------------------------------------------------------
// Inline reproduction of BTRemote::handleFragment() core logic
//
// Returns AssemblyResult. On COMPLETE, `completedData` holds the full payload
// and `completedId` holds the messageId.
// ---------------------------------------------------------------------------
static AssemblyResult handleFragment(
    const uint8_t *data,
    size_t length,
    std::map<uint8_t, FragmentBuffer> &fragmentBuffers,
    std::vector<uint8_t> &completedData,
    uint8_t &completedId)
{
    if (length < FRAGMENT_HEADER_SIZE) {
        return AssemblyResult::TOO_SHORT;
    }

    ProtocolMessageType protocolType = static_cast<ProtocolMessageType>(data[0]);
    uint8_t  messageId   = data[1];
    uint16_t fragmentNum = static_cast<uint16_t>((data[2] << 8) | data[3]);
    uint8_t  flags       = data[4];

    // Streaming messages bypass fragment reassembly
    if (protocolType == ProtocolMessageType::STREAM_START ||
        protocolType == ProtocolMessageType::STREAM_DATA  ||
        protocolType == ProtocolMessageType::STREAM_END) {
        return AssemblyResult::STREAM_ROUTED;
    }

    // Memory safety: limit concurrent messages
    if (fragmentBuffers.size() >= MAX_CONCURRENT_MESSAGES &&
        fragmentBuffers.find(messageId) == fragmentBuffers.end()) {
        WARN("Max concurrent messages reached, rejecting message ID %d", messageId);
        return AssemblyResult::BUFFER_FULL;
    }

    auto &buffer = fragmentBuffers[messageId];

    if (fragmentNum == 0) {
        buffer.data.clear();
        buffer.lastFragmentTime  = millis();
        buffer.receivedFragments = 0;
        buffer.protocolType      = protocolType;
    }

    // Timeout check (only meaningful for non-first fragments)
    if (fragmentNum != 0 && millis() - buffer.lastFragmentTime > FRAGMENT_TIMEOUT_MS) {
        WARN("Fragment timeout for message ID %d", messageId);
        fragmentBuffers.erase(messageId);
        return AssemblyResult::NACK;
    }

    buffer.lastFragmentTime = millis();

    const uint8_t *payload  = data + FRAGMENT_HEADER_SIZE;
    size_t         payloadSize = length - FRAGMENT_HEADER_SIZE;

    if (buffer.data.size() + payloadSize > MAX_BUFFER_SIZE) {
        WARN("Buffer overflow for message ID %d", messageId);
        fragmentBuffers.erase(messageId);
        return AssemblyResult::BUFFER_FULL;
    }

    buffer.data.insert(buffer.data.end(), payload, payload + payloadSize);
    buffer.receivedFragments++;

    if (flags & FLAG_LAST_FRAGMENT) {
        completedData = buffer.data;
        completedId   = messageId;
        fragmentBuffers.erase(messageId);
        return AssemblyResult::COMPLETE;
    }

    return AssemblyResult::PENDING;
}

// ---------------------------------------------------------------------------
// Helper: build a fragment packet
//   [0] = protocolType
//   [1] = messageId
//   [2] = fragmentNum high byte
//   [3] = fragmentNum low byte
//   [4] = flags
//   [5..] = payload bytes
// ---------------------------------------------------------------------------
static std::vector<uint8_t> makeFragment(
    ProtocolMessageType type,
    uint8_t messageId,
    uint16_t fragmentNum,
    uint8_t flags,
    const std::vector<uint8_t> &payload)
{
    std::vector<uint8_t> pkt;
    pkt.push_back(static_cast<uint8_t>(type));
    pkt.push_back(messageId);
    pkt.push_back(static_cast<uint8_t>((fragmentNum >> 8) & 0xFF));
    pkt.push_back(static_cast<uint8_t>(fragmentNum & 0xFF));
    pkt.push_back(flags);
    pkt.insert(pkt.end(), payload.begin(), payload.end());
    return pkt;
}

// ---------------------------------------------------------------------------
// Test state helpers
// ---------------------------------------------------------------------------
void setUp() {
    g_millis = 0;
}
void tearDown() {}

// ---------------------------------------------------------------------------
// BT-01: Single-fragment message (fragmentNum=0, FLAG_LAST set) → COMPLETE
// ---------------------------------------------------------------------------
static void test_BT01_single_fragment_message() {
    std::map<uint8_t, FragmentBuffer> buffers;
    std::vector<uint8_t> completed;
    uint8_t completedId = 0xFF;

    std::vector<uint8_t> payload = {0x01, 0x02, 0x03, 0x04};
    auto pkt = makeFragment(ProtocolMessageType::REQUEST, 0x42, 0, FLAG_LAST_FRAGMENT, payload);

    AssemblyResult r = handleFragment(pkt.data(), pkt.size(), buffers, completed, completedId);

    TEST_ASSERT_EQUAL_INT(static_cast<int>(AssemblyResult::COMPLETE), static_cast<int>(r));
    TEST_ASSERT_EQUAL_INT(0x42, completedId);
    TEST_ASSERT_EQUAL_INT(4, static_cast<int>(completed.size()));
    TEST_ASSERT_EQUAL_INT(0x01, completed[0]);
    TEST_ASSERT_EQUAL_INT(0x04, completed[3]);
    TEST_ASSERT_EQUAL_INT(0, static_cast<int>(buffers.size()));  // buffer erased after complete
}

// ---------------------------------------------------------------------------
// BT-02: Three-fragment message — COMPLETE only on third (FLAG_LAST)
// ---------------------------------------------------------------------------
static void test_BT02_multi_fragment_assembles_correctly() {
    std::map<uint8_t, FragmentBuffer> buffers;
    std::vector<uint8_t> completed;
    uint8_t completedId = 0xFF;
    uint8_t msgId = 0x01;

    std::vector<uint8_t> p1 = {0xAA, 0xBB};
    std::vector<uint8_t> p2 = {0xCC, 0xDD};
    std::vector<uint8_t> p3 = {0xEE, 0xFF};

    auto f1 = makeFragment(ProtocolMessageType::REQUEST, msgId, 0, 0x00, p1);
    auto f2 = makeFragment(ProtocolMessageType::REQUEST, msgId, 1, 0x00, p2);
    auto f3 = makeFragment(ProtocolMessageType::REQUEST, msgId, 2, FLAG_LAST_FRAGMENT, p3);

    AssemblyResult r1 = handleFragment(f1.data(), f1.size(), buffers, completed, completedId);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(AssemblyResult::PENDING),  static_cast<int>(r1));
    TEST_ASSERT_EQUAL_INT(1, static_cast<int>(buffers.size()));

    AssemblyResult r2 = handleFragment(f2.data(), f2.size(), buffers, completed, completedId);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(AssemblyResult::PENDING),  static_cast<int>(r2));

    AssemblyResult r3 = handleFragment(f3.data(), f3.size(), buffers, completed, completedId);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(AssemblyResult::COMPLETE), static_cast<int>(r3));
    TEST_ASSERT_EQUAL_INT(msgId, completedId);
    TEST_ASSERT_EQUAL_INT(6, static_cast<int>(completed.size()));
    TEST_ASSERT_EQUAL_INT(0xAA, completed[0]);
    TEST_ASSERT_EQUAL_INT(0xBB, completed[1]);
    TEST_ASSERT_EQUAL_INT(0xCC, completed[2]);
    TEST_ASSERT_EQUAL_INT(0xDD, completed[3]);
    TEST_ASSERT_EQUAL_INT(0xEE, completed[4]);
    TEST_ASSERT_EQUAL_INT(0xFF, completed[5]);
    TEST_ASSERT_EQUAL_INT(0, static_cast<int>(buffers.size()));
}

// ---------------------------------------------------------------------------
// BT-03: Fragment timeout — second fragment arrives after FRAGMENT_TIMEOUT_MS
// ---------------------------------------------------------------------------
static void test_BT03_fragment_timeout_returns_nack() {
    std::map<uint8_t, FragmentBuffer> buffers;
    std::vector<uint8_t> completed;
    uint8_t completedId = 0xFF;
    uint8_t msgId = 0x02;

    g_millis = 0;
    auto f1 = makeFragment(ProtocolMessageType::REQUEST, msgId, 0, 0x00, {0x01});
    handleFragment(f1.data(), f1.size(), buffers, completed, completedId);

    // Advance time past timeout
    g_millis = FRAGMENT_TIMEOUT_MS + 1000;
    auto f2 = makeFragment(ProtocolMessageType::REQUEST, msgId, 1, FLAG_LAST_FRAGMENT, {0x02});
    AssemblyResult r = handleFragment(f2.data(), f2.size(), buffers, completed, completedId);

    TEST_ASSERT_EQUAL_INT(static_cast<int>(AssemblyResult::NACK), static_cast<int>(r));
    TEST_ASSERT_EQUAL_INT(0, static_cast<int>(buffers.size()));  // buffer erased
}

// ---------------------------------------------------------------------------
// BT-04: Buffer overflow — payload exceeds MAX_BUFFER_SIZE
// ---------------------------------------------------------------------------
static void test_BT04_buffer_overflow_returns_buffer_full() {
    std::map<uint8_t, FragmentBuffer> buffers;
    std::vector<uint8_t> completed;
    uint8_t completedId = 0xFF;
    uint8_t msgId = 0x03;

    // Send a first fragment that partially fills the buffer
    std::vector<uint8_t> bigPayload(MAX_BUFFER_SIZE - 10, 0xAB);
    auto f1 = makeFragment(ProtocolMessageType::REQUEST, msgId, 0, 0x00, bigPayload);
    AssemblyResult r1 = handleFragment(f1.data(), f1.size(), buffers, completed, completedId);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(AssemblyResult::PENDING), static_cast<int>(r1));

    // Second fragment that would push total over MAX_BUFFER_SIZE
    std::vector<uint8_t> overflow(20, 0xCD);
    auto f2 = makeFragment(ProtocolMessageType::REQUEST, msgId, 1, FLAG_LAST_FRAGMENT, overflow);
    AssemblyResult r2 = handleFragment(f2.data(), f2.size(), buffers, completed, completedId);

    TEST_ASSERT_EQUAL_INT(static_cast<int>(AssemblyResult::BUFFER_FULL), static_cast<int>(r2));
    TEST_ASSERT_EQUAL_INT(0, static_cast<int>(buffers.size()));  // buffer erased
}

// ---------------------------------------------------------------------------
// BT-05: MAX_CONCURRENT_MESSAGES — opening MAX+1 different message IDs
// ---------------------------------------------------------------------------
static void test_BT05_max_concurrent_messages_limit() {
    std::map<uint8_t, FragmentBuffer> buffers;
    std::vector<uint8_t> completed;
    uint8_t completedId = 0xFF;

    // Open MAX_CONCURRENT_MESSAGES distinct in-progress messages
    for (uint8_t id = 0; id < static_cast<uint8_t>(MAX_CONCURRENT_MESSAGES); id++) {
        auto pkt = makeFragment(ProtocolMessageType::REQUEST, id, 0, 0x00, {0xAA});
        AssemblyResult r = handleFragment(pkt.data(), pkt.size(), buffers, completed, completedId);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(AssemblyResult::PENDING), static_cast<int>(r));
    }
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MAX_CONCURRENT_MESSAGES),
                          static_cast<int>(buffers.size()));

    // Opening one more NEW message ID must be rejected
    uint8_t extraId = static_cast<uint8_t>(MAX_CONCURRENT_MESSAGES);
    auto pkt = makeFragment(ProtocolMessageType::REQUEST, extraId, 0, 0x00, {0xBB});
    AssemblyResult r = handleFragment(pkt.data(), pkt.size(), buffers, completed, completedId);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(AssemblyResult::BUFFER_FULL), static_cast<int>(r));

    // Existing messages are still in the map
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MAX_CONCURRENT_MESSAGES),
                          static_cast<int>(buffers.size()));
}

// ---------------------------------------------------------------------------
// BT-06: Streaming messages are routed past reassembly (STREAM_ROUTED)
// ---------------------------------------------------------------------------
static void test_BT06_stream_messages_bypass_reassembly() {
    std::map<uint8_t, FragmentBuffer> buffers;
    std::vector<uint8_t> completed;
    uint8_t completedId = 0xFF;

    for (auto type : {ProtocolMessageType::STREAM_START,
                      ProtocolMessageType::STREAM_DATA,
                      ProtocolMessageType::STREAM_END}) {
        auto pkt = makeFragment(type, 0x01, 0, FLAG_LAST_FRAGMENT, {0x01, 0x02});
        AssemblyResult r = handleFragment(pkt.data(), pkt.size(), buffers, completed, completedId);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(AssemblyResult::STREAM_ROUTED),
                              static_cast<int>(r));
    }
    TEST_ASSERT_EQUAL_INT(0, static_cast<int>(buffers.size()));
}

// ---------------------------------------------------------------------------
// BT-07: Packet shorter than FRAGMENT_HEADER_SIZE → TOO_SHORT
// ---------------------------------------------------------------------------
static void test_BT07_too_short_packet() {
    std::map<uint8_t, FragmentBuffer> buffers;
    std::vector<uint8_t> completed;
    uint8_t completedId = 0xFF;

    uint8_t shortPkt[] = {0x01, 0x00, 0x00};  // only 3 bytes, need ≥5
    AssemblyResult r = handleFragment(shortPkt, 3, buffers, completed, completedId);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(AssemblyResult::TOO_SHORT), static_cast<int>(r));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_BT01_single_fragment_message);
    RUN_TEST(test_BT02_multi_fragment_assembles_correctly);
    RUN_TEST(test_BT03_fragment_timeout_returns_nack);
    RUN_TEST(test_BT04_buffer_overflow_returns_buffer_full);
    RUN_TEST(test_BT05_max_concurrent_messages_limit);
    RUN_TEST(test_BT06_stream_messages_bypass_reassembly);
    RUN_TEST(test_BT07_too_short_packet);
    return UNITY_END();
}
