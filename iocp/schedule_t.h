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

	void add_to_running(thread_t&&  td);
	void add_to_io(thread_t&& td);
	void add_to_idle(thread_t&& td);

	void switch_context(void* p);

	int gen_thread_id();
private:
	void schedule();

	void idle_wait();
private:
	friend class thread_t;

	int thread_count_;
	int thread_id_;
	HANDLE iocp_;
	std::list<thread_t> running_list_;
	std::list<thread_t> io_list_;
	std::list<thread_t> idle_list_;
	context_t ctx_;
};

inline int schedule_t::gen_thread_id()
{
	return ++thread_id_;
}
