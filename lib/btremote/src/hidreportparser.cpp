#include "hidreportparser.h"
#include "logging.h"

void HIDReportParser::parseAndPrintJSON(const uint8_t *descriptor, size_t length) {
	if (descriptor == nullptr || length == 0) {
		ERROR("Invalid HID descriptor");
		return;
	}

	JsonDocument doc = parseDescriptor(descriptor, length);

	// Serialize to Serial for debugging
	Serial.println("\n==========================================================");
	Serial.println("            HID Report Descriptor Analysis");
	Serial.println("==========================================================");
	serializeJsonPretty(doc, Serial);
	Serial.println("\n==========================================================");
	Serial.println("  Use 'reportLayout' to see the physical byte/bit mapping");
	Serial.println("==========================================================");
}

JsonDocument HIDReportParser::parseDescriptor(const uint8_t *descriptor, size_t length) {
	JsonDocument doc;
	JsonArray items = doc["items"].to<JsonArray>();

	size_t pos = 0;
	int collectionDepth = 0;

	// Global state tracking (these persist across items)
	uint16_t currentUsagePage = 0;
	uint32_t reportSize = 0;
	uint32_t reportCount = 0;
	int32_t logicalMin = 0;
	int32_t logicalMax = 0;
	uint8_t currentReportID = 0;

	// Local state tracking (these reset after each Main item)
	std::vector<uint16_t> usageStack;
	uint16_t usageMin = 0;
	uint16_t usageMax = 0;
	bool hasUsageMinMax = false;

	// Bit offset tracking for report layout
	uint32_t inputBitOffset = 0;
	uint32_t outputBitOffset = 0;
	uint32_t featureBitOffset = 0;

	// Track report layouts
	JsonArray reportLayout = doc["reportLayout"].to<JsonArray>();

	while (pos < length) {
		uint8_t prefix = descriptor[pos++];

		// Extract fields from prefix byte
		uint8_t bSize = prefix & 0x03;
		uint8_t bType = (prefix >> 2) & 0x03;
		uint8_t bTag = (prefix >> 4) & 0x0F;

		// Handle long items (bSize = 3)
		if (bSize == 3) {
			if (pos >= length) break;
			uint8_t dataSize = descriptor[pos++];
			JsonObject item = items.add<JsonObject>();
			item["type"] = "LongItem";
			item["tag"] = bTag;
			item["size"] = dataSize;
			pos += dataSize;
			if (pos > length) break;
			continue;
		}

		// Determine actual data size
		uint8_t dataSize = (bSize == 3) ? 4 : bSize;

		// Extract data bytes
		uint32_t data = 0;
		for (int i = 0; i < dataSize && pos < length; i++) {
			data |= ((uint32_t)descriptor[pos++]) << (i * 8);
		}

		// Create JSON object for this item
		JsonObject item = items.add<JsonObject>();
		item["offset"] = pos - 1 - dataSize;

		// Parse based on type
		switch (bType) {
			case ITEM_TYPE_MAIN:
				item["type"] = "Main";
				switch (bTag) {
					case MAIN_INPUT:
					case MAIN_OUTPUT:
					case MAIN_FEATURE: {
						const char* tagName = (bTag == MAIN_INPUT) ? "Input" :
						                      (bTag == MAIN_OUTPUT) ? "Output" : "Feature";
						item["tag"] = tagName;
						decodeIOFlags(item, data);

						// Calculate bit offset based on type
						uint32_t* bitOffset = (bTag == MAIN_INPUT) ? &inputBitOffset :
						                      (bTag == MAIN_OUTPUT) ? &outputBitOffset : &featureBitOffset;

						// Add layout information
						JsonObject layout = reportLayout.add<JsonObject>();
						layout["type"] = tagName;
						layout["reportID"] = currentReportID;
						layout["bitOffset"] = *bitOffset;
						layout["bitSize"] = reportSize;
						layout["count"] = reportCount;
						layout["totalBits"] = reportSize * reportCount;
						layout["byteOffset"] = *bitOffset / 8;
						layout["byteSize"] = (reportSize * reportCount + 7) / 8;
						layout["bitInByte"] = *bitOffset % 8;

						// Add usage information
						if (hasUsageMinMax && usageMin <= usageMax) {
							layout["usagePage"] = currentUsagePage;
							layout["usagePageName"] = getUsagePageName(currentUsagePage);
							layout["usageMin"] = usageMin;
							layout["usageMax"] = usageMax;
						} else if (!usageStack.empty()) {
							layout["usagePage"] = currentUsagePage;
							layout["usagePageName"] = getUsagePageName(currentUsagePage);
							JsonArray usages = layout["usages"].to<JsonArray>();
							for (size_t i = 0; i < usageStack.size() && i < reportCount; i++) {
								usages.add(usageStack[i]);
							}
						}

						// Add logical range
						layout["logicalMin"] = logicalMin;
						layout["logicalMax"] = logicalMax;

						// Generate human-readable description
						String description = "";
						if (currentUsagePage == 0x09) {
							description = "Buttons";
						} else if (currentUsagePage == 0x01) {
							if (!usageStack.empty()) {
								uint16_t usage = usageStack[0];
								if (usage == 0x30) description = "X Axis";
								else if (usage == 0x31) description = "Y Axis";
								else if (usage == 0x32) description = "Z Axis";
								else if (usage == 0x33) description = "Rx Axis";
								else if (usage == 0x34) description = "Ry Axis";
								else if (usage == 0x35) description = "Rz Axis";
								else if (usage == 0x39) description = "Hat Switch";
							}
						}
						if (description.length() > 0) {
							layout["description"] = description;
						}

						// Update bit offset for next item
						*bitOffset += reportSize * reportCount;

						// Clear local state
						usageStack.clear();
						usageMin = 0;
						usageMax = 0;
						hasUsageMinMax = false;
						break;
					}
					case MAIN_COLLECTION:
						item["tag"] = "Collection";
						item["collectionType"] = getCollectionTypeName(data & 0xFF);
						item["depth"] = collectionDepth++;
						break;
					case MAIN_END_COLLECTION:
						item["tag"] = "EndCollection";
						item["depth"] = --collectionDepth;
						break;
					default:
						item["tag"] = String("Unknown_") + String(bTag, HEX);
						item["data"] = data;
						break;
				}
				break;

			case ITEM_TYPE_GLOBAL:
				item["type"] = "Global";
				switch (bTag) {
					case GLOBAL_USAGE_PAGE:
						item["tag"] = "UsagePage";
						item["value"] = data;
						item["name"] = getUsagePageName(data);
						currentUsagePage = data;
						break;
					case GLOBAL_LOGICAL_MIN:
						item["tag"] = "LogicalMinimum";
						item["value"] = extractValue((const uint8_t*)&data, dataSize, true);
						logicalMin = extractValue((const uint8_t*)&data, dataSize, true);
						break;
					case GLOBAL_LOGICAL_MAX:
						item["tag"] = "LogicalMaximum";
						item["value"] = extractValue((const uint8_t*)&data, dataSize, true);
						logicalMax = extractValue((const uint8_t*)&data, dataSize, true);
						break;
					case GLOBAL_PHYSICAL_MIN:
						item["tag"] = "PhysicalMinimum";
						item["value"] = extractValue((const uint8_t*)&data, dataSize, true);
						break;
					case GLOBAL_PHYSICAL_MAX:
						item["tag"] = "PhysicalMaximum";
						item["value"] = extractValue((const uint8_t*)&data, dataSize, true);
						break;
					case GLOBAL_UNIT_EXPONENT:
						item["tag"] = "UnitExponent";
						item["value"] = extractValue((const uint8_t*)&data, dataSize, true);
						break;
					case GLOBAL_UNIT:
						item["tag"] = "Unit";
						item["value"] = data;
						break;
					case GLOBAL_REPORT_SIZE:
						item["tag"] = "ReportSize";
						item["value"] = data;
						reportSize = data;
						break;
					case GLOBAL_REPORT_ID:
						item["tag"] = "ReportID";
						item["value"] = data;
						currentReportID = data;
						break;
					case GLOBAL_REPORT_COUNT:
						item["tag"] = "ReportCount";
						item["value"] = data;
						reportCount = data;
						break;
					case GLOBAL_PUSH:
						item["tag"] = "Push";
						break;
					case GLOBAL_POP:
						item["tag"] = "Pop";
						break;
					default:
						item["tag"] = String("Unknown_") + String(bTag, HEX);
						item["data"] = data;
						break;
				}
				break;

			case ITEM_TYPE_LOCAL:
				item["type"] = "Local";
				switch (bTag) {
					case LOCAL_USAGE:
						item["tag"] = "Usage";
						item["value"] = data;
						usageStack.push_back(data);
						break;
					case LOCAL_USAGE_MIN:
						item["tag"] = "UsageMinimum";
						item["value"] = data;
						usageMin = data;
						hasUsageMinMax = true;
						break;
					case LOCAL_USAGE_MAX:
						item["tag"] = "UsageMaximum";
						item["value"] = data;
						usageMax = data;
						hasUsageMinMax = true;
						break;
					case LOCAL_DESIGNATOR_INDEX:
						item["tag"] = "DesignatorIndex";
						item["value"] = data;
						break;
					case LOCAL_DESIGNATOR_MIN:
						item["tag"] = "DesignatorMinimum";
						item["value"] = data;
						break;
					case LOCAL_DESIGNATOR_MAX:
						item["tag"] = "DesignatorMaximum";
						item["value"] = data;
						break;
					case LOCAL_STRING_INDEX:
						item["tag"] = "StringIndex";
						item["value"] = data;
						break;
					case LOCAL_STRING_MIN:
						item["tag"] = "StringMinimum";
						item["value"] = data;
						break;
					case LOCAL_STRING_MAX:
						item["tag"] = "StringMaximum";
						item["value"] = data;
						break;
					case LOCAL_DELIMITER:
						item["tag"] = "Delimiter";
						item["value"] = data;
						break;
					default:
						item["tag"] = String("Unknown_") + String(bTag, HEX);
						item["data"] = data;
						break;
				}
				break;

			case ITEM_TYPE_RESERVED:
				item["type"] = "Reserved";
				item["tag"] = bTag;
				item["data"] = data;
				break;
		}
	}

	doc["totalBytes"] = length;
	doc["parsed"] = pos;

	// Add summary information
	JsonObject summary = doc["summary"].to<JsonObject>();
	summary["inputReportBits"] = inputBitOffset;
	summary["inputReportBytes"] = (inputBitOffset + 7) / 8;
	summary["outputReportBits"] = outputBitOffset;
	summary["outputReportBytes"] = (outputBitOffset + 7) / 8;
	summary["featureReportBits"] = featureBitOffset;
	summary["featureReportBytes"] = (featureBitOffset + 7) / 8;

	return doc;
}

const char* HIDReportParser::getUsagePageName(uint16_t usagePage) {
	switch (usagePage) {
		case 0x01: return "Generic Desktop";
		case 0x02: return "Simulation Controls";
		case 0x03: return "VR Controls";
		case 0x04: return "Sport Controls";
		case 0x05: return "Game Controls";
		case 0x06: return "Generic Device Controls";
		case 0x07: return "Keyboard/Keypad";
		case 0x08: return "LEDs";
		case 0x09: return "Button";
		case 0x0A: return "Ordinal";
		case 0x0B: return "Telephony";
		case 0x0C: return "Consumer";
		case 0x0D: return "Digitizer";
		case 0x0F: return "PID Page";
		case 0x10: return "Unicode";
		case 0x14: return "Alphanumeric Display";
		case 0x40: return "Medical Instruments";
		default: return "Unknown";
	}
}

const char* HIDReportParser::getUsageName(uint16_t usagePage, uint16_t usage) {
	// Generic Desktop usages (0x01)
	if (usagePage == 0x01) {
		switch (usage) {
			case 0x01: return "Pointer";
			case 0x02: return "Mouse";
			case 0x04: return "Joystick";
			case 0x05: return "Game Pad";
			case 0x06: return "Keyboard";
			case 0x07: return "Keypad";
			case 0x08: return "Multi-axis Controller";
			case 0x30: return "X";
			case 0x31: return "Y";
			case 0x32: return "Z";
			case 0x33: return "Rx";
			case 0x34: return "Ry";
			case 0x35: return "Rz";
			case 0x36: return "Slider";
			case 0x37: return "Dial";
			case 0x38: return "Wheel";
			case 0x39: return "Hat switch";
			case 0x3A: return "Counted Buffer";
			case 0x3B: return "Byte Count";
			case 0x3C: return "Motion Wakeup";
			case 0x3D: return "Start";
			case 0x3E: return "Select";
			case 0x85: return "System Main Menu";
			case 0x90: return "D-pad Up";
			case 0x91: return "D-pad Down";
			case 0x92: return "D-pad Right";
			case 0x93: return "D-pad Left";
			default: return nullptr;
		}
	}
	// Button usages (0x09)
	else if (usagePage == 0x09) {
		return "Button";
	}

	return nullptr;
}

const char* HIDReportParser::getCollectionTypeName(uint8_t type) {
	switch (type) {
		case COLLECTION_PHYSICAL: return "Physical";
		case COLLECTION_APPLICATION: return "Application";
		case COLLECTION_LOGICAL: return "Logical";
		case COLLECTION_REPORT: return "Report";
		case COLLECTION_NAMED_ARRAY: return "Named Array";
		case COLLECTION_USAGE_SWITCH: return "Usage Switch";
		case COLLECTION_USAGE_MODIFIER: return "Usage Modifier";
		default: return "Unknown";
	}
}

void HIDReportParser::decodeIOFlags(JsonObject &obj, uint32_t flags) {
	obj["flags"] = flags;

	JsonObject decoded = obj["decoded"].to<JsonObject>();

	// Bit 0: Data (0) / Constant (1)
	decoded["dataType"] = (flags & 0x01) ? "Constant" : "Data";

	// Bit 1: Array (0) / Variable (1)
	decoded["arrayType"] = (flags & 0x02) ? "Variable" : "Array";

	// Bit 2: Absolute (0) / Relative (1)
	decoded["coordType"] = (flags & 0x04) ? "Relative" : "Absolute";

	// Bit 3: No Wrap (0) / Wrap (1)
	decoded["wrap"] = (flags & 0x08) ? "Wrap" : "NoWrap";

	// Bit 4: Linear (0) / Non Linear (1)
	decoded["linearity"] = (flags & 0x10) ? "NonLinear" : "Linear";

	// Bit 5: Preferred State (0) / No Preferred (1)
	decoded["preferredState"] = (flags & 0x20) ? "NoPreferred" : "PreferredState";

	// Bit 6: No Null position (0) / Null state(1)
	decoded["nullState"] = (flags & 0x40) ? "NullState" : "NoNullPosition";

	// Bit 8: Bit Field (0) / Buffered Bytes (1)
	decoded["bufferType"] = (flags & 0x100) ? "BufferedBytes" : "BitField";
}

int32_t HIDReportParser::extractValue(const uint8_t *data, uint8_t size, bool isSigned) {
	int32_t value = 0;

	// Extract bytes
	for (int i = 0; i < size; i++) {
		value |= ((int32_t)data[i]) << (i * 8);
	}

	// Sign extend if needed
	if (isSigned && size > 0) {
		int signBit = 1 << (size * 8 - 1);
		if (value & signBit) {
			// Extend sign bits
			int32_t mask = ~((1 << (size * 8)) - 1);
			value |= mask;
		}
	}

	return value;
}
