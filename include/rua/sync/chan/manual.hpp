#ifndef _RUA_SYNC_CHAN_MANUAL_HPP
#define _RUA_SYNC_CHAN_MANUAL_HPP

#include "../lockfree_list.hpp"

#include "../../chrono/tick.hpp"
#include "../../opt.hpp"
#include "../../sched/scheduler/abstract.hpp"
#include "../../types/util.hpp"

#include <atomic>

namespace rua {

template <typename T>
class chan {
public:
	constexpr chan() : _buf(), _waiters() {}

	chan(const chan &) = delete;

	chan &operator=(const chan &) = delete;

	template <typename... Args>
	bool emplace(Args &&... args) {
		_buf.emplace_front(std::forward<Args>(args)...);
		auto waiter_opt = _waiters.pop_back_if([&]() -> bool { return _buf; });
		if (!waiter_opt) {
			return false;
		}
		waiter_opt.value()->wake();
		return true;
	}

	rua::opt<T> try_pop() {
		return _buf.pop_back();
	}

	inline rua::opt<T> try_pop(ms timeout);

	rua::opt<T> try_pop(scheduler_i sch, ms timeout) {
		auto val_opt = _buf.pop_back();
		if (val_opt || !timeout || !sch) {
			return val_opt;
		}
		val_opt = _wait_and_pop(std::move(sch), timeout);
		return val_opt;
	}

	T pop() {
		return try_pop(duration_max()).value();
	}

	T pop(scheduler_i sch) {
		return try_pop(std::move(sch), duration_max()).value();
	}

protected:
	lockfree_list<T> _buf;
	lockfree_list<waker_i> _waiters;

	rua::opt<T> _wait_and_pop(scheduler_i sch, ms timeout) {
		assert(sch);
		assert(timeout);

		rua::opt<T> val_opt;

		auto wkr = sch->get_waker();

		if (!_waiters.emplace_front_if(
				[&]() -> bool {
					val_opt = _buf.pop_back();
					return !val_opt;
				},
				wkr)) {
			return val_opt;
		}

		if (timeout == duration_max()) {
			for (;;) {
				if (sch->sleep(timeout, true) && !_waiters.emplace_front_if(
													 [&]() -> bool {
														 val_opt =
															 _buf.pop_back();
														 return !val_opt;
													 },
													 wkr)) {
					return val_opt;
				}
			}
		}

		for (;;) {
			auto t = tick();
			auto r = sch->sleep(timeout, true);
			timeout -= tick() - t;
			if (timeout <= 0) {
				return _buf.pop_back();
			}
			if (r && !_waiters.emplace_front_if(
						 [&]() -> bool {
							 val_opt = _buf.pop_back();
							 return !val_opt;
						 },
						 wkr)) {
				return val_opt;
			}
		}
		return val_opt;
	}
};

template <typename T, typename V>
RUA_FORCE_INLINE chan<T> &operator<<(chan<T> &ch, V &&val) {
	ch.emplace(std::forward<V>(val));
	return ch;
}

} // namespace rua

#endif
