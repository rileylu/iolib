#pragma once
#include "common.h"
#include "singleton.h"
#include "noncopyable.h"
#include <functional>
#include <memory>
#include <list>
class schedule;
class thread
{
public:
	thread(const std::function<void()>& fun);
	thread(std::function<void()>&& fun);
	void join();

	~thread() = default;
private:
	class thread_t
		:noncopyable
	{
	public:
		void join();
		void detach();
		int get_id()
		{
			return id_;
		}
		thread_t(const std::function<void()>& fun);
		thread_t(std::function<void()>&& fun);
		thread_t(thread_t&& other)
			:sche_(other.sche_)
			, id_(other.id_)
			, is_finished_(other.is_finished_)
			, fun_(std::move(other.fun_))
			, ctx_(other.ctx_)
		{

		}
		thread_t& operator=(thread_t&& other);
		~thread_t();
	private:
		friend class schedule;
		static void fiber_start_(void* p);
	private:
		schedule & sche_;
		int id_;
		bool is_finished_;
		std::function<void()> fun_;
		std::list<std::weak_ptr<thread_t>> wait_list_;
		io_data iodata_;
		void* ctx_;
	};
	friend class schedule;
	std::weak_ptr<thread_t> imp_;
};

