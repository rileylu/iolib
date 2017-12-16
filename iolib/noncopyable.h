#pragma once

class noncopyable
{
public:
	virtual ~noncopyable() = default;
	noncopyable(const noncopyable&) = delete;
	noncopyable& operator=(const noncopyable&) = delete;
protected:
	noncopyable() = default;
};