#ifndef _RUA_STRING_CHAR_CODEC_STREAM_HPP
#define _RUA_STRING_CHAR_CODEC_STREAM_HPP

#ifdef _WIN32

#include "stream/win32.hpp"

namespace rua {

using namespace win32::_string_char_codec_stream;

} // namespace rua

#define RUA_L2U_READER(reader) (::rua::make_l2u_reader(reader))
#define RUA_U2L_WRITER(writer) (::rua::make_u2l_writer(writer))

#else

#include "stream/uni.hpp"

namespace rua {

using namespace uni::_string_char_codec_stream;

} // namespace rua

#define RUA_L2U_READER(reader) (reader)
#define RUA_U2L_WRITER(writer) (writer)

#endif

#endif
