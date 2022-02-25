#ifndef _RUA_THREAD_SLEEP_HPP
#define _RUA_THREAD_SLEEP_HPP

#include "../thread/dozer.hpp"

namespace rua {

template <typename Dozer = thread_dozer>
inline void sleep(duration timeout) {
	thread_dozer().sleep(timeout);
}

} // namespace rua

#endif
