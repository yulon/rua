#ifndef _RUA_MEM_GETTER_HPP
#define _RUA_MEM_GETTER_HPP

#include "get.hpp"

namespace rua {
	namespace mem {
		class getter {
			public:
				template <typename T>
				T &get(ptrdiff_t offset = 0) const {
					return mem::get<T>(this, offset);
				}

				template <typename T>
				T &aligned_get(ptrdiff_t ix = 0) {
					return get<T>(ix * sizeof(T));
				}
		};
	}
}

#endif
