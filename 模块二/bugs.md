**bug1 (in GBN)**

![image-20241021193154382](C:/Users/JOHN/AppData/Roaming/Typora/typora-user-images/image-20241021193154382.png)

原因：很多时候接收方发送的ack还没等被发送方接收 计时器就超时了 这时候需要重发前面的包 但是又会导致序号不匹配 需要一直发新的包 

在此之前接收方虽然已经发送了ack 似乎进入新的计时器周期之后 这些ack并没有被发送方接收到 于是只好一直发送重复的分组

我懂了 是因为由于接收方和发送方之间的信息并不对等 有可能接收方会收到一个非连续的ack 不过这也是正确的ack 因为接收方不接受到连续的正确分组 是不会产生新的ack的 但是我的代码里并没有接收这一ack 因为它只接受连续的ack 这是错误的

修改后的代码：

```cpp
		//如果校验和正确，确认号为sendBase，这时候要更新SendBase  ？？这里ackPkt.acknum有可能比SendBase大吗
		if (checkSum == ackPkt.checksum && ackPkt.acknum >= SendBase) {
			int movNum = ackPkt.acknum - SendBase + 1;   //可以理解成这次接收到ack 直接或间接地有多少包得到了确认
			SendBase = ackPkt.acknum + 1;
			this->waitingState = false;
			pUtils->printPacket("发送方正确收到确认", ackPkt);
			this->pktNumInWin -= movNum;
			pns->stopTimer(SENDER, this->win[0].seqnum);		//关闭定时器
			cout << "stop timer at seqnum " << win[0].seqnum << endl;
			if (this->SendBase < expectSequenceNumberSend) //此时窗口中还有包 要重启计时器
			{
				pns->startTimer(SENDER, Configuration::TIME_OUT, this->win[movNum].seqnum);
				cout << "start timer at seqnum " << win[movNum].seqnum << endl;
			}
			for (int i = 0; i < pktNumInWin; i++) {
				this->win[i] = this->win[i + movNum];  //把窗口里面的包往前移动movNum步
			}
		}
```

