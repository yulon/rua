#ifndef _RUA_STRING_CHAR_ENC_STREAM_HPP
#define _RUA_STRING_CHAR_ENC_STREAM_HPP

#ifdef _WIN32

#include "stream/win32.hpp"

namespace rua {

using loc_to_u8_reader = win32::loc_to_u8_reader;
using u8_to_loc_writer = win32::u8_to_loc_writer;

} // namespace rua

#define RUA_LOC_TO_U8_READER(reader) (::rua::loc_to_u8_reader(reader))
#define RUA_U8_TO_LOC_WRITER(writer) (::rua::u8_to_loc_writer(writer))

#else

#include "stream/uni.hpp"

namespace rua {

using loc_to_u8_reader = uni::loc_to_u8_reader;
using u8_to_loc_writer = uni::u8_to_loc_writer;

} // namespace rua

#define RUA_LOC_TO_U8_READER(reader) (reader)
#define RUA_U8_TO_LOC_WRITER(writer) (writer)

#endif

#endif
