#pragma once
#include <functional>
#include "noncopyable.h"
class schedule_t;
class thread_t :noncopyable
{
public:
	using context_t = void*;
	thread_t(schedule_t& sche, const std::function<void()>& fun);
	thread_t(schedule_t& sche, std::function<void()>&& fun);
	void join();
	void detach();

	thread_t(thread_t&& other);
	thread_t& operator=(thread_t&& other);

	~thread_t();
private:
	static void fiber_proc_(void* param);

	friend class schedule_t;
	bool is_finished_;
	schedule_t &sche_;
	std::function<void()> start_;
	context_t ctx_;
};

