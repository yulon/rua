#ifndef _rua_memory_hpp
#define _rua_memory_hpp

#include "binary/bytes.hpp"
#include "dype/any_ptr.hpp"
#include "util.hpp"

#ifdef _WIN32

#include <windows.h>

#elif defined(RUA_UNIX)

#include <sys/mman.h>
#include <sys/user.h>

#endif

#include <cstddef>

namespace rua {

#ifdef RUA_UNIX

RUA_CVAL int mem_none = PROT_NONE;
RUA_CVAL int mem_read = PROT_READ;
RUA_CVAL int mem_write = PROT_WRITE;
RUA_CVAL int mem_exec = PROT_EXEC;

#else

RUA_CVAL int mem_none = 0x1000;
RUA_CVAL int mem_read = 0x0001;
RUA_CVAL int mem_write = 0x0010;
RUA_CVAL int mem_exec = 0x0100;

#endif

inline bool mem_chmod(
	any_ptr ptr, size_t size, int flags = mem_read | mem_write | mem_exec) {

#ifdef _WIN32

	DWORD new_mode, old_mode;
	switch (flags) {
	case mem_read | mem_write | mem_exec:
		new_mode = PAGE_EXECUTE_READWRITE;
		break;
	case mem_read | mem_write:
		new_mode = PAGE_READWRITE;
		break;
	case mem_read:
		new_mode = PAGE_READONLY;
		break;
	case mem_write:
		new_mode = PAGE_WRITECOPY;
		break;
	case mem_exec:
		new_mode = PAGE_EXECUTE;
		break;
	case mem_read | mem_exec:
		new_mode = PAGE_EXECUTE_READ;
		break;
	case mem_write | mem_exec:
		new_mode = PAGE_EXECUTE_WRITECOPY;
		break;
	default:
		new_mode = PAGE_NOACCESS;
	}
	return VirtualProtect(ptr, size, new_mode, &old_mode);

#elif defined(RUA_UNIX)

	return mprotect((ptr >> PAGE_SHIFT) << PAGE_SHIFT, size, flags) == 0;

#endif

	return false;
}

inline bool
mem_chmod(bytes_view data, int flags = mem_read | mem_write | mem_exec) {
	return mem_chmod(data.data(), data.size(), flags);
}

} // namespace rua

#endif
