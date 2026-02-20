#include "parsedatastate.h"

#include "format.h"
#include "legodevice.h"
#include "logging.h"
#include "waitingstate.h"

ParseDataState::ParseDataState(LegoDevice *legoDevice, int messageType, int messageMode, int messageSize)
	: legoDevice(legoDevice)
	, messageMode(messageMode)
	, messageSize(messageSize)
	, messageType(messageType) {
	messagePayload = new uint8_t[this->messageSize];
	received = 0;
}

ParseDataState::~ParseDataState() {
	delete[] messagePayload;
}

void ParseDataState::processDataPacketDefault(int modeIndex, Mode *mode) {
	DEBUG("Processing data packet for mode %d with size %d", legoDevice->getSelectedModeIndex(), messageSize);
	mode->processDataPacket(messagePayload, messageSize);
}

void ParseDataState::processDataPacket_BoostColorAndDistanceSensor(int modeIndex, Mode *mode) {
	DEBUG("Processing data packet for mode %d with size %d", legoDevice->getSelectedModeIndex(), messageSize);
	// TODO: Implement special modes >8 handling here (Mode-Combos)
	processDataPacketDefault(modeIndex, mode);
}

void ParseDataState::processDataPacket(int modeIndex, Mode *mode) {
	switch (legoDevice->getDeviceId()) {
		case DEVICEID_BOOST_COLOR_DISTANCE_SENSOR: {
			processDataPacket_BoostColorAndDistanceSensor(modeIndex, mode);
			break;
		}
		default: {
			processDataPacketDefault(modeIndex, mode);
			break;
		}
	}
}

ProtocolState *ParseDataState::parse(int datapoint) {
	if (received < messageSize) {
		messagePayload[received++] = static_cast<uint8_t>(datapoint);
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

	if ((messageType & LUMP_INFO_MODE_PLUS_8) > 0) {
		messageMode += 8;
		messageType &= 31;
	}
	
	int modeIndex = legoDevice->getSelectedModeIndex();
	Mode *selectedMode = legoDevice->getMode(modeIndex);
	if (selectedMode != nullptr) {
		DEBUG("Processing data packet for mode %d with size %d, message mode is %d", modeIndex, messageSize, messageMode);
		processDataPacket(modeIndex, selectedMode);

		static unsigned long lastDataLog = 0;
		unsigned long currentMillis = millis();

		// Log sample every 10 seconds
		if (currentMillis - lastDataLog >= 10000 && messageSize > 0) {
			const char hexChars[] = "0123456789ABCDEF";
			std::string payloadHex;
			payloadHex.reserve(messageSize * 3); // 2 chars + space per byte

			for (int i = 0; i < messageSize; ++i) {
				int v = messagePayload[i] & 0xFF;
				payloadHex.push_back(hexChars[(v >> 4) & 0xF]);
				payloadHex.push_back(hexChars[v & 0xF]);
				if (i + 1 < messageSize) {
					payloadHex.push_back(' ');
				}
			}

			// Only printed to Serial to not overload the logging framework
			INFO("Mode %d / %d , Got data %s, message size is %d, messagetype is %d", modeIndex, messageMode, payloadHex.c_str(), messageSize, messageType);

			lastDataLog = currentMillis;
		}
	} else {
		WARN("Got datapacket, but no mode for index %d", modeIndex);
	}

	return new WaitingState(legoDevice);
}
