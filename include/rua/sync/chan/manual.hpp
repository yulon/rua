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

	// TODO: Because the lockfree_list{} is refactored, it may not be possible
	// to know if there are currently waiting persons, and the relevant code is
	// subject to correction.
	template <typename... Args>
	bool emplace(Args &&... args) {
		if (_buf.emplace(std::forward<Args>(args)...)) {
			return _wakes();
		}
		return _waiters || !_buf;
	}

	rua::opt<T> try_pop() {
		return _try_pop();
	}

	inline rua::opt<T> try_pop(ms timeout);

	rua::opt<T> try_pop(scheduler_i sch, ms timeout) {
		auto val_opt = _try_pop();
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

	bool _wakes(waker_i ignore_waiter = nullptr) {
		auto waiters = _waiters.pop();
		if (!waiters) {
			return false;
		}
		do {
			auto waiter = waiters.pop_front();
			if (waiter == ignore_waiter) {
				continue;
			}
			waiter->wake();
		} while (waiters);
		return true;
	}

	rua::opt<T> _try_pop(waker_i ignore_waiter = nullptr) {
		rua::opt<T> val_opt;

		auto buf = _buf.pop();
		if (!buf) {
			return val_opt;
		}

		val_opt = buf.pop_front();
		if (buf && _buf.prepend(std::move(buf))) {
			_wakes(ignore_waiter);
		}
		return val_opt;
	}

	rua::opt<T> _wait_and_pop(scheduler_i sch, ms timeout) {
		assert(sch);
		assert(timeout);

		rua::opt<T> val_opt;

		auto wkr = sch->get_waker();

		if (_waiters.emplace(wkr)) {
			val_opt = _try_pop(wkr);
			if (val_opt) {
				return val_opt;
			}
		}

		if (timeout == duration_max()) {
			for (;;) {
				if (sch->sleep(timeout, true)) {
					val_opt = _try_pop(wkr);
					if (val_opt) {
						return val_opt;
					}
					if (_waiters.emplace(wkr)) {
						val_opt = _try_pop(wkr);
						if (val_opt) {
							return val_opt;
						}
					}
				}
			}
		}

		for (;;) {
			auto t = tick();
			auto nto = sch->sleep(timeout, true);
			val_opt = _try_pop(wkr);
			if (val_opt || !nto) {
				return val_opt;
			}
			timeout -= tick() - t;
			if (timeout <= 0) {
				return val_opt;
			}
			if (_waiters.emplace(wkr)) {
				val_opt = _try_pop(wkr);
				if (val_opt) {
					return val_opt;
				}
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
