#pragma once



// detect stuff
#if defined(__unix__) || defined(__linux__)
#define CPMMAP_UNIX

#if __x86_64__ || __ppc64__
#define ENV64BIT
static_assert(sizeof(size_t) == 8, "Compiler is broken");
#else
#define ENV32BIT
static_assert(sizeof(size_t) == 4, "Compiler is broken");
#endif

#elif defined(_WIN32) || defined(_WIN64)
#define CPMMAP_WINDOWS

#if _WIN64
#define ENV64BIT
static_assert(sizeof(size_t) == 8, "_WIN64 defined but compiler says different");
#else
#define ENV32BIT
static_assert(sizeof(size_t) == 4, "_WIN32 defined but compiler says different");
#endif

#else
#error Memory mapping functions not implemented for this platform.
#endif


// includes
#include <string>
#include <stdexcept>

#if defined(CPMMAP_UNIX)
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#elif defined(CPMMAP_WINDOWS)
#include <windows.h>
#endif


namespace cpmmap
{
	class runtime_error : public std::runtime_error
	{
	public:
#if defined(CPMMAP_WINDOWS)
		runtime_error(const std::string& message, DWORD code)
			:std::runtime_error(concat_message(message, code))
		{
		}

		runtime_error(const std::string& message)
			:std::runtime_error(concat_message(message, GetLastError()))
		{
		}
#endif

#if defined (CPMMAP_UNIX)
		runtime_error(const std::string& message, int code)
			:std::runtime_error(concat_message(message, code))
		{
		}

		runtime_error(const std::string& message)
			:std::runtime_error(concat_message(message, errno))
		{
		}
#endif

	private:
		std::string concat_message(const std::string& message, long long code)
		{
			std::string msg{ message };
#if defined(CPMMAP_WINDOWS)
			DWORD e = static_cast<DWORD>(code);
			if (e != 0)
			{
				char* syserr = nullptr;
				e = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
					nullptr,
					e,
					0,
					(LPSTR)& syserr,
					0,
					nullptr);
				if (e && syserr)
				{
					msg += ": ";
					msg += syserr;
					LocalFree(syserr);
				}
			}
#endif

#if defined(CPMMAP_UNIX)
			int e = static_cast<int>(code);
			if (e != 0)
			{
				const char* syserr = strerror(e);
				if (syserr)
				{
					msg += ": ";
					msg += syserr;
				}
			}
#endif
			return msg;
		}
	};

	class mapped_file
	{
	public:

		// constructor
		mapped_file(const std::string& filename, bool readwrite = false, bool shared = false, size_t size = 0)
			:filesize(0)
		{
			open_file(filename, false, readwrite, shared);

			if (size == 0)
			{
				get_filesize();
			}
			else if (readwrite)
			{
				resize_file(size);
			}
			else
			{
				throw std::invalid_argument("When opening file in read-only mode, filesize must be set to 0 (auto).");
			}

			map_file();
		}

		// moves and copies and deletes
		mapped_file(const mapped_file&) = delete;
		mapped_file& operator=(const mapped_file&) = delete;
		mapped_file(mapped_file&& o) noexcept
			:pointer(o.pointer), filesize(o.filesize), rw(o.rw),
#if defined(CPMMAP_UNIX)
			file_handle(o.file_handle)
#elif defined(CPMMAP_WINDOWS)
			mapping_handle(o.mapping_handle),
			file_handle(o.file_handle)
#endif
		{
			o.pointer = nullptr;
			o.filesize = 0;
			o.rw = false;
#if defined(CPMMAP_UNIX)
			o.file_handle = 0;
#elif defined(CPMMAP_WINDOWS)
			o.mapping_handle = nullptr;
			o.file_handle = nullptr;
#endif
		}

		~mapped_file()
		{
			unmap_file();
			close_file();
		}

		// ops
		inline char& operator[](size_t pos) { if (pos > filesize) throw std::out_of_range("out of range"); return *(pointer + pos); }
		inline const char& operator[](size_t pos) const { if (pos > filesize) throw std::out_of_range("out of range"); return *(pointer + pos); }
		inline std::uint64_t size() const { return filesize; }
		inline char* get() { return pointer; }
		inline const char* get() const { return pointer; }

		// methods
		void resize(size_t newsize)
		{
			flush_file();
			unmap_file();
			resize_file(newsize);
			map_file();
		}

		void sync()
		{
			flush_file();
			update_timestamp();
		}

		static mapped_file create(const std::string & filename, size_t size, bool shared = false)
		{
			mapped_file f{};
			f.open_file(filename, true, true, shared);
			f.resize_file(size);
			f.map_file();
			return f;
		}

	private:
		char* pointer = nullptr;
		std::uint64_t filesize = 0;
		bool rw = false;

		mapped_file()
		{
		}


#if defined(CPMMAP_UNIX)
		int file_handle;

		void open_file(const std::string & filename, bool create, bool readwrite, bool shared)
		{
			file_handle = open(filename.c_str(), (readwrite ? O_RDWR : O_RDONLY) | (create ? O_CREAT : 0), 0666);

			if (file_handle == -1)
			{
				throw runtime_error("could not open file");
			}
			rw = readwrite;
		}

		void get_filesize()
		{
			// get file size
			struct stat filestats;
			auto statresult = fstat(file_handle, &filestats);
			if (statresult == -1)
			{
				close(file_handle);
				throw runtime_error("could not stat file");
			}
			filesize = filestats.st_size;
		}
		
		void resize_file(std::uint64_t size)
		{
			if (ftruncate(file_handle, size) == -1)
			{
				close(file_handle);
				throw runtime_error("could not truncate file");
			}

			filesize = size;
		}

		void map_file()
		{
			// memory map
			pointer = static_cast<char*>(mmap(
				0,
				filesize,
				rw ? PROT_WRITE : PROT_READ,
				MAP_SHARED,
				file_handle,
				0));

			if (!pointer || pointer == MAP_FAILED)
			{
				close(file_handle);
				throw runtime_error("could not map file");
			}
		}

		void update_timestamp()
		{
			timespec t[2]{ {{}, UTIME_NOW }, { {}, UTIME_NOW } };
			futimens(file_handle, t);
		}

		void unmap_file()
		{
			if (filesize && pointer)
			{
				munmap(pointer, filesize);
				pointer = nullptr;
			}
		}

		void close_file()
		{
			if (file_handle)
			{
				close(file_handle);
				file_handle = 0;
			}
			filesize = 0;
		}

		void flush_file()
		{
		}
#endif




#if defined(CPMMAP_WINDOWS)
		HANDLE mapping_handle = INVALID_HANDLE_VALUE;
		HANDLE file_handle = INVALID_HANDLE_VALUE;

		void open_file(const std::string & filename, bool create, bool readwrite, bool shared)
		{
			// convert the filename to wchar
			std::wstring wfilename;
			// the length is going to be at least filename.size()
			wfilename.resize(filename.size());
			int num = MultiByteToWideChar(CP_UTF8, 0, &filename[0], static_cast<int>(filename.size()), &wfilename[0], wfilename.size());
			if (num == 0) throw runtime_error("Could not convert filename to UTF-16");

			// open the file
			file_handle = CreateFileW(wfilename.c_str(),
				!readwrite ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,
				shared ? (FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE) : 0,
				0,
				create ? CREATE_ALWAYS : OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				0);
			if (!file_handle || file_handle == INVALID_HANDLE_VALUE)
			{
				throw runtime_error("could not open file");
			}

			rw = readwrite;
		}

		void get_filesize()
		{
			// get its size
			DWORD hi_size, lo_size;
			lo_size = GetFileSize(file_handle, &hi_size);
			filesize = static_cast<std::uint64_t>(lo_size) | (static_cast<std::uint64_t>(hi_size) << 32);
		}

		void resize_file(std::uint64_t size)
		{
			// either get the file size or resize it
			DWORD hi_size, lo_size;
			// resize the file
			hi_size = static_cast<DWORD>(size >> 32);
			lo_size = static_cast<DWORD>(size & 0xFFFFFFFF);
			LONG lhi_size = *reinterpret_cast<PLONG>(&hi_size);
			LONG llo_size = *reinterpret_cast<PLONG>(&lo_size);
			DWORD r = SetFilePointer(file_handle,
				llo_size,
				&lhi_size,
				FILE_BEGIN);
			if (r == INVALID_SET_FILE_POINTER) throw runtime_error("Could not resize file");
			r = SetEndOfFile(file_handle);
			if (!r) throw runtime_error("Could not resize file");
			r = SetFilePointer(file_handle, 0, nullptr, FILE_BEGIN);
			if (r == INVALID_SET_FILE_POINTER) throw runtime_error("Could not resize file");
			filesize = size;
		}

		void map_file()
		{
			auto hi_size = static_cast<DWORD>(filesize >> 32);
			auto lo_size = static_cast<DWORD>(filesize & 0xFFFFFFFF);

			// map the file into memory
			mapping_handle = CreateFileMapping(
				file_handle,
				0,
				rw ? PAGE_READWRITE : PAGE_READONLY,
				hi_size,
				lo_size,
				0);

			if (!mapping_handle || mapping_handle == INVALID_HANDLE_VALUE)
			{
				CloseHandle(file_handle);
				throw runtime_error("could not map file");
			}

			// get the pointer
			pointer = static_cast<char*>(::MapViewOfFile(
				mapping_handle,
				rw ? FILE_MAP_WRITE : FILE_MAP_READ,
				0, 0,
				static_cast<size_t>(filesize)));

			if (!pointer)
			{
				CloseHandle(mapping_handle);
				CloseHandle(file_handle);
				throw runtime_error("could not map file");
			}
		}

		void flush_file()
		{
			auto r = FlushViewOfFile(pointer, 0);
			if (!r) throw runtime_error("could not flush file");
			r = FlushFileBuffers(file_handle);
			if (!r) throw runtime_error("could not flush file");
		}

		void update_timestamp()
		{
			SYSTEMTIME st{};
			FILETIME ft{};
			GetSystemTime(&st);
			SystemTimeToFileTime(&st, &ft);
			SetFileTime(file_handle, nullptr, nullptr, &ft);
		}

		void unmap_file()
		{
			if (pointer)
			{
				UnmapViewOfFile(pointer);
				pointer = nullptr;
			}
			if (mapping_handle && mapping_handle != INVALID_HANDLE_VALUE)
			{
				CloseHandle(mapping_handle);
				mapping_handle = nullptr;
			}
		}

		void close_file()
		{
			if (file_handle && file_handle != INVALID_HANDLE_VALUE)
			{
				CloseHandle(file_handle);
				file_handle = nullptr;
			}
			filesize = 0;
		}
#endif
	};
}
