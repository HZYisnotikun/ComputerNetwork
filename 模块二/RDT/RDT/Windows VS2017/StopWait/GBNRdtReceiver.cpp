#include "stdafx.h"
#include "Global.h"
#include "GBNRdtReceiver.h"

GBNRdtReceiver::~GBNRdtReceiver() {

}

GBNRdtReceiver::GBNRdtReceiver() :expectSequenceNumberRcvd(0){
	lastAckPkt.acknum = -1; //初始状态下，上次发送的确认包的确认序号为-1，使得当第一个接受的数据包出错时该确认报文的确认号为-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//忽略该字段
	for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
		lastAckPkt.payload[i] = '.';
	}
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
}

void GBNRdtReceiver::receive(const Packet& packet) {
	int checksum = pUtils->calculateCheckSum(packet);
	if (checksum == packet.checksum && this->expectSequenceNumberRcvd == packet.seqnum) {
		//如果新来的包没有出错 并且序号等于期待收到的包的序号的话 更新期待号 发送一个新的ack

		//取出Message，向上递交给应用层
		Message msg;
		memcpy(msg.data, packet.payload, sizeof(packet.payload));
		pns->delivertoAppLayer(RECEIVER, msg);   //每次发送最新确认的包 之前的包也已经确认并发送

		lastAckPkt.acknum = packet.seqnum;
		lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt);
		pUtils->printPacket("接收方发送确认报文", lastAckPkt);

		this->expectSequenceNumberRcvd++;
	}
	else {
		if (packet.acknum != expectSequenceNumberRcvd)//如果乱序
			pUtils->printPacket("ERROR：接收方未收到正确报文：报文序号错误", packet);
		else
			pUtils->printPacket("ERROR：接收方未收到正确报文：检验和错误", packet);

		pUtils->printPacket("接受方重新发送上次的确认报文", lastAckPkt);//发送上一次的确认报文
		pns->sendToNetworkLayer(SENDER, lastAckPkt);//发送上一次的确认报文
	}
}