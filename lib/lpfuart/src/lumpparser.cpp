#include "lumpparser.h"
#include "legodevice.h"
#include "format.h"
#include "logging.h"

#include <cctype>
#include <cstring>

// ---------------------------------------------------------------------------
// LUMP protocol constants (kept local — replaces protocolstate.h)
// ---------------------------------------------------------------------------
static constexpr uint8_t LUMP_MSG_TYPE_SYS  = 0x00;
static constexpr uint8_t LUMP_MSG_TYPE_CMD  = 0x40;
static constexpr uint8_t LUMP_MSG_TYPE_INFO = 0x80;
static constexpr uint8_t LUMP_MSG_TYPE_DATA = 0xC0;

static constexpr uint8_t LUMP_CMD_TYPE     = 0x0;
static constexpr uint8_t LUMP_CMD_MODES    = 0x1;
static constexpr uint8_t LUMP_CMD_SPEED    = 0x2;
static constexpr uint8_t LUMP_CMD_SELECT   = 0x3;
static constexpr uint8_t LUMP_CMD_WRITE    = 0x4;
static constexpr uint8_t LUMP_CMD_EXT_MODE = 0x6;
static constexpr uint8_t LUMP_CMD_VERSION  = 0x7;

static constexpr uint8_t LUMP_SYS_SYNC = 0x00;
static constexpr uint8_t LUMP_SYS_NACK = 0x02;
static constexpr uint8_t LUMP_SYS_ACK  = 0x04;

static constexpr uint8_t LUMP_INFO_NAME        = 0x00;
static constexpr uint8_t LUMP_INFO_RAW         = 0x01;
static constexpr uint8_t LUMP_INFO_PCT         = 0x02;
static constexpr uint8_t LUMP_INFO_SI          = 0x03;
static constexpr uint8_t LUMP_INFO_UNITS       = 0x04;
static constexpr uint8_t LUMP_INFO_MAPPING     = 0x05;
static constexpr uint8_t LUMP_INFO_MODE_COMBOS = 0x06;
static constexpr uint8_t LUMP_INFO_MODE_PLUS_8 = 0x20;
static constexpr uint8_t LUMP_INFO_FORMAT      = 0x80;

// ---------------------------------------------------------------------------
// BCD helper — converts a BCD byte to a two-character decimal string
// e.g. 0x31 -> "31" (note: uses nibble swap like the original parsecommandstate)
// ---------------------------------------------------------------------------
static void bcdByteToStr(uint8_t bcd, char out[3]) {
    // The original code swaps nibbles before extracting digits
    uint8_t swapped = (uint8_t)(((bcd & 0x0F) << 4) | ((bcd >> 4) & 0x0F));
    out[0] = '0' + ((swapped >> 4) & 0x0F);
    out[1] = '0' + (swapped & 0x0F);
    out[2] = '\0';
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
LumpParser::LumpParser(LegoDevice *device)
    : head_(0)
    , count_(0)
    , extModeOffset_(0)
    , inSyncLoss_(false)
    , consecutiveErrors_(0)
    , syncLossDiscardStart_(0)
    , discardCapCount_(0)
    , device_(device) {
    memset(buf_, 0, sizeof(buf_));
    memset(&stats_, 0, sizeof(stats_));
    memset(discardCap_, 0, sizeof(discardCap_));
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void LumpParser::feedByte(uint8_t byte) {
    if (count_ >= ringBufSize) {
        // Overflow: discard the oldest byte to make room
        stats_.bufferOverflows++;
        head_ = (head_ + 1) % ringBufSize;
        count_--;
    }
    uint16_t tail = (head_ + count_) % ringBufSize;
    buf_[tail] = byte;
    count_++;
    processBuffer();
}

void LumpParser::feedBytes(const uint8_t *data, int len) {
    for (int i = 0; i < len; i++) {
        feedByte(data[i]);
    }
}

auto LumpParser::stats() const -> const LumpParserStats & {
    return stats_;
}

void LumpParser::resetStats() {
    memset(&stats_, 0, sizeof(stats_));
}

void LumpParser::reset() {
    head_                  = 0;
    count_                 = 0;
    extModeOffset_         = 0;
    inSyncLoss_            = false;
    consecutiveErrors_     = 0;
    syncLossDiscardStart_  = 0;
    discardCapCount_       = 0;
    memset(&stats_, 0, sizeof(stats_));
}

void LumpParser::clearBuffer() {
    head_                  = 0;
    count_                 = 0;
    extModeOffset_         = 0;
    inSyncLoss_            = false;
    consecutiveErrors_     = 0;
    syncLossDiscardStart_  = 0;
    discardCapCount_       = 0;
    // stats_ intentionally preserved — they cover the pre-switch phase
}

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------
auto LumpParser::decodePayloadSize(uint8_t header) -> int {
    uint8_t sizeEnc = (header >> 3) & 0x07;
    switch (sizeEnc) {
        case 0: return 1;
        case 1: return 2;
        case 2: return 4;
        case 3: return 8;
        case 4: return 16;
        case 5: return 32;
        default: return -1;  // 6 and 7 are reserved/invalid
    }
}

// ---------------------------------------------------------------------------
// processBuffer — inner sliding-window loop
// ---------------------------------------------------------------------------
void LumpParser::processBuffer() {
    while (count_ > 0) {
        uint8_t header = buf_[head_];
        uint8_t type   = header & 0xC0;

        // ---------------------------------------------------------------
        // System byte: type bits 7-6 == 0b00
        // ---------------------------------------------------------------
        if (type == LUMP_MSG_TYPE_SYS) {
            // Consume exactly 1 byte
            head_  = (head_ + 1) % ringBufSize;
            count_--;

            if (header == LUMP_SYS_SYNC || header == LUMP_SYS_NACK || header == LUMP_SYS_ACK) {
                dispatchSystemByte(header);
            } else {
                stats_.unknownSysBytes++;
            }
            continue;
        }

        // ---------------------------------------------------------------
        // Decode payload size from header
        // ---------------------------------------------------------------
        int payloadSize = decodePayloadSize(header);
        if (payloadSize < 0) {
            // Invalid/reserved size encoding — discard this byte
            stats_.invalidSizeBytes++;
            stats_.bytesDiscarded++;
            head_  = (head_ + 1) % ringBufSize;
            count_--;
            continue;
        }

        int frameSize = 1 + payloadSize + 1;  // header + payload + checksum

        // ---------------------------------------------------------------
        // INFO messages have an extra byte for INFO type (not in payload size)
        // ---------------------------------------------------------------
        if (type == LUMP_MSG_TYPE_INFO) {
            frameSize += 1;  // Add 1 byte for INFO type byte
        }

        // ---------------------------------------------------------------
        // Wait for more bytes if buffer doesn't have a complete frame
        // ---------------------------------------------------------------
        if (count_ < (uint16_t)frameSize) {
            break;
        }

        // ---------------------------------------------------------------
        // Compute expected checksum over header + payload bytes
        // ---------------------------------------------------------------
        uint8_t expected = 0xFF;
        for (int i = 0; i < frameSize - 1; i++) {
            expected ^= buf_[(head_ + i) % ringBufSize];
        }

        uint8_t actual = buf_[(head_ + frameSize - 1) % ringBufSize];

        if (expected == actual) {
            // -----------------------------------------------------------
            // Valid frame: copy payload to stack buffer and dispatch
            // For INFO messages, payload includes the INFO type byte
            // -----------------------------------------------------------
            int bytesToExtract = payloadSize;
            if (type == LUMP_MSG_TYPE_INFO) {
                bytesToExtract += 1;  // Include INFO type byte in payload
            }

            uint8_t payload[33];  // Max 32 data bytes + 1 INFO type byte
            for (int i = 0; i < bytesToExtract; i++) {
                payload[i] = buf_[(head_ + 1 + i) % ringBufSize];
            }

            // Advance ring buffer past this frame
            head_  = (head_ + frameSize) % ringBufSize;
            count_ -= frameSize;

            // Sync recovery tracking
            if (inSyncLoss_) {
                uint32_t episodeBytes = stats_.bytesDiscarded - syncLossDiscardStart_;
                INFO("LUMP sync recovered after discarding %lu bytes", episodeBytes);
                // Hex-dump the captured discard bytes
                if (discardCapCount_ > 0) {
                    // Build hex string: "xx xx xx ..."
                    char hexbuf[discardCapSize * 3 + 1];
                    int  pos = 0;
                    const char hex[] = "0123456789ABCDEF";
                    for (uint8_t i = 0; i < discardCapCount_; i++) {
                        hexbuf[pos++] = hex[(discardCap_[i] >> 4) & 0x0F];
                        hexbuf[pos++] = hex[ discardCap_[i]       & 0x0F];
                        hexbuf[pos++] = ' ';
                    }
                    if (pos > 0) hexbuf[pos - 1] = '\0';
                    else         hexbuf[0]        = '\0';
                    bool truncated = (discardCapCount_ == discardCapSize &&
                                      episodeBytes     > discardCapSize);
                    INFO("  Discarded bytes (first %u%s): %s",
                         discardCapCount_, truncated ? ", truncated" : "", hexbuf);
                }
                stats_.syncRecoveries++;
                inSyncLoss_      = false;
                discardCapCount_ = 0;
            }
            consecutiveErrors_ = 0;
            stats_.framesOk++;

            dispatchFrame(header, payload, bytesToExtract);
        } else {
            // -----------------------------------------------------------
            // Bad checksum: slide by 1 byte
            // -----------------------------------------------------------
            if (!inSyncLoss_) {
                WARN("LUMP sync lost after %lu good frames (checksum expected=0x%02X actual=0x%02X, header=0x%02X, type=0x%02X, payloadSize=0x%02X)",
                     stats_.framesOk, expected, actual, header, type, payloadSize);
                inSyncLoss_           = true;
                syncLossDiscardStart_ = stats_.bytesDiscarded;
                discardCapCount_      = 0;
            }
            consecutiveErrors_++;
            stats_.checksumErrors++;
            stats_.bytesDiscarded++;
            if (discardCapCount_ < discardCapSize) {
                discardCap_[discardCapCount_++] = buf_[head_];
            }
            head_  = (head_ + 1) % ringBufSize;
            count_--;

            if (consecutiveErrors_ > LumpParser::syncLossResetThreshold) {
                WARN("Too many consecutive checksum errors (%lu), triggering device reset",
                     consecutiveErrors_);
                device_->reset();
                return;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// dispatchSystemByte
// ---------------------------------------------------------------------------
void LumpParser::dispatchSystemByte(uint8_t sysByte) {
    if (sysByte == LUMP_SYS_SYNC) {
        // Silently ignore SYNC bytes — they are just keep-alive markers
        return;
    }
    if (sysByte == LUMP_SYS_ACK) {
        if (!device_->isHandshakeComplete() && device_->fullyInitialized()) {
            device_->markAsHandshakeComplete();
        }
        return;
    }
    if (sysByte == LUMP_SYS_NACK) {
        DEBUG("Received unexpected NACK from device");
        return;
    }
}

// ---------------------------------------------------------------------------
// dispatchFrame — routes validated frames to the appropriate handler
// ---------------------------------------------------------------------------
void LumpParser::dispatchFrame(uint8_t header, const uint8_t *payload, int payloadSize) {
    uint8_t type = header & 0xC0;

    // -----------------------------------------------------------------------
    // CMD frames (0x40)
    // -----------------------------------------------------------------------
    if (type == LUMP_MSG_TYPE_CMD) {
        uint8_t cmd = header & 0x07;

        switch (cmd) {
            case LUMP_CMD_TYPE: {
                INFO("Parsing LUMP_CMD_TYPE");
                // payload[0] = device type ID
                int deviceId = payload[0];
                std::string deviceName;

                switch (deviceId) {
                    case DEVICEID_EV3_COLOR_SENSOR:
                        deviceName = "MINDSTORMS EV3 Color Sensor";
                        break;
                    case DEVICEID_EV3_ULTRASONIC_SENSOR:
                        deviceName = "MINDSTORMS EV3 Ultrasonic Sensor";
                        break;
                    case DEVICEID_EV3_GYRO_SENSOR:
                        deviceName = "MINDSTORMS EV3 Gyro Sensor";
                        break;
                    case DEVICEID_EV3_INFRARED_SENSOR:
                        deviceName = "MINDSTORMS EV3 Infrared Sensor";
                        break;
                    case DEVICEID_WEDO20_TILT:
                        deviceName = "WeDo 2.0 Tilt Sensor";
                        break;
                    case DEVICEID_WEDO20_MOTION:
                        deviceName = "WeDo 2.0 Motion Sensor";
                        break;
                    case DEVICEID_BOOST_COLOR_DISTANCE_SENSOR:
                        deviceName = "BOOST Color and Distance Sensor";
                        break;
                    case DEVICEID_BOOST_INTERACTIVE_MOTOR:
                        deviceName = "BOOST Interactive Motor";
                        break;
                    case DEVICEID_TECHNIC_XL_MOTOR:
                        deviceName = "Technic XL Motor";
                        break;
                    case DEVICEID_TECHNIC_MEDIUM_MOTOR:
                        deviceName = "Technic Medium Motor";
                        break;
                    case DEVICEID_SPIKE_MEDIUM_MOTOR:
                        deviceName = "SPIKE Medium Motor";
                        break;
                    case DEVUCEID_SPIKE_LARGE_MOTOR:
                        deviceName = "SPIKE Large Motor";
                        break;
                    case DEVICEID_SPIKE_COLOR_SENSOR:
                        deviceName = "SPIKE Color Sensor";
                        break;
                    case DEVICEID_SPIKE_ULTRASONIC_SENSOR:
                        deviceName = "SPIKE Ultrasonic Sensor";
                        break;
                    case DEVICEID_SPIKE_PRIME_FORCE_SENSOR:
                        deviceName = "SPIKE Prime Force Sensor";
                        break;
                    case DEVICEID_TECHNIC_COLOR_LIGHT_MATRIX:
                        deviceName = "Technic Color Light Matrix";
                        break;
                    case DEVICEID_SPIKE_SMALL_MOTOR:
                        deviceName = "SPIKE Small Motor";
                        break;
                    case DEVICEID_TECHNIC_MEDIUM_ANGULAR_MOTOR:
                        deviceName = "Technic Medium Angular Motor";
                        break;
                    case DEVICEID_TECHNIC_LARGE_ANGULAR_MOTOR:
                        deviceName = "Technic Large Angular Motor";
                        break;
                    default:
                        deviceName = "Unknown Device";
                        break;
                }
                device_->setDeviceIdAndName(deviceId, deviceName);
                break;
            }

            case LUMP_CMD_MODES: {
                INFO("Parsing LUMP_CMD_MODES");
                // payload size determines the format:
                // 1 byte: numModes = payload[0]+1
                // 2 bytes: numModes = payload[0]+1, views = payload[1]+1
                // 4 bytes: numModes = payload[2]+1, views = payload[3]+1
                if (payloadSize == 1) {
                    int numModes = payload[0] + 1;
                    device_->initNumberOfModes(numModes);
                    INFO("Number of supported modes is %d (1 byte payload)", numModes);
                } else if (payloadSize == 2) {
                    int numModes = payload[0] + 1;
                    device_->initNumberOfModes(numModes);
                    INFO("Number of supported modes is %d (2 bytes payload)", numModes);
                } else if (payloadSize == 4) {
                    int numModes = payload[0] + 1;
                    device_->initNumberOfModes(numModes);
                    INFO("Number of supported modes is %d (4 bytes payload)", numModes);
                } else {
                    WARN("Unsupported payload length for CMD_MODES: %d", payloadSize);
                }
                break;
            }

            case LUMP_CMD_SPEED: {
                INFO("Parsing LUMP_CMD_SPEED");
                // 4-byte little-endian uint32 baud rate
                if (payloadSize >= 4) {
                    long speed = ((long)(payload[0] & 0xFF))
                               | ((long)(payload[1] & 0xFF) << 8)
                               | ((long)(payload[2] & 0xFF) << 16)
                               | ((long)(payload[3] & 0xFF) << 24);
                    device_->setSerialSpeed(speed);
                } else {
                    WARN("CMD_SPEED payload too short: %d bytes", payloadSize);
                }
                break;
            }

            case LUMP_CMD_SELECT: {
                // Hub -> device command; ignore if received from device
                WARN("CMD_SELECT received (hub->device, ignoring)");
                break;
            }

            case LUMP_CMD_WRITE: {
                // Hub -> device command; unexpected from device side
                WARN("CMD_WRITE received from device (unexpected, ignoring)");
                break;
            }

            case LUMP_CMD_EXT_MODE: {
                INFO("Parsing LUMP_CMD_EXT_MODE");
                // payload[0] = 0x00 (modes 0-7) or 0x08 (modes 8-15)
                if (payloadSize >= 1) {
                    extModeOffset_ = payload[0];
                    DEBUG("CMD_EXT_MODE: extModeOffset set to %d", (int)extModeOffset_);
                } else {
                    WARN("CMD_EXT_MODE payload too short: %d bytes", payloadSize);
                }
                break;
            }

            case LUMP_CMD_VERSION: {
                INFO("Parsing LUMP_CMD_VERSION");
                // 8 bytes: FW version (bytes 0-3 BCD LE) + HW version (bytes 4-7 BCD LE)
                if (payloadSize >= 8) {
                    char tmp[3];
                    std::string fwVersion;
                    fwVersion.reserve(12);
                    bcdByteToStr(payload[3], tmp); fwVersion += tmp;
                    fwVersion += '.';
                    bcdByteToStr(payload[2], tmp); fwVersion += tmp;
                    fwVersion += '.';
                    bcdByteToStr(payload[1], tmp); fwVersion += tmp;
                    fwVersion += '.';
                    bcdByteToStr(payload[0], tmp); fwVersion += tmp;

                    std::string hwVersion;
                    hwVersion.reserve(12);
                    bcdByteToStr(payload[7], tmp); hwVersion += tmp;
                    hwVersion += '.';
                    bcdByteToStr(payload[6], tmp); hwVersion += tmp;
                    hwVersion += '.';
                    bcdByteToStr(payload[5], tmp); hwVersion += tmp;
                    hwVersion += '.';
                    bcdByteToStr(payload[4], tmp); hwVersion += tmp;

                    device_->setVersions(fwVersion, hwVersion);
                } else {
                    WARN("CMD_VERSION payload too short: %d bytes", payloadSize);
                }
                break;
            }

            default:
                WARN("Unknown CMD command: 0x%02X (header=0x%02X)", cmd, header);
                break;
        }
        return;
    }

    // -----------------------------------------------------------------------
    // INFO frames (0x80)
    // -----------------------------------------------------------------------
    if (type == LUMP_MSG_TYPE_INFO) {
        if (payloadSize < 1) {
            WARN("INFO frame with empty payload, skipping");
            return;
        }

        int mode         = header & 0x07;
        uint8_t infoByte = payload[0];

        // Check for INFO_MODE_PLUS_8 flag in payload[0]
        if (infoByte & LUMP_INFO_MODE_PLUS_8) {
            mode += 8;
        }
        uint8_t infoType = infoByte & (uint8_t)(~LUMP_INFO_MODE_PLUS_8);  // strip bit 5

        // Validate mode index
        if (mode < 0 || mode >= 16) {
            WARN("INFO frame with invalid mode index %d, skipping", mode);
            return;
        }

        Mode *modeObj = device_->getMode(mode);
        if (modeObj == nullptr) {
            WARN("INFO frame: getMode(%d) returned nullptr, skipping", mode);
            return;
        }

        switch (infoType) {
            case LUMP_INFO_NAME: {
                INFO("Parsing LUMP_INFO_NAME");
                // payload[1..] = null-terminated ASCII name
                if (payloadSize < 2) {
                    WARN("INFO_NAME payload too short: %d bytes", payloadSize);
                    break;
                }
                char name[12];  // max 11 chars + null
                int  nameLen = 0;
                for (int i = 1; i < payloadSize && nameLen < 11; i++) {
                    uint8_t ch = payload[i];
                    if (ch == 0) break;
                    name[nameLen++] = (char)ch;
                }
                name[nameLen] = '\0';

                if (nameLen > 0 && isupper((unsigned char)name[0])) {
                    INFO("Mode %d name is '%s'", mode, name);
                    std::string nameStr(name, nameLen);
                    modeObj->setName(nameStr);
                } else {
                    WARN("Ignoring Name for mode %d: invalid (len=%d or not uppercase)", mode, nameLen);
                }
                break;
            }

            case LUMP_INFO_RAW: {
                INFO("Parsing LUMP_INFO_RAW");
                // payload[1..4] = float min, payload[5..8] = float max (little-endian IEEE 754)
                if (payloadSize < 9) {
                    WARN("INFO_RAW payload too short: %d bytes", payloadSize);
                    break;
                }
                union { uint8_t b[4]; float f; } uMin, uMax;
                memcpy(uMin.b, &payload[1], 4);
                memcpy(uMax.b, &payload[5], 4);
                // RAW min/max — Mode does not have setRawMinMax, but has no direct API.
                // Store via available method if present; otherwise silently skip.
                // (The original parseinfostate.cpp also skipped RAW in the INFO switch.)
                DEBUG("INFO_RAW for mode %d: min=%f max=%f (skipped)", mode, uMin.f, uMax.f);
                break;
            }

            case LUMP_INFO_PCT: {
                INFO("Parsing LUMP_INFO_PCT");                
                // payload[1..4] = float min, payload[5..8] = float max
                if (payloadSize < 9) {
                    WARN("INFO_PCT payload too short: %d bytes", payloadSize);
                    break;
                }
                union { uint8_t b[4]; float f; } uMin, uMax;
                memcpy(uMin.b, &payload[1], 4);
                memcpy(uMax.b, &payload[5], 4);
                modeObj->setPctMinMax(uMin.f, uMax.f);
                break;
            }

            case LUMP_INFO_SI: {
                INFO("Parsing LUMP_INFO_SI");
                // payload[1..4] = float min, payload[5..8] = float max
                if (payloadSize < 9) {
                    WARN("INFO_SI payload too short: %d bytes", payloadSize);
                    break;
                }
                union { uint8_t b[4]; float f; } uMin, uMax;
                memcpy(uMin.b, &payload[1], 4);
                memcpy(uMax.b, &payload[5], 4);
                modeObj->setSiMinMax(uMin.f, uMax.f);
                break;
            }

            case LUMP_INFO_UNITS: {
                INFO("Parsing LUMP_INFO_UNITS");
                // payload[1..] = null-terminated ASCII units string, max 4 chars
                if (payloadSize < 2) {
                    WARN("INFO_UNITS payload too short: %d bytes", payloadSize);
                    break;
                }
                char units[5];  // max 4 chars + null
                int  unitsLen = 0;
                for (int i = 1; i < payloadSize && unitsLen < 4; i++) {
                    uint8_t ch = payload[i];
                    if (ch == 0) break;
                    units[unitsLen++] = (char)ch;
                }
                units[unitsLen] = '\0';
                std::string unitsStr(units, unitsLen);
                modeObj->setUnits(unitsStr);
                break;
            }

            case LUMP_INFO_MAPPING: {
                INFO("Parsing LUMP_INFO_MAPPING");
                // payload[1] = input flags, payload[2] = output flags
                if (payloadSize < 3) {
                    WARN("INFO_MAPPING payload too short: %d bytes", payloadSize);
                    break;
                }
                uint8_t inputFlags  = payload[1];
                uint8_t outputFlags = payload[2];

                if (inputFlags & 128) modeObj->registerInputType(Mode::InputOutputType::SUPPORTS_NULL);
                if (inputFlags & 64)  modeObj->registerInputType(Mode::InputOutputType::SUPPORTS_FUNCTIONAL_MAPPING_20);
                if (inputFlags & 16)  modeObj->registerInputType(Mode::InputOutputType::ABS);
                if (inputFlags & 8)   modeObj->registerInputType(Mode::InputOutputType::REL);
                if (inputFlags & 4)   modeObj->registerInputType(Mode::InputOutputType::DIS);

                if (outputFlags & 128) modeObj->registerOutputType(Mode::InputOutputType::SUPPORTS_NULL);
                if (outputFlags & 64)  modeObj->registerOutputType(Mode::InputOutputType::SUPPORTS_FUNCTIONAL_MAPPING_20);
                if (outputFlags & 16)  modeObj->registerOutputType(Mode::InputOutputType::ABS);
                if (outputFlags & 8)   modeObj->registerOutputType(Mode::InputOutputType::REL);
                if (outputFlags & 4)   modeObj->registerOutputType(Mode::InputOutputType::DIS);
                break;
            }

            case LUMP_INFO_MODE_COMBOS: {
                INFO("Parsing LUMP_INFO_MODE_COMBOS");
                INFO("Got Info Mode Combos for mode %d, payload size %d", mode, payloadSize);
                for (int i = 0; i < payloadSize; i++) {
                    DEBUG("  Byte %d = 0x%02X", i, payload[i]);
                }
                // No action yet — combi-mode support is a future extension
                break;
            }

            case LUMP_INFO_FORMAT: {
                INFO("Parsing LUMP_INFO_FORMAT");
                // payload[1]=datasets, payload[2]=format type, payload[3]=figures, payload[4]=decimals
                if (payloadSize < 5) {
                    WARN("INFO_FORMAT payload too short: %d bytes", payloadSize);
                    break;
                }
                int datasets = payload[1];
                int fmtType  = payload[2];
                int figures  = payload[3];
                int decimals = payload[4];

                INFO("Mode %d format: datasets=%d, format=%d, figures=%d, decimals=%d",
                     mode, datasets, fmtType, figures, decimals);

                Format *fmt = new Format(datasets, Format::forId(fmtType), figures, decimals);
                modeObj->setFormat(fmt);
                break;
            }

            default: {
                // Unknown info types 0x07-0x0C and anything else: log at DEBUG, do not discard
                WARN("Unknown INFO type 0x%02X for mode %d, ignoring", infoType, mode);
                break;
            }
        }
        return;
    }

    // -----------------------------------------------------------------------
    // DATA frames (0xC0)
    // -----------------------------------------------------------------------
    if (type == LUMP_MSG_TYPE_DATA) {
        int mode       = (header & 0x07) + extModeOffset_;
        extModeOffset_ = 0;  // consume the offset

        if (mode < 0 || mode >= 16) {
            WARN("DATA frame with invalid mode index %d, skipping", mode);
            return;
        }

        device_->onDataFrame(mode, payload, payloadSize);
        // Signal the inter-frame gap only when the ring buffer is empty.
        // If more frames are batched (count_ > 0), the device is already
        // transmitting subsequent frames; sending a NACK now would collide.
        // When count_ == 0 we have fully caught up — the wire is idle.
        if (count_ == 0) {
            device_->onDataFrameDispatched();
        }
        return;
    }

    // SYS frames (type == 0x00) are handled before dispatchFrame() is called;
    // reaching here should never happen.
    WARN("dispatchFrame called with unexpected type 0x%02X (header=0x%02X)", type, header);
}
