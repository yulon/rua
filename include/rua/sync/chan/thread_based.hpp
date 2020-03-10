#ifndef _RUA_SYNC_CHAN_THREAD_BASED_HPP
#define _RUA_SYNC_CHAN_THREAD_BASED_HPP

#include "base.hpp"

#include "../../sched/util.hpp"
#include "../../thread/scheduler.hpp"

namespace rua {

template <typename T>
using thread_chan = basic_chan<T, thread_scheduler_getter>;

} // namespace rua

#endif
