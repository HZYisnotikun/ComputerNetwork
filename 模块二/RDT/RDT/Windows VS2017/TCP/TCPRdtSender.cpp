#include "stdafx.h"
#include "Global.h"
#include "TCPRdtSender.h"

TCPRdtSender::TCPRdtSender() :SendBase(0), expectSequenceNumberSend(0), waitingState(false), windowSize(4), pktNumInWin(0), SameInRow(0), lastAck(-1) {

}

TCPRdtSender::~TCPRdtSender() {

}

void TCPRdtSender::printWindow() {
	cout << "window now: " << endl;
	cout << "------------------------------------" << endl;
	for (int i = 0; i < pktNumInWin; i++)
		cout << " | " << win[i].seqnum;
	cout << " | " << endl;
	cout << "------------------------------------" << endl;

}

bool TCPRdtSender::getWaitingState() {
	return waitingState;
}

bool TCPRdtSender::InWindow(int seqnum) {
	if (SendBase < expectSequenceNumberSend && seqnum >= SendBase && seqnum < expectSequenceNumberSend)
		return true;
	else if (SendBase > expectSequenceNumberSend && (seqnum >= SendBase || seqnum < expectSequenceNumberSend))
		return true;
	else
		return false;
}

void TCPRdtSender::receive(const Packet& ackPkt) {
	//�����ش�ʵ��
	if (ackPkt.acknum == lastAck) {
		SameInRow++;
		cout << "����" << SameInRow << "������ACK" << endl;
	}
	else {
		lastAck = ackPkt.acknum;
		SameInRow = 0;
	}
	if (SameInRow == 3) {
		cout << "������������ACK�� " << ackPkt.acknum << " ���������ش�" << endl;
		//SameInRow = 0;
		timeoutHandler(0);
		return;
	}
	if (this->pktNumInWin > 0) {//ֻҪ�����ﻹ�а� �ͽ�������Ĺ���
		//���У����Ƿ���ȷ
		int checkSum = pUtils->calculateCheckSum(ackPkt);
		//���У�����ȷ��ȷ�Ϻ�ΪsendBase����ʱ��Ҫ����SendBase  ��������ackPkt.acknum�п��ܱ�SendBase����
		if (checkSum == ackPkt.checksum && InWindow(ackPkt.acknum)) {
			int movNum = (ackPkt.acknum - SendBase) + ((ackPkt.acknum >= SendBase) ? 1 : 9);   //����������ν��յ�ack ֱ�ӻ��ӵ��ж��ٰ��õ���ȷ��
			SendBase = (ackPkt.acknum + 1) % 8;
			this->waitingState = false;
			pUtils->printPacket("���ͷ���ȷ�յ�ȷ��", ackPkt);
			this->pktNumInWin -= movNum;
			pns->stopTimer(SENDER, this->win[0].seqnum);		//�رն�ʱ��
			cout << "stop timer at seqnum " << win[0].seqnum << endl;
			if (pktNumInWin) //��ʱ�����л��а� Ҫ������ʱ��
			{
				pns->startTimer(SENDER, Configuration::TIME_OUT, this->win[movNum].seqnum);
				cout << "start timer at seqnum " << win[movNum].seqnum << endl;
			}
			for (int i = 0; i < pktNumInWin; i++) {
				this->win[i] = this->win[i + movNum];  //�Ѵ�������İ���ǰ�ƶ�movNum��
			}
			printWindow();
		}
	}
}

bool TCPRdtSender::send(const Message& message) {
	if (waitingState) {
		std::cout << "failed to send message, the window is full" << std::endl;
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
	this->expectSequenceNumberSend %= 8;
	pktNumInWin++;
	if ((expectSequenceNumberSend - (SendBase + windowSize)) % 8 == 0) {
		this->waitingState = true;
		cout << "the window is full!" << endl;
	}
	printWindow();
}

void TCPRdtSender::timeoutHandler(int seqnum) {
	//�ж��ǲ��ǿ����ش�
	if (SameInRow == 3)
		SameInRow = 0;
	else
		cout << "��ʱ����ʱ�������ط���һ��û��ȷ�ϵİ�" << endl;
	pns->stopTimer(SENDER, this->win[0].seqnum);  //���ȹرն�ʱ��
	cout << "stop timer at seqnum " << win[0].seqnum << endl;
	pns->startTimer(SENDER, Configuration::TIME_OUT, this->win[0].seqnum);			//�����������ͷ���ʱ��
	cout << "start timer at seqnum " << win[0].seqnum << endl;
	pUtils->printPacket("�ط�����", this->win[0]);
	pns->sendToNetworkLayer(RECEIVER, this->win[0]);
}

