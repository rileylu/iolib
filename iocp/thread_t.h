#pragma once
#include <functional>
#include "noncopyable.h"
#include <list>
#include <memory>
class schedule_t;

class thread_t
	:noncopyable
{
public:
	using context_t = void*;
	thread_t(schedule_t& sche, const std::function<void()>& fun);
	thread_t(schedule_t& sche, std::function<void()>&& fun);
	void join();
	void detach();

	thread_t(thread_t&& other) noexcept;
	thread_t& operator=(thread_t&& other) noexcept;

	~thread_t() noexcept;

	int get_id() const;
private:
	static void fiber_proc_(void* param);
	friend class schedule_t;
	int id_;
	bool is_finished_;
	schedule_t &sche_;
	std::function<void()> start_;
	std::list<thread_t>::iterator pos_;
	context_t ctx_;
};

inline int thread_t::get_id() const
{
	return id_;
}

inline thread_t::thread_t(thread_t && other) noexcept
	:id_(other.id_)
	, is_finished_(other.is_finished_)
	, sche_(other.sche_)
	, start_(std::move(other.start_))
	, pos_(other.pos_)
	, ctx_(other.ctx_)
{
}

inline thread_t& thread_t::operator=(thread_t && other) noexcept
{
	if (this != &other)
	{
		id_ = other.id_;
		is_finished_ = other.is_finished_;
		std::ref(sche_) = other.sche_;
		start_ = std::move(other.start_);
		pos_ = other.pos_;
		ctx_ = other.ctx_;
	}
	return *this;
}

inline thread_t::~thread_t() noexcept
{
}


