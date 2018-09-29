#ifndef _RUA_MEM_GET_HPP
#define _RUA_MEM_GET_HPP

#include <cstddef>

namespace rua {
	namespace mem {
		template <typename T>
		inline T &get(const void *ptr, ptrdiff_t offset = 0) {
			return *reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(ptr) + offset);
		}

		template <typename T>
		inline T &aligned_get(const void *ptr, ptrdiff_t ix = 0) {
			return *reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(ptr) + ix * sizeof(T));
		}
	}
}

#endif
