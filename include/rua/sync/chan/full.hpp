#ifndef _RUA_SYNC_CHAN_FULL_HPP
#define _RUA_SYNC_CHAN_FULL_HPP

#include "basic.hpp"

#include "../../sched/util.hpp"

namespace rua {

template <typename T>
inline rua::opt<T> chan<T>::try_pop(ms timeout) {
	auto val_opt = _buf.pop_back();
	if (val_opt || !timeout) {
		return val_opt;
	}
	return _try_pop(this_scheduler(), timeout);
}

} // namespace rua

#endif
