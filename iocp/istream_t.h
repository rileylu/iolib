#pragma once

class istream_t
{
public:
	virtual ~istream_t() = default;
	virtual int read(void* buf, int len) = 0;
	virtual int write(const void* buf, int len) = 0;
protected:
	istream_t() = default;
};