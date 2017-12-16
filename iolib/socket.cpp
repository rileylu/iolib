#include "socket.h"
#include "singleton.h"
#include "schedule.h"
namespace iolib
{
	socket::socket(int af, int type, int protocol)
		:impl_(std::make_unique<socket_t>(af, type, protocol))
	{
	}

	int socket::connect(sockaddr* addr, int addr_len)
	{
		return impl_->connect(addr, addr_len);
	}

	int socket::read(void* buf, int len)
	{
		return impl_->read(buf, len);
	}

	int socket::write(const void* buf, int len)
	{
		return impl_->write(buf, len);
	}


	socket::socket_t::socket_t(int af, int type, int protocol)
		: sche_(*(singleton<schedule>::get_instance()))
		, sock_(INVALID_SOCKET)
	{
		ZeroMemory(&iodata_, sizeof(iodata_));
		sock_ = ::WSASocketW(af, type, protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
		bool v = true;
		::setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, (char*)&v, sizeof(v));
		::setsockopt(sock_, IPPROTO_TCP, TCP_NODELAY, (char*)&v, sizeof(v));
		if (sock_ == INVALID_SOCKET)
		{
			perror("socket_t constructor");
		}
		::CreateIoCompletionPort((HANDLE)sock_, sche_.get_iocp_hd(), (ULONG_PTR)(sche_.get_thread(GetCurrentFiber()).get()), 0);
	}

	int socket::socket_t::connect(sockaddr* addr, int addr_len)
	{
		DWORD dwBytes;
		SOCKADDR_IN local;
		ZeroMemory(&local, sizeof(local));
		local.sin_family = AF_INET;
		local.sin_addr.S_un.S_addr = INADDR_ANY;
		local.sin_port = 0;
		GUID guid = WSAID_CONNECTEX;
		LPFN_CONNECTEX connectEx;
		WSAIoctl(sock_
			, SIO_GET_EXTENSION_FUNCTION_POINTER
			, &guid
			, sizeof(guid)
			, &connectEx
			, sizeof(connectEx)
			, &dwBytes
			, NULL
			, NULL);
		ZeroMemory(&iodata_, sizeof(iodata_));
		::bind(sock_, (sockaddr*)&local, sizeof(local));
		char c;
		iodata_.wsabuf_.buf = &c;
		iodata_.wsabuf_.len = 1;
		connectEx(sock_, addr, addr_len, NULL, 0, &dwBytes, (LPOVERLAPPED)&iodata_);
		sche_.add_to_io(::GetCurrentFiber(), sche_.get_thread(::GetCurrentFiber()).get());
		::SwitchToFiber(sche_.get_ctx());
		::setsockopt(sock_, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
		int seconds, bytes;
		bytes = sizeof(seconds);
		int res = ::getsockopt(sock_, SOL_SOCKET, SO_CONNECT_TIME, (char*)&seconds, (PINT)&bytes);
		if (res != NO_ERROR)
		{
			perror("socket_t connect");
			return -1;
		}
		else
		{
			if (seconds == 0xffffffff)
			{
				perror("socket_t connect");
				return -1;
			}
		}
		return 0;
	}

	int socket::socket_t::read(void* buf, int len)
	{
		ZeroMemory(&iodata_, sizeof(iodata_));
		iodata_.wsabuf_.buf = (char*)buf;
		iodata_.wsabuf_.len = len;
		DWORD flags = 0;
		int res = ::WSARecv(sock_, &iodata_.wsabuf_, 1, 0, &flags, (LPOVERLAPPED)&iodata_, NULL);
		sche_.add_to_io(::GetCurrentFiber(), sche_.get_thread(::GetCurrentFiber()).get());
		SwitchToFiber(sche_.get_ctx());
		if (res < 0 && GetLastError() != WSA_IO_PENDING)
		{
			perror("socket_t read");
		}
		return iodata_.bytes_transferred;
	}

	int socket::socket_t::write(const void* buf, int len)
	{
		ZeroMemory(&iodata_, sizeof(iodata_));
		iodata_.wsabuf_.buf = (char*)buf;
		iodata_.wsabuf_.len = len;
		int res = WSASend(sock_, &iodata_.wsabuf_, 1, 0, 0, &iodata_, nullptr);
		sche_.add_to_io(::GetCurrentFiber(), sche_.get_thread(::GetCurrentFiber()).get());
		SwitchToFiber(sche_.get_ctx());
		return iodata_.bytes_transferred;
	}

	socket::socket_t::~socket_t()
	{
		if (sock_ != INVALID_SOCKET)
		{
			::shutdown(sock_, SD_BOTH);
			::closesocket(sock_);
		}
	}


}
