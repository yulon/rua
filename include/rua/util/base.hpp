#ifndef _RUA_UTIL_BASE_HPP
#define _RUA_UTIL_BASE_HPP

#include "macros.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <type_traits>
#include <utility>

namespace rua {

using uint = unsigned int;
using ushort = unsigned short;
using schar = signed char;
using uchar = unsigned char;
using ssize_t = typename std::make_signed<size_t>::type;
using max_align_t = std::max_align_t;

RUA_CVAL auto nullpos = (std::numeric_limits<size_t>::max)();

#define RUA_FULLPTR(T)                                                         \
	(reinterpret_cast<T *>((std::numeric_limits<uintptr_t>::max)()))

} // namespace rua

#endif
