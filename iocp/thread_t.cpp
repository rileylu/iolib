#include "thread_t.h"
#include "schedule_t.h"
#include <Windows.h>

thread_t::thread_t(schedule_t& env, const std::function<void()>& fun)
	:id_(env.gen_thread_id())
	, sche_(env)
	, start_(fun)
	, is_finished_(false)
	, ctx_(nullptr)
{
	ctx_ = ::CreateFiber(0, &thread_t::fiber_proc_, this);
	sche_.add_to_running(std::move(*this));
	sche_.thread_count_++;
	::SwitchToFiber(sche_.ctx_);
}

thread_t::thread_t(schedule_t& env, std::function<void()>&& fun)
	:id_(env.gen_thread_id())
	, sche_(env)
	, start_(std::move(fun))
	, is_finished_(false)
	, ctx_(nullptr)
{
	ctx_ = ::CreateFiber(0, &thread_t::fiber_proc_, this);
	sche_.add_to_running(std::move(*this));
	sche_.thread_count_++;
	::SwitchToFiber(sche_.ctx_);
}

void thread_t::join()
{
	//while (!is_finished_)
	//	sche_.switch_context(std::move(*this));
}
void thread_t::detach()
{
}
void thread_t::fiber_proc_(void* param)
{
	thread_t *td_ptr = reinterpret_cast<thread_t*>(param);
	td_ptr->start_();
	td_ptr->is_finished_ = true;
	td_ptr->sche_.add_to_idle(std::move(*td_ptr));
}

