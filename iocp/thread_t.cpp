#include "thread_t.h"
#include "schedule_t.h"
#include <Windows.h>
#include <algorithm>

thread_t::thread_t(void* p)
	:sche_(schedule_t::get_sche())
	, id_(0)
	, start_([] {})
	, is_finished_(false)
	, ctx_(p)
{}



thread_t::thread_t(const std::function<void()>& fun)
	: sche_(schedule_t::get_sche())
	, id_(sche_->gen_thread_id())
	, start_(fun)
	, is_finished_(false)
	, ctx_(nullptr)
{
	sche_->thread_count_++;
	ctx_ = ::CreateFiber(0, &thread_t::fiber_proc_, this);
	::SwitchToFiber(ctx_);
}

thread_t::thread_t(std::function<void()>&& fun)
	: sche_(schedule_t::get_sche())
	, id_(sche_->gen_thread_id())
	, start_(std::move(fun))
	, is_finished_(false)
	, ctx_(nullptr)
{
	sche_->thread_count_++;
	ctx_ = ::CreateFiber(0, &thread_t::fiber_proc_, this);
	::SwitchToFiber(ctx_);
}

void thread_t::join()
{
	while (!is_finished_)
	{
		auto pos = std::find_if(sche_->thread_pools_.cbegin(), sche_->thread_pools_.cend(), [](const std::pair<const int, thread_t>& tp) {
			return tp.second.ctx_ == GetCurrentFiber();
		});
		sche_->add_to_wait(pos->first);
		::SwitchToFiber(sche_->ctx_);
	}
	//while (!is_finished_)
	//	sche_.switch_context(std::move(*this));
}
void thread_t::detach()
{
}
void thread_t::fiber_proc_(void* param)
{
	thread_t *td_ptr = reinterpret_cast<thread_t*>(param);
	int id = td_ptr->id_;
	schedule_t* sche = schedule_t::get_sche();
	sche->thread_pools_[id] = std::move(*td_ptr);
	sche->add_to_running(id);
	sche->thread_pools_[id].start_();
	sche->thread_pools_[id].is_finished_ = true;
	sche->thread_count_--;
	sche->add_to_idle(id);
	::SwitchToFiber(sche->ctx_);
}

