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

	int modeIndex = legoDevice->getSelectedModeIndex();

	static unsigned long lastDataLog = 0;
	unsigned long currentMillis = millis();

	// Log sample every 10 seconds
	if (currentMillis - lastDataLog >= 0 && messageSize > 0) {
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

		// Zu testem

		/*

		Mode 2 / 2 , Got data FA FF FF FF, message size is 4, messagetype is 210
		Mode 2 / 2 , Got data F9 FF FF FF, message size is 4, messagetype is 210
		Mode 2 / 2 , Got data F8 FF FF FF, message size is 4, messagetype is 210
		Mode 2 / 2 , Got data F6 FF FF FF, message size is 4, messagetype is 210
		Mode 2 / 2 , Got data F4 FF FF FF, message size is 4, messagetype is 210
		Mode 2 / 2 , Got data F2 FF FF FF, message size is 4, messagetype is 210
		Mode 2 / 2 , Got data F0 FF FF FF, message size is 4, messagetype is 210
		Mode 2 / 2 , Got data EE FF FF FF, message size is 4, messagetype is 210
		Mode 2 / 2 , Got data EC FF FF FF, message size is 4, messagetype is 210
		Mode 2 / 2 , Got data EA FF FF FF, message size is 4, messagetype is 210

		// Alles Messagetype 210 0xD2
		0xD2 0xE6 0xFF 0xFF 0xFF 0x34 // Prüfsumme 52
		0xD2 0xE4 0xFF 0xFF 0xFF 0x36 // Prüfsumme 54
		0xD2 0xE2 0xFF 0xFF 0xFF 0x30 // Prüfsumme 45

		Das ist schräg:

		Mode 2 / 2 , Got data E6 FF FF FF, message size is 4, messagetype is 210
[WARN] 48456 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 210, expected 52, message size 4
Mode 2 / 4 , Got data FF FF FF D2 E2 FF FF FF 30 D2 E0 FF FF D2 DE FF, message size is 16, messagetype is 228
[WARN] 48476 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 210, expected 218, message size 16
Mode 2 / 4 , Got data FF FF FF D2 D4 FF FF FF, message size is 8, messagetype is 220
[WARN] 48503 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 6, expected 37, message size 8
Mode 2 / 2 , Got data D2 FF FF FF, message size is 4, messagetype is 210
Mode 2 / 2 , Got data D0 FF FF FF, message size is 4, messagetype is 210
Mode 2 / 2 , Got data C8 D2 C6 FF, message size is 4, messagetype is 210
[WARN] 48535 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 255, expected 14, message size 4
[WARN] 48546 1 lib/lpfuart/src/waitingstate.cpp#74:parse() - Not implemented yet! type 0, data point 20, size 4 
Mode 2 / 2 , Got data C4 FF FF FF, message size is 4, messagetype is 210
[WARN] 48560 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 210, expected 22, message size 4
Mode 2 / 2 , Got data FF, message size is 1, messagetype is 194
[WARN] 48572 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 255, expected 194, message size 1
Mode 2 / 2 , Got data BA FF D2 B8, message size is 4, messagetype is 210
[WARN] 48587 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 255, expected 2, message size 4
Mode 2 / 2 , Got data D2 AE FF FF, message size is 4, messagetype is 210
[WARN] 48611 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 255, expected 81, message size 4
Mode 2 / 2 , Got data AC FF FF FF, message size is 4, messagetype is 210
Mode 2 / 2 , Got data AA FF FF FF, message size is 4, messagetype is 210
[WARN] 48629 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 210, expected 120, message size 4
[WARN] 48650 1 lib/lpfuart/src/parseinfostate.cpp#207:parse() - Checksum failed in command! Got 255, expected 59
[WARN] 48657 1 lib/lpfuart/src/parsecommandstate.cpp#145:parse() - Checksum failed in command! Got 255, expected 87
Mode 2 / 2 , Got data 76 FF D2 66, message size is 4, messagetype is 210
[WARN] 48671 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 255, expected 16, message size 4
Mode 2 / 2 , Got data 5E D2 5C FF, message size is 4, messagetype is 210
[WARN] 48685 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 255, expected 2, message size 4
Mode 2 / 2 , Got data D2 52 FF FF, message size is 4, messagetype is 210
[WARN] 48710 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 210, expected 173, message size 4
Mode 2 / 2 , Got data 42 FF D2 40, message size is 4, messagetype is 210
[WARN] 48724 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 255, expected 2, message size 4
Mode 2 / 2 , Got data 3E FF FF FF, message size is 4, messagetype is 210
[WARN] 48738 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 210, expected 236, message size 4
Mode 2 / 2 , Got data 34 FF FF FF, message size is 4, messagetype is 210
Mode 2 / 2 , Got data 32 FF FF FF, message size is 4, messagetype is 210
[WARN] 48766 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 210, expected 224, message size 4
Mode 2 / 2 , Got data D2 2E FF D2 2C FF FF FF FE D2 2A FF FF FF D2 D2, message size is 16, messagetype is 226
[WARN] 48789 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 32, expected 230, message size 16
Mode 2 / 2 , Got data D2 16 FF FF, message size is 4, messagetype is 210
[WARN] 48805 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 255, expected 233, message size 4
Mode 2 / 2 , Got data D2 0C FF FF, message size is 4, messagetype is 210
[WARN] 48829 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 255, expected 243, message size 4
Mode 2 / 2 , Got data D2 D2 FA FE, message size is 4, messagetype is 210
[WARN] 48843 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 255, expected 41, message size 4
Mode 2 / 2 , Got data D2 F2 D2 F0, message size is 4, messagetype is 210
[WARN] 48867 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 254, expected 47, message size 4
Mode 2 / 2 , Got data D2 E6 FE FF, message size is 4, messagetype is 210
[WARN] 48881 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 255, expected 24, message size 4
Mode 2 / 2 , Got data DC FE FF FF, message size is 4, messagetype is 210
Mode 2 / 2 , Got data D2 D2 FE FF, message size is 4, messagetype is 210
[WARN] 48909 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 255, expected 44, message size 4
[WARN] 48920 1 lib/lpfuart/src/waitingstate.cpp#74:parse() - Not implemented yet! type 0, data point 1, size 1 
Mode 2 / 2 , Got data D0 FE FF FF, message size is 4, messagetype is 210
[WARN] 48935 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 210, expected 3, message size 4
Mode 2 / 6 , Got data FE FF, message size is 2, messagetype is 206
[WARN] 48948 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 210, expected 48, message size 2
Mode 2 / 4 , Got data FE FF, message size is 2, messagetype is 204
[WARN] 48971 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 255, expected 50, message size 2
[WARN] 48982 1 lib/lpfuart/src/waitingstate.cpp#74:parse() - Not implemented yet! type 0, data point 31, size 8 
Mode 2 / 2 , Got data C2 FE FF FF, message size is 4, messagetype is 210
Mode 2 / 2 , Got data C0 FE FF D2, message size is 4, messagetype is 210
[WARN] 49001 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 176, expected 62, message size 4
Mode 2 / 2 , Got data AE FE D2 A6, message size is 4, messagetype is 210
[WARN] 49025 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 254, expected 9, message size 4
Mode 2 / 2 , Got data A4 D2 9C FE, message size is 4, messagetype is 210
[WARN] 49040 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 255, expected 57, message size 4
Mode 2 / 2 , Got data 9A D2 D2 8A, message size is 4, messagetype is 210
[WARN] 49054 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 254, expected 61, message size 4
[WARN] 49070 1 lib/lpfuart/src/parsecommandstate.cpp#145:parse() - Checksum failed in command! Got 254, expected 246
[WARN] 49102 1 lib/lpfuart/src/parseinfostate.cpp#207:parse() - Checksum failed in command! Got 255, expected 174
Mode 2 / 2 , Got data 1A D2 18 FE, message size is 4, messagetype is 210
[WARN] 49117 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 255, expected 3, message size 4
Mode 2 / 2 , Got data 10 D2 0E FE, message size is 4, messagetype is 210
[WARN] 49131 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 255, expected 31, message size 4
Mode 2 / 2 , Got data 06 FE D2 04, message size is 4, messagetype is 210
[WARN] 49146 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 254, expected 3, message size 4
Mode 2 / 7 , Got data D2 02 FE D2, message size is 4, messagetype is 215
[WARN] 49161 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 250, expected 212, message size 4
Mode 2 / 2 , Got data F8 FD FF FF, message size is 4, messagetype is 210
[WARN] 49186 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 210, expected 40, message size 4
Mode 2 / 2 , Got data F4 FD FF FF, message size is 4, messagetype is 210
Mode 2 / 2 , Got data F2 FD FF FF, message size is 4, messagetype is 210
Mode 2 / 2 , Got data F0 FD FF FF, message size is 4, messagetype is 210
Mode 2 / 2 , Got data EE D2 EC FD, message size is 4, messagetype is 210
[WARN] 49222 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 255, expected 0, message size 4
Mode 2 / 2 , Got data EA FD FF FF, message size is 4, messagetype is 210
[WARN] 49238 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 210, expected 58, message size 4
Mode 2 / 0 , Got data FD D2 D8 FD FF FF D2 D6 D2 CE FD D2 CC FD D2 C4 FD D2 C2 FD FF D2 D2 B2 FD D2 B0 FD FF D2 D2 A6, message size is 32, messagetype is 232
[WARN] 49277 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 253, expected 107, message size 32
Mode 2 / 2 , Got data A4 FD FF FF, message size is 4, messagetype is 210
[WARN] 49292 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 210, expected 116, message size 4
[WARN] 49314 1 lib/lpfuart/src/parseinfostate.cpp#207:parse() - Checksum failed in command! Got 210, expected 151
[WARN] 49317 1 lib/lpfuart/src/parseinfostate.cpp#207:parse() - Checksum failed in command! Got 253, expected 216
Mode 2 / 2 , Got data 80 FD FF FF, message size is 4, messagetype is 210
Mode 2 / 2 , Got data 7E FD FF D2, message size is 4, messagetype is 210
[WARN] 49344 1 lib/lpfuart/src/parsedatastate.cpp#83:parse() - Checksum failed in data! Got 124, expected 131, message size 4
[INFO] 49570 1 lib/lpfuart/src/legodevice.cpp#284:loop() - Did't receive data for some time, performing a device reset
[INFO] 49570 1 lib/lpfuart/src/legodevice.cpp#28:reset() - Performing a device reset
		
		*/


		// Only printed to Serial to not overload the logging framework
		Serial.printf("Mode %d / %d , Got data %s, message size is %d, messagetype is %d\n", modeIndex, messageMode, payloadHex.c_str(), messageSize, messageType);

		lastDataLog = currentMillis;
	}

	int checksum = 0xff ^ messageType;
	for (int i = 0; i < messageSize; i++) {
		checksum ^= messagePayload[i];
	}

	if (checksum != datapoint) {
		WARN("Checksum failed in data! Got %d, expected %d, message size %d", datapoint, checksum, messageSize);
		return new WaitingState(legoDevice);
	}

	if ((messageType & LUMP_INFO_MODE_PLUS_8) > 0) {
		messageMode += 8;
		messageType &= 31;
	}
	
	Mode *selectedMode = legoDevice->getMode(modeIndex);
	if (selectedMode != nullptr) {
		DEBUG("Processing data packet for mode %d with size %d, message mode is %d", modeIndex, messageSize, messageMode);
		processDataPacket(modeIndex, selectedMode);
	} else {
		WARN("Got datapacket, but no mode for index %d", modeIndex);
	}

	return new WaitingState(legoDevice);
}
