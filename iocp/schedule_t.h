#pragma once
#include "common.h"
#include <list>
#include <map>
class thread_t;
class schedule_t
{
public:
	using context_t = void*;
	schedule_t();
	~schedule_t();

	void add_to_running(int id);
	void add_to_io(int id);
	void add_to_io(void* p);
	void add_to_wait(int id);
	void add_to_idle(int id);

	void switch_context(void* p);

	int gen_thread_id();
	HANDLE get_iocp_handle() const;
	static schedule_t* get_sche();
private:
	void schedule();

	void idle_wait();
private:
	friend class thread_t;

	int thread_count_;
	int thread_id_;
	HANDLE iocp_;
	std::list<int> running_list_;
	std::list<int> io_list_;
	std::list<int> idle_list_;
	std::list<int> wait_list_;
	std::map<const int, thread_t> thread_pools_;
	context_t ctx_;
	static DWORD tls_;
};

inline int schedule_t::gen_thread_id()
{
	return ++thread_id_;
}
inline HANDLE schedule_t::get_iocp_handle() const
{
	return iocp_;
}

inline schedule_t* schedule_t::get_sche()
{
	return (schedule_t*)TlsGetValue(tls_);
}
