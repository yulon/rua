#ifndef _rua_conc_once_hpp
#define _rua_conc_once_hpp

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
		if ($mtx.try_lock()) {
			if ($executed.load()) {
				return {};
			}
			std::forward<Callable>(callable)(std::forward<Args>(args)...);
			$executed.store(true);
			$mtx.unlock();
			return {};
		}
		return $mtx.lock() >> [this]() {
			assert($executed.load());
			$mtx.unlock();
		};
	}

private:
	std::atomic<bool> $executed;
	mutex $mtx;
};

} // namespace rua

#endif
