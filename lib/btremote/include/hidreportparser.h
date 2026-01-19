#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

/**
 * @brief Parser for HID Report Descriptors
 *
 * Parses binary HID report descriptors according to the USB HID specification
 * and converts them to human-readable JSON format for debugging purposes.
 */
class HIDReportParser {
public:
	/**
	 * @brief Parse a HID report descriptor and output as JSON to Serial
	 *
	 * @param descriptor Pointer to the descriptor data
	 * @param length Length of the descriptor in bytes
	 */
	static void parseAndPrintJSON(const uint8_t *descriptor, size_t length);

private:
	// HID Item Type (bits 2-3 of prefix byte)
	enum ItemType {
		ITEM_TYPE_MAIN = 0,
		ITEM_TYPE_GLOBAL = 1,
		ITEM_TYPE_LOCAL = 2,
		ITEM_TYPE_RESERVED = 3
	};

	// Main Item Tags
	enum MainItemTag {
		MAIN_INPUT = 0x8,
		MAIN_OUTPUT = 0x9,
		MAIN_COLLECTION = 0xA,
		MAIN_FEATURE = 0xB,
		MAIN_END_COLLECTION = 0xC
	};

	// Global Item Tags
	enum GlobalItemTag {
		GLOBAL_USAGE_PAGE = 0x0,
		GLOBAL_LOGICAL_MIN = 0x1,
		GLOBAL_LOGICAL_MAX = 0x2,
		GLOBAL_PHYSICAL_MIN = 0x3,
		GLOBAL_PHYSICAL_MAX = 0x4,
		GLOBAL_UNIT_EXPONENT = 0x5,
		GLOBAL_UNIT = 0x6,
		GLOBAL_REPORT_SIZE = 0x7,
		GLOBAL_REPORT_ID = 0x8,
		GLOBAL_REPORT_COUNT = 0x9,
		GLOBAL_PUSH = 0xA,
		GLOBAL_POP = 0xB
	};

	// Local Item Tags
	enum LocalItemTag {
		LOCAL_USAGE = 0x0,
		LOCAL_USAGE_MIN = 0x1,
		LOCAL_USAGE_MAX = 0x2,
		LOCAL_DESIGNATOR_INDEX = 0x3,
		LOCAL_DESIGNATOR_MIN = 0x4,
		LOCAL_DESIGNATOR_MAX = 0x5,
		LOCAL_STRING_INDEX = 0x7,
		LOCAL_STRING_MIN = 0x8,
		LOCAL_STRING_MAX = 0x9,
		LOCAL_DELIMITER = 0xA
	};

	// Collection types
	enum CollectionType {
		COLLECTION_PHYSICAL = 0x00,
		COLLECTION_APPLICATION = 0x01,
		COLLECTION_LOGICAL = 0x02,
		COLLECTION_REPORT = 0x03,
		COLLECTION_NAMED_ARRAY = 0x04,
		COLLECTION_USAGE_SWITCH = 0x05,
		COLLECTION_USAGE_MODIFIER = 0x06
	};

	/**
	 * @brief Parse the descriptor and build a JSON document
	 */
	static JsonDocument parseDescriptor(const uint8_t *descriptor, size_t length);

	/**
	 * @brief Get the name of a usage page
	 */
	static const char* getUsagePageName(uint16_t usagePage);

	/**
	 * @brief Get the name of a usage for a given usage page
	 */
	static const char* getUsageName(uint16_t usagePage, uint16_t usage);

	/**
	 * @brief Get the name of a collection type
	 */
	static const char* getCollectionTypeName(uint8_t type);

	/**
	 * @brief Decode input/output/feature flags
	 */
	static void decodeIOFlags(JsonObject &obj, uint32_t flags);

	/**
	 * @brief Extract signed or unsigned value from item data
	 */
	static int32_t extractValue(const uint8_t *data, uint8_t size, bool isSigned = false);
};
