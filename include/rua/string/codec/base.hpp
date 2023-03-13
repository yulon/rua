#ifndef _rua_string_codec_base_hpp
#define _rua_string_codec_base_hpp

#ifdef _WIN32

#include "base/win32.hpp"

namespace rua {

using namespace win32::_string_codec_base;

} // namespace rua

#define RUA_L2U(str) (::rua::l2u(str))
#define RUA_U2L(u8_str) (::rua::u2l(u8_str))

#else

#include "base/uni.hpp"

namespace rua {

using namespace uni::_string_codec_base;

} // namespace rua

#define RUA_L2U(str) (str)
#define RUA_U2L(u8_str) (u8_str)

#endif

#endif
