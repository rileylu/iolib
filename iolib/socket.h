#pragma once
//#include "common.h"
//#include "istream_t.h"
//#include "thread_t.h"
//
//class schedule_t;
//
//class socket_t :public istream_t
//{
//public:
//	socket_t(thread_t& belong_to, int af, int type, int protocol);
//	int connect(sockaddr* addr, int addr_len);
//	int read(void* buf, int len) override;
//	int write(const void* buf, int len) override;
//	virtual ~socket_t();
//private:
//	friend class schedule_t;
//	schedule_t* env_;
//	SOCKET sock_;
//	PER_IO_DATA iodata_;
//	thread_t& belong_to_;
//
//};

#include "noncopyable.h"
#include "common.h"
#include <memory>
class schedule;
namespace iolib
{
	class socket

	{
	public:
		socket(int af, int type, int protocol);
		int connect(sockaddr* addr, int addr_len);
		int read(void* buf, int len);
		int write(const void*buf, int len);
		int accept(SOCKET listen);
		class socket_t
			:noncopyable
		{
		public:
			socket_t(int af, int type, int protocol);
			int connect(sockaddr* addr, int addr_len);
			int read(void* buf, int len);
			int write(const void*buf, int len);
			int accept(SOCKET listen);
			~socket_t();
		private:
			friend class socket;
			schedule & sche_;
			SOCKET sock_;
			io_data iodata_;
		};
	private:
		std::unique_ptr<socket_t> impl_;
	};

}
