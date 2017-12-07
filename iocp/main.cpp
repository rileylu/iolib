//#include "fiber.h"
//#include <string>
//#include <memory>

#include "schedule_t.h"
#include "thread_t.h"
#include "socket_t.h"
#include <iostream>

//void fun()
//{
	//SOCKET s = fiber::fiber_socket(AF_INET, SOCK_STREAM, 0);
	//SOCKADDR_IN addr;
	//ZeroMemory(&addr, sizeof(SOCKADDR_IN));
	//addr.sin_family = AF_INET;
	//InetPton(AF_INET, L"146.222.65.65", &addr.sin_addr.S_un.S_addr);
	////InetPton(AF_INET, L"192.168.23.128", &addr.sin_addr.S_un.S_addr);
	//addr.sin_port = htons(21);
	//int res = fiber::fiber_connect(s, (sockaddr *)&addr, sizeof(addr));
	//std::unique_ptr<char[]> buf = std::make_unique<char[]>(4096);
	//res = fiber::fiber_recv(s, buf.get(), 4096);
	//std::string fn(std::to_string(reinterpret_cast<long long>(::GetCurrentFiber())));
	//fn.append("_");
	//fn.append(std::to_string(s));
	//fn.append(".txt");
	//HANDLE fd = fiber::fiber_open(fn.c_str(), GENERIC_WRITE, CREATE_ALWAYS);
	//res = fiber::fiber_write(fd, buf.get(), res);
	////::closesocket(s);
	//::CloseHandle(fd);
//}

int main()
{
	//fiber::fiber_init(4);

	//for (int i = 0; i < 1; ++i)
	//	fiber::fiber_create_thread(0, fun);
	//fiber::fiber_destory();
	schedule_t sche;
	auto fun = [] {
		socket_t s(AF_INET, SOCK_STREAM, 0);
		SOCKADDR_IN addr;
		ZeroMemory(&addr, sizeof(addr));
		addr.sin_family = AF_INET;
		::InetPton(AF_INET, L"127.0.0.1", &addr.sin_addr.S_un.S_addr);
		addr.sin_port = htons(21);
		s.connect((sockaddr*)&addr, sizeof(addr));
		char buf[1024];
		ZeroMemory(buf, sizeof(buf));
		int res = s.read(buf, sizeof(buf));

		std::cout << buf << std::endl;
	};
	thread_t t(fun);
}
