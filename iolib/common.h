#pragma once
#include "net.h"
struct io_data :public OVERLAPPED
{
	enum io_type
	{
		NEW,
		END,
		EXIT,
		OTHER
	};
	io_data()
		:OVERLAPPED{ 0 }
		, wsabuf_{ 0 }
		, bytes_transferred{ 0 }
		, type_{ OTHER }
	{

	}
	WSABUF wsabuf_;
	DWORD bytes_transferred;
	io_type type_;
};
