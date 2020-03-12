#ifndef _RUA_SYNC_CHAN_BASE_HPP
#define _RUA_SYNC_CHAN_BASE_HPP

#include "../lockfree_list.hpp"

#include "../../chrono/tick.hpp"
#include "../../opt.hpp"
#include "../../sched/scheduler.hpp"

#include <utility>

namespace rua {

template <typename T>
class chan_base {
public:
	constexpr chan_base() : _buf(), _waiters() {}

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

	template <typename GetScheduler>
	rua::opt<T> try_pop(GetScheduler &&get_sch, ms timeout) {
		auto val_opt = _buf.pop_back();
		if (val_opt || !timeout) {
			return val_opt;
		}

		scheduler_i sch(get_sch());
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

	template <typename GetScheduler>
	T pop(GetScheduler &&get_sch) {
		return try_pop(std::forward<GetScheduler>(get_sch), duration_max())
			.value();
	}

private:
	lockfree_list<T> _buf;
	lockfree_list<scheduler::signaler_i> _waiters;
};

} // namespace rua

#endif
