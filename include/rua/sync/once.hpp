#ifndef _rua_sync_once_hpp
#define _rua_sync_once_hpp

#include "mutex.hpp"
#include "then.hpp"

#include "../util.hpp"

#include <atomic>
#include <cassert>

namespace rua {

class once {
public:
	constexpr once() : $executed(false) {}

	template <typename Callable, typename... Args>
	future<> call(Callable &&callable, Args &&...args) {
		if ($executed.load()) {
			return {};
		}
		auto ul = $mtx.try_lock();
		if (ul) {
			if ($executed.load()) {
				return {};
			}
			std::forward<Callable>(callable)(std::forward<Args>(args)...);
			$executed.store(true);
			ul();
			return {};
		}
		return $mtx.lock() >> [this](mutex::unlocker ul) {
			assert($executed.load());
			ul();
		};
	}

private:
	std::atomic<bool> $executed;
	mutex $mtx;
};

} // namespace rua

#endif
