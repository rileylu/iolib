#include "FiberEnvironment.h"
#include "singleton.h"
#include <algorithm>

IOCP* FiberEnvironment::iocp_ = nullptr;
void* FiberEnvironment::block_context_ = nullptr;
std::forward_list<void*> FiberEnvironment::finish_list_;

FiberEnvironment::FiberEnvironment(int num)
	:td_num_(num), empty_(true), block_thread_t_(INVALID_HANDLE_VALUE)
{
	InitializeCriticalSection(&running_cs_);
	InitializeConditionVariable(&running_v_);
	iocp_ = Singleton<IOCP>::get_instance(std::bind(&FiberEnvironment::io_fun_, this), td_num_);
	block_thread_t_ = (HANDLE)::_beginthreadex(nullptr, 0, &FiberEnvironment::schedule_next_, this, 0, 0);
}

void FiberEnvironment::io_fun_()
{
	DWORD bytes_transferred;
	Per_IO_Data *per_io_data = nullptr;
	Context *context = nullptr;
	BOOL bRet = false;
	std::unique_ptr<OVERLAPPED_ENTRY[]> overlapped_entry_array(new OVERLAPPED_ENTRY[100]);
	ULONG numEntriesRemoved = 0;
	std::forward_list<Context*> jobs;
	while (true)
	{
		bRet = ::GetQueuedCompletionStatusEx(iocp_->get_handle(), overlapped_entry_array.get(), 100, &numEntriesRemoved, WSA_INFINITE, false);
		for (int i = 0; i < numEntriesRemoved; ++i)
		{
			per_io_data = (Per_IO_Data*)overlapped_entry_array[i].lpOverlapped;
			context = (Context*)overlapped_entry_array[i].lpCompletionKey;
			context->io_data_->bytes_transferred = overlapped_entry_array[i].dwNumberOfBytesTransferred;
			jobs.push_front(context);
		}
		EnterCriticalSection(&running_cs_);
		auto it = running_list_.begin();
		running_list_.splice_after(it, std::move(jobs));
		empty_ = false;
		LeaveCriticalSection(&running_cs_);
		WakeConditionVariable(&running_v_);
	}
}

unsigned __stdcall FiberEnvironment::schedule_next_(PVOID args)
{
	block_context_ = ::ConvertThreadToFiber(nullptr);
	FiberEnvironment* fb = (FiberEnvironment*)args;
	while (true)
	{
		while (fb->empty_)
			::SleepConditionVariableCS(&fb->running_v_, &fb->running_cs_, INFINITE);
		std::forward_list<Context*> copy_list(std::move(fb->running_list_));
		fb->empty_ = true;
		::LeaveCriticalSection(&fb->running_cs_);
		std::for_each(copy_list.begin(), copy_list.end(), [](Context* ctx)
		{
			SwitchToFiber(ctx->orgin_);
		});
		std::for_each(finish_list_.begin(), finish_list_.end(), [](void *p) {
			::DeleteFiber(p);
		});
		finish_list_.clear();
	}
}

void FiberEnvironment::fun_(PVOID args)
{
	Context *context = reinterpret_cast<Context*>(args);
	FiberEnvironment* env = reinterpret_cast<FiberEnvironment*>(context->belong_to_);
	context->fun_();
	finish_list_.push_front(context->orgin_);
	::SwitchToFiber(block_context_);
}

FiberEnvironment::~FiberEnvironment()
{
	iocp_->stop();
	WaitForSingleObject(block_thread_t_, INFINITE);
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
	::SwitchToFiber(block_context_);
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
	::SwitchToFiber(block_context_);
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
	::SwitchToFiber(block_context_);
	return context->io_data_->bytes_transferred;
}

int FiberEnvironment::send(SOCKET s, const void * buf, int size)
{
	Context *context = reinterpret_cast<Context*>(::GetFiberData());
	context->io_data_->wsabuf_.buf = (CHAR*)buf;
	context->io_data_->wsabuf_.len = size;
	ZeroMemory(&context->io_data_->overlapped_, sizeof(OVERLAPPED));
	int ret = ::WSASend(s, &context->io_data_->wsabuf_, 1, 0, 0, (LPWSAOVERLAPPED)context->io_data_.get(), nullptr);
	::SwitchToFiber(block_context_);
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
	::SwitchToFiber(block_context_);
	return context->io_data_->bytes_transferred;
}

int FiberEnvironment::write(HANDLE hd, const void * buf, int len)
{
	Context* context = reinterpret_cast<Context*>(::GetFiberData());
	context->io_data_->wsabuf_.buf = (CHAR*)buf;
	context->io_data_->wsabuf_.len = len;
	ZeroMemory(&context->io_data_->overlapped_, sizeof(OVERLAPPED));
	int ret = ::WriteFile(hd, buf, len, NULL, (LPOVERLAPPED)context->io_data_.get());
	::SwitchToFiber(block_context_);
	return context->io_data_->bytes_transferred;
}

void * FiberEnvironment::create_thread(int stacksize, std::function<void()> fun)
{
	Context *context = new Context;
	context->belong_to_ = this;
	context->fun_ = fun;
	context->orgin_ = ::CreateFiber(0, &FiberEnvironment::fun_, context);
	PostQueuedCompletionStatus(iocp_->get_handle(), 0, (ULONG_PTR)context, 0);
	return context->orgin_;
}
