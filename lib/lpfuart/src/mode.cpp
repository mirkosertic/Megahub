#include "mode.h"

#include "logging.h"

Mode::Mode()
	: pctMin_(0.0f)
	, pctMax_(100.0f)
	, siMin_(0.0f)
	, siMax_(1023.0f)
	, format_(nullptr)
	, datasets_(nullptr) {
	name_ = "";
	units_ = "";
	for (int i = 0; i < 5; i++) {
		inputTypes_[i] = false;
		outputTypes_[i] = false;
	}
}

Mode::~Mode() {
	if (datasets_ != nullptr) {
		delete[] datasets_;
	}
	if (format_ != nullptr) {
		delete format_;
	}
}

void Mode::reset() {

	if (datasets_ != nullptr) {
		delete[] datasets_;
	}
	if (format_ != nullptr) {
		delete format_;
	}

	pctMin_ = 0.0f;
	pctMax_ = 100.0;
	siMin_ = 0.0f;
	siMax_ = 1023.0f;
	format_ = nullptr;
	datasets_ = nullptr;
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

	if (datasets_ != nullptr) {
		delete[] datasets_;
	}
	datasets_ = new Dataset[format->getDatasets()];
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

void Mode::processDataPacket(int *payload, int payloadSize) {
	const char hexChars[] = "0123456789ABCDEF";
	std::string payloadHex;
	payloadHex.reserve(payloadSize * 3); // 2 chars + space per byte

	for (int i = 0; i < payloadSize; ++i) {
		int v = payload[i] & 0xFF;
		payloadHex.push_back(hexChars[(v >> 4) & 0xF]);
		payloadHex.push_back(hexChars[v & 0xF]);
		if (i + 1 < payloadSize) {
			payloadHex.push_back(' ');
		}
	}

	int datasetSize = 0;
	switch (format_->getFormatType()) {
		case Format::FormatType::DATA8: {
			datasetSize = 1;
			break;
		}
		case Format::FormatType::DATA16: {
			datasetSize = 2;
			break;
		}
		case Format::FormatType::DATA32: {
			datasetSize = 4;
			break;
		}
		case Format::FormatType::DATAFLOAT: {
			datasetSize = 4; // ?
			break;
		}
		default:
			WARN("Unknown format type in mode : %d, ignoring data message", format_->getFormatType());
			return;
	}

	DEBUG("Got data %s, expecting %d datasets of type %d", payloadHex.c_str(), format_->getDatasets(), format_->getFormatType());

	int expectedSize = format_->getDatasets() * datasetSize;
	if (expectedSize != payloadSize) {
		WARN("Got data %s, expecting %d datasets of type %d, but wrong size. Expected %d, got %d", payloadHex.c_str(), format_->getDatasets(), format_->getFormatType(), expectedSize, datasetSize);
	}

	int *startPtr = payload;
	for (int i = 0; i < format_->getDatasets(); i++) {
		Dataset &ds = datasets_[i];
		ds.readData(format_->getFormatType(), startPtr);
		startPtr += datasetSize;
	}
}
