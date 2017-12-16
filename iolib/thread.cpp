#include "thread.h"
#include "schedule.h"

thread::thread(const std::function<void()>& fun)
	:imp_(singleton<schedule>::get_instance()->create_thread(fun))
{
}

thread::thread(std::function<void()>&& fun)
	: imp_(singleton<schedule>::get_instance()->create_thread(std::move(fun)))
{
}

void thread::join()
{
	if (!imp_.expired())
		imp_.lock()->join();
}

void thread::thread_t::join()
{
	//problem
	while (!is_finished_)
	{
		if (::GetCurrentFiber() == sche_.get_ctx())
			sche_.run();
		else
		{
			auto t = sche_.get_thread(GetCurrentFiber());
			wait_list_.push_back(t);
			sche_.add_to_io(GetCurrentFiber(), t.get());
			SwitchToFiber(sche_.get_ctx());
		}
	}
}

void thread::thread_t::detach()
{
}

thread::thread_t::thread_t(const std::function<void()>& fun)
	:sche_(*(singleton<schedule>::get_instance()))
	, id_(sche_.get_id())
	, is_finished_(false)
	, fun_(fun)
	, ctx_(nullptr)
{
	ctx_ = ::CreateFiber(0, &thread::thread_t::fiber_start_, this);
	sche_.add_to_running(ctx_);
}

thread::thread_t::thread_t(std::function<void()>&& fun)
	:sche_(*(singleton<schedule>::get_instance()))
	, id_(sche_.get_id())
	, is_finished_(false)
	, fun_(std::move(fun))
	, ctx_(nullptr)
{
	ctx_ = ::CreateFiber(0, &thread::thread_t::fiber_start_, this);
	sche_.add_to_running(ctx_);
}

thread::thread_t & thread::thread_t::operator=(thread_t && other)
{
	if (this != &other)
	{
		std::ref(sche_) = other.sche_;
		id_ = other.id_;
		is_finished_ = other.is_finished_;
		fun_ = std::move(other.fun_);
		ctx_ = other.ctx_;
	}
	return *this;
}

void thread::thread_t::fiber_start_(void * p)
{
	thread_t *td = reinterpret_cast<thread_t*>(p);
	td->fun_();
	td->is_finished_ = true;
	td->sche_.add_to_idle(td->ctx_);
	std::for_each(td->wait_list_.begin(), td->wait_list_.end(), [td](std::weak_ptr<thread_t> &t) {
		if (!t.expired())
		{
			td->sche_.commit_package(t);
		}
	});
	::SwitchToFiber(td->sche_.get_ctx());
}

thread::thread_t::~thread_t()
{
	::DeleteFiber(ctx_);
}
