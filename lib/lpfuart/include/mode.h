#ifndef MODE_H
#define MODE_H

#include <Arduino.h>

#include "format.h"

#include <string>

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
	void setFormat(Format *format);

	std::string getName();
	std::string getUnits();
	float getPctMin();
	float getPctMax();
	float getSiMin();
	float getSiMax();

private:
	std::string name;
	std::string units;
	float pctMin;
	float pctMax;
	float siMin;
	float siMax;
	Format *format;
	bool inputTypes[5];
	bool outputTypes[5];
};

#endif // MODE_H
