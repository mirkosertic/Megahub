#include "mode.h"

Mode::Mode()
	: pctMin_(0.0f)
	, pctMax_(100.0f)
	, siMin_(0.0f)
	, siMax_(1023.0f)
	, format_(nullptr) {
	name_ = "";
	units_ = "";
	for (int i = 0; i < 5; i++) {
		inputTypes_[i] = false;
		outputTypes_[i] = false;
	}
}

Mode::~Mode() {
	if (format_ != nullptr) {
		delete format_;
	}
}

void Mode::reset() {
	pctMin_ = 0.0f;
	pctMax_ = 100.0;
	siMin_ = 0.0f;
	siMax_ = 1023.0f;
	format_ = nullptr;
	name_ = "";
	units_ = "";
	for (int i = 0; i < 5; i++) {
		inputTypes_[i] = false;
		outputTypes_[i] = false;
	}
}

void Mode::registerInputType(InputOutputType inputType) {
	inputTypes_[static_cast<int>(inputType)] = true;
}

void Mode::registerOutputType(InputOutputType outputType) {
	outputTypes_[static_cast<int>(outputType)] = true;
}

void Mode::setName(const std::string &name) {
	this->name_ = name;
}

void Mode::setUnits(const std::string &units) {
	this->units_ = units;
}

void Mode::setPctMinMax(float min, float max) {
	pctMin_ = min;
	pctMax_ = max;
}

void Mode::setSiMinMax(float min, float max) {
	siMin_ = min;
	siMax_ = max;
}

void Mode::setFormat(Format *format) {
	if (this->format_ != nullptr) {
		delete this->format_;
	}
	this->format_ = format;
}

std::string Mode::getName() {
	return name_;
}

std::string Mode::getUnits() {
	return units_;
}

float Mode::getPctMin() {
	return pctMin_;
}

float Mode::getPctMax() {
	return pctMax_;
}

float Mode::getSiMin() {
	return siMin_;
}

float Mode::getSiMax() {
	return siMax_;
}
