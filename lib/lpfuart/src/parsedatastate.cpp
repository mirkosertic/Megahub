#include "parsedatastate.h"

#include "format.h"
#include "legodevice.h"
#include "logging.h"
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

	Mode *selectedMode = legoDevice->selectedMode();
	if (selectedMode != nullptr) {
		selectedMode->processDataPacket(messagePayload, messageSize);
	} else {
		WARN("Got datapacket, but no mode is selected for device!");
	}

	return new WaitingState(legoDevice);
}
