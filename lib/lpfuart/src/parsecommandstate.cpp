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
	messagePayload = new uint8_t[messageSize];
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
		case DEVICEID_EV3_COLOR_SENSOR:
			deviceName = "MINDSTORMS EV3 Color Sensor";
			break;
		case DEVICEID_EV3_ULTRASONIC_SENSOR:
			deviceName = "MINDSTORMS EV3 Ultrasonic Sensor";
			break;
		case DEVICEID_EV3_GYRO_SENSOR:
			deviceName = "MINDSTORMS EV3 Gyro Sensor";
			break;
		case DEVICEID_EV3_INFRARED_SENSOR:
			deviceName = "MINDSTORMS EV3 Infrared Sensor";
			break;
		case DEVICEID_WEDO20_TILT:
			deviceName = "WeDo 2.0 Tilt Sensor";
			break;
		case DEVICEID_WEDO20_MOTION:
			deviceName = "WeDo 2.0 Motion Sensor";
			break;
		case DEVICEID_BOOST_COLOR_DISTANCE_SENSOR:
			deviceName = "BOOST Color and Distance Sensor";
			break;
		case DEVICEID_BOOST_INTERACTIVE_MOTOR:
			deviceName = "BOOST Interactive Motor";
			break;
		case DEVICEID_TECHNIC_MEDIUM_MOTOR:
			deviceName = "Technic Medium Motor";
			break;
		case DEVICEID_TECHNIC_XL_MOTOR:
			deviceName = "Technic XL Motor";
			break;
		case DEVICEID_SPIKE_MEDIUM_MOTOR:
			deviceName = "SPIKE Medium Motor";
			break;
		case DEVUCEID_SPIKE_LARGE_MOTOR:
			deviceName = "SPIKE Large Motor";
			break;
		case DEVICEID_SPIKE_COLOR_SENSOR:
			deviceName = "SPIKE Color Sensor";
			break;
		case DEVICEID_SPIKE_ULTRASONIC_SENSOR:
			deviceName = "SPIKE Ultrasonic Sensor";
			break;
		case DEVICEID_SPIKE_PRIME_FORCE_SENSOR:
			deviceName = "SPIKE Prime Force Sensor";
			break;
		case DEVICEID_TECHNIC_COLOR_LIGHT_MATRIX:
			deviceName = "Technic Color Light Matrix";
			break;
		case DEVICEID_SPIKE_SMALL_MOTOR:
			deviceName = "SPIKE Small Motor";
			break;
		case DEVICEID_TECHNIC_MEDIUM_ANGULAR_MOTOR:
			deviceName = "Technic Medium Angular Motor";
			break;
		case DEVICEID_TECHNIC_LARGE_ANGULAR_MOTOR:
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
	if (messageSize == 1) {
		legoDevice->initNumberOfModes(modes);
		INFO("Number of supported modes is %d (1 byte payload)", modes);
	} else if (messageSize == 2) {
		legoDevice->initNumberOfModes(modes);
		INFO("Number of supported modes is %d (2 bytes payload)", modes);

		int views = messagePayload[1] + 1;
		INFO("Number of supported views is %d", views);
	} else if (messageSize == 4) {
		int modes2 = messagePayload[2] + 1;
		int views2 = messagePayload[3] + 1;

		legoDevice->initNumberOfModes(modes2);
		INFO("Number of supported modes is %d (4 bytes payload)", modes2);
	} else {
		WARN("Unsupported payload length for CMD_MODES : %d", messageSize);
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
		messagePayload[received++] = static_cast<uint8_t>(datapoint);
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
