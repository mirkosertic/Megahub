#ifndef PARSECOMMANDSTATE_H
#define PARSECOMMANDSTATE_H

#include "protocolstate.h"

#include <string>

class LegoDevice;

class ParseCommandState : public ProtocolState {
public:
	ParseCommandState(LegoDevice *legoDevice, int messageType, int messageMode, int messageSize);
	virtual ~ParseCommandState();

	virtual ProtocolState *parse(int datapoint) override;

	static std::string bcdByteToString(int bcd);

private:
	void parseCMDType();
	void parseCMDModes();
	void parseCMDVersion();
	void parseCMDSpeed();

	LegoDevice *legoDevice;
	int messageMode;
	int messageSize;
	int *messagePayload;
	int received;
	int messageType;
};

#endif // PARSECOMMANDSTATE_H
