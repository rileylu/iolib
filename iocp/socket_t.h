#pragma once
#include "common.h"
#include "istream_t.h"

class schedule_t;

class socket_t :public istream_t
{
public:
	socket_t(int af, int type, int protocol);
	int connect(sockaddr* addr, int addr_len);
	int read(void* buf, int len) override;
	int write(const void* buf, int len) override;
	virtual ~socket_t();
private:
	friend class schedule_t;
	schedule_t* env_;
	SOCKET sock_;
	PER_IO_DATA iodata_;
};

