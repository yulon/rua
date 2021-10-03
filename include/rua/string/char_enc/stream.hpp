#ifndef _RUA_STRING_CHAR_ENC_STREAM_HPP
#define _RUA_STRING_CHAR_ENC_STREAM_HPP

#ifdef _WIN32

#include "stream/win32.hpp"

namespace rua {

using namespace win32::_string_char_enc_stream;

} // namespace rua

#define RUA_LOC_TO_U8_READER(reader) (::rua::make_loc_to_u8_reader(reader))
#define RUA_U8_TO_LOC_WRITER(writer) (::rua::make_u8_to_loc_writer(writer))

#else

#include "stream/uni.hpp"

namespace rua {

using namespace uni::_string_char_enc_stream;

} // namespace rua

#define RUA_LOC_TO_U8_READER(reader) (reader)
#define RUA_U8_TO_LOC_WRITER(writer) (writer)

#endif

#endif
