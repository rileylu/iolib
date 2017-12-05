#include "thread_t.h"
#include "schedule_t.h"
#include <Windows.h>

thread_t::thread_t(schedule_t& env, const std::function<void()>& fun)
	:sche_(env)
	, start_(fun)
	, is_finished_(false)
	, ctx_(nullptr)
{
	ctx_ = ::CreateFiber(0, &thread_t::fiber_proc_, this);
	sche_.add_to_running(std::move(*this));
}

thread_t::thread_t(schedule_t& env, std::function<void()>&& fun)
	: sche_(env)
	, start_(std::move(fun))
{
}

void thread_t::join()
{
	while (!is_finished_)
		sche_.switch_context(std::move(*this));
}

void thread_t::detach()
{

}

thread_t::thread_t(thread_t && other)
	:sche_(other.sche_)
	, start_(std::move(other.start_))
	, ctx_(other.ctx_)
{
}

thread_t & thread_t::operator=(thread_t && other)
{
	sche_ = other.sche_;
	start_ = std::move(other.start_);
	ctx_ = other.ctx_;
	return *this;
}

thread_t::~thread_t()
{
	sche_.add_to_idle(std::move(*this));
}

void thread_t::fiber_proc_(void* param)
{
	thread_t *td_ptr = reinterpret_cast<thread_t*>(param);
	td_ptr->start_();
	td_ptr->is_finished_ = true;
	td_ptr->sche_.switch_context(std::move(*td_ptr));
}
