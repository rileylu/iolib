#pragma once
#include "net.h"
#include <vector>
#include <list>

class thread_t;
class fiberenv_t
{
public:
	fiberenv_t();
	~fiberenv_t();
private:
	HANDLE iocp_;
	std::vector<HANDLE> io_tds_;
	std::vector<HANDLE> worker_tds_;
	std::list<thread_t> running_list_;
	std::list<thread_t> waiting_list_;
	std::list<thread_t> idle_list_;
};

