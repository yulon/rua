#ifndef _RUA_SYNC_CHAN_HPP
#define _RUA_SYNC_CHAN_HPP

#include "tsque.hpp"

#include "../macros.hpp"
#include "../sched.hpp"

#include <atomic>
#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

namespace rua {

template <typename T>
class chan {
public:
	constexpr chan() : _buf(), _waiters() {}

	chan(const chan &) = delete;

	chan &operator=(const chan &) = delete;

	T pop() {
		for (;;) {
			auto val_opt = _buf.pop();
			if (val_opt) {
				return std::move(val_opt.value());
			}
			auto sch = get_scheduler();
			auto sig = sch->make_signaler();
			auto n = _waiters.emplace(sig);

			val_opt = _buf.pop();
			if (val_opt) {
				if (!_waiters.erase(n)) {
					sig->reset();
				}
				return std::move(val_opt.value());
			}
			sch->wait(sig);
		}
	}

	rua::optional<T> try_get() {
		return _buf.pop();
	}

	template <typename... A>
	void emplace(A &&... a) {
		_buf.emplace(std::forward<A>(a)...);
		auto waiter_opt = _waiters.pop();
		if (!waiter_opt) {
			return;
		}
		waiter_opt.value()->signal();
	}

private:
	tsque<T> _buf;
	tsque<scheduler::signaler_i> _waiters;
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
