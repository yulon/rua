#ifndef _RUA_TYPES_DEF_HPP
#define _RUA_TYPES_DEF_HPP

#include "../macros.hpp"

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

////////////////////////////////////////////////////////////////////////////

RUA_INLINE_CONST auto nullpos = (std::numeric_limits<size_t>::max)();

////////////////////////////////////////////////////////////////////////////

#if defined(__cpp_lib_variant) || defined(__cpp_lib_any)

template <typename T>
using in_place_type_t = std::in_place_type_t<T>;

template <size_t I>
using in_place_index_t = std::in_place_index_t<I>;

#else

template <typename T>
struct in_place_type_t {};

template <size_t I>
struct in_place_index_t {};

#endif

} // namespace rua

#endif
