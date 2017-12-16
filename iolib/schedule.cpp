#include "schedule.h"
#include "thread.h"
#include <algorithm>

schedule::schedule()
	:td_id_(0)
	, td_count_(0)
	, iocp_(INVALID_HANDLE_VALUE)
	, ctx_(nullptr)
{
	ctx_ = ::ConvertThreadToFiber(nullptr);
	SYSTEM_INFO sys;
	ZeroMemory(&sys, sizeof(sys));
	::GetSystemInfo(&sys);
	int n = sys.dwNumberOfProcessors;
	iocp_ = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, n);
	for (int i = 0; i < (n << 1); ++i)
		io_tds_.push_back((HANDLE)::_beginthreadex(nullptr, 0, &schedule::io_workers_, this, 0, nullptr));
	InitializeCriticalSection(&cs_);
	InitializeConditionVariable(&cv_);
}

schedule::~schedule()
{
	::ConvertFiberToThread();
	::WaitForMultipleObjects(io_tds_.size(), io_tds_.data(), TRUE, INFINITE);
	std::for_each(io_tds_.cbegin(), io_tds_.cend(), [](const HANDLE& hd) {
		::CloseHandle(hd);
	});
}

unsigned schedule::io_workers_(void * p)
{
	schedule* sche = reinterpret_cast<schedule*>(p);
	DWORD bytes = 0;
	ULONG_PTR com = 0;
	LPOVERLAPPED io = nullptr;
	bool res = false;
	while (true)
	{
		res = ::GetQueuedCompletionStatus(sche->iocp_, &bytes, (PULONG_PTR)&com, (LPOVERLAPPED*)&io, INFINITE);
		if (res)
		{
			thread::thread_t* t = reinterpret_cast<thread::thread_t*>(com);
			io_data *iodata = reinterpret_cast<io_data*>(io);
			if (iodata->type_ == io_data::EXIT)
				break;
			iodata->bytes_transferred = bytes;
			EnterCriticalSection(&sche->cs_);
			sche->tasks_.push_back(t);
			WakeConditionVariable(&sche->cv_);
			LeaveCriticalSection(&sche->cs_);
		}
		else
		{
			continue;
		}
	}
	return 0;
}

void schedule::run()
{
	while (td_count_ > 0)
	{

		while (!running_list_.empty())
		{
			PVOID id = running_list_.front();
			running_list_.pop_front();
			SwitchToFiber(all_thread_[id]->ctx_);
		}
		if (!io_list_.empty())
		{
			EnterCriticalSection(&cs_);
			while (tasks_.empty() && td_count_ > 0)
				::SleepConditionVariableCS(&cv_, &cs_, 5000000);
			if (td_count_ == 0)
			{
				LeaveCriticalSection(&cs_);
				continue;
			}
			std::list<thread::thread_t*> tmp = std::move(tasks_);
			LeaveCriticalSection(&cs_);
			std::for_each(tmp.cbegin(), tmp.cend(), [this](const thread::thread_t* t) {
				//io_list_.erase(std::find(io_list_.cbegin(), io_list_.cend(), t->ctx_));
				io_list_.erase(t->ctx_);
				add_to_running(t->ctx_);
			});
		}
		//really slow
		std::list<PVOID> idle = std::move(idle_list_);
		std::for_each(idle.cbegin(), idle.cend(), [this](PVOID id) {
			//::DeleteFiber(all_thread_[id]->ctx_);
			all_thread_.erase(id);
			--td_count_;
		});
	}
	for (int i = 0; i < io_tds_.size(); ++i)
	{
		ZeroMemory(&term_, sizeof(term_));
		term_.type_ = io_data::EXIT;
		PostQueuedCompletionStatus(iocp_, 0, (ULONG_PTR)this, (LPOVERLAPPED)&term_);
	}
}

std::shared_ptr<thread::thread_t>  schedule::create_thread(const std::function<void()>& fun)
{
	++td_id_;
	auto t = std::make_unique<thread::thread_t>(fun);
	PVOID ctx = t->ctx_;
	all_thread_[ctx] = std::move(t);
	++td_count_;
	return all_thread_[ctx];
}

