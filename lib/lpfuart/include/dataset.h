#ifndef DATASET_H
#define DATASET_H

#include <cstdint>

#include "format.h"

class Dataset {
public:
	Dataset();
	void readData(Format::FormatType type, const uint8_t *payload);

	int getDataAsInt();
	float getDataAsFloat();
	Format::FormatType getType();

private:
	Format::FormatType formatType_;
	int intValue_;
	float floatValue_;
};

#endif
