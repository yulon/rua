#ifndef _RUA_SYNC_CHAN_BASIC_HPP
#define _RUA_SYNC_CHAN_BASIC_HPP

#include "base.hpp"

#include <utility>

namespace rua {

template <typename T, typename DefaultSchedulerGetter>
class basic_chan : public chan_base<T> {
public:
	constexpr basic_chan() : chan_base<T>() {}

	rua::opt<T> try_pop(ms timeout = 0) {
		return chan_base<T>::try_pop(DefaultSchedulerGetter::get, timeout);
	}

	template <typename GetScheduler>
	rua::opt<T> try_pop(GetScheduler &&get_sch, ms timeout = 0) {
		return chan_base<T>::try_pop(
			std::forward<GetScheduler>(get_sch), timeout);
	}

	T pop() {
		return chan_base<T>::try_pop(
				   DefaultSchedulerGetter::get, duration_max())
			.value();
	}

	template <typename GetScheduler>
	T pop(GetScheduler &&get_sch) {
		return chan_base<T>::pop(std::forward<GetScheduler>(get_sch));
	}
};

template <typename T, typename DefaultSchedulerGetter, typename R>
RUA_FORCE_INLINE basic_chan<T, DefaultSchedulerGetter> &
operator<<(R &receiver, basic_chan<T, DefaultSchedulerGetter> &ch) {
	receiver = ch.pop();
	return ch;
}

template <typename T, typename DefaultSchedulerGetter, typename V>
RUA_FORCE_INLINE basic_chan<T, DefaultSchedulerGetter> &
operator<<(basic_chan<T, DefaultSchedulerGetter> &ch, V &&val) {
	ch.emplace(std::forward<V>(val));
	return ch;
}

} // namespace rua

#endif
