#include "fiberenv_t.h"



fiberenv_t::fiberenv_t()
	:iocp_(INVALID_HANDLE_VALUE)
{
	iocp_ = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
}


fiberenv_t::~fiberenv_t()
{
}
