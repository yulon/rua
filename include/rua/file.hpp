#ifndef _RUA_FILE_HPP
#define _RUA_FILE_HPP

#include "util.hpp"

#if defined(_WIN32)

#include "file/win32.hpp"

namespace rua {

using file_path = win32::file_path;
using namespace win32::_wkdir;
using file_info = win32::file_info;
using file = win32::file;
using namespace win32::_make_file;

using dir_entry_info = win32::dir_entry_info;
using dir_entry = win32::dir_entry;
using view_dir = win32::view_dir;

} // namespace rua

#elif defined(RUA_UNIX)

#include "file/posix.hpp"

namespace rua {

using file_path = posix::file_path;
using namespace posix::_wkdir;
using file_info = posix::file_info;
using file = posix::file;
using namespace posix::_make_file;

using dir_entry_info = posix::dir_entry_info;
using dir_entry = posix::dir_entry;
using view_dir = posix::view_dir;

} // namespace rua

#else

#error rua::file: not supported this platform!

#endif

#endif
