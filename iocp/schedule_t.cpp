#include "schedule_t.h"
#include <Windows.h>
#include <algorithm>
#include "thread_t.h"

schedule_t::schedule_t()
	:thread_count_(0)
	, thread_id_(0)
	, iocp_(INVALID_HANDLE_VALUE)
	, ctx_(nullptr)
{
	iocp_ = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
	ctx_ = ::ConvertThreadToFiber(nullptr);
}


schedule_t::~schedule_t()
{
	schedule();
}

void schedule_t::switch_context(void* p)
{
	auto pos = std::find_if(io_list_.begin(), io_list_.end(), [p](thread_t& t) {
		return t.ctx_ == p;
	});
	if (pos == io_list_.end())
		perror("switch_context");
	add_to_io(std::move(*pos));
	::SwitchToFiber(ctx_);
}

std::list<thread_t>::iterator schedule_t::add_to_running(thread_t&&  td)
{
	auto pos = running_list_.insert(running_list_.end(), std::move(td));
	return pos;
}

void schedule_t::add_to_io(thread_t&& td)
{

	auto pos = io_list_.insert(io_list_.end(), std::move(td));
	pos->pos_ = pos;
}

void schedule_t::add_to_idle(thread_t&& td)
{
	idle_list_.push_back(std::move(td));
}

void schedule_t::schedule()
{
	while (thread_count_ > 0)
	{
		std::for_each(running_list_.begin(), running_list_.end(), [](thread_t& td) {
			::SwitchToFiber(td.ctx_);
		});
		running_list_.clear();
		//std::for_each(io_list_.begin(), io_list_.end(), [](thread_t& td) {
		//	::SwitchToFiber(td.ctx_);
		//});
		if (io_list_.size() > 0)
			idle_wait();
	}
}

void schedule_t::idle_wait()
{
	ULONG_PTR completionkey;
	LPOVERLAPPED lpoverlapped;
	DWORD bytes_transferred;
	BOOL res;
	while (true)
	{
		res = ::GetQueuedCompletionStatus(iocp_, &bytes_transferred, (PULONG_PTR)&completionkey, (LPOVERLAPPED*)&lpoverlapped, INFINITE);
		if (res)
		{
			thread_t *td = reinterpret_cast<thread_t*>(completionkey);
			io_list_.erase(td->pos_);
			add_to_running(std::move(*td));
		}
		else
			continue;
	}
}
