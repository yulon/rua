#ifndef _RUA_FILE_HPP
#define _RUA_FILE_HPP

#include "macros.hpp"

#if defined(_WIN32)

#include "file/win32.hpp"

namespace rua {

using file_path = win32::file_path;
using namespace win32::_wkdir;
using file_stat = win32::file_stat;
using file = win32::file;
using namespace win32::_make_file;

using dir_entry_stat = win32::dir_entry_stat;
using dir_entry = win32::dir_entry;
using dir_iterator = win32::dir_iterator;

} // namespace rua

#elif defined(RUA_UNIX)

#include "file/posix.hpp"

namespace rua {

using file_path = posix::file_path;
using namespace posix::_wkdir;
using file_stat = posix::file_stat;
using file = posix::file;
using namespace posix::_make_file;

using dir_entry_stat = posix::dir_entry_stat;
using dir_entry = posix::dir_entry;
using dir_iterator = posix::dir_iterator;

} // namespace rua

#else

#error rua::file: not supported this platform!

#endif

#endif
