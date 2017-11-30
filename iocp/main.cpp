#include "fiber.h"
#include <string>

void fun()
{
	SOCKET s = fiber::fiber_socket(AF_INET, SOCK_STREAM, 0);
	SOCKADDR_IN addr;
	ZeroMemory(&addr, sizeof(SOCKADDR_IN));
	addr.sin_family = AF_INET;
	InetPton(AF_INET, L"127.0.0.1", &addr.sin_addr.S_un.S_addr);
	addr.sin_port = htons(21);
	int res = fiber::fiber_connect(s, (sockaddr *)&addr, sizeof(addr));
	char buf[4096];
	res = fiber::fiber_recv(s, buf, 4096);
	std::string fn(std::to_string(reinterpret_cast<long long>(::GetCurrentFiber())));
	fn.append("_");
	fn.append(std::to_string(s));
	fn.append(".txt");
	HANDLE fd = fiber::fiber_open(fn.c_str(), GENERIC_WRITE, CREATE_ALWAYS);
	res = fiber::fiber_write(fd, buf, res);
	//::closesocket(s);
	::CloseHandle(fd);
}

int main()
{
	fiber::fiber_init();
	for (int i = 0; i < 100000; ++i)
		fiber::fiber_create_thread(0, fun);
	fiber::fiber_destory();
}
