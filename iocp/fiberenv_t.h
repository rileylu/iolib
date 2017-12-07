#pragma once
#include "net.h"
#include <vector>
#include <list>

class thread_t;
class fiberenv_t
{
public:
	fiberenv_t();
	~fiberenv_t();
private:
	HANDLE iocp_;
};

