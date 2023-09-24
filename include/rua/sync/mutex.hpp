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
	class unlocker {
	public:
		constexpr unlocker() : $mtx(nullptr) {}

		~unlocker() {
			(*this)();
		}

		unlocker(const unlocker &) = delete;

		unlocker &operator=(const unlocker &) = delete;

		unlocker(unlocker &&src) : $mtx(exchange(src.$mtx, nullptr)) {}

		unlocker &operator=(unlocker &&src) {
			$mtx = exchange(src.$mtx, nullptr);
			return *this;
		}

		explicit operator bool() const {
			return $mtx;
		}

		void operator()() {
			if (!$mtx) {
				return;
			}

			assert($mtx->$c.load());

			auto wtr_opt = $mtx->$wtrs.pop_back_if(
				[this]() -> bool { return --$mtx->$c > 0; });
			if (!wtr_opt) {
				return;
			}
			(*wtr_opt)->fulfill(std::move(*this));

			assert(!$mtx);
		}

	private:
		mutex *$mtx;

		explicit unlocker(mutex &mtx) : $mtx(&mtx) {}

		friend mutex;
	};

	constexpr mutex() : $c(0), $wtrs() {}

	mutex(const mutex &) = delete;

	mutex &operator=(const mutex &) = delete;

	unlocker try_lock() {
		size_t old_val = 0;
		return $c.compare_exchange_strong(old_val, 1) ? unlocker(*this)
													  : unlocker();
	}

	future<unlocker> lock() {
		if (++$c == 1) {
			return unlocker(*this);
		}

		auto prm = new newable_promise<unlocker>;

		auto is_emplaced = $wtrs.emplace_front_if_non_empty_or(
			[this]() -> bool { return $c.load() > 1; }, prm);
		if (!is_emplaced) {
			return future<unlocker>(*prm);
		}

		prm->unuse();

		return unlocker(*this);
	}

private:
	std::atomic<size_t> $c;
	lockfree_list<promise<unlocker> *> $wtrs;

	friend unlocker;
};

} // namespace rua

#endif
