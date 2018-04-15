#ifndef _RUA_MEM_PROTECT_HPP
#define _RUA_MEM_PROTECT_HPP

#include "../gnc/any_ptr.hpp"

#include "../macros.hpp"

#ifdef _WIN32
	#include <windows.h>
#elif defined(RUA_UNIX)
	#include <sys/mman.h>
	#include <sys/user.h>
#else
	#error rua::mem::protect: not supported this platform!
#endif

#include <cstddef>

namespace rua {
	namespace mem {
		#ifdef RUA_UNIX
			static constexpr int protect_none = PROT_NONE;
			static constexpr int protect_read = PROT_READ;
			static constexpr int protect_write = PROT_WRITE;
			static constexpr int protect_exec = PROT_EXEC;
		#else
			static constexpr int protect_none = 0x1000;
			static constexpr int protect_read = 0x0001;
			static constexpr int protect_write = 0x0010;
			static constexpr int protect_exec = 0x0100;
		#endif

		static inline bool protect(any_ptr ptr, size_t size, int flags = protect_read | protect_write | protect_exec) {
			#ifdef _WIN32
				DWORD new_protect, old_protect;
				switch (flags) {
					case protect_read | protect_write | protect_exec:
						new_protect = PAGE_EXECUTE_READWRITE;
						break;
					case protect_read | protect_write:
						new_protect = PAGE_READWRITE;
						break;
					case protect_read:
						new_protect = PAGE_READONLY;
						break;
					case protect_write:
						new_protect = PAGE_WRITECOPY;
						break;
					case protect_exec:
						new_protect = PAGE_EXECUTE;
						break;
					case protect_read | protect_exec:
						new_protect = PAGE_EXECUTE_READ;
						break;
					case protect_write | protect_exec:
						new_protect = PAGE_EXECUTE_WRITECOPY;
						break;
					default:
						new_protect = PAGE_NOACCESS;
				}
				return VirtualProtect(ptr, size, new_protect, &old_protect);
			#elif defined(RUA_UNIX)
				return mprotect(any_ptr((ptr.value() >> PAGE_SHIFT) << PAGE_SHIFT), size, flags) == 0;
			#endif
		}
	}
}

#endif
