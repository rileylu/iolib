#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <process.h>
#include <Windows.h>

#pragma comment(lib,"ws2_32.lib")


class Net
{
public:
	static Net* init_net()
	{
		if (net_ == nullptr)
			net_ = new Net;
		return net_;
	}
	~Net()
	{
		WSACleanup();
	}
private:
	Net()
	{
		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);
	}
private:
	static Net *net_;
};
