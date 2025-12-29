#ifndef PROTOCOLSTATE_H
#define PROTOCOLSTATE_H

#include <Arduino.h>

class ProtocolState {
public:
	static const int LUMP_MSG_TYPE_SYS = 0 << 6;
	static const int LUMP_MSG_TYPE_CMD = 1 << 6;
	static const int LUMP_MSG_TYPE_INFO = 2 << 6;
	static const int LUMP_MSG_TYPE_DATA = 3 << 6;
	static const int LUMP_MSG_SIZE_1 = 0 << 3;
	static const int LUMP_MSG_SIZE_2 = 1 << 3;
	static const int LUMP_MSG_SIZE_4 = 2 << 3;
	static const int LUMP_MSG_SIZE_8 = 3 << 3;
	static const int LUMP_MSG_SIZE_16 = 4 << 3;
	static const int LUMP_MSG_SIZE_32 = 5 << 3;
	static const int LUMP_CMD_TYPE = 0x0;
	static const int LUMP_CMD_MODES = 0x1;
	static const int LUMP_CMD_SPEED = 0x2;
	static const int LUMP_CMD_SELECT = 0x3;
	static const int LUMP_CMD_VERSION = 0x7;
	static const int LUMP_SYS_SYNC = 0x0;
	static const int LUMP_SYS_NACK = 0x2;
	static const int LUMP_SYS_ACK = 0x4;
	static const int LUMP_SYS_ESC = 0x6;
	static const int LUMP_INFO_NAME = 0x00;
	static const int LUMP_INFO_RAW = 0x01;
	static const int LUMP_INFO_PCT = 0x02;
	static const int LUMP_INFO_SI = 0x03;
	static const int LUMP_INFO_UNITS = 0x04;
	static const int LUMP_INFO_MAPPING = 0x05;
	static const int LUMP_INFO_MODE_COMBOS = 0x06;
	static const int LUMP_INFO_UNK7 = 0x07;
	static const int LUMP_INFO_UNK8 = 0x08;
	static const int LUMP_INFO_UNK9 = 0x09;
	static const int LUMP_INFO_UNK10 = 0x0A;
	static const int LUMP_INFO_UNK11 = 0x0B;
	static const int LUMP_INFO_UNK12 = 0x0C;
	static const int LUMP_INFO_MODE_PLUS_8 = 0x20;
	static const int LUMP_INFO_FORMAT = 0x80;

	virtual ~ProtocolState() { }
	virtual ProtocolState *parse(int datapoint) = 0;
};

#endif // PROTOCOLSTATE_H
