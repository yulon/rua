#ifndef _RUA_GNC_BLOCK_HPP
#define _RUA_GNC_BLOCK_HPP

#include "getter.hpp"

#include "../bytes.hpp"

namespace rua {
	template <size_t Sz>
	class block : public getter, public bytes::operation<block<Sz>> {
		public:
			static constexpr size_t size() {
				return Sz;
			}

		private:
			uint8_t _raw[Sz];
	};
}

#endif
