#ifndef _RUA_BYTE_HPP
#define _RUA_BYTE_HPP

#include "macros.hpp"

#if RUA_CPP >= RUA_CPP_17 || defined(__cpp_lib_byte)

#include <cstddef>

namespace rua {

using byte = std::byte;

} // namespace rua

#else

namespace rua {

enum class byte : unsigned char {};

} // namespace rua

namespace std {

inline unsigned char to_integer(rua::byte b) {
	return static_cast<unsigned char>(b);
}

} // namespace std

#endif

#endif
