#include "parsedatastate.h"

#include "logging.h"
#include "format.h"
#include "legodevice.h"
#include "mode.h"
#include "waitingstate.h"

ParseDataState::ParseDataState(LegoDevice *legoDevice, int messageType, int messageMode, int messageSize)
	: legoDevice(legoDevice)
	, messageMode(messageMode)
	, messageSize(messageSize)
	, messageType(messageType) {
	messagePayload = new int[this->messageSize];
	received = 0;
}

ParseDataState::~ParseDataState() {
	delete[] messagePayload;
}

ProtocolState *ParseDataState::parse(int datapoint) {
	if (received < messageSize) {
		messagePayload[received++] = datapoint;
		return this;
	}

	int checksum = 0xff ^ messageType;
	for (int i = 0; i < messageSize; i++) {
		checksum ^= messagePayload[i];
	}

	if (checksum != datapoint) {
		WARN("Checksum failed in data! Got %d, expected %d", datapoint, checksum);
		return new WaitingState(legoDevice);
	}

	const char hexChars[] = "0123456789ABCDEF";
	std::string payloadHex;
	payloadHex.reserve(messageSize * 3); // 2 chars + space per byte

	for (int i = 0; i < messageSize; ++i) {
		int v = messagePayload[i] & 0xFF;
		payloadHex.push_back(hexChars[(v >> 4) & 0xF]);
		payloadHex.push_back(hexChars[v & 0xF]);
		if (i + 1 < messageSize) payloadHex.push_back(' ');
	}

	DEBUG("Got data %s", payloadHex.c_str());

	return new WaitingState(legoDevice);
}
