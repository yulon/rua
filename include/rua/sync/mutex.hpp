#ifndef _RUA_SYNC_MUTEX_HPP
#define _RUA_SYNC_MUTEX_HPP

#include "promise.hpp"

#include "../lockfree_list.hpp"
#include "../util.hpp"

#include <atomic>
#include <cassert>

namespace rua {

class mutex {
public:
	constexpr mutex() : _c(0), _wtrs() {}

	mutex(const mutex &) = delete;

	mutex &operator=(const mutex &) = delete;

	bool try_lock() {
		for (;;) {
			auto c = ++_c;
			if (c == 1) {
				return true;
			}
			c = --_c;
			if (c) {
				return false;
			}
		}
	}

	future<> lock() {
		future<> fut;

		auto c = ++_c;
		if (c == 1) {
			return fut;
		}

		promise<> prm;
		fut = prm.get_promising_future();
		auto is_emplaced = _wtrs.emplace_front_if_non_empty_or(
			[this]() -> bool { return _c.load() == 1; }, std::move(prm));
		if (!is_emplaced) {
			fut.reset();
		}
		return fut;
	}

	void unlock() {
		auto wtr_opt = _wtrs.pop_back_if([this]() -> bool { return --_c > 0; });
		if (!wtr_opt) {
			return;
		}
		wtr_opt->deliver(/*[this]() mutable { unlock(); }*/);
	}

private:
	std::atomic<uintptr_t> _c;
	lockfree_list<promise<>> _wtrs;
};

} // namespace rua

#endif
