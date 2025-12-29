#ifndef PARSEDATASTATE_H
#define PARSEDATASTATE_H

#include "protocolstate.h"

class LegoDevice;

class ParseDataState : public ProtocolState {
public:
	ParseDataState(LegoDevice *legoDevice, int messageType, int messageMode, int messageSize);
	virtual ~ParseDataState();

	virtual ProtocolState *parse(int datapoint) override;

private:
	LegoDevice *legoDevice;
	int messageMode;
	int messageSize;
	int *messagePayload;
	int received;
	int messageType;
};

#endif // PARSEDATASTATE_H
