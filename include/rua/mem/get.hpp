#ifndef _RUA_MEM_GET_HPP
#define _RUA_MEM_GET_HPP

#include "../generic/any_ptr.hpp"

#include <cstddef>

namespace rua {
	namespace mem {
		template <typename T>
		static inline T &get(any_ptr ptr, ptrdiff_t offset = 0) {
			return *(ptr + offset).to<T *>();
		}

		template <typename T>
		static inline T &aligned_get(any_ptr ptr, ptrdiff_t ix = 0) {
			return get<T>(ptr, ix * sizeof(T));
		}
	}
}

#endif
