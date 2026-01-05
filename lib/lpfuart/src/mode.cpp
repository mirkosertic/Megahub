#include "mode.h"

Mode::Mode()
	: pctMin(0.0f)
	, pctMax(100.0f)
	, siMin(0.0f)
	, siMax(1023.0f)
	, format(nullptr) {
	name = "";
	units = "";
	for (int i = 0; i < 5; i++) {
		inputTypes[i] = false;
		outputTypes[i] = false;
	}
}

Mode::~Mode() {
	if (format != nullptr) {
		delete format;
	}
}

void Mode::registerInputType(InputOutputType inputType) {
	inputTypes[static_cast<int>(inputType)] = true;
}

void Mode::registerOutputType(InputOutputType outputType) {
	outputTypes[static_cast<int>(outputType)] = true;
}

void Mode::setName(const std::string &name) {
	this->name = name;
}

void Mode::setUnits(const std::string &units) {
	this->units = units;
}

void Mode::setPctMinMax(float min, float max) {
	pctMin = min;
	pctMax = max;
}

void Mode::setSiMinMax(float min, float max) {
	siMin = min;
	siMax = max;
}

void Mode::setFormat(Format *format) {
	if (this->format != nullptr) {
		delete this->format;
	}
	this->format = format;
}

std::string Mode::getName() {
	return name;
}

std::string Mode::getUnits() {
	return units;
}

float Mode::getPctMin() {
	return pctMin;
}

float Mode::getPctMax() {
	return pctMax;
}

float Mode::getSiMin() {
	return siMin;
}

float Mode::getSiMax() {
	return siMax;
}
