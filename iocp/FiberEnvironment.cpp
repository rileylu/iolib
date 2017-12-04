#include "FiberEnvironment.h"
#include "singleton.h"
#include <algorithm>
#include <memory>

DWORD FiberEnvironment::tls_ = TlsAlloc();
DWORD FiberEnvironment::joinable_tls_ = TlsAlloc();
FiberEnvironment::FiberEnvironment(int num)
	:iocp_(new IOCP(std::bind(&FiberEnvironment::io_fun_, this), num)),td_num_(num)
{
	iocp_->start();
}

void FiberEnvironment::io_fun_()
{
	::TlsSetValue(tls_, ::ConvertThreadToFiber(nullptr));
	::TlsSetValue(joinable_tls_, new std::list<Context*>);
	DWORD bytes_transferred;
	Per_IO_Data *per_io_data = nullptr;
	Context *context = nullptr;
	BOOL bRet = false;
	DWORD dwBytes;
	while (true)
	{
		bRet = ::GetQueuedCompletionStatus(iocp_->get_handle(), &dwBytes, (PULONG_PTR)&context, (LPOVERLAPPED*)&per_io_data, WSA_INFINITE);
		if (bRet)
		{
			switch (context->cmd_)
			{
			case FiberEnvironment::Context::NEW:
				context->cmd_ = Context::OTHER;
				::SwitchToFiber(context->orgin_);
				break;
			default:
				context->io_data_->bytes_transferred = dwBytes;
				::SwitchToFiber(context->orgin_);
				break;
			}
		}
		else
			continue;
		std::list<Context*> *l =(std::list<Context*>*)::TlsGetValue(joinable_tls_);
		if (!l->empty())
		{
			for (auto* c : *l)
			{
				::DeleteFiber(c->orgin_);
				delete c;
			}
			l->clear();
		}
	}
}

void FiberEnvironment::fun_(PVOID args)
{
	Context *context = reinterpret_cast<Context*>(args);
	context->fun_();
	std::list<Context*> *l =(std::list<Context*>*)::TlsGetValue(joinable_tls_);
	l->push_back(context);
	::SwitchToFiber(TlsGetValue(tls_));
}

FiberEnvironment::~FiberEnvironment()
{
	iocp_->stop();
}

SOCKET FiberEnvironment::socket(int af, int type, int protocol)
{
	Context* context = reinterpret_cast<Context*>(::GetFiberData());
	SOCKET s = ::WSASocketW(af, type, protocol, 0, 0, WSA_FLAG_OVERLAPPED);
	bool value = TRUE;
	setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&value, sizeof(bool));
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&value, sizeof(bool));
	::CreateIoCompletionPort((HANDLE)s, iocp_->get_handle(), (ULONG_PTR)context, 0);
	return s;
}

int FiberEnvironment::connect(SOCKET s, sockaddr * addr, int addrlen)
{
	Context* context = reinterpret_cast<Context*>(::GetFiberData());
	DWORD dwBytes;
	ADDRINFO hint, *local;
	ZeroMemory(&hint, sizeof(hint));
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_flags = AI_PASSIVE;
	::GetAddrInfoA(nullptr, "0", &hint, &local);
	::bind(s, local->ai_addr, local->ai_addrlen);
	GUID guid = WSAID_CONNECTEX;
	LPFN_CONNECTEX connectEx;
	WSAIoctl(s
		, SIO_GET_EXTENSION_FUNCTION_POINTER
		, &guid
		, sizeof(guid)
		, &connectEx
		, sizeof(connectEx)
		, &dwBytes
		, nullptr
		, nullptr);
	std::unique_ptr<char[]> buf(new char[1024]);
	context->io_data_->wsabuf_.buf = buf.get();
	context->io_data_->wsabuf_.len = 1024;
	ZeroMemory(&context->io_data_->overlapped_, sizeof(OVERLAPPED));
	//lock context
	connectEx(s, addr, addrlen, nullptr, 0, &dwBytes, reinterpret_cast<LPOVERLAPPED>(context->io_data_.get()));
	setsockopt(s, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
	freeaddrinfo(local);
	//release context
	::SwitchToFiber(TlsGetValue(tls_));
	int seconds;
	int bytes = sizeof(seconds);
	int iResult = 0;
	iResult = getsockopt(s, SOL_SOCKET, SO_CONNECT_TIME, (char*)&seconds, (PINT)&bytes);
	if (iResult != NO_ERROR)
	{
		perror("connect");
		return -1;
	}
	else
	{
		if (seconds == 0xFFFFFFFF)
		{
			perror("connect");
			return -1;
		}
	}

	return 0;
}

int FiberEnvironment::recv(SOCKET s, void * buf, int size)
{
	Context *context = reinterpret_cast<Context*>(::GetFiberData());
	context->io_data_->wsabuf_.buf = (CHAR *)buf;
	context->io_data_->wsabuf_.len = size;
	ZeroMemory(&context->io_data_->overlapped_, sizeof(OVERLAPPED));
	DWORD flags = 0;
	int ret = WSARecv(s, &context->io_data_->wsabuf_, 1, 0, &flags, (LPWSAOVERLAPPED)context->io_data_.get(), nullptr);
	::SwitchToFiber(TlsGetValue(tls_));
	return context->io_data_->bytes_transferred;
}

int FiberEnvironment::send(SOCKET s, const void * buf, int size)
{
	Context *context = reinterpret_cast<Context*>(::GetFiberData());
	context->io_data_->wsabuf_.buf = (CHAR*)buf;
	context->io_data_->wsabuf_.len = size;
	ZeroMemory(&context->io_data_->overlapped_, sizeof(OVERLAPPED));
	int ret = ::WSASend(s, &context->io_data_->wsabuf_, 1, 0, 0, (LPWSAOVERLAPPED)context->io_data_.get(), nullptr);
	::SwitchToFiber(TlsGetValue(tls_));
	return context->io_data_->bytes_transferred;
}

HANDLE FiberEnvironment::open(const char * fn, DWORD access, DWORD creationDisposition)
{
	Context* context = reinterpret_cast<Context*>(::GetFiberData());
	HANDLE hd = ::CreateFileA(fn, access, FILE_SHARE_READ, NULL, creationDisposition, FILE_FLAG_OVERLAPPED, NULL);
	::CreateIoCompletionPort(hd, iocp_->get_handle(), (ULONG_PTR)context, 0);
	return hd;
}

int FiberEnvironment::read(HANDLE hd, void * buf, int len)
{
	Context* context = reinterpret_cast<Context*>(::GetFiberData());
	context->io_data_->wsabuf_.buf = (CHAR*)buf;
	context->io_data_->wsabuf_.len = len;
	ZeroMemory(&context->io_data_->overlapped_, sizeof(OVERLAPPED));
	int ret = ::ReadFile(hd, buf, len, NULL, (LPOVERLAPPED)context->io_data_.get());
	::SwitchToFiber(TlsGetValue(tls_));
	return context->io_data_->bytes_transferred;
}

int FiberEnvironment::write(HANDLE hd, const void * buf, int len)
{
	Context* context = reinterpret_cast<Context*>(::GetFiberData());
	context->io_data_->wsabuf_.buf = (CHAR*)buf;
	context->io_data_->wsabuf_.len = len;
	ZeroMemory(&context->io_data_->overlapped_, sizeof(OVERLAPPED));
	int ret = ::WriteFile(hd, buf, len, NULL, (LPOVERLAPPED)context->io_data_.get());
	::SwitchToFiber(TlsGetValue(tls_));
	return context->io_data_->bytes_transferred;
}

void * FiberEnvironment::create_thread(int stacksize, std::function<void()> fun)
{
	Context *context = new Context;
	context->fun_ = fun;
	context->cmd_ = Context::NEW;
	context->orgin_ = ::CreateFiber(stacksize, &FiberEnvironment::fun_, context);
	PostQueuedCompletionStatus(iocp_->get_handle(), 0, (ULONG_PTR)context, 0);
	return context->orgin_;
}
