#include "common.h"
#include "thread.h"
#include "schedule.h"
#include "socket.h"
#include <iostream>
#include <memory>
#include <string>
#include "file.h"

int main()
{
	std::unique_ptr<schedule> sche(singleton<schedule>::get_instance());
	//auto fun = [] {
	//	std::cout << "hello world" << std::endl;
	//	auto fun = [] {
	//		std::cout << "inside" << std::endl;
	//	};
	//	std::vector<thread> tds;
	//	for (int i = 0; i < 1000; ++i)
	//		tds.emplace_back(fun);
	//	std::for_each(tds.begin(), tds.end(), [](thread& t) {
	//		t.join();
	//	});


	//};
	//std::vector<thread> tds;
	//for (int i = 0; i < 1000; ++i)
	//	tds.emplace_back(fun);
	//std::for_each(tds.begin(), tds.end(), [](thread& t) {
	//	t.join();
	//});
	//tds.clear();
	auto fun = []
	{
		iolib::socket t(AF_INET, SOCK_STREAM, 0);
		SOCKADDR_IN addr;
		ZeroMemory(&addr, sizeof(addr));
		addr.sin_family = AF_INET;
		//InetPton(AF_INET, L"10.211.55.3", &addr.sin_addr.S_un.S_addr);
		//addr.sin_port = htons(21);
		InetPton(AF_INET, L"10.211.55.4", &addr.sin_addr.S_un.S_addr);
		addr.sin_port = htons(80);
		int res = t.connect((SOCKADDR*)&addr, sizeof(addr));
		if (res < 0)
		{
			perror("connect");
			return;
		}

		std::string cmd = "GET /\r\n";
		res = t.write(cmd.c_str(), cmd.size());
		if (res < 0)
			return;
		char buf[4096];
		res = t.read(buf, 4096);
		if (res < 0)
			return;
		buf[res] = '\0';
		std::string fn(std::to_string(reinterpret_cast<long long>(GetCurrentFiber())));
		fn.append(".html");
		iolib::file f;
		f.open(fn.c_str(), GENERIC_WRITE, CREATE_ALWAYS);
		f.write(buf, res);
	};
	std::vector<thread> tds;
	for (int i = 0; i < 1000; ++i)
		tds.emplace_back(fun);
	std::for_each(tds.begin(), tds.end(), [](thread& td)
	{
		td.join();
	});
	getchar();
	return 0;
}
