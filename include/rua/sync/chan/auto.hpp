#ifndef _RUA_SYNC_CHAN_AUTO_HPP
#define _RUA_SYNC_CHAN_AUTO_HPP

#include "manual.hpp"

#include "../../macros.hpp"
#include "../../sched/scheduler.hpp"

#include <utility>

namespace rua {

template <typename T>
inline rua::opt<T> chan<T>::try_pop(ms timeout) {
	auto val_opt = _buf.pop_back();
	if (val_opt || !timeout) {
		return val_opt;
	}
	return _wait_and_pop(this_scheduler(), timeout);
}

template <typename T, typename R>
RUA_FORCE_INLINE chan<T> &operator<<(R &receiver, chan<T> &ch) {
	receiver = ch.pop();
	return ch;
}

template <typename T, typename V>
RUA_FORCE_INLINE chan<T> &operator<<(chan<T> &ch, V &&val) {
	ch.emplace(std::forward<V>(val));
	return ch;
}

} // namespace rua

#endif
