#ifndef _RUA_FILE_TIMES_HPP
#define _RUA_FILE_TIMES_HPP

#include "../time/real.hpp"
#include "../util.hpp"

namespace rua {

struct file_times {
	time modified_time, creation_time, access_time;
};

} // namespace rua

#endif
