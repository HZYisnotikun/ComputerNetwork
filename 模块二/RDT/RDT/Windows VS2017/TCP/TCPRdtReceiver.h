#pragma once
#include "RdtReceiver.h"

class TCPRdtReceiver :public RdtReceiver {
private:
	int expectSequenceNumberRcvd;
	Packet lastAckPkt;


public:
	TCPRdtReceiver();
	virtual ~TCPRdtReceiver();

	void receive(const Packet& packet);
};