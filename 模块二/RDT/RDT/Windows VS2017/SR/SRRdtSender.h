#pragma once
#include "RdtSender.h"
#include <iostream>

class SRRdtSender :public RdtSender {
private:
	int SendBase;
	Packet win[4];
	int PktInWin;
	int expectedSequenceNumberSend;
	int waitingState;
	int windowSize;
	bool is_ack[4]; //记录窗口中包的确认情况

public:
	bool send(const Message& message);						//发送应用层下来的Message，由NetworkService调用,如果发送方成功地将Message发送到网络层，返回true;如果因为发送方处于等待确认状态或发送窗口已满而拒绝发送Message，则返回false
	void receive(const Packet& ackPkt);						//接受确认Ack，将被NetworkService调用	
	void timeoutHandler(int seqNum);					//Timeout handler，将被NetworkService调用
	bool getWaitingState();								//返回RdtSender是否处于等待状态，如果发送方正等待确认或者发送窗口已满，返回true
	int findPacket(int seqnum);
	bool inWindow(const int seqnum);
	void printWindow();

	SRRdtSender();
	~SRRdtSender();
};