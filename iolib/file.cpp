#include "file.h"
#include "singleton.h"
#include "schedule.h"

namespace iolib
{
	file::file()
		:impl_(std::make_unique<file_t>())
	{
	}


	file::~file()
	{
	}

	int file::open(const std::string& fn, DWORD access, DWORD creationDisposition)
	{
		return impl_->open(fn, access, creationDisposition);
	}

	int file::read(void* buf, int len)
	{
		return impl_->read(buf, len);
	}
	int file::write(const void* buf, int len)
	{
		return impl_->write(buf, len);
	}

	file::file_t::file_t()
		:sche_(*singleton<schedule>::get_instance())
		, fd_(INVALID_HANDLE_VALUE)
	{
	}

	file::file_t::~file_t()
	{
		::CloseHandle(fd_);
	}

	int file::file_t::open(const std::string& fn, DWORD access, DWORD creationDisposition)
	{
		fd_ = CreateFileA(fn.c_str(), access, FILE_SHARE_READ, NULL, creationDisposition, FILE_FLAG_OVERLAPPED, NULL);
		::CreateIoCompletionPort(fd_, sche_.get_iocp_hd(), (ULONG_PTR)(sche_.get_thread(GetCurrentFiber()).get()), 0);
		return 0;
	}

	int file::file_t::read(void* buf, int len)
	{
		ZeroMemory(&iodata_, sizeof(iodata_));
		iodata_.wsabuf_.buf = (char*)buf;
		iodata_.wsabuf_.len = len;
		int ret = ReadFile(fd_, buf, len, NULL, &iodata_);
		sche_.add_to_io(::GetCurrentFiber(), sche_.get_thread(::GetCurrentFiber()).get());
		::SwitchToFiber(sche_.get_ctx());
		return iodata_.bytes_transferred;
	}

	int file::file_t::write(const void* buf, int len)
	{
		ZeroMemory(&iodata_, sizeof(iodata_));
		iodata_.wsabuf_.buf = (char*)buf;
		iodata_.wsabuf_.len = len;
		int ret = WriteFile(fd_, buf, len, NULL, &iodata_);
		sche_.add_to_io(::GetCurrentFiber(), sche_.get_thread(::GetCurrentFiber()).get());
		::SwitchToFiber(sche_.get_ctx());
		return iodata_.bytes_transferred;
	}


}
