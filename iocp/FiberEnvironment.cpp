#include "FiberEnvironment.h"
#include "singleton.h"
#include <algorithm>
#include <memory>

IOCP* FiberEnvironment::iocp_ = nullptr;
std::vector<std::list<FiberEnvironment::Context*>> FiberEnvironment::running_context_;
std::vector<::CRITICAL_SECTION> FiberEnvironment::cs_;
std::vector<::CONDITION_VARIABLE> FiberEnvironment::cv_;
std::vector<bool> FiberEnvironment::empty_;
DWORD FiberEnvironment::tls_ = TlsAlloc();
FiberEnvironment::FiberEnvironment(int num)
	:td_num_(num), id_(0)
{
	cs_.resize(num);
	cv_.resize(num);
	empty_.resize(num);
	for (auto &t : cs_)
		InitializeCriticalSection(&t);
	for (auto &t : cv_)
		InitializeConditionVariable(&t);
	for (auto &t : empty_)
		t = true;
	InitializeCriticalSection(&idcs_);
	running_context_.resize(td_num_);
	iocp_ = Singleton<IOCP>::get_instance(std::bind(&FiberEnvironment::io_fun_, this), td_num_);
	for (int i = 0; i < td_num_; ++i)
	{
		schedule_tds_.push_back((HANDLE)::_beginthreadex(nullptr, 0, &FiberEnvironment::schedule_next_, (void*)i, 0, 0));
	}
}

void FiberEnvironment::io_fun_()
{
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
			if (context->cmd_ == FiberEnvironment::Context::NEW)
			{
				EnterCriticalSection(&idcs_);
				int id = id_++ % td_num_;
				LeaveCriticalSection(&idcs_);
				context->id_ = id;
			}
			::EnterCriticalSection(&cs_[context->id_]);
			running_context_[context->id_].emplace_back(context);
			empty_[context->id_] = false;
			::LeaveCriticalSection(&cs_[context->id_]);
			::WakeConditionVariable(&cv_[context->id_]);
		}
		else
			continue;
	}
}

unsigned __stdcall FiberEnvironment::schedule_next_(PVOID args)
{
	int i = (int)args;
	PVOID block_context = ::ConvertThreadToFiber(nullptr);
	TlsSetValue(tls_, block_context);
	while (true)
	{
		Context *ctx;
		::EnterCriticalSection(&cs_[i]);
		while (empty_[i])
			::SleepConditionVariableCS(&cv_[i], &cs_[i], INFINITE);
		ctx = running_context_[i].front();
		running_context_[i].pop_front();
		empty_[i] = running_context_[i].empty();
		::LeaveCriticalSection(&cs_[i]);
		::SwitchToFiber(ctx->orgin_);
	}
}

void FiberEnvironment::fun_(PVOID args)
{
	Context *context = reinterpret_cast<Context*>(args);
	context->fun_();
	::SwitchToFiber(TlsGetValue(tls_));
}

FiberEnvironment::~FiberEnvironment()
{
	iocp_->stop();
	WaitForMultipleObjects(td_num_, schedule_tds_.data(), TRUE, INFINITE);
}

ADDRINFOEX FiberEnvironment::getaddrinfo(PCTSTR ip, PCTSTR port)
{
	Context* context = reinterpret_cast<Context*>(::GetFiberData());
	ADDRINFOEX hints, *result;
	ZeroMemory(&hints, sizeof(ADDRINFOEX));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	ZeroMemory(&context->io_data_->overlapped_, sizeof(OVERLAPPED));
	int ret = ::GetAddrInfoEx(ip, port, NS_DNS, NULL, &hints, &result, NULL, (LPOVERLAPPED)context->io_data_.get(), NULL, NULL);
	::SwitchToFiber(TlsGetValue(tls_));
	ADDRINFOEX addr = *result;
	FreeAddrInfoEx(result);
	return addr;
}

SOCKET FiberEnvironment::socket(int af, int type, int protocol)
{
	Context* context = reinterpret_cast<Context*>(::GetFiberData());
	SOCKET s = ::WSASocketW(af, type, protocol, 0, 0, WSA_FLAG_OVERLAPPED);
	bool value = TRUE;
	setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&value, sizeof(bool));
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&value, sizeof(bool));
	::CreateIoCompletionPort((HANDLE)s, Singleton<IOCP>::get()->get_handle(), (ULONG_PTR)context, 0);
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
	connectEx(s, addr, addrlen, nullptr, 0, &dwBytes, reinterpret_cast<LPOVERLAPPED>(context->io_data_.get()));
	setsockopt(s, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
	freeaddrinfo(local);
	::SwitchToFiber(TlsGetValue(tls_));
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
	context->belong_to_ = this;
	context->fun_ = fun;
	context->cmd_ = Context::NEW;
	context->orgin_ = ::CreateFiber(stacksize, &FiberEnvironment::fun_, context);
	PostQueuedCompletionStatus(iocp_->get_handle(), 0, (ULONG_PTR)context, 0);
	return context->orgin_;
}
