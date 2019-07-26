#ifndef _RUA_IN_PLACE_HPP
#define _RUA_IN_PLACE_HPP

#include "macros.hpp"

#if defined(__cpp_lib_optional) || defined(__cpp_lib_variant) ||               \
	defined(__cpp_lib_any)

#include <utility>

namespace rua {

using in_place_t = std::in_place_t;

} // namespace rua

#else

#include <cstddef>

namespace rua {

struct in_place_t {
	explicit in_place_t() = default;
};

} // namespace rua

#endif

namespace rua {

RUA_MULTIDEF_VAR constexpr in_place_t in_place{};

} // namespace rua

#endif
