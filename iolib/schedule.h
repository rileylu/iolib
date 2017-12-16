#pragma once
#include "common.h"
#include "thread.h"
#include <vector>
#include <list>
#include <map>
#include <memory>
#include <algorithm>
class schedule
{
public:
	struct task
	{
		thread::thread_t* td_;
		io_data* iodata_;
	};
	schedule();
	~schedule();
	void *get_ctx() const
	{
		return ctx_;
	}
	void run();
	int get_id()
	{
		return td_id_;
	}
	std::shared_ptr<thread::thread_t> create_thread(const std::function<void()>& fun);
	std::shared_ptr<thread::thread_t> get_thread(PVOID id)
	{
		return all_thread_[id];
	}

	void add_to_running(PVOID id)
	{
		running_list_.push_back(id);
	}

	void add_to_idle(PVOID id)
	{
		idle_list_.push_back(id);
	}
	void add_to_io(PVOID id, thread::thread_t* td)
	{
		io_list_[id] = td;
	}

	void decrement_td_count()
	{
		--td_count_;
	}
	void remove_thread(PVOID id)
	{
		all_thread_.erase(id);
	}

	void commit_package(std::weak_ptr<thread::thread_t> td)
	{
		ZeroMemory(&td.lock()->iodata_, sizeof(io_data));
		td.lock()->iodata_.type_ = io_data::OTHER;
		PostQueuedCompletionStatus(iocp_, 0, (ULONG_PTR)td.lock().get(), (LPOVERLAPPED)&td.lock()->iodata_);
	}

	HANDLE get_iocp_hd() const
	{
		return iocp_;
	}

private:
	static unsigned __stdcall io_workers_(void* p);

	int td_count_;
	int td_id_;
	HANDLE iocp_;
	std::vector<HANDLE> io_tds_;

	std::list<thread::thread_t*> tasks_;
	CRITICAL_SECTION cs_;
	CONDITION_VARIABLE cv_;

	std::list<PVOID> running_list_;
	//std::list<PVOID> io_list_;
	std::list<PVOID> idle_list_;
	std::map<PVOID, thread::thread_t*> io_list_;
	std::map<PVOID, std::shared_ptr<thread::thread_t>> all_thread_;
	io_data term_;
	void* ctx_;
};
