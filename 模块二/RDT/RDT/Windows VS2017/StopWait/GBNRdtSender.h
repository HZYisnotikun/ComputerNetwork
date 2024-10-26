//#ifndef GBN_RDT_SENDER_H
//#define GBN_RDT_SENDER_H
#pragma
#include "RdtSender.h"

class GBNRdtSender :public RdtSender
{
private:
	int SendBase;
	int expectSequenceNumberSend;	// ��һ��������� 
	bool waitingState;				// �Ƿ��ڵȴ�Ack��״̬
	Packet win[4];            //������һʱ�̴����еİ�
	int windowSize;
	int pktNumInWin;           //��ʱ��������İ�������

public:

	bool getWaitingState();
	bool send(const Message& message);						//����Ӧ�ò�������Message����NetworkServiceSimulator����,������ͷ��ɹ��ؽ�Message���͵�����㣬����true;�����Ϊ���ͷ����ڵȴ���ȷȷ��״̬���ܾ�����Message���򷵻�false
	void receive(const Packet& ackPkt);						//����ȷ��Ack������NetworkServiceSimulator����	
	void timeoutHandler(int seqNum);					//Timeout handler������NetworkServiceSimulator����

public:
	GBNRdtSender();
	virtual ~GBNRdtSender();
};


//#endif