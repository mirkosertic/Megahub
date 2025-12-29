#include "format.h"

Format::Format(int datasets, FormatType formatType, int figures, int decimals)
	: datasets(datasets)
	, formatType(formatType)
	, figures(figures)
	, decimals(decimals) {
}

Format::FormatType Format::forId(int id) {
	switch (id) {
		case 0x00:
			return FormatType::DATA8;
		case 0x01:
			return FormatType::DATA16;
		case 0x02:
			return FormatType::DATA32;
		case 0x03:
			return FormatType::DATAFLOAT;
		default:
			Serial.print("Not supported format: ");
			Serial.println(id);
			return FormatType::DATA8; // Default fallback
	}
}
