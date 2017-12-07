#include "schedule_t.h"
#include <Windows.h>
#include <algorithm>
#include "thread_t.h"

DWORD schedule_t::tls_;

schedule_t::schedule_t()
	:thread_count_(0)
	, thread_id_(0)
	, iocp_(INVALID_HANDLE_VALUE)
	, ctx_(nullptr)
{
	tls_ = ::TlsAlloc();
	::TlsSetValue(tls_, this);
	iocp_ = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
	ctx_ = ::ConvertThreadToFiber(nullptr);
}


schedule_t::~schedule_t()
{
	schedule();
}

void schedule_t::switch_context(void* p)
{
	auto pos = std::find_if(running_list_.begin(), running_list_.end(), [p, this](int id) {
		return thread_pools_[id].ctx_ == p;
	});
	if (pos == running_list_.end())
		perror("switch_context");
	int id = *pos;
	running_list_.erase(pos);
	io_list_.push_back(id);
	::SwitchToFiber(ctx_);
}

void schedule_t::add_to_running(int id)
{
	running_list_.push_back(id);
}

void schedule_t::add_to_io(int id)
{
	io_list_.push_back(id);
}

void schedule_t::add_to_io(void * p)
{
	//auto pp = std::find_if(running_list_.begin(), running_list_.end(), [p](thread_t &t) {
	//	return t.ctx_ == p;
	//});
	//pp = io_list_.insert(io_list_.end(), std::move(*pp));
	//pp->io_list_pos_ = pp;
	//return pp;
	auto it = std::find_if(running_list_.begin(), running_list_.end(), [p, this](int id) {
		return id == thread_pools_[id].id_;
	});
	io_list_.push_back(*it);
}

void schedule_t::add_to_idle(int id)
{
	idle_list_.push_back(id);
}

void schedule_t::schedule()
{

	while (thread_count_ > 0)
	{
		if (io_list_.size() > 0)
			idle_wait();
		std::list<int> tmp = std::move(running_list_);
		for (auto p = tmp.cbegin(); p != tmp.cend(); ++p)
			::SwitchToFiber(thread_pools_[*p].ctx_);
	}
}

void schedule_t::idle_wait()
{
	ULONG_PTR completionkey;
	LPOVERLAPPED lpoverlapped;
	DWORD bytes_transferred;
	BOOL res;
	res = ::GetQueuedCompletionStatus(iocp_, &bytes_transferred, (PULONG_PTR)&completionkey, (LPOVERLAPPED*)&lpoverlapped, INFINITE);
	if (res)
	{
		void *p = (void*)completionkey;
		auto it = find_if(io_list_.cbegin(), io_list_.cend(), [p, this](int id) {
			return thread_pools_[id].ctx_ == p;
		});
		PER_IO_DATA *iodata = reinterpret_cast<PER_IO_DATA*>(lpoverlapped);
		iodata->bytes_transferred_ = bytes_transferred;
		int id = *it;
		io_list_.erase(it);
		add_to_running(id);
	}
}
