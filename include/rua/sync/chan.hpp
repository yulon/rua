#ifndef _RUA_SYNC_CHAN_HPP
#define _RUA_SYNC_CHAN_HPP

#include "lockfree_list.hpp"

#include "../optional.hpp"
#include "../sched/dozer.hpp"
#include "../time/tick.hpp"
#include "../types/util.hpp"

#include <atomic>

namespace rua {

template <typename T>
class chan {
public:
	constexpr chan() : _buf(), _waiters() {}

	chan(const chan &) = delete;

	chan &operator=(const chan &) = delete;

	template <typename... Args>
	bool emplace(Args &&...args) {
		_buf.emplace_front(std::forward<Args>(args)...);
		auto waiter_opt = _waiters.pop_back_if([&]() -> bool { return _buf; });
		if (!waiter_opt) {
			return false;
		}
		waiter_opt.value()->wake();
		return true;
	}

	optional<T> try_pop() {
		return _buf.pop_back();
	}

	optional<T> try_pop(duration timeout) {
		auto val_opt = _buf.pop_back();
		if (val_opt || !timeout) {
			return val_opt;
		}
		return _wait_and_pop(this_dozer(), timeout);
	}

	optional<T> try_pop(dozer dzr, duration timeout) {
		auto val_opt = _buf.pop_back();
		if (val_opt || !timeout || !dzr) {
			return val_opt;
		}
		val_opt = _wait_and_pop(std::move(dzr), timeout);
		return val_opt;
	}

	T pop() {
		return try_pop(duration_max()).value();
	}

	T pop(dozer dzr) {
		return try_pop(std::move(dzr), duration_max()).value();
	}

protected:
	lockfree_list<T> _buf;
	lockfree_list<waker> _waiters;

	optional<T> _wait_and_pop(dozer dzr, duration timeout) {
		assert(dzr);
		assert(timeout);

		optional<T> val_opt;

		auto wkr = dzr->get_waker();

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
				if (dzr->doze(timeout) && !_waiters.emplace_front_if(
											  [&]() -> bool {
												  val_opt = _buf.pop_back();
												  return !val_opt;
											  },
											  wkr)) {
					return val_opt;
				}
			}
		}

		for (;;) {
			auto t = tick();
			auto r = dzr->doze(timeout);
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
inline chan<T> &operator<<(chan<T> &ch, V &&val) {
	ch.emplace(std::forward<V>(val));
	return ch;
}

template <typename T, typename R>
inline chan<T> &operator<<(R &receiver, chan<T> &ch) {
	receiver = ch.pop();
	return ch;
}

} // namespace rua

#endif
