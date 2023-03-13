#ifndef _rua_file_times_hpp
#define _rua_file_times_hpp

#include "../time/real.hpp"
#include "../util.hpp"

namespace rua {

struct file_times {
	time modified_time, creation_time, access_time;
};

} // namespace rua

#endif
