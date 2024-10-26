#include "stdafx.h"
#include "Global.h"
#include "GBNRdtSender.h"

GBNRdtSender::GBNRdtSender() :SendBase(0), expectSequenceNumberSend(0), waitingState(false), windowSize(4), pktNumInWin(0) 
{

}


GBNRdtSender::~GBNRdtSender() {

}

bool GBNRdtSender::getWaitingState() {
	return waitingState;
}

void GBNRdtSender::receive(const Packet &ackPkt) {
	if (this->pktNumInWin > 0) {//ֻҪ�����ﻹ�а� �ͽ�������Ĺ���
		//���У����Ƿ���ȷ
		int checkSum = pUtils->calculateCheckSum(ackPkt);

		//���У�����ȷ��ȷ�Ϻ�ΪsendBase����ʱ��Ҫ����SendBase  ��������ackPkt.acknum�п��ܱ�SendBase����
		if (checkSum == ackPkt.checksum && ackPkt.acknum >= SendBase) {
			int movNum = ackPkt.acknum - SendBase + 1;   //����������ν��յ�ack ֱ�ӻ��ӵ��ж��ٰ��õ���ȷ��
			SendBase = ackPkt.acknum + 1;
			this->waitingState = false;
			pUtils->printPacket("���ͷ���ȷ�յ�ȷ��", ackPkt);
			this->pktNumInWin -= movNum;
			pns->stopTimer(SENDER, this->win[0].seqnum);		//�رն�ʱ��
			cout << "stop timer at seqnum " << win[0].seqnum << endl;
			if (this->SendBase < expectSequenceNumberSend) //��ʱ�����л��а� Ҫ������ʱ��
			{
				pns->startTimer(SENDER, Configuration::TIME_OUT, this->win[movNum].seqnum);
				cout << "start timer at seqnum " << win[movNum].seqnum << endl;
			}
			for (int i = 0; i < pktNumInWin; i++) {
				this->win[i] = this->win[i + movNum];  //�Ѵ�������İ���ǰ�ƶ�movNum��
			}
		}
	}
	return;
}

bool GBNRdtSender::send(const Message &message) {
	if (waitingState || expectSequenceNumberSend >= SendBase + windowSize) {
		return false;
	}

	this->win[pktNumInWin].acknum = -1; //���Ը��ֶ�
	this->win[pktNumInWin].seqnum = this->expectSequenceNumberSend;
	this->win[pktNumInWin].checksum = 0;
	memcpy(this->win[pktNumInWin].payload, message.data, sizeof(message.data));
	this->win[pktNumInWin].checksum = pUtils->calculateCheckSum(this->win[pktNumInWin]);
	pUtils->printPacket("���ͷ����ͱ���", this->win[pktNumInWin]);
	pns->sendToNetworkLayer(RECEIVER, this->win[pktNumInWin]); //����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�	
	if (SendBase == expectSequenceNumberSend) {
		pns->startTimer(SENDER, Configuration::TIME_OUT, this->win[pktNumInWin].seqnum); //�������ͷ���ʱ��
		cout << "start timer at seqnum " << win[pktNumInWin].seqnum << endl;
	}

	expectSequenceNumberSend++;
	pktNumInWin++;
	if (expectSequenceNumberSend == SendBase + windowSize) {
		this->waitingState = true;
		cout << "the window is full!" << endl;
	}

	return true;
}

void GBNRdtSender::timeoutHandler(int seqNum) {
	cout << "��ʱ����ʱ�������ط�û��ȷ�ϵİ�" << endl;
	pns->stopTimer(SENDER, this->win[0].seqnum);  //���ȹرն�ʱ��
	cout << "stop timer at seqnum " << win[0].seqnum << endl;
	pns->startTimer(SENDER, Configuration::TIME_OUT, this->win[0].seqnum);			//�����������ͷ���ʱ��
	cout << "start timer at seqnum " << win[0].seqnum << endl;
	for (int i = 0; i < expectSequenceNumberSend - SendBase; i++) {
		pUtils->printPacket("�ط�����+1", this->win[i]);
		pns->sendToNetworkLayer(RECEIVER, this->win[i]);
	}
	return;
}

