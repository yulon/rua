#ifndef _RUA_MEM_GET_HPP
#define _RUA_MEM_GET_HPP

#include "../macros.hpp"

#include <cstdint>

namespace rua {
	namespace mem {
		template <typename T>
		RUA_FORCE_INLINE const T &get(const void *ptr, ptrdiff_t offset = 0) {
			return *reinterpret_cast<const T *>(reinterpret_cast<uintptr_t>(ptr) + offset);
		}

		template <typename T>
		RUA_FORCE_INLINE T &get(void *ptr, ptrdiff_t offset = 0) {
			return *reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(ptr) + offset);
		}

		template <typename T>
		RUA_FORCE_INLINE const T &aligned_get(const void *ptr, ptrdiff_t ix = 0) {
			return *reinterpret_cast<const T *>(reinterpret_cast<uintptr_t>(ptr) + ix * sizeof(T));
		}

		template <typename T>
		RUA_FORCE_INLINE T &aligned_get(void *ptr, ptrdiff_t ix = 0) {
			return *reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(ptr) + ix * sizeof(T));
		}
	}
}

#endif
