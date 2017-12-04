#pragma once
#include "net.h"
#include <vector>
#include <functional>
class IOCP
{
public:
	using Fun = std::function<void()>;
	IOCP(Fun f, int num);
	void stop();
	~IOCP();
	static unsigned __stdcall fun(PVOID args);
	HANDLE get_handle() const;
	void start();
private:
	HANDLE iocp_;
	std::vector<HANDLE> tds_;
	Fun io_fun_;
	int num_;
};

inline HANDLE IOCP::get_handle() const
{
	return iocp_;
}
