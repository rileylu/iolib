#pragma once
#include "IOCP.h"
#include <memory>
#include <list>
#include <vector>
class FiberEnvironment
{
public:
	struct Per_IO_Data
	{
		OVERLAPPED overlapped_;
		WSABUF wsabuf_;
		int bytes_transferred;
	};
	struct Context
	{
		bool joinable_;
		int id_;
		PVOID orgin_;
		std::function<void()> fun_;
		std::unique_ptr<Per_IO_Data> io_data_;
		enum Cmd
		{
			NEW,
			OTHER
		} cmd_;
		Context() :joinable_(false), io_data_(new Per_IO_Data), cmd_(OTHER) {}
	};
	FiberEnvironment(int num = 4);
	~FiberEnvironment();

	SOCKET socket(int af, int type, int protocol);
	int connect(SOCKET s, sockaddr* addr, int addrlen);
	int recv(SOCKET s, void* buf, int size);
	int send(SOCKET s, const void* buf, int size);

	HANDLE open(const char* fn, DWORD access, DWORD creationDisposition);
	int read(HANDLE hd, void* buf, int len);
	int write(HANDLE hd, const void* buf, int len);

	void* create_thread(int stacksize, std::function<void()> fun);

private:
	void io_fun_();
	static VOID fun_(PVOID args);

private:
	IOCP *iocp_;
	int td_num_;
	static std::vector<CRITICAL_SECTION> cs_;
	static DWORD tls_;
	static DWORD joinable_tls_;
};
