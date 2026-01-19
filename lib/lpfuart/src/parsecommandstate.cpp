#include "parsecommandstate.h"

#include "legodevice.h"
#include "logging.h"
#include "waitingstate.h"

ParseCommandState::ParseCommandState(LegoDevice *legoDevice, int messageType, int messageMode, int messageSize)
	: legoDevice(legoDevice)
	, messageMode(messageMode)
	, messageSize(messageSize)
	, messageType(messageType) {
	received = 0;
	messagePayload = new int[messageSize];
}

ParseCommandState::~ParseCommandState() {
	delete[] messagePayload;
}

std::string ParseCommandState::bcdByteToString(int bcd) {
	int swapped = (byte) (((bcd & 0x0F) << 4) | ((bcd >> 4) & 0x0F));
	int highNibble = (swapped >> 4) & 0x0F;
	int lowNibble = swapped & 0x0F;
	return std::string((String(highNibble) + String(lowNibble)).c_str());
}

void ParseCommandState::parseCMDType() {
	int deviceId = messagePayload[0];
	std::string deviceName;

	switch (deviceId) {
		case 29:
			deviceName = "MINDSTORMS EV3 Color Sensor";
			break;
		case 30:
			deviceName = "MINDSTORMS EV3 Ultrasonic Sensor";
			break;
		case 32:
			deviceName = "MINDSTORMS EV3 Gyro Sensor";
			break;
		case 33:
			deviceName = "MINDSTORMS EV3 Infrared Sensor";
			break;
		case 34:
			deviceName = "WeDo 2.0 Tilt Sensor";
			break;
		case 35:
			deviceName = "WeDo 2.0 Motion Sensor";
			break;
		case 37:
			deviceName = "BOOST Color and Distance Sensor";
			break;
		case 38:
			deviceName = "BOOST Interactive Motor";
			break;
		case 46:
			deviceName = "Technic Medium Motor";
			break;
		case 45:
			deviceName = "Technic XL Motor";
			break;
		case 48:
			deviceName = "SPIKE Medium Motor";
			break;
		case 49:
			deviceName = "SPIKE Large Motor";
			break;
		case 61:
			deviceName = "SPIKE Color Sensor";
			break;
		case 62:
			deviceName = "SPIKE Ultrasonic Sensor";
			break;
		case 63:
			deviceName = "SPIKE Prime Force Sensor";
			break;
		case 64:
			deviceName = "Technic Color Light Matrix";
			break;
		case 65:
			deviceName = "SPIKE Small Motor";
			break;
		case 75:
			deviceName = "Technic Medium Angular Motor";
			break;
		case 76:
			deviceName = "Technic Large Angular Motor";
			break;
		default:
			deviceName = "Unknown Device";
			break;
	}

	legoDevice->setDeviceIdAndName(deviceId, deviceName);
}

void ParseCommandState::parseCMDModes() {
	int modes = messagePayload[0] + 1;
	legoDevice->initNumberOfModes(modes);
	INFO("Number of supported modes is %d", modes);

	if (messageSize == 2) {
		int views = messagePayload[1] + 1;
		INFO("Number of supported views is %d", views);
	} else if (messageSize == 5) {
		int modes2 = messagePayload[2] + 1;
		int views2 = messagePayload[3] + 1;
		INFO("Number of supported modes2 is %d", modes2);
		INFO("Number of supported views2 is %d", views2);
	}
}

void ParseCommandState::parseCMDVersion() {
	std::string fwVersion = bcdByteToString(messagePayload[3]) + "." + bcdByteToString(messagePayload[2]) + "." + bcdByteToString(messagePayload[1]) + "." + bcdByteToString(messagePayload[0]);

	std::string hwVersion = bcdByteToString(messagePayload[7]) + "." + bcdByteToString(messagePayload[6]) + "." + bcdByteToString(messagePayload[5]) + "." + bcdByteToString(messagePayload[4]);

	legoDevice->setVersions(fwVersion, hwVersion);
}

void ParseCommandState::parseCMDSpeed() {
	long speedChangeTo = ((long) (messagePayload[0] & 0xFF)) | ((long) (messagePayload[1] & 0xFF) << 8) | ((long) (messagePayload[2] & 0xFF) << 16) | ((long) (messagePayload[3] & 0xFF) << 24);

	legoDevice->setSerialSpeed(speedChangeTo);
}

ProtocolState *ParseCommandState::parse(int datapoint) {
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

	switch (messageMode) {
		case LUMP_CMD_TYPE:
			parseCMDType();
			break;
		case LUMP_CMD_MODES:
			parseCMDModes();
			break;
		case LUMP_CMD_SPEED:
			parseCMDSpeed();
			break;
		case LUMP_CMD_VERSION:
			parseCMDVersion();
			break;
		default:
			DEBUG("Unknown message mode %d in command!", messageMode);
			break;
	}

	return new WaitingState(legoDevice);
}
