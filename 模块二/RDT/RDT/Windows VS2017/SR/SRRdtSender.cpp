#include "stdafx.h"
#include "Global.h"
#include "SRRdtSender.h"

SRRdtSender::SRRdtSender() :SendBase(0), expectedSequenceNumberSend(0), PktInWin(0), windowSize(4), waitingState(false){
	for (int i = 0; i < 4; i++) {
		this->is_ack[i] = false;
	}
}

SRRdtSender::~SRRdtSender() {

}

void SRRdtSender::printWindow() {
	cout << "window now: " << endl;
	cout << "------------------------------------" << endl;
	for (int i = 0; i < PktInWin; i++)
		if (!is_ack[i])
			cout << " | " << win[i].seqnum;
		else cout << " | " << -1;
	cout << " | " << endl;
	cout << "------------------------------------" << endl;
}

int SRRdtSender::findPacket(int Seqnum) {
	int i = 0;
	for (; i < 4; i++) {
		if (this->win[i].seqnum == Seqnum) {
			return i;
		}
	}
	if (i == 4) return -1;
}

bool SRRdtSender::getWaitingState() {
	return this->waitingState;
}

bool SRRdtSender::inWindow(const int seqnum) {
	if (SendBase < expectedSequenceNumberSend && seqnum >= SendBase && seqnum < expectedSequenceNumberSend)
		return true;
	else if (SendBase > expectedSequenceNumberSend && (seqnum >= SendBase || seqnum < expectedSequenceNumberSend))
		return true;
	else
		return false;
}

bool SRRdtSender::send(const Message &message) {
	if (this->waitingState == true) {
		std::cout << "failed to send message, the window is full" << std::endl;
		return false;
	}
	this->win[this->PktInWin].acknum = -1; //����
	this->win[this->PktInWin].checksum = -1;
	this->win[PktInWin].seqnum = this->expectedSequenceNumberSend;
	memcpy(this->win[this->PktInWin].payload, message.data, sizeof(message.data));
	this->win[PktInWin].checksum = pUtils->calculateCheckSum(this->win[PktInWin]);
	pUtils->printPacket("���ͷ����ͱ���", this->win[PktInWin]);
	pns->sendToNetworkLayer(RECEIVER, this->win[PktInWin]); //����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�	
	pns->startTimer(SENDER, Configuration::TIME_OUT, this->win[PktInWin].seqnum); //�������ͷ���ʱ��
	cout << "start timer at seqnum " << win[PktInWin].seqnum << endl;
	

	this->expectedSequenceNumberSend++;
	this->expectedSequenceNumberSend %= 8;   //��λ����
	PktInWin++;
	cout << "expect: " << expectedSequenceNumberSend << " SendBase: " << SendBase << endl;
	if ((this->expectedSequenceNumberSend - (SendBase + windowSize)) % 8 == 0) {  
		this->waitingState = true;
		cout << "the window is full!" << endl;
	}
	printWindow();

	return true;
}

void SRRdtSender::receive(const Packet &ackPkt) {
	if (this->PktInWin > 0) {//ֻҪ�����ﻹ�а� �ͽ�������Ĺ���
		//���У����Ƿ���ȷ
		int checkSum = pUtils->calculateCheckSum(ackPkt);
		if (checkSum == ackPkt.checksum && inWindow(ackPkt.acknum)) {  //???
			this->is_ack[findPacket(ackPkt.acknum)] = true;
			if (ackPkt.acknum == SendBase) {  
				//���ȷ�Ϻŵ���SendBase ��ǰ��һ���µ�SendBase
				int ii = 0;
				for (ii = 0; ii < 4; ii++) {
					if (!is_ack[ii]) {
						SendBase = (SendBase + ii) % 8;
						break;
					}
				}
				if (ii == 4) this->SendBase = expectedSequenceNumberSend;
				this->PktInWin -= ii;
				for (int i = ii; i < 4; i++) {
					this->win[i - ii] = this->win[i];
					this->is_ack[i - ii] = this->is_ack[i];  //ʣ�µİ���ǰ�ƶ�
				}
				for (int i = 4 - ii; i < 4; i++)
					this->is_ack[i] = false;  //�ڳ����ĵط� ȫ����false
			}
			this->waitingState = false; //����ȴ�
			pUtils->printPacket("���ͷ���ȷ�յ�ȷ��", ackPkt);
			pns->stopTimer(SENDER, ackPkt.acknum);		//�رն�ʱ��
			cout << "stop timer at seqnum " << ackPkt.acknum << endl;
			printWindow();
		}
	}
	return;
}

void SRRdtSender::timeoutHandler(int seqNum) {
	cout << seqNum << "�Ű���ʱ�������ش��ð�" << endl;
	pns->stopTimer(SENDER, seqNum);
	cout << "stop timer at seqnum " << seqNum << endl;
	pUtils->printPacket("�ط�����", this->win[this->findPacket(seqNum)]);
	pns->sendToNetworkLayer(RECEIVER, this->win[this->findPacket(seqNum)]);
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//�����������ͷ���ʱ��
	cout << "start timer at seqnum " << seqNum << endl;
}