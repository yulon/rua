#ifndef _RUA_THREAD_THIS_THREAD_POSIX_HPP
#define _RUA_THREAD_THIS_THREAD_POSIX_HPP

#include "../this_thread_id/posix.hpp"
#include "../thread/posix.hpp"

#include "../../macros.hpp"

namespace rua { namespace posix {

namespace _thread_this_thread {

RUA_FORCE_INLINE thread this_thread() {
	return thread(this_thread_id());
}

} // namespace _thread_this_thread

using namespace _thread_this_thread;

}} // namespace rua::posix

#endif
