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

//��chrome�������һ����ҳʱ������������ҳ�����⣬���ᷢ������һ������ /favicon.ico������ȸ��������ͼ��
//���Serverʵ���ϴ��˶���ỰSocket����Ҫһ��List��������Щ�ỰSocket
//����򿪵ĵ�ǰ�ĻỰsocket

list<SOCKET> sessionSockets;
string rootDirectory = "../serverRoot";

string parseHttpRequest(const string& request) {
	// �ָ������к�ͷ��
	size_t firstLineEnd = request.find("\r\n");
	if (firstLineEnd == string::npos) {
		throw "�����ĸ�ʽ����\r\n\r\n";
	}
	string requestLine = request.substr(0, firstLineEnd);
	cout << "Request Line: " << requestLine << endl;

	// �ָ����󷽷���URI
	size_t methodEnd = requestLine.find(" ");
	if (methodEnd == string::npos) {
		throw "�����ĸ�ʽ����\r\n\r\n";
	}
	string method = requestLine.substr(0, methodEnd);
	string uriAndVersion = requestLine.substr(methodEnd + 1);

	size_t uriEnd = uriAndVersion.find(" ");
	string url = uriAndVersion.substr(0, uriEnd);
	//cout << "Requested URI: " << url << endl;

	return url;
	// �����Ҫ��һ������URI�еĲ����ȣ�����ʹ����������
}

// ���ڽ�HTTP�����URIӳ�䵽�ļ�ϵͳ·���ĺ���
string uriToFilepath(const string& uri) {
	// ȷ��URI��"/"��ͷ
	if (uri[0] != '/') {
		throw "����url��ʽ����\r\n\r\n";
	}

	// �Ƴ�URIĩβ��"/"
	string path = rootDirectory + uri;
	//path += uri.substr(1); // ������һ��

	return path;
}

// ���ڶ�ȡ�ļ����ݲ����صĺ���
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

//ȷ�����������
std::string contentType(const std::string& filepath) {
	// ��ȡ�ļ���չ��
	size_t dotPos = filepath.rfind('.');
	if (dotPos == std::string::npos) {
		return "application/octet-stream";  // Ĭ������
	}

	std::string extension = filepath.substr(dotPos + 1);
	//std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

	// ӳ����չ����MIME����
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
	// ��Ӹ�����չ��������ӳ��

	return "application/octet-stream";  // δ֪����
}

// ���ڴ���HTTP����ĺ���
string handleRequest(const string& request) {
	// ���������Ի�ȡURI
	string uri = parseHttpRequest(request);

	// ӳ��URI���ļ�ϵͳ·��
	string filepath = uriToFilepath(uri);

	cout << "loading from server filepath:" + filepath << endl;

	// ����ļ��Ƿ����
	if (_access(filepath.c_str(), 0) == -1) {
		// �ļ�������
		//throw "HTTP/1.1 404 not Found\r\n\r\n";
		return "HTTP/1.1 404 Not Found\r\n\r\n";
	}

	// ��ȡ�ļ�����
	string content = readFile(filepath);

	if (content.empty()) {
		// �ļ����ڵ��޷���ȡ��������Ȩ������
		//throw "HTTP/1.1 403 Forbidden\r\n\r\n";
		return "HTTP/1.1 403 Forbidden\r\n\r\n";
	}

	// ����HTTP��Ӧ
	//string httpResponse = "HTTP/1.1 200 OK\r\nContent-Length: " + to_string(content.length()) + "\r\n\r\n" + content;
	return content;
}

void main() {
	WSADATA wsaData;
	/*
		select()�������ṩ��fd_set�����ݽṹ��ʵ������long���͵����飬
		ÿһ������Ԫ�ض�����һ�򿪵��ļ������������socket��������������ļ��������ܵ����豸�����������ϵ��������ϵ�Ĺ����ɳ���Ա���.
		������select()ʱ�����ں˸���IO״̬�޸�fd_set�����ݣ��ɴ���ִ֪ͨ����select()�Ľ����ĸ�socket���ļ���������˿ɶ����д�¼���
	*/
	fd_set rfds;				//���ڼ��socket�Ƿ������ݵ����ĵ��ļ�������������socket������ģʽ�µȴ������¼�֪ͨ�������ݵ�����
	fd_set wfds;				//���ڼ��socket�Ƿ���Է��͵��ļ�������������socket������ģʽ�µȴ������¼�֪ͨ�����Է������ݣ�
	//bool first_connetion = true;

	int nRc = WSAStartup(0x0202, &wsaData);

	if (nRc) {
		printf("Winsock  startup failed with error!\n");
	}

	if (wsaData.wVersion != 0x0202) {
		printf("Winsock version is not correct!\n");
	}

	printf("Winsock  startup Ok!\n");


	//����socket
	SOCKET srvSocket;

	//��������ַ�Ϳͻ��˵�ַ
	sockaddr_in addr, clientAddr;

	//�Ựsocket�������client����ͨ��
	SOCKET sessionSocket;

	//ip��ַ����
	int addrLen;

	//�û������ַ����
	char ipAddress[16];
	int port;

	//�������Ľ����õ�url
	string url;

	//ȷ�������ļ���������Ϣ
	string content_Type;

	//��������socket
	srvSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (srvSocket != INVALID_SOCKET)
		printf("Socket create Ok!\n");

	// ��ȡ�û������IP��ַ�Ͷ˿ں�
	cout << "Please enter the IP address to bind: ";
	cin.getline(ipAddress, 16);
	cout << "Please enter the port to bind: ";
	cin >> port;

	// ���ַ���IP��ַת��Ϊ�����ֽ���
	unsigned long ipAddr = inet_addr(ipAddress);
	if (ipAddr == INADDR_NONE) {
		cout << "Invalid IP address format." << endl;
		return;
	}

	//���÷������Ķ˿ں͵�ַ
	addr.sin_family = AF_INET;
	addr.sin_port = htons(unsigned short(port));
	addr.sin_addr.S_un.S_addr = htonl(ipAddr);

	//binding
	int rtn = bind(srvSocket, (LPSOCKADDR)&addr, sizeof(addr));
	if (rtn != SOCKET_ERROR)
		printf("Socket bind Ok!\n");
	

	//����
	rtn = listen(srvSocket, 5);
	if (rtn != SOCKET_ERROR)
		printf("Socket listen Ok!\n");

	clientAddr.sin_family = AF_INET;
	addrLen = sizeof(clientAddr);

	//���ý��ջ�����
	char recvBuf[4096];

	u_long blockMode = 1;//��srvSock��Ϊ������ģʽ�Լ����ͻ���������

	//����ioctlsocket����srvSocket��Ϊ������ģʽ���ĳɷ������fd_setԪ�ص�״̬����ÿ��Ԫ�ض�Ӧ�ľ���Ƿ�ɶ����д
	if ((rtn = ioctlsocket(srvSocket, FIONBIO, &blockMode) == SOCKET_ERROR)) { //FIONBIO��������ֹ�׽ӿ�s�ķ�����ģʽ��
		cout << "ioctlsocket() failed with error!\n";
		return;
	}
	cout << "ioctlsocket() for server socket ok!	Waiting for client connection\n";

	while (true) {
		//���rfds��wfds����
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);

		//��srvSocket����rfds����
		//�������ͻ�������������ʱ��rfds������srvSocket��Ӧ�ĵ�״̬Ϊ�ɶ�
		//��������������þ��ǣ����õȴ��ͻ���������
		FD_SET(srvSocket, &rfds);

		for (list<SOCKET>::iterator itor = sessionSockets.begin(); itor != sessionSockets.end(); itor++) {
			//��sessionSockets���ÿ���Ựsocket����rfds�����wfds����
			//�������ͻ��˷������ݹ���ʱ��rfds������sessionSocket�Ķ�Ӧ��״̬Ϊ�ɶ��������Է������ݵ��ͻ���ʱ��wfds������sessionSocket�Ķ�Ӧ��״̬Ϊ��д
			//�����������������þ��ǣ�
			//���õȴ����лỰSOKCET�ɽ������ݻ�ɷ�������
			FD_SET(*itor, &rfds);
			FD_SET(*itor, &wfds);
		}

		/*
			select����ԭ������Ҫ�������ļ����������ϣ��ɶ�����д�����쳣����ʼ������select��������״̬��
			���пɶ�д�¼����������õĵȴ�ʱ��timeout���˾ͻ᷵�أ�����֮ǰ�Զ�ȥ�����������¼��������ļ�������������ʱ�������¼��������ļ����������ϡ�
			��select�����ļ��ϲ�û�и����û������а����ļ����������ļ�����������Ҫ�û��������б�������(ͨ��FD_ISSET���ÿ�������״̬)��
		*/
		//��ʼ�ȴ����ȴ�rfds���Ƿ��������¼���wfds���Ƿ��п�д�¼�
		//The select function returns the total number of socket handles that are ready and contained in the fd_set structure
		//�����ܹ����Զ���д�ľ������
		int nTotal = select(0, &rfds, &wfds, NULL, NULL);

		//���srvSock�յ��������󣬽��ܿͻ���������
		if (FD_ISSET(srvSocket, &rfds)) {
			nTotal--;   //��Ϊ�ͻ���������Ҳ��ɶ��¼������-1��ʣ�µľ��������пɶ��¼��ľ�����������ж��ٸ�socket�յ������ݣ�

			//�����µĻỰSOCKET
			sessionSocket = accept(srvSocket, (LPSOCKADDR)&clientAddr, &addrLen);
			if (sessionSocket != INVALID_SOCKET)
				printf("Socket listen one client request!\n");

			//�ѻỰSOCKET��Ϊ������ģʽ
			if ((rtn = ioctlsocket(sessionSocket, FIONBIO, &blockMode) == SOCKET_ERROR)) { //FIONBIO��������ֹ�׽ӿ�s�ķ�����ģʽ��
				cout << "ioctlsocket() failed with error!\n";
				return;
			}
			cout << "ioctlsocket() for session socket ok!	Waiting for client data\n";

			//���õȴ��ỰSOKCET�ɽ������ݻ�ɷ�������
			FD_SET(sessionSocket, &rfds);
			FD_SET(sessionSocket, &wfds);

			//���µĻỰsocket����Ựsocket�б�
			sessionSockets.push_back(sessionSocket);
		}

		//���ỰSOCKET�Ƿ������ݵ���
		if (nTotal > 0) { //������пɶ��¼���˵���ǻỰsocket�����ݵ���
			//�������лỰsocket������Ựsocket�����ݵ���������ܿͻ�������
			for (list<SOCKET>::iterator itor = sessionSockets.begin(); itor != sessionSockets.end(); itor++) {
				if (FD_ISSET(*itor, &rfds)) { //����������ĵ�ǰsocket�����ݵ���
					//receiving data from client
					memset(recvBuf, '\0', 4096);
					rtn = recv(*itor, recvBuf, 4096, 0);
					if (rtn > 0) {
						// ��ȡ�ͻ���IP��ַ�Ͷ˿ں�
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
						//�����ӵģ�2022-10-26
						//�������������Ӧ������������FD_ISSET�жϱ������ĵ�ǰsocket�Ƿ���Է���������
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
						//���Է���404
						//string content = "sldfljdfjhjfjkjdkfhg"; //���ʲô
						//string resp;
						//resp.append("HTTP/1.1 404 Not Found\r\n");    //��һ��״̬���״̬����
						//resp.append("Server: VerySimpleServer\r\n");
						//resp.append("Content-Type:  text/html; charset=ISO-8859-1\r\n");
						////resp.append("Content-Length: ").append(to_string(content.length())).append("\r\n");
						//resp.append("Content-Length: ").append("0").append("\r\n"); //��ʹ����404״̬�룬ҲҪָ��Content-Length������0
						//resp.append("\r\n");
						//resp.append(content);  //����Content-Length��0������content��ʲô�Ѿ�����Ҫ��

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
