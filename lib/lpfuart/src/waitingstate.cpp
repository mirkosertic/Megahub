#include "waitingstate.h"

#include "legodevice.h"
#include "logging.h"
#include "parsecommandstate.h"
#include "parsedatastate.h"
#include "parseinfostate.h"

WaitingState::WaitingState(LegoDevice *legoDevice)
	: legoDevice(legoDevice) {
}

ProtocolState *WaitingState::parse(int datapoint) {

	if (datapoint == LUMP_SYS_SYNC) {
		DEBUG("Received SYNC message");
		return this;
	}

	if (datapoint == LUMP_SYS_ACK) {
		INFO("Received ACK message");
		if (!legoDevice->isHandshakeComplete() && legoDevice->fullyInitialized()) {
			INFO("Device seems to be fully initialized");
			legoDevice->markAsHandshakeComplete();
		}

		return this;
	}

	if (datapoint == LUMP_SYS_NACK) {
		INFO("Received NACK message");
		return this;
	}

	int messageType = (datapoint & (128 + 64));
	int messageSize = (datapoint & (32 + 16 + 8));
	int messageMode = ((datapoint & (4 + 2 + 1)));

	int size = 0;
	switch (messageSize) {
		case LUMP_MSG_SIZE_1:
			size = 1;
			break;
		case LUMP_MSG_SIZE_2:
			size = 2;
			break;
		case LUMP_MSG_SIZE_4:
			size = 4;
			break;
		case LUMP_MSG_SIZE_8:
			size = 8;
			break;
		case LUMP_MSG_SIZE_16:
			size = 16;
			break;
		case LUMP_MSG_SIZE_32:
			size = 32;
			break;
	}

	int x = 7 << 3;

	if (size == 0) {
		WARN("Unknown message size %d in datapoint %d and mode %d", messageSize, datapoint, messageMode);
		return this;
	}

	if ((datapoint & LUMP_MSG_TYPE_DATA) >= LUMP_MSG_TYPE_DATA && legoDevice->isInDataMode()) {
		return new ParseDataState(legoDevice, datapoint, messageMode, size);
	} else if ((datapoint & LUMP_MSG_TYPE_CMD) >= LUMP_MSG_TYPE_CMD) {
		return new ParseCommandState(legoDevice, datapoint, messageMode, size);
	} else if ((datapoint & LUMP_MSG_TYPE_INFO) >= LUMP_MSG_TYPE_INFO) {
		return new ParseInfoState(legoDevice, datapoint, messageMode, size);
	}

	WARN("Not implemented yet! type %d, data point %d, size %d ", messageType, datapoint, size);

	return this;
}
