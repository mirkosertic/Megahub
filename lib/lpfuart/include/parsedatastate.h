#ifndef PARSEDATASTATE_H
#define PARSEDATASTATE_H

#include <cstdint>

#include "protocolstate.h"
#include "mode.h"

class LegoDevice;

class ParseDataState : public ProtocolState {
public:
	ParseDataState(LegoDevice *legoDevice, int messageType, int messageMode, int messageSize);
	virtual ~ParseDataState();

	virtual ProtocolState *parse(int datapoint) override;

private:
	void processDataPacketDefault(int modeIndex, Mode* mode);
	void processDataPacket_BoostColorAndDistanceSensor(int modeIndex, Mode* mode);
	void processDataPacket(int modeIndex, Mode* mode);

	LegoDevice *legoDevice;
	int messageMode;
	int messageSize;
	uint8_t *messagePayload;
	int received;
	int messageType;
};

#endif // PARSEDATASTATE_H
