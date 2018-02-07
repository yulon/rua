#include "hook.hpp"

namespace rua {
	size_t _hooked_count = 0;
	std::mutex _hook_mtx;
}
