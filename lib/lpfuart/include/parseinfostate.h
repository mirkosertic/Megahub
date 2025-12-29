#ifndef PARSEINFOSTATE_H
#define PARSEINFOSTATE_H

#include "protocolstate.h"

class LegoDevice;

class ParseInfoState : public ProtocolState {
public:
	ParseInfoState(LegoDevice *legoDevice, int messageType, int messageMode, int messageSize);
	virtual ~ParseInfoState();

	virtual ProtocolState *parse(int datapoint) override;

private:
	void parseINFOName();
	void parseINFOMapping();
	void parseINFOPct();
	void parseINFOSI();
	void parseINFOUnits();
	void parseINFOFormat();
	void parseINFO();

	LegoDevice *legoDevice;
	int messageMode;
	int messageSize;
	int *messagePayload;
	int received;
	int messageType;
};

#endif // PARSEINFOSTATE_H
