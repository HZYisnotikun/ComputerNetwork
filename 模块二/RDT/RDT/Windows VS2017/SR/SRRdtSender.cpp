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
	this->win[this->PktInWin].acknum = -1; //忽略
	this->win[this->PktInWin].checksum = -1;
	this->win[PktInWin].seqnum = this->expectedSequenceNumberSend;
	memcpy(this->win[this->PktInWin].payload, message.data, sizeof(message.data));
	this->win[PktInWin].checksum = pUtils->calculateCheckSum(this->win[PktInWin]);
	pUtils->printPacket("发送方发送报文", this->win[PktInWin]);
	pns->sendToNetworkLayer(RECEIVER, this->win[PktInWin]); //调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方	
	pns->startTimer(SENDER, Configuration::TIME_OUT, this->win[PktInWin].seqnum); //启动发送方定时器
	cout << "start timer at seqnum " << win[PktInWin].seqnum << endl;
	

	this->expectedSequenceNumberSend++;
	this->expectedSequenceNumberSend %= 8;   //三位编码
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
	if (this->PktInWin > 0) {//只要窗口里还有包 就进行下面的过程
		//检查校验和是否正确
		int checkSum = pUtils->calculateCheckSum(ackPkt);
		if (checkSum == ackPkt.checksum && inWindow(ackPkt.acknum)) {  //???
			this->is_ack[findPacket(ackPkt.acknum)] = true;
			if (ackPkt.acknum == SendBase) {  
				//如果确认号等于SendBase 往前找一个新的SendBase
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
					this->is_ack[i - ii] = this->is_ack[i];  //剩下的包往前移动
				}
				for (int i = 4 - ii; i < 4; i++)
					this->is_ack[i] = false;  //腾出来的地方 全部置false
			}
			this->waitingState = false; //解除等待
			pUtils->printPacket("发送方正确收到确认", ackPkt);
			pns->stopTimer(SENDER, ackPkt.acknum);		//关闭定时器
			cout << "stop timer at seqnum " << ackPkt.acknum << endl;
			printWindow();
		}
	}
	return;
}

void SRRdtSender::timeoutHandler(int seqNum) {
	cout << seqNum << "号包超时，正在重传该包" << endl;
	pns->stopTimer(SENDER, seqNum);
	cout << "stop timer at seqnum " << seqNum << endl;
	pUtils->printPacket("重发报文", this->win[this->findPacket(seqNum)]);
	pns->sendToNetworkLayer(RECEIVER, this->win[this->findPacket(seqNum)]);
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//重新启动发送方定时器
	cout << "start timer at seqnum " << seqNum << endl;
}