#include "stdafx.h"
#include "Global.h"
#include "GBNRdtReceiver.h"

GBNRdtReceiver::~GBNRdtReceiver() {

}

GBNRdtReceiver::GBNRdtReceiver() :expectSequenceNumberRcvd(0){
	lastAckPkt.acknum = -1; //��ʼ״̬�£��ϴη��͵�ȷ�ϰ���ȷ�����Ϊ-1��ʹ�õ���һ�����ܵ����ݰ�����ʱ��ȷ�ϱ��ĵ�ȷ�Ϻ�Ϊ-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//���Ը��ֶ�
	for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
		lastAckPkt.payload[i] = '.';
	}
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
}

void GBNRdtReceiver::receive(const Packet& packet) {
	int checksum = pUtils->calculateCheckSum(packet);
	if (checksum == packet.checksum && this->expectSequenceNumberRcvd == packet.seqnum) {
		//��������İ�û�г��� ������ŵ����ڴ��յ��İ�����ŵĻ� �����ڴ��� ����һ���µ�ack

		//ȡ��Message�����ϵݽ���Ӧ�ò�
		Message msg;
		memcpy(msg.data, packet.payload, sizeof(packet.payload));
		pns->delivertoAppLayer(RECEIVER, msg);   //ÿ�η�������ȷ�ϵİ� ֮ǰ�İ�Ҳ�Ѿ�ȷ�ϲ�����

		lastAckPkt.acknum = packet.seqnum;
		lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt);
		pUtils->printPacket("���շ�����ȷ�ϱ���", lastAckPkt);

		this->expectSequenceNumberRcvd++;
	}
	else {
		if (packet.acknum != expectSequenceNumberRcvd)//�������
			pUtils->printPacket("ERROR�����շ�δ�յ���ȷ���ģ�������Ŵ���", packet);
		else
			pUtils->printPacket("ERROR�����շ�δ�յ���ȷ���ģ�����ʹ���", packet);

		pUtils->printPacket("���ܷ����·����ϴε�ȷ�ϱ���", lastAckPkt);//������һ�ε�ȷ�ϱ���
		pns->sendToNetworkLayer(SENDER, lastAckPkt);//������һ�ε�ȷ�ϱ���
	}
}