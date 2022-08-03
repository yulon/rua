#ifndef _RUA_SYNC_ONCE_HPP
#define _RUA_SYNC_ONCE_HPP

#include "mutex.hpp"
#include "then.hpp"

#include "../util.hpp"

#include <atomic>
#include <cassert>

namespace rua {

class once {
public:
	constexpr once() : _executed(false) {}

	template <typename Callable, typename... Args>
	future<> call(Callable &&callable, Args &&...args) {
		if (_executed.load()) {
			return {};
		}
		if (_mtx.try_lock()) {
			if (_executed.load()) {
				return {};
			}
			std::forward<Callable>(callable)(std::forward<Args>(args)...);
			_executed.store(true);
			_mtx.unlock();
			return {};
		}
		return _mtx.lock() | [this]() {
			assert(_executed.load());
			_mtx.unlock();
		};
	}

private:
	std::atomic<bool> _executed;
	mutex _mtx;
};

} // namespace rua

#endif
