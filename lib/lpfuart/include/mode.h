#ifndef MODE_H
#define MODE_H

#include <Arduino.h>

#include "dataset.h"
#include "format.h"

#include <memory>
#include <string>
#include <vector>

class Mode {
public:
	enum class InputOutputType {
		SUPPORTS_FUNCTIONAL_MAPPING_20,
		ABS,
		REL,
		DIS,
		SUPPORTS_NULL
	};

	Mode();
	~Mode();

	void registerInputType(InputOutputType inputType);
	void registerOutputType(InputOutputType outputType);
	void setName(const std::string &name);
	void setUnits(const std::string &units);
	void setPctMinMax(float min, float max);
	void setSiMinMax(float min, float max);
	void setFormat(std::unique_ptr<Format> format);

	std::string getName();
	std::string getUnits();
	float getPctMin();
	float getPctMax();
	float getSiMin();
	float getSiMax();

	void reset();

	void processDataPacket(const uint8_t *payload, int payloadSize);
	Dataset *getDataset(int index);
	Format* getFormat();

private:
	std::string name_;
	std::string units_;
	float pctMin_;
	float pctMax_;
	float siMin_;
	float siMax_;
	std::unique_ptr<Format> format_;
	bool inputTypes_[5];
	bool outputTypes_[5];
	std::vector<Dataset> datasets_;
};

#endif // MODE_H
