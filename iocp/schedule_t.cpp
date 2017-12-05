#include "schedule_t.h"
#include <Windows.h>
#include <algorithm>
#include "thread_t.h"

schedule_t::schedule_t()
	:ctx_(nullptr)
{
	ctx_ = ::ConvertThreadToFiber(nullptr);
}


schedule_t::~schedule_t()
{
}

void schedule_t::switch_context(thread_t &&td)
{
	context_t ctx = td.ctx_;
	waiting_list_.push_back(std::move(td));
	::SwitchToFiber(ctx);
}

void schedule_t::add_to_running(thread_t && td)
{
	running_list_.push_back(std::move(td));
}

void schedule_t::add_to_waiting(thread_t && td)
{
	waiting_list_.push_back(std::move(td));
}

void schedule_t::add_to_idle(thread_t && td)
{
	idle_list_.push_back(std::move(td));
}

void schedule_t::schedule()
{
	std::for_each(running_list_.begin(), running_list_.end(), [](thread_t& td) {
		::SwitchToFiber(td.ctx_);
	});
	std::for_each(waiting_list_.begin(), waiting_list_.end(), [](thread_t& td) {
		::SwitchToFiber(td.ctx_);
	});
}
