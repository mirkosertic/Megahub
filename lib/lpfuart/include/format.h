#ifndef FORMAT_H
#define FORMAT_H

#include <Arduino.h>

class Format {
public:
	enum class FormatType {
		DATA8 = 0x00,
		DATA16 = 0x01,
		DATA32 = 0x02,
		DATAFLOAT = 0x03,
		UNKNOWN = 0xff
	};

	static FormatType forId(int id);

	Format(int datasets, FormatType formatType, int figures, int decimals);
	int getDatasets() const { return datasets; }
	FormatType getFormatType() const { return formatType; }
	int getFigures() const { return figures; }
	int getDecimals() const { return decimals; }

private:
	int datasets;
	FormatType formatType;
	int figures;
	int decimals;
};

#endif // FORMAT_H
