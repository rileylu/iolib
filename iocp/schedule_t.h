#pragma once
#include "net.h"
#include <list>
class thread_t;
class schedule_t
{
public:
	using context_t = void*;
	schedule_t();
	~schedule_t();

	//void switch_context(thread_t&& td);
	void add_to_running(thread_t&& td);
	void add_to_waiting(thread_t&& td);
	void add_to_idle(thread_t&& td);

	void switch_context(void* p);

private:
	void schedule();

	void idle_wait();
private:
	int thread_count_;
	HANDLE iocp_;
	std::list<thread_t> running_list_;
	std::list<thread_t> io_list_;
	std::list<thread_t> idle_list_;
	context_t ctx_;
};

