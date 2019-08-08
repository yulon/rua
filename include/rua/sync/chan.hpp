#ifndef _RUA_SYNC_CHAN_HPP
#define _RUA_SYNC_CHAN_HPP

#include "lf_forward_list.hpp"

#include "../chrono/clock.hpp"
#include "../optional.hpp"
#include "../sched.hpp"

#include <queue>
#include <utility>

namespace rua {

template <typename T>
class chan {
public:
	constexpr chan() : _buf(), _waiters() {}

	chan(const chan &) = delete;

	chan &operator=(const chan &) = delete;

	rua::optional<T> try_pop(ms timeout = 0) {
		auto val_opt = _buf.pop_front();
		if (val_opt || !timeout) {
			return val_opt;
		}

		auto sch = get_scheduler();
		auto sig = sch->make_signaler();
		auto it = _waiters.emplace_front(sig);

		if (it.is_back()) {
			val_opt = _buf.pop_front();
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
				if (sch->wait(sig, timeout)) {
					val_opt = _buf.pop_front();
					if (val_opt) {
						return val_opt;
					}
					it = _waiters.emplace_front(sig);
				}
			}
		}

		for (;;) {
			auto t = tick();
			auto r = sch->wait(sig, timeout);
			val_opt = _buf.pop_front();
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

	T pop() {
		return try_pop(duration_max()).value();
	}

	template <typename... A>
	void emplace(A &&... a) {
		if (!_buf.emplace_front(std::forward<A>(a)...).is_back()) {
			return;
		}
		auto waiter_opt = _waiters.pop_back();
		if (!waiter_opt) {
			return;
		}
		waiter_opt.value()->signal();
	}

private:
	lf_forward_list<T> _buf;
	lf_forward_list<scheduler::signaler_i> _waiters;
};

template <typename T, typename R>
chan<T> &operator>>(chan<T> &ch, R &receiver) {
	receiver = ch.pop();
	return ch;
}

template <typename T, typename R>
chan<T> &operator<<(R &receiver, chan<T> &ch) {
	receiver = ch.pop();
	return ch;
}

template <typename T, typename A>
chan<T> &operator<<(chan<T> &ch, A &&val) {
	ch.emplace(std::forward<A>(val));
	return ch;
}

template <typename T, typename A>
chan<T> &operator>>(A &&val, chan<T> &ch) {
	ch.emplace(std::forward<A>(val));
	return ch;
}

} // namespace rua

#endif
