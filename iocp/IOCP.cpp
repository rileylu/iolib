#include "IOCP.h"

IOCP::IOCP(Fun f, int num)
	:iocp_(INVALID_HANDLE_VALUE), io_fun_(f)
{
	iocp_ = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, num);
	for (int i = 0; i < num; ++i)
	{
		HANDLE td = (HANDLE)::_beginthreadex(nullptr, 0, &IOCP::fun, &io_fun_, NULL, 0);
		tds_.push_back(td);
	}
}

void IOCP::stop()
{
	::WaitForMultipleObjects(tds_.size(), tds_.data(), TRUE, INFINITE);
}


IOCP::~IOCP()
{
	stop();
	for (auto &td : tds_)
		CloseHandle(td);
	CloseHandle(iocp_);
}

unsigned __stdcall IOCP::fun(PVOID args)
{
	Fun* f = reinterpret_cast<Fun*>(args);
	(*f)();
	return 0;
}
