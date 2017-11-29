#include "fiber.h"
#include "singleton.h"
#include "FiberEnvironment.h"

namespace fiber {
	FiberEnvironment* fiberinv = nullptr;
	void fiber_init()
	{
		fiberinv = Singleton<FiberEnvironment>::get_instance(8);
	}
	ADDRINFOEX getaddrinfo(PCTSTR ip, PCTSTR port)
	{
		return fiberinv->getaddrinfo(ip, port);
	}
	SOCKET fiber_socket(int af, int type, int protocol)
	{
		return fiberinv->socket(af, type, protocol);
	}
	int fiber_connect(SOCKET s, sockaddr* addr, int len)
	{
		return fiberinv->connect(s, addr, len);
	}
	int fiber_recv(SOCKET s, void* buf, int len)
	{
		return fiberinv->recv(s, buf, len);
	}
	int fiber_send(SOCKET s, const void * buf, int len)
	{
		return fiberinv->send(s, buf, len);
	}
	HANDLE fiber_open(const char * fn, DWORD access, DWORD creationDeposition)
	{
		return fiberinv->open(fn, access, creationDeposition);
	}
	int fiber_read(HANDLE hd, void * buf, int len)
	{
		return fiberinv->read(hd, buf, len);
	}
	int fiber_write(HANDLE hd, const void * buf, int len)
	{
		return fiberinv->write(hd, buf, len);
	}
	void fiber_destory()
	{
		delete fiberinv;
	}
	void * fiber_create_thread(int stacksize, std::function<void()> fun)
	{
		return fiberinv->create_thread(stacksize, fun);
	}
}
