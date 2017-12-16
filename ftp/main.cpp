#include <common.h>
#include <schedule.h>
#include <singleton.h>
#include <thread.h>
#include <memory>
#include <iostream>

int main()
{
	std::unique_ptr<schedule> sche(singleton<schedule>::get_instance());
	auto fun = [] {
		std::cout << "Hello world" << std::endl;
	};
	thread t(fun);
	t.join();
	return 0;
}