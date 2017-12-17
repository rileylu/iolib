#include <common.h>
#include <schedule.h>
#include <singleton.h>
#include <thread.h>
#include <memory>
#include <iostream>
#include "file.h"

int main()
{
	std::unique_ptr<schedule> sche(singleton<schedule>::get_instance());
	auto fun = [] {
		std::string fn(std::to_string(reinterpret_cast<long long>(GetCurrentFiber())));
		std::string buf("hello world");
		fn.append(".html");
		iolib::file f;
		f.open(fn.c_str(), GENERIC_WRITE, CREATE_ALWAYS);
		f.write(buf.data(), buf.size());
	};
	std::vector<thread> tds;
	for (int i = 0; i < 100000; ++i)
		tds.emplace_back(fun);
	std::for_each(tds.begin(), tds.end(), [](thread& t)
	{
		t.join();
	});
	return 0;
}
