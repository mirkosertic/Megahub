// ---------------------------------------------------------------------------
// Unit tests for Mode — T-B: reset idempotency, setFormat, processDataPacket
// Uses Unity test framework (PlatformIO native environment)
//
// Run with: pio test -e native --filter test_mode
//
// This file reproduces Mode/Format/Dataset logic inline to avoid Arduino.h
// dependency on host (same pattern as test_lumpparser).
// ---------------------------------------------------------------------------

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <unity.h>
#include <vector>

// ---------------------------------------------------------------------------
// Logging stubs
// ---------------------------------------------------------------------------
#define INFO(msg, ...)                                                                                                 \
	do {                                                                                                               \
		printf("[INFO] " msg "\n", ##__VA_ARGS__);                                                                     \
	} while (0)
#define WARN(msg, ...)                                                                                                 \
	do {                                                                                                               \
		printf("[WARN] " msg "\n", ##__VA_ARGS__);                                                                     \
	} while (0)
#define ERROR(msg, ...)                                                                                                \
	do {                                                                                                               \
		printf("[ERRO] " msg "\n", ##__VA_ARGS__);                                                                     \
	} while (0)
#define DEBUG(msg, ...)                                                                                                \
	do {                                                                                                               \
	} while (0)

// ---------------------------------------------------------------------------
// Inline reproduction of Format
// (from lib/lpfuart/include/format.h + src/format.cpp)
// ---------------------------------------------------------------------------
class Format {
  public:
	enum class FormatType {
		DATA8 = 0x00,
		DATA16 = 0x01,
		DATA32 = 0x02,
		DATAFLOAT = 0x03,
		UNKNOWN = 0xff
	};
	static FormatType forId(int id) {
		switch (id) {
			case 0:
				return FormatType::DATA8;
			case 1:
				return FormatType::DATA16;
			case 2:
				return FormatType::DATA32;
			case 3:
				return FormatType::DATAFLOAT;
			default:
				return FormatType::UNKNOWN;
		}
	}
	Format(int ds, FormatType ft, int fig, int dec) : datasets_(ds), formatType_(ft), figures_(fig), decimals_(dec) {}
	int getDatasets() const { return datasets_; }
	FormatType getFormatType() const { return formatType_; }
	int getFigures() const { return figures_; }
	int getDecimals() const { return decimals_; }

  private:
	int datasets_, figures_, decimals_;
	FormatType formatType_;
};

// ---------------------------------------------------------------------------
// Inline reproduction of Dataset
// (from lib/lpfuart/include/dataset.h + src/dataset.cpp)
// ---------------------------------------------------------------------------
class Dataset {
  public:
	Dataset() : formatType_(Format::FormatType::UNKNOWN), intValue_(0), floatValue_(0.0f) {}

	Format::FormatType getType() { return formatType_; }
	int getDataAsInt() { return intValue_; }
	float getDataAsFloat() { return floatValue_; }

	void readData(Format::FormatType type, const uint8_t* payload) {
		formatType_ = type;
		switch (type) {
			case Format::FormatType::DATA8:
				intValue_ = static_cast<int>(static_cast<int8_t>(payload[0]));
				floatValue_ = static_cast<float>(intValue_);
				break;
			case Format::FormatType::DATA16: {
				int16_t v = static_cast<int16_t>(payload[0] | (payload[1] << 8));
				intValue_ = v;
				floatValue_ = static_cast<float>(v);
				break;
			}
			case Format::FormatType::DATA32: {
				int32_t v =
				    static_cast<int32_t>(payload[0] | (payload[1] << 8) | (payload[2] << 16) | (payload[3] << 24));
				intValue_ = v;
				floatValue_ = static_cast<float>(v);
				break;
			}
			case Format::FormatType::DATAFLOAT: {
				union {
					uint8_t b[4];
					float f;
				} u;
				memcpy(u.b, payload, 4);
				floatValue_ = u.f;
				intValue_ = static_cast<int>(u.f);
				break;
			}
			default:
				break;
		}
	}

  private:
	Format::FormatType formatType_;
	int intValue_;
	float floatValue_;
};

// ---------------------------------------------------------------------------
// Inline reproduction of Mode
// (from lib/lpfuart/include/mode.h + src/mode.cpp)
// ---------------------------------------------------------------------------
class Mode {
  public:
	enum class InputOutputType {
		SUPPORTS_FUNCTIONAL_MAPPING_20,
		ABS,
		REL,
		DIS,
		SUPPORTS_NULL
	};

	Mode() : name_(""), units_(""), pctMin_(0.0f), pctMax_(100.0f), siMin_(0.0f), siMax_(1023.0f) {
		for (int i = 0; i < 5; i++) {
			inputTypes_[i] = false;
			outputTypes_[i] = false;
		}
	}
	~Mode() {}

	void reset() {
		format_.reset();
		datasets_.clear();
		pctMin_ = 0.0f;
		pctMax_ = 100.0f;
		siMin_ = 0.0f;
		siMax_ = 1023.0f;
		name_ = "";
		units_ = "";
		for (int i = 0; i < 5; i++) {
			inputTypes_[i] = false;
			outputTypes_[i] = false;
		}
	}

	void setName(const std::string& name) { name_ = name; }
	void setUnits(const std::string& units) { units_ = units; }
	void setPctMinMax(float mn, float mx) {
		pctMin_ = mn;
		pctMax_ = mx;
	}
	void setSiMinMax(float mn, float mx) {
		siMin_ = mn;
		siMax_ = mx;
	}

	void setFormat(std::unique_ptr<Format> format) {
		format_ = std::move(format);
		datasets_.assign(format_->getDatasets(), Dataset{});
	}

	const std::string& getName() { return name_; }
	const std::string& getUnits() { return units_; }
	float getPctMin() { return pctMin_; }
	float getPctMax() { return pctMax_; }
	float getSiMin() { return siMin_; }
	float getSiMax() { return siMax_; }
	Format* getFormat() { return format_.get(); }

	void processDataPacket(const uint8_t* payload, int payloadSize) {
		if (format_ == nullptr) {
			WARN("Not fully initialized yet!");
			return;
		}
		int datasetSize = 0;
		switch (format_->getFormatType()) {
			case Format::FormatType::DATA8:
				datasetSize = 1;
				break;
			case Format::FormatType::DATA16:
				datasetSize = 2;
				break;
			case Format::FormatType::DATA32:
				datasetSize = 4;
				break;
			case Format::FormatType::DATAFLOAT:
				datasetSize = 4;
				break;
			default:
				WARN("Unknown format type: %d", static_cast<int>(format_->getFormatType()));
				return;
		}
		int expectedSize = format_->getDatasets() * datasetSize;
		if (expectedSize != payloadSize) {
			WARN("Wrong payload size. Expected %d, got %d", expectedSize, payloadSize);
			return;
		}
		const uint8_t* ptr = payload;
		for (int i = 0; i < format_->getDatasets(); i++) {
			datasets_[i].readData(format_->getFormatType(), ptr);
			ptr += datasetSize;
		}
	}

	Dataset* getDataset(int index) { return &datasets_[index]; }

  private:
	std::string name_, units_;
	float pctMin_, pctMax_, siMin_, siMax_;
	std::unique_ptr<Format> format_;
	bool inputTypes_[5];
	bool outputTypes_[5];
	std::vector<Dataset> datasets_;
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

void setUp() {}
void tearDown() {}

// MODE-01: Constructor initialises all fields to documented defaults
static void test_MODE01_default_state() {
	Mode m;
	TEST_ASSERT_EQUAL_STRING("", m.getName().c_str());
	TEST_ASSERT_EQUAL_STRING("", m.getUnits().c_str());
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, m.getPctMin());
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, m.getPctMax());
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, m.getSiMin());
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 1023.0f, m.getSiMax());
	TEST_ASSERT_NULL(m.getFormat());
}

// MODE-02: setFormat allocates correct number of Dataset slots
static void test_MODE02_setFormat_allocates_datasets() {
	Mode m;
	m.setFormat(std::make_unique<Format>(3, Format::FormatType::DATA8, 3, 0));
	TEST_ASSERT_NOT_NULL(m.getFormat());
	TEST_ASSERT_EQUAL_INT(3, m.getFormat()->getDatasets());
	TEST_ASSERT_NOT_NULL(m.getDataset(0));
	TEST_ASSERT_NOT_NULL(m.getDataset(1));
	TEST_ASSERT_NOT_NULL(m.getDataset(2));
}

// MODE-03: reset() clears format_, datasets_, and all metadata fields
static void test_MODE03_reset_clears_everything() {
	Mode m;
	m.setName("COLOR");
	m.setUnits("pct");
	m.setPctMinMax(10.0f, 90.0f);
	m.setSiMinMax(5.0f, 500.0f);
	m.setFormat(std::make_unique<Format>(2, Format::FormatType::DATA16, 5, 0));

	m.reset();

	TEST_ASSERT_NULL(m.getFormat());
	TEST_ASSERT_EQUAL_STRING("", m.getName().c_str());
	TEST_ASSERT_EQUAL_STRING("", m.getUnits().c_str());
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, m.getPctMin());
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, m.getPctMax());
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, m.getSiMin());
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 1023.0f, m.getSiMax());
}

// MODE-04: reset() called twice must not crash (idempotency)
static void test_MODE04_reset_idempotent_no_crash() {
	Mode m;
	m.setFormat(std::make_unique<Format>(2, Format::FormatType::DATA8, 3, 0));
	m.reset();
	m.reset(); // second reset on already-reset Mode — must not crash
	TEST_ASSERT_NULL(m.getFormat());
}

// MODE-05: reset() then setFormat again works (device reconnect scenario)
static void test_MODE05_reset_then_setFormat_again() {
	Mode m;
	m.setFormat(std::make_unique<Format>(2, Format::FormatType::DATA8, 3, 0));
	m.reset();
	m.setFormat(std::make_unique<Format>(4, Format::FormatType::DATA16, 5, 2));
	TEST_ASSERT_NOT_NULL(m.getFormat());
	TEST_ASSERT_EQUAL_INT(4, m.getFormat()->getDatasets());
	TEST_ASSERT_NOT_NULL(m.getDataset(0));
	TEST_ASSERT_NOT_NULL(m.getDataset(3));
}

// MODE-06: processDataPacket before setFormat logs WARN and returns, no crash
static void test_MODE06_processDataPacket_before_setFormat_no_crash() {
	Mode m;
	uint8_t payload[] = {0x01, 0x02};
	m.processDataPacket(payload, 2);
	TEST_ASSERT_NULL(m.getFormat());
}

// MODE-07: processDataPacket with DATA8 — signed byte values
static void test_MODE07_processDataPacket_DATA8_signed() {
	Mode m;
	m.setFormat(std::make_unique<Format>(2, Format::FormatType::DATA8, 3, 0));
	uint8_t payload[] = {42, 0xFF}; // 42, -1 (signed)
	m.processDataPacket(payload, 2);
	TEST_ASSERT_EQUAL_INT(42, m.getDataset(0)->getDataAsInt());
	TEST_ASSERT_EQUAL_INT(-1, m.getDataset(1)->getDataAsInt());
}

// MODE-08: processDataPacket with DATA16 — little-endian
static void test_MODE08_processDataPacket_DATA16_little_endian() {
	Mode m;
	m.setFormat(std::make_unique<Format>(1, Format::FormatType::DATA16, 5, 0));
	uint8_t payload[] = {0x00, 0x01}; // little-endian → 256
	m.processDataPacket(payload, 2);
	TEST_ASSERT_EQUAL_INT(256, m.getDataset(0)->getDataAsInt());
}

// MODE-09: processDataPacket with wrong size — WARN logged, datasets unchanged
static void test_MODE09_processDataPacket_wrong_size_no_crash() {
	Mode m;
	m.setFormat(std::make_unique<Format>(2, Format::FormatType::DATA8, 3, 0));
	uint8_t payload[] = {0x01, 0x02, 0x03}; // 3 bytes, but expected 2
	m.processDataPacket(payload, 3);
	// Dataset values stay at default (0)
	TEST_ASSERT_EQUAL_INT(0, m.getDataset(0)->getDataAsInt());
	TEST_ASSERT_EQUAL_INT(0, m.getDataset(1)->getDataAsInt());
}

// MODE-10: processDataPacket with DATAFLOAT
static void test_MODE10_processDataPacket_DATAFLOAT() {
	Mode m;
	m.setFormat(std::make_unique<Format>(1, Format::FormatType::DATAFLOAT, 4, 1));
	float expected = 3.14f;
	union {
		float f;
		uint8_t b[4];
	} u;
	u.f = expected;
	m.processDataPacket(u.b, 4);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, expected, m.getDataset(0)->getDataAsFloat());
}

// MODE-11: multiple reset/setFormat cycles (stress reconnect)
static void test_MODE11_multiple_reconnect_cycles() {
	Mode m;
	for (int cycle = 0; cycle < 5; cycle++) {
		m.setName("SENSOR");
		m.setFormat(std::make_unique<Format>(3, Format::FormatType::DATA8, 3, 0));
		uint8_t payload[] = {(uint8_t) cycle, (uint8_t) (cycle + 1), (uint8_t) (cycle + 2)};
		m.processDataPacket(payload, 3);
		TEST_ASSERT_EQUAL_INT(cycle, m.getDataset(0)->getDataAsInt());
		TEST_ASSERT_EQUAL_INT(cycle + 1, m.getDataset(1)->getDataAsInt());
		m.reset();
		TEST_ASSERT_NULL(m.getFormat());
		TEST_ASSERT_EQUAL_STRING("", m.getName().c_str());
	}
}

int main() {
	UNITY_BEGIN();
	RUN_TEST(test_MODE01_default_state);
	RUN_TEST(test_MODE02_setFormat_allocates_datasets);
	RUN_TEST(test_MODE03_reset_clears_everything);
	RUN_TEST(test_MODE04_reset_idempotent_no_crash);
	RUN_TEST(test_MODE05_reset_then_setFormat_again);
	RUN_TEST(test_MODE06_processDataPacket_before_setFormat_no_crash);
	RUN_TEST(test_MODE07_processDataPacket_DATA8_signed);
	RUN_TEST(test_MODE08_processDataPacket_DATA16_little_endian);
	RUN_TEST(test_MODE09_processDataPacket_wrong_size_no_crash);
	RUN_TEST(test_MODE10_processDataPacket_DATAFLOAT);
	RUN_TEST(test_MODE11_multiple_reconnect_cycles);
	return UNITY_END();
}
