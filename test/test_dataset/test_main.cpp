// ---------------------------------------------------------------------------
// Unit tests for Dataset byte parsing — DS-01 through DS-16
// Uses Unity test framework (PlatformIO native environment)
//
// Run with: pio test -e native --filter test_dataset
// ---------------------------------------------------------------------------

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <unity.h>

// Logging stubs
#define INFO(msg, ...)                                                                                                 \
	do {                                                                                                               \
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
// Inline reproduction of FormatType enum (matches Format::FormatType)
// ---------------------------------------------------------------------------
enum class FormatType : uint8_t {
	DATA8 = 0,
	DATA16 = 1,
	DATA32 = 2,
	DATAFLOAT = 3,
	UNKNOWN = 0xff
};

// ---------------------------------------------------------------------------
// Inline reproduction of Dataset logic (mirrors dataset.cpp)
// ---------------------------------------------------------------------------
class Dataset {
  public:
	Dataset() : formatType_(FormatType::UNKNOWN), intValue_(0), floatValue_(0.0f) {}

	void readData(FormatType type, const uint8_t* payload) {
		formatType_ = type;
		switch (type) {
			case FormatType::DATA8: {
				intValue_ = static_cast<int8_t>(payload[0]);
				break;
			}
			case FormatType::DATA16: {
				int16_t v;
				std::memcpy(&v, payload, 2);
				intValue_ = v;
				break;
			}
			case FormatType::DATA32: {
				int32_t v;
				std::memcpy(&v, payload, 4);
				intValue_ = v;
				break;
			}
			case FormatType::DATAFLOAT: {
				std::memcpy(&floatValue_, payload, 4);
				break;
			}
			default:
				break;
		}
	}

	int getDataAsInt() const {
		if (formatType_ == FormatType::DATAFLOAT) {
			return (int) floatValue_;
		}
		return intValue_;
	}
	float getDataAsFloat() const {
		if (formatType_ == FormatType::DATAFLOAT) {
			return floatValue_;
		}
		return (float) intValue_;
	}
	FormatType getType() const { return formatType_; }

  private:
	FormatType formatType_;
	int intValue_;
	float floatValue_;
};

// ---------------------------------------------------------------------------
// Helper: build little-endian byte array from integer
// ---------------------------------------------------------------------------
static void le16(uint8_t* buf, int16_t v) {
	std::memcpy(buf, &v, 2);
}
static void le32(uint8_t* buf, int32_t v) {
	std::memcpy(buf, &v, 4);
}
static void lef32(uint8_t* buf, float v) {
	std::memcpy(buf, &v, 4);
}

// ---------------------------------------------------------------------------
// setUp / tearDown (Unity requires these)
// ---------------------------------------------------------------------------
void setUp() {}
void tearDown() {}

// ---------------------------------------------------------------------------
// DATA8 tests
// ---------------------------------------------------------------------------
void test_data8_positive_value() {
	Dataset d;
	uint8_t payload[] = {42};
	d.readData(FormatType::DATA8, payload);
	TEST_ASSERT_EQUAL(FormatType::DATA8, d.getType());
	TEST_ASSERT_EQUAL_INT(42, d.getDataAsInt());
	TEST_ASSERT_EQUAL_FLOAT(42.0f, d.getDataAsFloat());
}

void test_data8_zero() {
	Dataset d;
	uint8_t payload[] = {0};
	d.readData(FormatType::DATA8, payload);
	TEST_ASSERT_EQUAL_INT(0, d.getDataAsInt());
}

void test_data8_negative_value() {
	Dataset d;
	uint8_t payload[] = {0xFF}; // -1 as signed int8_t
	d.readData(FormatType::DATA8, payload);
	TEST_ASSERT_EQUAL_INT(-1, d.getDataAsInt());
}

void test_data8_min_value() {
	Dataset d;
	uint8_t payload[] = {0x80}; // -128 as signed int8_t
	d.readData(FormatType::DATA8, payload);
	TEST_ASSERT_EQUAL_INT(-128, d.getDataAsInt());
}

void test_data8_max_value() {
	Dataset d;
	uint8_t payload[] = {0x7F}; // 127
	d.readData(FormatType::DATA8, payload);
	TEST_ASSERT_EQUAL_INT(127, d.getDataAsInt());
}

// ---------------------------------------------------------------------------
// DATA16 tests
// ---------------------------------------------------------------------------
void test_data16_positive_little_endian() {
	Dataset d;
	uint8_t payload[2];
	le16(payload, 1000);
	d.readData(FormatType::DATA16, payload);
	TEST_ASSERT_EQUAL(FormatType::DATA16, d.getType());
	TEST_ASSERT_EQUAL_INT(1000, d.getDataAsInt());
}

void test_data16_negative() {
	Dataset d;
	uint8_t payload[2];
	le16(payload, -500);
	d.readData(FormatType::DATA16, payload);
	TEST_ASSERT_EQUAL_INT(-500, d.getDataAsInt());
}

void test_data16_min_value() {
	Dataset d;
	uint8_t payload[2];
	le16(payload, -32768);
	d.readData(FormatType::DATA16, payload);
	TEST_ASSERT_EQUAL_INT(-32768, d.getDataAsInt());
}

void test_data16_max_value() {
	Dataset d;
	uint8_t payload[2];
	le16(payload, 32767);
	d.readData(FormatType::DATA16, payload);
	TEST_ASSERT_EQUAL_INT(32767, d.getDataAsInt());
}

// ---------------------------------------------------------------------------
// DATA32 tests
// ---------------------------------------------------------------------------
void test_data32_positive_little_endian() {
	Dataset d;
	uint8_t payload[4];
	le32(payload, 100000);
	d.readData(FormatType::DATA32, payload);
	TEST_ASSERT_EQUAL(FormatType::DATA32, d.getType());
	TEST_ASSERT_EQUAL_INT(100000, d.getDataAsInt());
}

void test_data32_negative() {
	Dataset d;
	uint8_t payload[4];
	le32(payload, -99999);
	d.readData(FormatType::DATA32, payload);
	TEST_ASSERT_EQUAL_INT(-99999, d.getDataAsInt());
}

void test_data32_zero() {
	Dataset d;
	uint8_t payload[4] = {0, 0, 0, 0};
	d.readData(FormatType::DATA32, payload);
	TEST_ASSERT_EQUAL_INT(0, d.getDataAsInt());
}

// ---------------------------------------------------------------------------
// DATAFLOAT tests
// ---------------------------------------------------------------------------
void test_datafloat_positive() {
	Dataset d;
	uint8_t payload[4];
	lef32(payload, 3.14f);
	d.readData(FormatType::DATAFLOAT, payload);
	TEST_ASSERT_EQUAL(FormatType::DATAFLOAT, d.getType());
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.14f, d.getDataAsFloat());
}

void test_datafloat_negative() {
	Dataset d;
	uint8_t payload[4];
	lef32(payload, -1.5f);
	d.readData(FormatType::DATAFLOAT, payload);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.5f, d.getDataAsFloat());
}

void test_datafloat_zero() {
	Dataset d;
	uint8_t payload[4];
	lef32(payload, 0.0f);
	d.readData(FormatType::DATAFLOAT, payload);
	TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, d.getDataAsFloat());
}

void test_datafloat_as_int_truncates() {
	Dataset d;
	uint8_t payload[4];
	lef32(payload, 7.9f);
	d.readData(FormatType::DATAFLOAT, payload);
	TEST_ASSERT_EQUAL_INT(7, d.getDataAsInt()); // truncation, not rounding
}

// ---------------------------------------------------------------------------
// Cross-type: getDataAsFloat from integer types
// ---------------------------------------------------------------------------
void test_data8_get_as_float() {
	Dataset d;
	uint8_t payload[] = {50};
	d.readData(FormatType::DATA8, payload);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.0f, d.getDataAsFloat());
}

void test_data16_get_as_float() {
	Dataset d;
	uint8_t payload[2];
	le16(payload, -200);
	d.readData(FormatType::DATA16, payload);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, -200.0f, d.getDataAsFloat());
}

// ---------------------------------------------------------------------------
// Initial state
// ---------------------------------------------------------------------------
void test_default_type_is_unknown() {
	Dataset d;
	TEST_ASSERT_EQUAL(FormatType::UNKNOWN, d.getType());
	TEST_ASSERT_EQUAL_INT(0, d.getDataAsInt());
	TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, d.getDataAsFloat());
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
	UNITY_BEGIN();

	RUN_TEST(test_default_type_is_unknown);

	RUN_TEST(test_data8_positive_value);
	RUN_TEST(test_data8_zero);
	RUN_TEST(test_data8_negative_value);
	RUN_TEST(test_data8_min_value);
	RUN_TEST(test_data8_max_value);

	RUN_TEST(test_data16_positive_little_endian);
	RUN_TEST(test_data16_negative);
	RUN_TEST(test_data16_min_value);
	RUN_TEST(test_data16_max_value);

	RUN_TEST(test_data32_positive_little_endian);
	RUN_TEST(test_data32_negative);
	RUN_TEST(test_data32_zero);

	RUN_TEST(test_datafloat_positive);
	RUN_TEST(test_datafloat_negative);
	RUN_TEST(test_datafloat_zero);
	RUN_TEST(test_datafloat_as_int_truncates);

	RUN_TEST(test_data8_get_as_float);
	RUN_TEST(test_data16_get_as_float);

	return UNITY_END();
}
