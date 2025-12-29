#ifndef WAITINGSTATE_H
#define WAITINGSTATE_H

#include "protocolstate.h"

class LegoDevice;

class WaitingState : public ProtocolState {
public:
	WaitingState(LegoDevice *legoDevice);
	virtual ~WaitingState() { }

	virtual ProtocolState *parse(int datapoint) override;

private:
	LegoDevice *legoDevice;
};

#endif // WAITINGSTATE_H
