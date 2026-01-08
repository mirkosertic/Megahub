#ifndef DATASET_H
#define DATASET_H

#include "format.h"

class Dataset {
public:
	Dataset();
	void readData(Format::FormatType type, int *payload);

	int getDataAsInt();
	float getDataAsFloat();
	Format::FormatType getType();

private:
	Format::FormatType formatType_;
	int intValue_;
	float floatValue_;
};

#endif
