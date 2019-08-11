#ifndef _RUA_STRING_ENCODING_BASE_HPP
#define _RUA_STRING_ENCODING_BASE_HPP

#ifdef _WIN32

#include "base/win32.hpp"

namespace rua {

using namespace win32::_string_encoding_base;

} // namespace rua

#define RUA_LOC_TO_U8(str) (::rua::loc_to_u8(str))
#define RUA_U8_TO_LOC(u8_str) (::rua::u8_to_loc(u8_str))

#else

#include "base/uni.hpp"

namespace rua {

using namespace uni::_string_encoding_base;

} // namespace rua

#define RUA_LOC_TO_U8(str) (str)
#define RUA_U8_TO_LOC(u8_str) (u8_str)

#endif

#endif
