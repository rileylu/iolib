#pragma once
#include "common.h"
#include "noncopyable.h"
#include <memory>
#include <string>
class schedule;
namespace iolib
{
	class file
	{
	public:
		file();
		~file();
		int open(const std::string& fn, DWORD access, DWORD creationDisposition);
		int read(void* buf, int len);
		int write(const void* buf, int len);
		class file_t
			:noncopyable
		{
		public:
			file_t();
			~file_t();
			int open(const std::string& fn, DWORD access, DWORD creationDisposition);
			int read(void* buf, int len);
			int write(const void* buf, int len);
		private:
			friend class file;
			schedule& sche_;
			HANDLE fd_;
			io_data iodata_;
		};
	private:
		std::unique_ptr<file_t> impl_;
	};


}
