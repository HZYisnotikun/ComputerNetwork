#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "winsock2.h"
#include <ws2tcpip.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <list>
#include <fstream>
#include <sstream>
#include <io.h>
using namespace std;

#pragma comment(lib,"ws2_32.lib")

//当chrome浏览器打开一个网页时，除了请求网页本身外，还会发出另外一个请求 /favicon.ico，请求谷歌浏览器的图标
//因此Server实际上打开了多个会话Socket，需要一个List来保存这些会话Socket
//保存打开的当前的会话socket

list<SOCKET> sessionSockets;
string rootDirectory = "../serverRoot";

string parseHttpRequest(const string& request) {
	// 分割请求行和头部
	size_t firstLineEnd = request.find("\r\n");
	if (firstLineEnd == string::npos) {
		throw "请求报文格式错误！\r\n\r\n";
	}
	string requestLine = request.substr(0, firstLineEnd);
	cout << "Request Line: " << requestLine << endl;

	// 分割请求方法和URI
	size_t methodEnd = requestLine.find(" ");
	if (methodEnd == string::npos) {
		throw "请求报文格式错误！\r\n\r\n";
	}
	string method = requestLine.substr(0, methodEnd);
	string uriAndVersion = requestLine.substr(methodEnd + 1);

	size_t uriEnd = uriAndVersion.find(" ");
	string url = uriAndVersion.substr(0, uriEnd);
	//cout << "Requested URI: " << url << endl;

	return url;
	// 如果需要进一步解析URI中的参数等，可以使用其他方法
}

// 用于将HTTP请求的URI映射到文件系统路径的函数
string uriToFilepath(const string& uri) {
	// 确保URI以"/"开头
	if (uri[0] != '/') {
		throw "请求url格式错误！\r\n\r\n";
	}

	// 移除URI末尾的"/"
	string path = rootDirectory + uri;
	//path += uri.substr(1); // 添加最后一部

	return path;
}

// 用于读取文件内容并返回的函数
string readFile(const string& filepath) {
	ifstream file(filepath, ios::binary);
	if (!file.is_open()) {
		return "";
		cout << "file failed to open" << endl;
	}

	stringstream buffer;
	buffer << file.rdbuf();
	file.close();

	return buffer.str();
}

//确定请求的类型
std::string contentType(const std::string& filepath) {
	// 获取文件扩展名
	size_t dotPos = filepath.rfind('.');
	if (dotPos == std::string::npos) {
		return "application/octet-stream";  // 默认类型
	}

	std::string extension = filepath.substr(dotPos + 1);
	//std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

	// 映射扩展名到MIME类型
	if (extension == "html") {
		return "Content-Type: text/html; charset=ISO-8859-1";
	}
	else if (extension == "jpg" || extension == "jpeg") {
		return "image/jpeg";
	}
	else if (extension == "png") {
		return "image/png";
	}
	else if (extension == "css") {
		return "text/css";
	}
	else if (extension == "js") {
		return "application/javascript";
	}
	else if (extension == "txt") {
		return "text/plain";
	}
	else if (extension == "ico") {
		return "image/x-icon";
	}
	else if (extension == "gif") {
		return "image/gif";
	}
	else if (extension == "svg") {
		return "image/svg+xml";
	}
	else if (extension == "xml") {
		return "application/xml";
	}
	else if (extension == "pdf") {
		return "application/pdf";
	}
	else if (extension == "json") {
		return "application/json";
	}
	// 添加更多扩展名和类型映射

	return "application/octet-stream";  // 未知类型
}

// 用于处理HTTP请求的函数
string handleRequest(const string& request) {
	// 解析请求以获取URI
	string uri = parseHttpRequest(request);

	// 映射URI到文件系统路径
	string filepath = uriToFilepath(uri);

	cout << "loading from server filepath:" + filepath << endl;

	// 检查文件是否存在
	if (_access(filepath.c_str(), 0) == -1) {
		// 文件不存在
		//throw "HTTP/1.1 404 not Found\r\n\r\n";
		return "HTTP/1.1 404 Not Found\r\n\r\n";
	}

	// 读取文件内容
	string content = readFile(filepath);

	if (content.empty()) {
		// 文件存在但无法读取，可能是权限问题
		//throw "HTTP/1.1 403 Forbidden\r\n\r\n";
		return "HTTP/1.1 403 Forbidden\r\n\r\n";
	}

	// 返回HTTP响应
	//string httpResponse = "HTTP/1.1 200 OK\r\nContent-Length: " + to_string(content.length()) + "\r\n\r\n" + content;
	return content;
}

void main() {
	WSADATA wsaData;
	/*
		select()机制中提供的fd_set的数据结构，实际上是long类型的数组，
		每一个数组元素都能与一打开的文件句柄（不管是socket句柄，还是其他文件或命名管道或设备句柄）建立联系，建立联系的工作由程序员完成.
		当调用select()时，由内核根据IO状态修改fd_set的内容，由此来通知执行了select()的进程哪个socket或文件句柄发生了可读或可写事件。
	*/
	fd_set rfds;				//用于检查socket是否有数据到来的的文件描述符，用于socket非阻塞模式下等待网络事件通知（有数据到来）
	fd_set wfds;				//用于检查socket是否可以发送的文件描述符，用于socket非阻塞模式下等待网络事件通知（可以发送数据）
	//bool first_connetion = true;

	int nRc = WSAStartup(0x0202, &wsaData);

	if (nRc) {
		printf("Winsock  startup failed with error!\n");
	}

	if (wsaData.wVersion != 0x0202) {
		printf("Winsock version is not correct!\n");
	}

	printf("Winsock  startup Ok!\n");


	//监听socket
	SOCKET srvSocket;

	//服务器地址和客户端地址
	sockaddr_in addr, clientAddr;

	//会话socket，负责和client进程通信
	SOCKET sessionSocket;

	//ip地址长度
	int addrLen;

	//用户输入地址配置
	char ipAddress[16];
	int port;

	//从请求报文解析得到url
	string url;

	//确定请求文件的类型信息
	string content_Type;

	//创建监听socket
	srvSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (srvSocket != INVALID_SOCKET)
		printf("Socket create Ok!\n");

	// 获取用户输入的IP地址和端口号
	cout << "Please enter the IP address to bind: ";
	cin.getline(ipAddress, 16);
	cout << "Please enter the port to bind: ";
	cin >> port;

	// 将字符串IP地址转换为网络字节序
	unsigned long ipAddr = inet_addr(ipAddress);
	if (ipAddr == INADDR_NONE) {
		cout << "Invalid IP address format." << endl;
		return;
	}

	//设置服务器的端口和地址
	addr.sin_family = AF_INET;
	addr.sin_port = htons(unsigned short(port));
	addr.sin_addr.S_un.S_addr = htonl(ipAddr);

	//binding
	int rtn = bind(srvSocket, (LPSOCKADDR)&addr, sizeof(addr));
	if (rtn != SOCKET_ERROR)
		printf("Socket bind Ok!\n");
	

	//监听
	rtn = listen(srvSocket, 5);
	if (rtn != SOCKET_ERROR)
		printf("Socket listen Ok!\n");

	clientAddr.sin_family = AF_INET;
	addrLen = sizeof(clientAddr);

	//设置接收缓冲区
	char recvBuf[4096];

	u_long blockMode = 1;//将srvSock设为非阻塞模式以监听客户连接请求

	//调用ioctlsocket，将srvSocket改为非阻塞模式，改成反复检查fd_set元素的状态，看每个元素对应的句柄是否可读或可写
	if ((rtn = ioctlsocket(srvSocket, FIONBIO, &blockMode) == SOCKET_ERROR)) { //FIONBIO：允许或禁止套接口s的非阻塞模式。
		cout << "ioctlsocket() failed with error!\n";
		return;
	}
	cout << "ioctlsocket() for server socket ok!	Waiting for client connection\n";

	while (true) {
		//清空rfds和wfds数组
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);

		//将srvSocket加入rfds数组
		//即：当客户端连接请求到来时，rfds数组里srvSocket对应的的状态为可读
		//因此这条语句的作用就是：设置等待客户连接请求
		FD_SET(srvSocket, &rfds);

		for (list<SOCKET>::iterator itor = sessionSockets.begin(); itor != sessionSockets.end(); itor++) {
			//将sessionSockets里的每个会话socket加入rfds数组和wfds数组
			//即：当客户端发送数据过来时，rfds数组里sessionSocket的对应的状态为可读；当可以发送数据到客户端时，wfds数组里sessionSocket的对应的状态为可写
			//因此下面二条语句的作用就是：
			//设置等待所有会话SOKCET可接受数据或可发送数据
			FD_SET(*itor, &rfds);
			FD_SET(*itor, &wfds);
		}

		/*
			select工作原理：传入要监听的文件描述符集合（可读、可写，有异常）开始监听，select处于阻塞状态。
			当有可读写事件发生或设置的等待时间timeout到了就会返回，返回之前自动去除集合中无事件发生的文件描述符，返回时传出有事件发生的文件描述符集合。
			但select传出的集合并没有告诉用户集合中包括哪几个就绪的文件描述符，需要用户后续进行遍历操作(通过FD_ISSET检查每个句柄的状态)。
		*/
		//开始等待，等待rfds里是否有输入事件，wfds里是否有可写事件
		//The select function returns the total number of socket handles that are ready and contained in the fd_set structure
		//返回总共可以读或写的句柄个数
		int nTotal = select(0, &rfds, &wfds, NULL, NULL);

		//如果srvSock收到连接请求，接受客户连接请求
		if (FD_ISSET(srvSocket, &rfds)) {
			nTotal--;   //因为客户端请求到来也算可读事件，因此-1，剩下的就是真正有可读事件的句柄个数（即有多少个socket收到了数据）

			//产生新的会话SOCKET
			sessionSocket = accept(srvSocket, (LPSOCKADDR)&clientAddr, &addrLen);
			if (sessionSocket != INVALID_SOCKET)
				printf("Socket listen one client request!\n");

			//把会话SOCKET设为非阻塞模式
			if ((rtn = ioctlsocket(sessionSocket, FIONBIO, &blockMode) == SOCKET_ERROR)) { //FIONBIO：允许或禁止套接口s的非阻塞模式。
				cout << "ioctlsocket() failed with error!\n";
				return;
			}
			cout << "ioctlsocket() for session socket ok!	Waiting for client data\n";

			//设置等待会话SOKCET可接受数据或可发送数据
			FD_SET(sessionSocket, &rfds);
			FD_SET(sessionSocket, &wfds);

			//把新的会话socket加入会话socket列表
			sessionSockets.push_back(sessionSocket);
		}

		//检查会话SOCKET是否有数据到来
		if (nTotal > 0) { //如果还有可读事件，说明是会话socket有数据到来
			//遍历所有会话socket，如果会话socket有数据到来，则接受客户的数据
			for (list<SOCKET>::iterator itor = sessionSockets.begin(); itor != sessionSockets.end(); itor++) {
				if (FD_ISSET(*itor, &rfds)) { //如果遍历到的当前socket有数据到来
					//receiving data from client
					memset(recvBuf, '\0', 4096);
					rtn = recv(*itor, recvBuf, 4096, 0);
					if (rtn > 0) {
						// 获取客户端IP地址和端口号
						sockaddr_in clientAddr;
						int clientAddrSize = sizeof(clientAddr);
						if (getpeername(*itor, (sockaddr*)&clientAddr, &clientAddrSize) == 0) {
							char ClientIpAddress[INET_ADDRSTRLEN];
							inet_ntop(AF_INET, &clientAddr.sin_addr, ClientIpAddress, sizeof(ClientIpAddress));
							unsigned short port = ntohs(clientAddr.sin_port);
							cout << "Client IP: " << ClientIpAddress << ", Port: " << port << endl;
						}
						url = parseHttpRequest(recvBuf);
						content_Type = contentType(url);
						printf("Received %d bytes from client: \n%s", rtn, recvBuf);
						//新增加的：2022-10-26
						//向服务器发送响应，这里懒得用FD_ISSET判断遍历到的当前socket是否可以发送数据了
						string content;
						try {
							content = handleRequest(recvBuf);
						}
						catch (const char* e) {
							cout << "ERROR: " << e << endl;
							return;
						}
						string resp;
						resp.append("HTTP/1.1 200 OK\r\n");
						resp.append("Server: VerySimpleServer\r\n");
						resp.append("Content-Type: " + content_Type + "\r\n");
						resp.append("Content-Length: ").append(to_string(content.length())).append("\r\n");
						resp.append("\r\n");
						resp.append(content);

						//2023-10-11
						//测试返回404
						//string content = "sldfljdfjhjfjkjdkfhg"; //随便什么
						//string resp;
						//resp.append("HTTP/1.1 404 Not Found\r\n");    //第一行状态吗和状态描述
						//resp.append("Server: VerySimpleServer\r\n");
						//resp.append("Content-Type:  text/html; charset=ISO-8859-1\r\n");
						////resp.append("Content-Length: ").append(to_string(content.length())).append("\r\n");
						//resp.append("Content-Length: ").append("0").append("\r\n"); //即使返回404状态码，也要指定Content-Length，比如0
						//resp.append("\r\n");
						//resp.append(content);  //由于Content-Length是0，所以content是什么已经不重要了

						rtn = send(*itor, resp.c_str(), resp.length(), 0);
						if (rtn > 0) {
							printf("Send %d bytes to client: %s\n", rtn, resp.c_str());
						}
						else cout << "send responding gram failed" << endl;
						cout << endl << "--------------------------------------------------------" << endl;
					}
				}
			}
		}
	}
}
