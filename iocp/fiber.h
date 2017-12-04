#pragma once
#include "net.h"
#include <functional>

namespace fiber
{
	void fiber_init(int num);

	SOCKET fiber_socket(int af, int type, int protocol);
	int fiber_connect(SOCKET s, sockaddr* addr, int len);
	int fiber_recv(SOCKET s, void* buf, int len);
	int fiber_send(SOCKET s, const void* buf, int len);

	HANDLE fiber_open(const char* fn, DWORD access, DWORD creationDeposition);

	int fiber_read(HANDLE hd, void* buf, int len);
	int fiber_write(HANDLE hd, const void* buf, int len);

	void fiber_destory();
	void* fiber_create_thread(int stacksize, std::function<void()> fun);
}
