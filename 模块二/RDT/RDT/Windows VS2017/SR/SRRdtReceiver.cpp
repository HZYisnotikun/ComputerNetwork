#include "stdafx.h"
#include "Global.h"
#include "SRRdtReceiver.h"

SRRdtReceiver::SRRdtReceiver() :rcvBase(0), windowSize(4), PktInWin(0) {
	lastAckPkt.acknum = -1; //��ʼ״̬�£��ϴη��͵�ȷ�ϰ���ȷ�����Ϊ-1��ʹ�õ���һ�����ܵ����ݰ�����ʱ��ȷ�ϱ��ĵ�ȷ�Ϻ�Ϊ-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//���Ը��ֶ�
	for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
		lastAckPkt.payload[i] = '.';
	}
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);

	for (int i = 0; i < 4; i++)
		this->is_acked[i] = false;
}

SRRdtReceiver::~SRRdtReceiver() {

}

bool SRRdtReceiver::inWindow(const int seqnum) {
	int next = (rcvBase + 4) % 8;
	if (rcvBase < next && seqnum >= rcvBase && seqnum < next)
		return true;
	else if (rcvBase > next && (seqnum >= rcvBase || seqnum < next))
		return true;
	else
		return false;
}

void SRRdtReceiver::receive(const Packet& packet) {
	if (!inWindow(packet.seqnum)) {
		lastAckPkt.acknum = packet.seqnum;
		lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt);
		pUtils->printPacket("���շ�����ȷ�ϱ���", lastAckPkt);
		return;
	}
	int checksum = pUtils->calculateCheckSum(packet);
	if (checksum == packet.checksum) {
		//�ڴ����������²���
		//���Ȳ�����ô����ȷ��
		lastAckPkt.acknum = packet.seqnum;
		lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt);
		pUtils->printPacket("���շ�����ȷ�ϱ���", lastAckPkt);
		
		int idx = (packet.seqnum - rcvBase >= 0) ? (packet.seqnum - rcvBase) : (packet.seqnum + 8 - rcvBase);;  //here!!!
		this->win[idx] = packet;        //������봰��
		this->is_acked[idx] = true;
		this->PktInWin++;
		cout << "packet amount in rcv window: " << PktInWin << endl;

		if (packet.seqnum == rcvBase) {  
			//rcvBase��ȷ�� ��ʼ�������� ͬʱ���´���
			Message msg;
			int ii = 0;
			for (; ii < 4 && is_acked[ii]; ii++) {
				memcpy(msg.data, win[ii].payload, sizeof(win[ii].payload));
				pns->delivertoAppLayer(RECEIVER, msg);   
				this->PktInWin--;
			}
			for (int i = ii; i < 4; i++) {
				this->win[i - ii] = this->win[i];
				this->is_acked[i - ii] = this->is_acked[i];  //ʣ�µİ���ǰ�ƶ�
			}
			for (int i = 4 - ii; i < 4; i++)
				this->is_acked[i] = false;  //�ڳ����ĵط� ȫ����false
			rcvBase = (rcvBase + ii)% 8;     //rcvBase��ǰ�ƶ�iiλ ����mod8
		}
	}
}