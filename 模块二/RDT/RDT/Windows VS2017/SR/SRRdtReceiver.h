#pragma once
#include "RdtReceiver.h"

class SRRdtReceiver :public RdtReceiver{
private:
	int rcvBase;
	int windowSize;
	Packet lastAckPkt;
	Packet win[4];
	bool is_acked[4];
	int PktInWin;

public:
	void receive(const Packet& packet);
	bool inWindow(const int seqnum);

	SRRdtReceiver();
	virtual ~SRRdtReceiver();
};