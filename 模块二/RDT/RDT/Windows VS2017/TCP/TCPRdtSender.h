#pragma once
#include "RdtSender.h"

class TCPRdtSender :public RdtSender {
private:
	int SendBase;
	int expectSequenceNumberSend;	// 下一个发送序号 
	bool waitingState;				// 是否处于等待Ack的状态
	Packet win[4];            //保存这一时刻窗口中的包
	int windowSize;
	int pktNumInWin;           //此时窗口里面的包的数量
	int SameInRow;
	int lastAck;

public:
	bool getWaitingState();
	bool send(const Message& message);						//发送应用层下来的Message，由NetworkServiceSimulator调用,如果发送方成功地将Message发送到网络层，返回true;如果因为发送方处于等待正确确认状态而拒绝发送Message，则返回false
	void receive(const Packet& ackPkt);						//接受确认Ack，将被NetworkServiceSimulator调用	
	void timeoutHandler(int seqNum);					//Timeout handler，将被NetworkServiceSimulator调用
	bool InWindow(int seqnum);                         //判断是否在窗口中
	void printWindow();

	TCPRdtSender();
	virtual ~TCPRdtSender();
};