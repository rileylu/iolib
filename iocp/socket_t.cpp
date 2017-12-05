#include "socket_t.h"
#include "fiberenv_t.h"

socket_t::socket_t(fiberenv_t& env, int af, int type, int protocol)
	:env_(env), iodata_{ 0 }
	, sock_(INVALID_SOCKET)
{
	sock_ = ::WSASocketW(af, type, protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
	bool v = true;
	::setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, (char*)&v, sizeof(v));
	::setsockopt(sock_, IPPROTO_TCP, TCP_NODELAY, (char*)&v, sizeof(v));
	if (sock_ == INVALID_SOCKET)
	{
		perror("socket_t constructor");
	}
}

int socket_t::connect(sockaddr * addr, int addr_len)
{
	DWORD dwBytes;
	SOCKADDR_IN local;
	ZeroMemory(&local, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_addr.S_un.S_addr = INADDR_ANY;
	local.sin_port = 0;
	GUID guid = WSAID_CONNECTEX;
	LPFN_CONNECTEX connectEx;
	WSAIoctl(sock_
		, SIO_GET_EXTENSION_FUNCTION_POINTER
		, &guid
		, sizeof(guid)
		, &connectEx
		, sizeof(connectEx)
		, &dwBytes
		, NULL
		, NULL);
	ZeroMemory(&iodata_, sizeof(iodata_));
	connectEx(sock_, addr, addr_len, NULL, 0, &dwBytes, (LPOVERLAPPED)&iodata_.overlapped_);
	//env_.swtich_context();
	::setsockopt(sock_, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
	int seconds, bytes;
	bytes = sizeof(seconds);
	int res = ::getsockopt(sock_, SOL_SOCKET, SO_CONNECT_TIME, (char*)&seconds, (PINT)&bytes);
	if (res != NO_ERROR)
	{
		perror("socket_t connect");
		return -1;
	}
	else
	{
		if (seconds == 0xffffffff)
		{
			perror("socket_t connect");
			return -1;
		}
	}
	return 0;
}

int socket_t::read(void * buf, int len)
{
	ZeroMemory(&iodata_, sizeof(iodata_));
	iodata_.wsaBuf_.buf = (char*)buf;
	iodata_.wsaBuf_.len = len;
	int res = ::WSARecv(sock_, &iodata_.wsaBuf_, 1, NULL, 0, (LPOVERLAPPED)&iodata_, NULL);
	//env_.switch_context();
	if (res < 0 && GetLastError() != WSA_IO_PENDING)
	{
		perror("socket_t read");
	}
	return iodata_.bytes_transferred_;
}

int socket_t::write(const void * buf, int len)
{
	ZeroMemory(&iodata_, sizeof(iodata_));
	iodata_.wsaBuf_.buf = (char*)buf;
	iodata_.wsaBuf_.len = len;
	int res = ::WSASend(sock_, &iodata_.wsaBuf_, 1, NULL, 0, (LPOVERLAPPED)&iodata_, NULL);
	//env_.switch_context();
	if (res < 0 && GetLastError() != WSA_IO_PENDING)
	{
		perror("socket_t write");
	}
	return iodata_.bytes_transferred_;
}



socket_t::~socket_t()
{
	if (sock_ != INVALID_SOCKET)
	{
		::shutdown(sock_, SD_BOTH);
		::closesocket(sock_);
	}
}
