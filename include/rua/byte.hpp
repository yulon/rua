#ifndef _RUA_BYTE_HPP
#define _RUA_BYTE_HPP

#include "macros.hpp"

#if RUA_CPP >= RUA_CPP_17 || defined(__cpp_lib_byte)
#include <cstddef>
#endif

namespace rua {

class byte {
public:
	byte() = default;

	RUA_FORCE_INLINE constexpr byte(unsigned char n) : _n(n) {}

	RUA_FORCE_INLINE constexpr operator unsigned char() const {
		return _n;
	}

#if RUA_CPP >= RUA_CPP_17 || defined(__cpp_lib_byte)

	RUA_FORCE_INLINE constexpr byte(std::byte b) :
		_n(static_cast<unsigned char>(b)) {}

	RUA_FORCE_INLINE constexpr operator std::byte() const {
		return std::byte{_n};
	}

#endif

private:
	unsigned char _n;
};

} // namespace rua

#endif
