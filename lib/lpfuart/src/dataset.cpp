#include "dataset.h"

#include "logging.h"

#include <cstdint>
#include <cstring>

Dataset::Dataset()
	: formatType_(Format::FormatType::UNKNOWN)
	, intValue_(0)
	, floatValue_(0.0f) {
}

void Dataset::readData(Format::FormatType type, int *payload) {
	formatType_ = type;
	switch (type) {
		case Format::FormatType::DATA8: {
			// 8-bit signed integer little endian
			intValue_ = payload[0];
			break;
		}
		case Format::FormatType::DATA16: {
			// 16-bit signed integer little endian
			int16_t value16;
			std::memcpy(&value16, payload, 2);
			intValue_ = value16;
			break;
		}
		case Format::FormatType::DATA32: {
			// 32-bit signed integer little endian
			int32_t value32;
			std::memcpy(&value32, payload, 4);
			intValue_ = value32;
			break;
		}
		case Format::FormatType::DATAFLOAT: {
			// 32-bot little endian IEEE 754 floating point
			float valueFloat;
			std::memcpy(&valueFloat, payload, 4);
			floatValue_ = valueFloat;
			break;
		}
		default:
			WARN("Unsupported data format");
			break;
	}
}

int Dataset::getDataAsInt() {
	switch (formatType_) {
		case Format::FormatType::DATA8: {
			return intValue_;
		}
		case Format::FormatType::DATA16: {
			return intValue_;
		}
		case Format::FormatType::DATA32: {
			return intValue_;
		}
		case Format::FormatType::DATAFLOAT: {
			return (int) floatValue_;
		}
		default: {
			WARN("Unsupported data format");
			return 0;
		}
	}
}

float Dataset::getDataAsFloat() {
	switch (formatType_) {
		case Format::FormatType::DATA8: {
			return (float) intValue_;
		}
		case Format::FormatType::DATA16: {
			return (float) intValue_;
		}
		case Format::FormatType::DATA32: {
			return (float) intValue_;
		}
		case Format::FormatType::DATAFLOAT: {
			return floatValue_;
		}
		default: {
			WARN("Unsupported data format");
			return 0.0f;
		}
	}
}

Format::FormatType Dataset::getType() {
	return formatType_;
}
