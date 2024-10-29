#pragma once
#include "RdtSender.h"

class TCPRdtSender :public RdtSender {
private:
	int SendBase;
	int expectSequenceNumberSend;	// ��һ��������� 
	bool waitingState;				// �Ƿ��ڵȴ�Ack��״̬
	Packet win[4];            //������һʱ�̴����еİ�
	int windowSize;
	int pktNumInWin;           //��ʱ��������İ�������
	int SameInRow;
	int lastAck;

public:
	bool getWaitingState();
	bool send(const Message& message);						//����Ӧ�ò�������Message����NetworkServiceSimulator����,������ͷ��ɹ��ؽ�Message���͵�����㣬����true;�����Ϊ���ͷ����ڵȴ���ȷȷ��״̬���ܾ�����Message���򷵻�false
	void receive(const Packet& ackPkt);						//����ȷ��Ack������NetworkServiceSimulator����	
	void timeoutHandler(int seqNum);					//Timeout handler������NetworkServiceSimulator����
	bool InWindow(int seqnum);                         //�ж��Ƿ��ڴ�����
	void printWindow();

	TCPRdtSender();
	virtual ~TCPRdtSender();
};