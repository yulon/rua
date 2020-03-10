#ifndef _RUA_SYNC_CHAN_BASIC_CHAN_HPP
#define _RUA_SYNC_CHAN_BASIC_CHAN_HPP

#include "../lockfree_list.hpp"

#include "../../chrono/tick.hpp"
#include "../../macros.hpp"
#include "../../opt.hpp"
#include "../../sched/scheduler.hpp"
#include "../../type_traits/std_patch.hpp"

#include <cassert>
#include <queue>
#include <utility>

namespace rua {

template <typename T, typename DefaultSchedulerGetter>
class basic_chan {
public:
	constexpr basic_chan() : _buf(), _waiters() {}

	basic_chan(const basic_chan &) = delete;

	basic_chan &operator=(const basic_chan &) = delete;

	template <typename... Args>
	bool emplace(Args &&... args) {
		_buf.emplace_front(std::forward<Args>(args)...);
		auto waiter_opt = _waiters.pop_back();
		if (!waiter_opt) {
			return false;
		}
		waiter_opt.value()->signal();
		return true;
	}

	rua::opt<T> try_pop() {
		return _buf.pop_back();
	}

	rua::opt<T> try_pop(ms timeout) {
		auto val_opt = _buf.pop_back();
		if (val_opt || !timeout) {
			return val_opt;
		}
		return _wait_and_pop(DefaultSchedulerGetter::get(), timeout);
	}

	rua::opt<T> try_pop(scheduler_i sch, ms timeout = 0) {
		auto val_opt = _buf.pop_back();
		if (val_opt || !timeout) {
			return val_opt;
		}
		return _wait_and_pop(std::move(sch), timeout);
	}

	T pop() {
		return try_pop(duration_max()).value();
	}

	T pop(scheduler_i sch) {
		return try_pop(std::move(sch), duration_max()).value();
	}

private:
	lockfree_list<T> _buf;
	lockfree_list<scheduler::signaler_i> _waiters;

	rua::opt<T> _wait_and_pop(scheduler_i sch, ms timeout) {
		assert(timeout);

		rua::opt<T> val_opt;

		auto sig = sch->get_signaler();
		auto it = _waiters.emplace_front(sig);

		if (it.is_back()) {
			val_opt = _buf.pop_back();
			if (val_opt) {
				if (!_waiters.erase(it) && !_buf.is_empty()) {
					auto waiter_opt = _waiters.pop_back();
					if (waiter_opt && waiter_opt.value() != sig) {
						waiter_opt.value()->signal();
					}
				}
				return val_opt;
			}
		}

		if (timeout == duration_max()) {
			for (;;) {
				if (sch->wait(timeout)) {
					val_opt = _buf.pop_back();
					if (val_opt) {
						return val_opt;
					}
					it = _waiters.emplace_front(sig);
				}
			}
		}

		for (;;) {
			auto t = tick();
			auto r = sch->wait(timeout);
			val_opt = _buf.pop_back();
			if (val_opt || !r) {
				return val_opt;
			}
			timeout -= tick() - t;
			if (timeout <= 0) {
				return val_opt;
			}
			it = _waiters.emplace_front(sig);
		}
		return val_opt;
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
