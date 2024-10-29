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
	bool is_ack[4]; //��¼�����а���ȷ�����

public:
	bool send(const Message& message);						//����Ӧ�ò�������Message����NetworkService����,������ͷ��ɹ��ؽ�Message���͵�����㣬����true;�����Ϊ���ͷ����ڵȴ�ȷ��״̬���ʹ����������ܾ�����Message���򷵻�false
	void receive(const Packet& ackPkt);						//����ȷ��Ack������NetworkService����	
	void timeoutHandler(int seqNum);					//Timeout handler������NetworkService����
	bool getWaitingState();								//����RdtSender�Ƿ��ڵȴ�״̬��������ͷ����ȴ�ȷ�ϻ��߷��ʹ�������������true
	int findPacket(int seqnum);
	bool inWindow(const int seqnum);
	void printWindow();

	SRRdtSender();
	~SRRdtSender();
};