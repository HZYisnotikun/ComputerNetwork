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
	//快速重传实现
	if (ackPkt.acknum == lastAck) {
		SameInRow++;
		cout << "连续" << SameInRow << "个冗余ACK" << endl;
	}
	else {
		lastAck = ackPkt.acknum;
		SameInRow = 0;
	}
	if (SameInRow == 3) {
		cout << "连续三个冗余ACK： " << ackPkt.acknum << " 触发快速重传" << endl;
		//SameInRow = 0;
		timeoutHandler(0);
		return;
	}
	if (this->pktNumInWin > 0) {//只要窗口里还有包 就进行下面的过程
		//检查校验和是否正确
		int checkSum = pUtils->calculateCheckSum(ackPkt);
		//如果校验和正确，确认号为sendBase，这时候要更新SendBase  ？？这里ackPkt.acknum有可能比SendBase大吗
		if (checkSum == ackPkt.checksum && InWindow(ackPkt.acknum)) {
			int movNum = (ackPkt.acknum - SendBase) + ((ackPkt.acknum >= SendBase) ? 1 : 9);   //可以理解成这次接收到ack 直接或间接地有多少包得到了确认
			SendBase = (ackPkt.acknum + 1) % 8;
			this->waitingState = false;
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
		}
	}
}

bool TCPRdtSender::send(const Message& message) {
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
	if ((expectSequenceNumberSend - (SendBase + windowSize)) % 8 == 0) {
		this->waitingState = true;
		cout << "the window is full!" << endl;
	}
	printWindow();
}

void TCPRdtSender::timeoutHandler(int seqnum) {
	//判断是不是快速重传
	if (SameInRow == 3)
		SameInRow = 0;
	else
		cout << "计时器超时，下面重发第一个没被确认的包" << endl;
	pns->stopTimer(SENDER, this->win[0].seqnum);  //首先关闭定时器
	cout << "stop timer at seqnum " << win[0].seqnum << endl;
	pns->startTimer(SENDER, Configuration::TIME_OUT, this->win[0].seqnum);			//重新启动发送方定时器
	cout << "start timer at seqnum " << win[0].seqnum << endl;
	pUtils->printPacket("重发报文", this->win[0]);
	pns->sendToNetworkLayer(RECEIVER, this->win[0]);
}

