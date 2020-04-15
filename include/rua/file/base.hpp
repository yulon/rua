#ifndef _RUA_FILE_BASE_HPP
#define _RUA_FILE_BASE_HPP

#include "../chrono/time.hpp"
#include "../types/util.hpp"

namespace rua {

struct file_times {
	time modified_time, creation_time, access_time;
};

} // namespace rua

#endif
