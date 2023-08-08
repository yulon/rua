#ifndef _rua_sync_mutex_hpp
#define _rua_sync_mutex_hpp

#include "future.hpp"
#include "promise.hpp"

#include "../lockfree_list.hpp"
#include "../util.hpp"

#include <atomic>
#include <cassert>

namespace rua {

class mutex {
public:
	constexpr mutex() : $c(0), $wtrs() {}

	mutex(const mutex &) = delete;

	mutex &operator=(const mutex &) = delete;

	bool try_lock() {
		size_t old_val = 0;
		return $c.compare_exchange_strong(old_val, 1);
	}

	future<> lock() {
		auto c = ++$c;
		if (c == 1) {
			return future<>();
		}

		auto prm = new newable_promise<>;

		auto is_emplaced = $wtrs.emplace_front_if_non_empty_or(
			[this]() -> bool { return $c.load() > 1; }, prm);
		if (!is_emplaced) {
			return future<>(*prm);
		}

		prm->unuse();

		return future<>();
	}

	void unlock() {
		assert($c.load());

		auto wtr_opt = $wtrs.pop_back_if([this]() -> bool { return --$c > 0; });
		if (!wtr_opt) {
			return;
		}
		(*wtr_opt)->fulfill();
	}

private:
	std::atomic<uintptr_t> $c;
	lockfree_list<promise<> *> $wtrs;
};

} // namespace rua

#endif
