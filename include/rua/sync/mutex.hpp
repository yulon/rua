#ifndef _RUA_SYNC_MUTEX_HPP
#define _RUA_SYNC_MUTEX_HPP

#include "tsque.hpp"

#include "../sched.hpp"

#include <atomic>
#include <cstdint>
#include <queue>

namespace rua {

class mutex {
public:
	constexpr mutex() : _locked(false), _waiters() {}

	mutex(const mutex &) = delete;

	mutex &operator=(const mutex &) = delete;

	bool try_lock() {
		return !_locked.exchange(true);
	}

	bool lock(ms timeout = duration_max()) {
		if (try_lock()) {
			return true;
		}
		auto sch = get_scheduler();
		auto sig = sch->make_signaler();
		auto n = _waiters.emplace(sig);

		if (try_lock()) {
			if (!_waiters.erase(n)) {
				sig->reset();
			}
			return true;
		}
		return sch->wait(sig, timeout);
	}

	void unlock() {
		auto waiter_opt = _waiters.pop();
		if (!waiter_opt.has_value()) {
			_locked.store(false);
			return;
		}
		waiter_opt.value()->signal();
	}

private:
	std::atomic<bool> _locked;
	tsque<scheduler::signaler_i> _waiters;
};

} // namespace rua

#endif
