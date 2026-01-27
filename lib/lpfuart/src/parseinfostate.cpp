#include "parseinfostate.h"

#include "format.h"
#include "legodevice.h"
#include "logging.h"
#include "mode.h"
#include "waitingstate.h"

#include <string>

ParseInfoState::ParseInfoState(LegoDevice *legoDevice, int messageType, int messageMode, int messageSize)
	: legoDevice(legoDevice)
	, messageMode(messageMode)
	, messageSize(messageSize + 1)
	, messageType(messageType) {
	messagePayload = new int[this->messageSize];
	received = 0;
}

ParseInfoState::~ParseInfoState() {
	delete[] messagePayload;
}

void ParseInfoState::parseINFOName() {
	std::string name = "";
	for (int i = 1; i < messageSize; i++) {
		int ch = messagePayload[i];
		if (ch != 0) {
			name += (char) ch;
		} else {
			break;
		}
	}
	if (name.length() > 0 && isUpperCase(name.at(0))) {
		INFO("Mode %d name is '%s'", messageMode, name.c_str());

		legoDevice->getMode(messageMode)->setName(name);
	} else {
		WARN("Ignoring Name for mode %d as it seems to be invalid (length %d or no uppercase character at start)", messageMode, name.length());
	}
}

void ParseInfoState::parseINFOMapping() {
	int input = messagePayload[1];
	int output = messagePayload[2];
	Mode *mode = legoDevice->getMode(messageMode);

	if ((input & 128) > 0) {
		mode->registerInputType(Mode::InputOutputType::SUPPORTS_NULL);
	}
	if ((input & 64) > 0) {
		mode->registerInputType(Mode::InputOutputType::SUPPORTS_FUNCTIONAL_MAPPING_20);
	}
	if ((input & 16) > 0) {
		mode->registerInputType(Mode::InputOutputType::ABS);
	}
	if ((input & 8) > 0) {
		mode->registerInputType(Mode::InputOutputType::REL);
	}
	if ((input & 4) > 0) {
		mode->registerInputType(Mode::InputOutputType::DIS);
	}

	if ((output & 128) > 0) {
		mode->registerOutputType(Mode::InputOutputType::SUPPORTS_NULL);
	}
	if ((output & 64) > 0) {
		mode->registerOutputType(Mode::InputOutputType::SUPPORTS_FUNCTIONAL_MAPPING_20);
	}
	if ((output & 16) > 0) {
		mode->registerOutputType(Mode::InputOutputType::ABS);
	}
	if ((output & 8) > 0) {
		mode->registerOutputType(Mode::InputOutputType::REL);
	}
	if ((output & 4) > 0) {
		mode->registerOutputType(Mode::InputOutputType::DIS);
	}
}

void ParseInfoState::parseINFOPct() {
	// Convert bytes to float (little endian)
	union {
		byte bytes[4];
		float value;
	} minConverter, maxConverter;

	minConverter.bytes[0] = (byte) messagePayload[1];
	minConverter.bytes[1] = (byte) messagePayload[2];
	minConverter.bytes[2] = (byte) messagePayload[3];
	minConverter.bytes[3] = (byte) messagePayload[4];

	maxConverter.bytes[0] = (byte) messagePayload[5];
	maxConverter.bytes[1] = (byte) messagePayload[6];
	maxConverter.bytes[2] = (byte) messagePayload[7];
	maxConverter.bytes[3] = (byte) messagePayload[8];

	legoDevice->getMode(messageMode)->setPctMinMax(minConverter.value, maxConverter.value);
}

void ParseInfoState::parseINFOSI() {
	// Convert bytes to float (little endian)
	union {
		byte bytes[4];
		float value;
	} minConverter, maxConverter;

	minConverter.bytes[0] = (byte) messagePayload[1];
	minConverter.bytes[1] = (byte) messagePayload[2];
	minConverter.bytes[2] = (byte) messagePayload[3];
	minConverter.bytes[3] = (byte) messagePayload[4];

	maxConverter.bytes[0] = (byte) messagePayload[5];
	maxConverter.bytes[1] = (byte) messagePayload[6];
	maxConverter.bytes[2] = (byte) messagePayload[7];
	maxConverter.bytes[3] = (byte) messagePayload[8];

	legoDevice->getMode(messageMode)->setSiMinMax(minConverter.value, maxConverter.value);
}

void ParseInfoState::parseINFOUnits() {
	std::string units = "";
	for (int i = 1; i < messageSize; i++) {
		int ch = messagePayload[i];
		if (ch != 0) {
			units += (char) ch;
		}
	}
	legoDevice->getMode(messageMode)->setUnits(units);
}

void ParseInfoState::parseINFOFormat() {
	int datasets = messagePayload[1];
	int format = messagePayload[2];
	int figures = messagePayload[3];
	int decimals = messagePayload[4];

	INFO("Mode %d format: datasets=%d, format=%d, figures=%d, decimals=%d", messageMode, datasets, format, figures, decimals);

	legoDevice->getMode(messageMode)->setFormat(new Format(datasets, Format::forId(format), figures, decimals));
}

void ParseInfoState::parseINFO() {
	int messageType = messagePayload[0];
	if ((messageType & LUMP_INFO_MODE_PLUS_8) > 0) {
		messageMode += 8;
		messageType &= 31;
	}
	switch (messageType) {
		case LUMP_INFO_NAME:
			parseINFOName();
			break;
		case LUMP_INFO_SI:
			parseINFOSI();
			break;
		case LUMP_INFO_RAW:
			// Skip this for now
			break;
		case LUMP_INFO_PCT:
			parseINFOPct();
			break;
		case LUMP_INFO_UNITS:
			parseINFOUnits();
			break;
		case LUMP_INFO_MAPPING:
			parseINFOMapping();
			break;
		case LUMP_INFO_MODE_COMBOS:
			// Skip this for now
			break;
		case LUMP_INFO_FORMAT:
			parseINFOFormat();
			break;
		case LUMP_INFO_UNK7:
		case LUMP_INFO_UNK8:
		case LUMP_INFO_UNK9:
		case LUMP_INFO_UNK10:
		case LUMP_INFO_UNK11:
		case LUMP_INFO_UNK12:
			DEBUG("Ignoring info message type %d for mode %d, parsed message type is %d", messagePayload[0], messageMode, messageType);
			break;
		default:
			WARN("Unknown info message type %d for mode %d, parsed message type is %d", messagePayload[0], messageMode, messageType);
			break;
	}
}

ProtocolState *ParseInfoState::parse(int datapoint) {
	if (received < messageSize) {
		messagePayload[received++] = datapoint;
		return this;
	}

	int checksum = 0xff ^ messageType;
	for (int i = 0; i < messageSize; i++) {
		checksum ^= messagePayload[i];
	}

	if (checksum != datapoint) {
		WARN("Checksum failed in command! Got %d, expected %d", datapoint, checksum);
		return new WaitingState(legoDevice);
	}

	parseINFO();

	return new WaitingState(legoDevice);
}
