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

bool GBNRdtSender::InWindow(int seqnum) {
	if (SendBase < expectSequenceNumberSend && seqnum >= SendBase && seqnum < expectSequenceNumberSend)
		return true;
	else if (SendBase > expectSequenceNumberSend && (seqnum >= SendBase || seqnum < expectSequenceNumberSend))
		return true;
	else
		return false;
}

void GBNRdtSender::printWindow() {
	cout << "window now: " << endl;
	cout << "------------------------------------" << endl;
	for (int i = 0; i < pktNumInWin; i++)
		cout << " | " << win[i].seqnum;
	cout << " | " << endl;
	cout << "------------------------------------" << endl;

}

void GBNRdtSender::receive(const Packet &ackPkt) {
	if (this->pktNumInWin > 0) {//只要窗口里还有包 就进行下面的过程
		//检查校验和是否正确
		int checkSum = pUtils->calculateCheckSum(ackPkt);

		//如果校验和正确，确认号为sendBase，这时候要更新SendBase  ？？这里ackPkt.acknum有可能比SendBase大吗
		if (checkSum == ackPkt.checksum && InWindow(ackPkt.acknum)) {
			int movNum = (ackPkt.acknum - SendBase) + ((ackPkt.acknum >= SendBase) ? 1 : 9);   //可以理解成这次接收到ack 直接或间接地有多少包得到了确认
			SendBase = (ackPkt.acknum + 1) % 8;
			pUtils->printPacket("发送方正确收到确认", ackPkt);
			this->pktNumInWin -= movNum;
			pns->stopTimer(SENDER, this->win[0].seqnum);		//关闭定时器
			cout << "stop timer at seqnum " << win[0].seqnum << endl;
			
			if (pktNumInWin) //此时窗口中还有包 要重启计时器
			{
				pns->startTimer(SENDER, Configuration::TIME_OUT, this->win[movNum].seqnum);
				cout << "start timer at seqnum " << win[movNum].seqnum << endl;
			}
			for (int i = 0; i < pktNumInWin; i++) {
				this->win[i] = this->win[i + movNum];  //把窗口里面的包往前移动movNum步
			}
			printWindow();
			this->waitingState = false;
		}
	}
	return;
}

bool GBNRdtSender::send(const Message &message) {
	if (waitingState) {
		std::cout << "failed to send message, the window is full" << std::endl;
		return false;
	}

	this->win[pktNumInWin].acknum = -1; //忽略该字段
	this->win[pktNumInWin].seqnum = this->expectSequenceNumberSend;
	this->win[pktNumInWin].checksum = 0;
	memcpy(this->win[pktNumInWin].payload, message.data, sizeof(message.data));
	this->win[pktNumInWin].checksum = pUtils->calculateCheckSum(this->win[pktNumInWin]);
	pUtils->printPacket("发送方发送报文", this->win[pktNumInWin]);
	pns->sendToNetworkLayer(RECEIVER, this->win[pktNumInWin]); //调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方	
	if (SendBase == expectSequenceNumberSend) {
		pns->startTimer(SENDER, Configuration::TIME_OUT, this->win[pktNumInWin].seqnum); //启动发送方定时器
		cout << "start timer at seqnum " << win[pktNumInWin].seqnum << endl;
	}

	expectSequenceNumberSend++;
	this->expectSequenceNumberSend %= 8;
	pktNumInWin++;
	printWindow();
	if ((expectSequenceNumberSend - (SendBase + windowSize)) % 8 == 0) {
		this->waitingState = true;
		cout << "the window is full!" << endl;
	}

	return true;
}

void GBNRdtSender::timeoutHandler(int seqNum) {
	cout << "计时器超时，下面重发没被确认的包" << endl;
	pns->stopTimer(SENDER, this->win[0].seqnum);  //首先关闭定时器
	cout << "stop timer at seqnum " << win[0].seqnum << endl;
	pns->startTimer(SENDER, Configuration::TIME_OUT, this->win[0].seqnum);			//重新启动发送方定时器
	cout << "start timer at seqnum " << win[0].seqnum << endl;
	int win_len = (expectSequenceNumberSend >= SendBase) ? (expectSequenceNumberSend - SendBase) : (expectSequenceNumberSend + 8 - SendBase);
	for (int i = 0; i < win_len; i++) {
		pUtils->printPacket("重发报文+1", this->win[i]);
		pns->sendToNetworkLayer(RECEIVER, this->win[i]);
	}
	return;
}

