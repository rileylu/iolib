#pragma once
#include "net.h"

struct PER_IO_DATA
{
	OVERLAPPED overlapped_;
	WSABUF wsaBuf_;
	DWORD bytes_transferred_;
};