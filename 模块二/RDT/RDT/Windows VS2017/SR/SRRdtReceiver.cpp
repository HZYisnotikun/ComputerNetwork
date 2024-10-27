#include "stdafx.h"
#include "Global.h"
#include "SRRdtReceiver.h"

SRRdtReceiver::SRRdtReceiver() :rcvBase(0), windowSize(4), PktInWin(0) {
	lastAckPkt.acknum = -1; //初始状态下，上次发送的确认包的确认序号为-1，使得当第一个接受的数据包出错时该确认报文的确认号为-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//忽略该字段
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
		pUtils->printPacket("接收方发送确认报文", lastAckPkt);
		return;
	}
	int checksum = pUtils->calculateCheckSum(packet);
	if (checksum == packet.checksum) {
		//在窗口内做如下操作
		//首先不管怎么样先确认
		lastAckPkt.acknum = packet.seqnum;
		lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt);
		pUtils->printPacket("接收方发送确认报文", lastAckPkt);
		
		int idx = (packet.seqnum - rcvBase >= 0) ? (packet.seqnum - rcvBase) : (packet.seqnum + 8 - rcvBase);;  //here!!!
		this->win[idx] = packet;        //按序存入窗口
		this->is_acked[idx] = true;
		this->PktInWin++;
		cout << "packet amount in rcv window: " << PktInWin << endl;

		if (packet.seqnum == rcvBase) {  
			//rcvBase被确认 开始发送数据 同时更新窗口
			Message msg;
			int ii = 0;
			for (; ii < 4 && is_acked[ii]; ii++) {
				memcpy(msg.data, win[ii].payload, sizeof(win[ii].payload));
				pns->delivertoAppLayer(RECEIVER, msg);   
				this->PktInWin--;
			}
			for (int i = ii; i < 4; i++) {
				this->win[i - ii] = this->win[i];
				this->is_acked[i - ii] = this->is_acked[i];  //剩下的包往前移动
			}
			for (int i = 4 - ii; i < 4; i++)
				this->is_acked[i] = false;  //腾出来的地方 全部置false
			rcvBase = (rcvBase + ii)% 8;     //rcvBase往前移动ii位 还得mod8
		}
	}
}